#include "HttpServer.h"
#include "HttpContext.h"
#include "HttpWebService.h"
#include "../TcpConnection.h"
#include "../EventLoop.h"
#include "../Decoder/HttpContentDecoder.h"
#include "../Common/ConfigManager.h"
#include <sstream>
#include <cassert>
#include <functional>
#include <nlohmann/json.hpp>

HttpServer::HttpServer(EventLoop* loop, int nPort)
:server_(loop, nPort)
,web_service_(std::make_unique<HttpWebService>())
{
    
    // 由于httpServer 计算很少，以读写io为主，则让计算和消息都在io线程处理(不要放入任务线程池反而性能更低)。
    server_.SetMessageCallBack(std::bind(&HttpServer::OnMessage, 
        this, std::placeholders::_1, 
        std::placeholders::_2)
    );

    /*
    server_.SetMessageCallBack(std::bind(&HttpServer::AsyncOnMessage, 
        this, std::placeholders::_1, 
        std::placeholders::_2)
    );
    */

    server_.SetConnectionCallBack([](const std::shared_ptr<TcpConnection>& con){
        auto http_decoder = std::make_unique<HttpContentDecoder>();
        con->SetDecoder(std::move(http_decoder));
    });

    web_service_->ServerStatic("/", "../www");   // 映射 URL 根路径到 ../www 目录
}

HttpServer::~HttpServer() = default;

void HttpServer::Start(int option, int nEventLoopThread, int nTaskThreadNum /*= std::thread::hardware_concurrency()*/)
{
    server_.Start(option, nEventLoopThread, nTaskThreadNum);
}



// 异步实现 注释版本同步实现 因为解析这个过程计算过程很快，所以这个仅仅是用来学习，实际过程中应该使用同步
void HttpServer::AsyncOnMessage(const std::shared_ptr<TcpConnection>& con, std::string& strMsg)
{
    ThreadPool* pool = server_.GetThreadPool();
    if(!pool)
    {
        return;
    }


    EventLoop* loop = con->GetLoop();
    loop->AssertInLoopThread("HttpServer::AsyncOnMessage");
    std::weak_ptr<TcpConnection> weakConPtr = con;

     // 将业务逻辑提交到线程池处理
    pool->AddTask([this](EventLoop* loop, const std::weak_ptr<TcpConnection>& conWeakPtr, std::string strMsg){
        //auto con = weakConPtr.lock();
        //if(con)
        {
            // 模拟耗时业务（例如解析、数据库查询）
            thread_local HttpContext async_thread_context;
            async_thread_context.Reset();
            bool bOk = async_thread_context.ParseRequest(strMsg);
            if(!bOk)
            {
                // 解析失败或需要更多数据，这里简单返回 400
                std::string body = "Bad Request";
                AsyncSendHttpResponse(loop, conWeakPtr, body, 400, async_thread_context.KeepAlive());
                return;
            }

            const std::string& strPath = async_thread_context.GetPath();
            const std::string& strMethod = async_thread_context.GetMethod();
            bool bKeepAlive = async_thread_context.KeepAlive();
            std::cout << "HttpServer::AsyncOnMessage keepalive:=" << (bKeepAlive ? 1:0) << std::endl; 
            if("POST" != strMethod)
            {
                // 注意安全检查，避免出目录
                // 简单路由：返回 "Hello, World!"
                if("/" == strPath)
                {
                    std::string body = "<h1>Hello, World!</h1>";
                    AsyncSendHttpResponse(loop, conWeakPtr, body, 200, bKeepAlive);
                }
                else
                {
                    std::string filepath = std::move(web_service_->GetSystemFilePath(strPath));
                    if('/' == filepath.back())
                    {
                        filepath += "index.html";
                    }

                    web_service_->AysncSendStaticFile(filepath, loop, conWeakPtr, bKeepAlive);
                }


                return;
            }

            
            std::string strHandleKey(strPath.data() + 1, strPath.size()-1);  
            auto func = this->GetHadlerByMethod(strHandleKey);
            if(!func)
            {
                // 简单路由：返回 "Hello, World!"
                std::string body = "Not Found";
                AsyncSendHttpResponse(loop, conWeakPtr, body, 404, bKeepAlive);
                return;
            }

            {
                const std::string& strBody = async_thread_context.GetBody();
                std::string result;
                try
                {
                    result = func(strBody);
                }
                catch(const std::exception& e)
                {
                    std::cerr << "HttpServer func error: " << e.what() << std::endl;
                }
                        
                AsyncSendHttpResponse(loop, conWeakPtr, result, 200, bKeepAlive);
            }
        }
        
    }, loop, weakConPtr, std::move(strMsg));
}

void HttpServer::AsyncSendHttpResponse(EventLoop* loop, const std::weak_ptr<TcpConnection>& conWeakPtr, const std::string& strContent, int nStatusCode, bool bKeepAlive)
{
    if(!loop)
    {
        return;
    }

    auto response = HttpContext::GenerateResponseBySolveRequest(bKeepAlive, nStatusCode, strContent);


    //std::string strResponse = response.str();
    //std::weak_ptr<TcpConnection> weakConPtr = con;
    
    {
        loop->RunInLoop([this, bKeepAlive, conWeakPtr, response = std::move(response), strContent = std::move(strContent)](){ 
            //if(weakConPtr.lock()) weakConPtr.lock()->Send(strResponse);
                std::shared_ptr<TcpConnection> con = conWeakPtr.lock(); 
                if(con && !con->IsWriteClosed())
                {
                    con->Send(response.first);
                    if(response.second)
                    {
                        HttpServer::SendChunkedData(con, strContent);
                    }

                    if(!bKeepAlive)
                    {
                        con->CloseWhenWriteFinish();
                    }
                } 
        });
    }
    
}


// 同步实现版本
void HttpServer::OnMessage(const std::shared_ptr<TcpConnection>& con, std::string& strMsg)
{
    http_server_context_.Reset();
    bool bOk = http_server_context_.ParseRequest(strMsg);
    if(!bOk)
    {
        // 解析失败或需要更多数据，这里简单返回 400
        std::string body = "Bad Request";
        SendHttpResponse(con, body, 400, http_server_context_.KeepAlive());
        return;
    }

    const std::string& strMethod = http_server_context_.GetMethod();
    if("POST" != strMethod)
    {
        // 非post 统一当做get来处理. 
        // 假设静态文件根目录为 "./www"，且所有 GET 请求都尝试作为静态文件处理
        const std::string& strPath = http_server_context_.GetPath();
        // 注意安全检查，避免出目录
            // 简单路由：返回 "Hello, World!"
            std::string body = "<h1>Hello, World!</h1>";
            SendHttpResponse(con, body, 200, http_server_context_.KeepAlive());

            /*
        if(strPath == "/")
        {
            // 注意安全检查，避免出目录
            // 简单路由：返回 "Hello, World!"
            std::string body = "<h1>Hello, World!</h1>";
            SendHttpResponse(con, body, 200, http_server_context_.KeepAlive());
        }
        else
        {
            std::string filepath = std::move(web_service_->GetSystemFilePath(strPath));
            if('/' == filepath.back())
            {
                filepath += "index.html";
            }

            web_service_->SendStaticFile(filepath, con, http_server_context_.KeepAlive());
        }
            */

        return;
    }

    const std::string& strPath = http_server_context_.GetPath();
    std::string strHandleKey(strPath.data() + 1, strPath.size()-1);  
    auto func = this->GetHadlerByMethod(strHandleKey);
    if(func)
    {
        const std::string& strBody = http_server_context_.GetBody();
        std::string result = func(strBody);
        SendHttpResponse(con, result, 200, http_server_context_.KeepAlive());
    }
    else
    {     
        // 简单路由：返回 "Hello, World!"
        std::string body = "Not Found";
        SendHttpResponse(con, body, 404, http_server_context_.KeepAlive());
    }
}

void HttpServer::SendHttpResponse(const std::shared_ptr<TcpConnection>& con, const std::string& strContent, int nStatusCode, bool bKeepAlive)
{
    auto response = HttpContext::GenerateResponseBySolveRequest(bKeepAlive, nStatusCode, strContent);
    con->Send(response.first);
    if(response.second)
    {
         HttpServer::SendChunkedData(con, strContent);
    }

    if(!bKeepAlive)
    {
        con->CloseWhenWriteFinish();
    }
}


void HttpServer::RegisterMethod(const std::string& strMethod, Handler handler)
{
    method_handlers_[strMethod] = handler;
}


HttpServer::Handler HttpServer::GetHadlerByMethod(const std::string& strMethod)
{
    auto itr = method_handlers_.find(strMethod);
    if(itr == method_handlers_.end())
    {
        return nullptr;
    }

    return (itr->second);
}


void HttpServer::SendChunkedData(const std::shared_ptr<TcpConnection>& con, const std::string& strContent)
{
    
    auto& cfg = ConfigManager::getInstance();
    int chunk_length = cfg.getInt("Http", "chunked_length", 10);
     

    int content_length = strContent.length(); 
    for(int i = 0; i < content_length; i += chunk_length)
    {
        int remain_length = content_length - i;
        int real_length = (remain_length < chunk_length)? remain_length : chunk_length;
        std::string tmp_chunk_data = strContent.substr(i, real_length);

        std::ostringstream ss;
        ss << std::hex << real_length << "\r\n" << tmp_chunk_data << "\r\n";
        con->Send(ss.str());
    }

    std::cout << std::endl;
    con->Send("0\r\n\r\n");
}



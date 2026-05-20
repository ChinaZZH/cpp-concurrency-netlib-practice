#include "HttpServer.h"
#include "HttpContext.h"
#include "TcpConnection.h"
#include "EventLoop.h"
#include <sstream>
#include <cassert>

HttpServer::HttpServer(EventLoop* loop, int nPort)
:server_(loop, nPort)
{
    server_.SetMessageCallBack(std::bind(&HttpServer::AsyncOnMessage, 
        this, std::placeholders::_1, 
        std::placeholders::_2)
    );
}

void HttpServer::Start(int option, int nThreadNum /*= std::thread::hardware_concurrency() - 1*/)
{
    server_.Start(option, nThreadNum);
}



// 异步实现 注释版本同步实现 因为解析这个过程计算过程很快，所以这个仅仅是用来学习，实际过程中应该使用同步
void HttpServer::AsyncOnMessage(const std::shared_ptr<TcpConnection>& con, std::string& strMsg)
{
    //int fd = con->GetFd();
    ThreadPool* pool = server_.GetThreadPool();
    if(!pool)
    {
        return;
    }


    std::weak_ptr<TcpConnection> weakConPtr = con;
     // 将业务逻辑提交到线程池处理
    pool->AddTask([this](const std::weak_ptr<TcpConnection>& conWeakPtr, std::string strMsg){
        //auto con = weakConPtr.lock();
        //if(con)
        {
            // 模拟耗时业务（例如解析、数据库查询）
            size_t consumed = 0;
            
            thread_local HttpContext context;
            context.Reset();
            bool bOk = context.PraseRequest(strMsg, consumed);
            if(bOk)
            {
                // 简单路由：返回 "Hello, World!"
                std::string body = "<h1>Hello, World!</h1>";
                AsyncSendHttpResponse(conWeakPtr, body, 200);
            }else{
                // 解析失败或需要更多数据，这里简单返回 400
                std::string body = "Bad Request";
                AsyncSendHttpResponse(conWeakPtr, body, 400);
            }
        }
        
    }, weakConPtr, std::move(strMsg));
}

void HttpServer::AsyncSendHttpResponse(const std::weak_ptr<TcpConnection>& conWeakPtr, const std::string& strContent, int nStatusCode)
{
    std::string strCode;
    strCode = (200 == nStatusCode? "OK": "Bad Request");

    std::ostringstream response;
    response << "HTTP/1.1 " << nStatusCode << " " << strCode.c_str() << " \r\n"
             << "Content-Length: " << strContent.size() << "\r\n"
             << "Content-Type: text/html\r\n"
             << "\r\n"
             << strContent << "\r\n";


    std::string strResponse = response.str();
    //std::weak_ptr<TcpConnection> weakConPtr = con;
    EventLoop* loop = server_.GetEventLoop();
    if(loop)
    {
        std::thread::id reactorThreadId = loop->GetReactorThreadId();
        loop->RunInLoop([this, reactorThreadId, conWeakPtr, strResponse = std::move(strResponse)](){ 
            //if(weakConPtr.lock()) weakConPtr.lock()->Send(strResponse);
                assert(reactorThreadId == std::this_thread::get_id());
                std::shared_ptr<TcpConnection> con = conWeakPtr.lock(); 
                if(con && !con->IsClosed())
                {
                    if(con->IsPause())
                    {
                        con->WaitForWaterToLowMask(strResponse);
                    }else{
                        con->Send(strResponse);
                    }
                } 
        });
    }
    
}


// 同步实现版本
void HttpServer::OnMessage(const std::shared_ptr<TcpConnection>& con, std::string& strMsg)
{
    size_t consumed = 0;
    auto ctx = con->GetHttpContext();
    ctx->Reset();
    bool bOk = ctx->PraseRequest(strMsg, consumed);
    if(bOk)
    {
        // 简单路由：返回 "Hello, World!"
        std::string body = "<h1>Hello, World!</h1>";
        
        SendHttpResponse(con, body, 200);
    }else{
        // 解析失败或需要更多数据，这里简单返回 400
        std::string body = "Bad Request";
        SendHttpResponse(con, body, 400);
    }
}

void HttpServer::SendHttpResponse(const std::shared_ptr<TcpConnection>& con, const std::string& strContent, int nStatusCode)
{
    std::string strCode;
    strCode = (200 == nStatusCode? "OK": "Bad Request");

    std::ostringstream response;
    response << "HTTP/1.1 " << nStatusCode << " " << strCode.c_str() << " \r\n"
             << "Content-Length: " << strContent.size() << "\r\n"
             << "Content-Type: text/html\r\n"
             << "\r\n"
             << strContent << "\r\n";

    if(con->IsPause())
    {
        con->WaitForWaterToLowMask(response.str());
    }else{
        con->Send(response.str());
    }
    
}


/*
编译运行后，用浏览器或 curl 访问 http://localhost:8888，应看到 Hello, World!。
*/
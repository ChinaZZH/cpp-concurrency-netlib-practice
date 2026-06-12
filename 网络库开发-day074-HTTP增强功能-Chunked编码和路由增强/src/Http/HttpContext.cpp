#include "HttpContext.h"
#include "../Common/ConfigManager.h"
#include <iostream>
#include <algorithm>
#include <sstream>



HttpContext::HttpContext()
:method_()
,path_()
,version_()
,body_()
,nullptr_header()
,status_code_(0)
,response_body_()
{

}


const std::string& HttpContext::GetHeader(const std::string& key) const
{
    auto itr = std::find_if(headers_.begin(), headers_.end(), 
                            [&key](const std::pair<std::string, std::string>& pairHead){ 
                                return pairHead.first.compare(key);
                        });


    if(itr == headers_.end())
    {
        return nullptr_header;
    }

    return (itr->second);
}


// 重置状态，用于复用对象(例如 keep_alive)
void HttpContext::Reset()
{
    method_.clear();
    path_.clear();
    version_.clear();
    headers_.clear();
    body_.clear();
    nullptr_header.clear();

    status_code_ = 0;
    response_body_.clear();
}


/*
Get的请求报文格式
GET /index.html?name=john&age=25 HTTP/1.1\r\n
Host: www.example.com\r\n
User-Agent: Mozilla/5.0\r\n
Accept: text/html\r\n
\r\n
*/

// 解析数据， 返回true表示解析数据完成(可能还有剩余数据未使用)
bool HttpContext::ParseRequest(const std::string& data)
{
     enum ParseRequestState{
        kExpectRequestLine,
        kExpectHeaders,
        kExpectBody,
        kGotAll,
    };

    size_t pos = 0;
    ParseRequestState request_state = kExpectRequestLine;
    // 解析RequestLine  GET /index.html?name=john&age=25 HTTP/1.1\r\n
    if(kExpectRequestLine == request_state)
    {
        size_t request_line = data.find("\r\n", pos);
        if(request_line == std::string::npos)
        {
            return false;
        }

        std::string_view strLine(data.data(), request_line);
        // << "HttpContext::PraseRequest request_line:=" << request_line << std::endl;
        bool bRequestLine = ProcessRequestLine(strLine);
        if(!bRequestLine)
        {
            return false;
        }
        
        pos = request_line + 2;
        request_state = kExpectHeaders;
    }
    

    // 解析后面的 KeyName
    /*
    Host: www.example.com\r\n
    User-Agent: Mozilla/5.0\r\n
    Accept: text/html\r\n
    \r\n
    */
   
    // header list 没有"\r\n\r\n" 直接返回错误，就不做解析了。
    size_t header_end_pos = data.find("\r\n\r\n", pos);
    if(std::string::npos == header_end_pos)
    {
        return false;
    }

    header_end_pos += 2;
    while(request_state == kExpectHeaders)
    {
        // 找不到\r\n则出错
        size_t eol = data.find("\r\n", pos);
        if(std::string::npos == eol)
        {
            return false;
        }

        
        std::string_view strHeader(data.data() + pos, eol - pos);
        if(!strHeader.empty())
        {
           if(!ProcessHeader(strHeader))
           {
                return false;
           }
        }
        
        pos = eol + 2;

        // pos移动到"\r\n\r\n"中的第2个\r的时候就可以结束整个header解析流程了。
        if(pos == header_end_pos)
        {
            pos += 2; // 移动到header结束，开启body内容
            request_state = kExpectBody;
            break;
        }
    }


    // 处理特殊的头部字段
    this->ProcessSpecficHeader();

    if(kExpectBody == request_state)
    {
        auto itr = std::find_if(headers_.begin(), headers_.end(), [](const auto& header_data){
            return (0 == header_data.first.compare("Content-Length"));
        });

        if(itr == headers_.end())
        {
            request_state = kGotAll;
            return true;
        }
        
        const std::string& str_content_len = (itr->second);
        size_t content_len = std::strtoul(str_content_len.c_str(), nullptr, 10);
        int remain_len = data.size() - pos;
        if(remain_len < content_len)
        {
            return false;
        }

        body_.assign(data.data() + pos, content_len);
        request_state = kGotAll;
    }

    return  (kGotAll == request_state); 
}


bool HttpContext::ProcessRequestLine(const std::string_view& line)
{
    size_t method_index = line.find(' ');
    if(method_index == std::string_view::npos){
        return false;
    }

    size_t path_index = line.find(' ', method_index + 1);
    if(path_index == std::string_view::npos)
    {
        return false;
    }

    method_ = line.substr(0, method_index);
    path_ = line.substr(method_index + 1, path_index - method_index -1);
    version_ = line.substr(path_index + 1);
    return version_.find("HTTP/") == 0;

}

bool HttpContext::ProcessHeader(const std::string_view& line)
{
    size_t key_index = line.find(':');
    if(key_index == std::string_view::npos){
        return false;
    }

    auto trimFunc = [](std::string_view& sv){
        size_t start_index = sv.find_first_not_of(" \t");
        if(start_index == std::string_view::npos)  start_index = sv.size();

        size_t end_index = sv.find_last_not_of(" \t");
        if(end_index == std::string_view::npos) end_index = sv.size();

        sv = sv.substr(start_index, end_index - start_index + 1);
    };

    std::string_view strKey = line.substr(0, key_index);
    trimFunc(strKey);

    std::string_view strValue = line.substr(key_index + 1);
    trimFunc(strValue);

    headers_.emplace_back(std::pair<std::string, std::string>(strKey, strValue));
    return true;
}



bool HttpContext::ParseResponse(const std::string& data)
{
    size_t header_end_pos = data.find("\r\n\r\n");
    if(std::string::npos == header_end_pos)
    {
        return false;
    }

    size_t space = data.find(' ');
    if(std::string::npos == space) {
        return false;
    }

    const char* status_code_start = data.data() + space + 1;
    while(' ' == (*status_code_start) || '\t' == (*status_code_start))
    {
        status_code_start += 1;
    }

    // 得到status_code,并且走完code部分
    int tmp_status_code = std::strtoul(status_code_start, nullptr, 10);
    // 没有必要解析这个
    /*
    status_code_start += 1;
    while(' ' != (*status_code_start))
    {
        status_code_start += 1;
    }

    // 到达空格，继续过滤空格
    while(' ' == (*status_code_start) || '\t' == (*status_code_start))
    {
        status_code_start += 1;
    }

    const char*  code_msg_start = status_code_start;
    const char*  code_msg_end = code_msg_start;
    int msg_char_num = 0;
    while(' ' != (*code_msg_end))
    {
        msg_char_num   += 1;
        code_msg_end   += 1;
    }

    std::string code_msg(code_msg_start, msg_char_num);
    */

    status_code_ = tmp_status_code;
    response_body_ = std::move(data.substr(header_end_pos + 4));
    return true;
}


void HttpContext::ProcessSpecficHeader()
{
    // std::vector<std::pair<std::string, std::string>>  headers_;
    //std::string str_connection = "Connection";
    auto itr = std::find_if(headers_.begin(), headers_.end(), [](const auto& header_info){
        return (header_info.first == "Connection");
    });

    if(itr != headers_.end())
    {
        const auto& header_value = (itr->second);
        if(header_value == "keep-alive")
        {
            keep_alive_ = true;
        }
        else if(header_value == "close"){
            keep_alive_ = false;
        }
    }
    else
    {
        // HTTP/1.1 默认 keep-alive，HTTP/1.0 默认 close
        keep_alive_ = (version_ == "HTTP/1.1");
    }
}


std::pair<std::string, bool> HttpContext::GenerateResponseBySolveRequest(bool keep_alive, int status_code, 
    const std::string& context, const std::string& context_type /*= "text/html"*/, int send_file_size /*= 0*/)
{
    // send_file_size > 0 代表了http在这个消息里面是先发的消息头，然后再发send_file 零拷贝

    bool bChunk = false;
    auto& cfg = ConfigManager::getInstance();
    int chunk_flag = cfg.getInt("Http", "chunked", 1);
    int chunk_length = cfg.getInt("Http", "chunked_length", 10);
    if(chunk_flag > 0 && chunk_length > 0 && context.length() > chunk_length) 
    {
        bChunk = true;
    }

    if(send_file_size > 0) // 零拷贝流程不参与chunk
    {
        bChunk = false;
    }

    std::string strKeepAlive = (keep_alive? ("Connection: keep-alive\r\n") : ("Connection: close\r\n"));
    std::string once_or_chunk;
    if(bChunk)
    {
        once_or_chunk.assign("Transfer-Encoding: chunked\r\n");
    }else{
        std::ostringstream content_length;
        content_length << "Content-Length: ";
        (send_file_size > 0) ? (content_length << send_file_size) : (content_length << context.size());  // 0 == send_file_size 不处于零拷贝执行流程， send_file_size > 0后面要执行零拷贝
        content_length << "\r\n";

        once_or_chunk.assign(content_length.str());
    }
    

    std::string strCode = std::move(HttpContext::GetStatusCodeMsg(status_code));
    std::ostringstream response;
    response << "HTTP/1.1 " << status_code << " " << strCode.c_str() << " \r\n"
             << once_or_chunk.c_str()
             << "Content-Type: " << context_type.c_str() << "\r\n"
             << strKeepAlive.c_str()
             << "\r\n";

    if(!bChunk && !context.empty())
    {
        response << context;
    }
            
    std::cout << "HttpContext::GenerateResponseBySolveRequest" << std::endl;
    std::cout << response.str() << std::endl; 
    return std::pair<std::string, bool>(response.str(), bChunk);
}


std::string HttpContext::GetStatusCodeMsg(int nStatusCode)
{
    if(200 == nStatusCode)
    {
        return "OK";
    }

    if(400 == nStatusCode)
    {
        return "Bad Request";
    }

    if(404 == nStatusCode)
    {
        return "Not Found";
    }
    
    return "Unknow Error";
}
#include "HttpContext.h"
#include <iostream>


HttpContext::HttpContext()
:state_(kExpectRequestLine)
,method_()
,path_()
,version_()
{

}


std::string HttpContext::GetHeader(const std::string& key) const
{
    auto itr = headers_.find(key);
    if(itr == headers_.end())
    {
        return std::string();
    }

    return (itr->second);
}


// 重置状态，用于复用对象(例如 keep_alive)
void HttpContext::Reset()
{
    state_ = (kExpectRequestLine);
    method_.clear();
    path_.clear();
    version_.clear();
    headers_.clear();
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
bool HttpContext::PraseRequest(const std::string& data, size_t& consumed)
{
    this->Reset();
    size_t pos = 0;
    // 解析RequestLine  GET /index.html?name=john&age=25 HTTP/1.1\r\n
    {
        size_t request_line = data.find("\r\n", pos);
        if(request_line == std::string::npos)
        {
            return false;
        }

        std::string strLine = data.substr(pos, request_line);
        bool bRequestLine = ProcessRequestLine(strLine);
        if(!bRequestLine)
        {
            return false;
        }
        
        pos = request_line + 2;
        consumed = pos;
        state_ = kExpectHeaders;
    }
    

    // 解析后面的 KeyName
    /*
    Host: www.example.com\r\n
    User-Agent: Mozilla/5.0\r\n
    Accept: text/html\r\n
    \r\n
    */
   
    int flag = ((state_ != kGotAll && pos < data.size())?1:0);
    while(state_ != kGotAll && pos < data.size())
    {
        size_t eol = data.find("\r\n", pos);
        if(std::string::npos == eol)
        {
            break;
        }

        std::string strHeader = data.substr(pos, eol - pos);
        if(strHeader.empty())
        {
            state_ = kGotAll;
            consumed = eol + 2;
            break;
        }
        
        if(!ProcessHeader(strHeader))
        {
            return false;
        }

        pos = eol + 2;
        consumed = pos;
    }

    return  (kGotAll == state_); 
}


bool HttpContext::ProcessRequestLine(const std::string& line)
{
    size_t method_index = line.find(' ');
    if(method_index == std::string::npos){
        return false;
    }

    size_t path_index = line.find(' ', method_index + 1);
    if(path_index == std::string::npos)
    {
        return false;
    }

    method_ = line.substr(0, method_index);
    path_ = line.substr(method_index + 1, path_index - method_index -1);
    version_ = line.substr(path_index + 1);
    return version_.find("HTTP/") == 0;

}

bool HttpContext::ProcessHeader(const std::string& line)
{
    size_t key_index = line.find(':');
    if(key_index == std::string::npos){
        return false;
    }

    std::string strKey = line.substr(0, key_index);
    strKey.erase(0, strKey.find_first_not_of(" \t"));
    strKey.erase(strKey.find_last_not_of(" \t") + 1);

    std::string strValue = line.substr(key_index + 1);
    strValue.erase(0, strValue.find_first_not_of(" \t"));
    strValue.erase(strValue.find_last_not_of(" \t") + 1);

    headers_[strKey] = strValue;
    return true;
}


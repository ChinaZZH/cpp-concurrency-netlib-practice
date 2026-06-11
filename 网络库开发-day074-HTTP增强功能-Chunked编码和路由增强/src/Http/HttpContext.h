// HttpContext.h

#pragma once

#include <string>
#include <unordered_map>
#include <vector>

/*
Get的请求报文格式
GET /index.html?name=john&age=25 HTTP/1.1\r\n
Host: www.example.com\r\n
User-Agent: Mozilla/5.0\r\n
Accept: text/html\r\n
\r\n
*/

class HttpContext{
public:
   
    HttpContext();

    // 解析数据， 返回true表示解析数据完成(可能还有剩余数据未使用)
    bool ParseRequest(const std::string& data);

    bool ParseResponse(const std::string& data);

    // 解析结果返回
    const std::string& GetMethod() const { return method_; }

    const std::string& GetPath() const { return path_; }

    const std::string& GetVersion() const { return version_; }

    const std::string& GetBody() const { return body_; }

    const std::string& GetHeader(const std::string& key) const;

    // 重置状态，用于复用对象(例如 keep_alive)
    void Reset();

    // response相关
    int GetStatusCode() const { return status_code_; }

    const std::string& GetResponseBody() const { return response_body_; }
    
    bool KeepAlive() const { return keep_alive_; }
    
private:
    bool ProcessRequestLine(const std::string_view& line);

    bool ProcessHeader(const std::string_view& line);

    void ProcessSpecficHeader();

private:
    // request内容
    std::string     method_;
    std::string     path_;
    std::string     version_;
    std::vector<std::pair<std::string, std::string>>  headers_; // 由于这个数量极少，则用std::vector进行优化
    bool keep_alive_ = true;

    std::string body_;
    std::string nullptr_header;


    // response内容
    int status_code_;
    //std::string code_msg_;
    std::string response_body_;
};
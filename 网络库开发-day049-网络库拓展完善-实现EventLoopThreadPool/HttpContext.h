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
    enum ParseState{
        kExpectRequestLine,
        kExpectHeaders,
        kGotAll,
    };

    HttpContext();

    // 解析数据， 返回true表示解析数据完成(可能还有剩余数据未使用)
    bool PraseRequest(const std::string& data, size_t& consumed);

    bool IsComplete() const { return state_ == kGotAll; }

    // 解析结果返回
    std::string GetMethod() const { return method_; }

    std::string GetPath() const { return path_; }

    std::string GetVersion() const { return version_; }

    std::string GetHeader(const std::string& key) const;

    // 重置状态，用于复用对象(例如 keep_alive)
    void Reset();

private:
    bool ProcessRequestLine(const std::string_view& line);

    bool ProcessHeader(const std::string_view& line);

    ParseState      state_;
    std::string     method_;
    std::string     path_;
    std::string     version_;
    std::vector<std::pair<std::string, std::string>>  headers_; // 由于这个数量极少，则用std::vector进行优化
};
#pragma once

#include <string>

// 当前只实现了同步机制，日后再拓展异步机制
class SimpleHttpClient
{
public:
    struct Response
    {
        bool success;
        int status_code;
        std::string body;
        std::string error_msg;
    };

    // 同步post
    static Response Post(const std::string& host, int port, const std::string& path, 
        const std::string& body, int timeout_sec = 3, int retry_times = 1);

    // 同步get
    static Response Get(const std::string& host, int port, const std::string& path, 
        int timeout_sec = 3, int retry_times = 1);


private:
    static std::string BuildHttpRequest(const std::string& method, const std::string& path, 
        const std::string& host, int port, const std::string& body = "");


    static Response SendRequest(const std::string& host, int port, const std::string& request, int timeout_sec);
};
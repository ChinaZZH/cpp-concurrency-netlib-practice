#include "SimpleHttpClient.h"
#include "HttpContext.h"
#include "../ClientSocket.h"
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include <cstring>
#include <sstream>
#include <thread>
#include <chrono>
#include <iostream>


// 同步post
/*
Get的请求报文格式
GET /index.html?name=john&age=25 HTTP/1.1\r\n
Host: www.example.com\r\n
User-Agent: Mozilla/5.0\r\n
Accept: text/html\r\n
Content-Length:100
\r\n
Content-body实体内容本身
*/
SimpleHttpClient::Response SimpleHttpClient::Post(const std::string& host, int port, const std::string& path, 
        const std::string& body, int timeout_sec /*= 3*/, int retry_times /*= 1*/)
{

    std::string req = std::move(SimpleHttpClient::BuildHttpRequest("POST", path, host, port, body));
    for(int i = 0; i < retry_times; ++i)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        auto res = SimpleHttpClient::BlockedSendRequest(host, port, req, timeout_sec);
        if(res.success)
        {
            return res;
        }

        std::ostringstream ss;
        ss << "SimpleHttpClient::Post  success:" <<  res.success;
        ss << " status_code:" <<  res.status_code;
        ss << " body:" <<  res.body;
        ss << " error_msg:" <<  res.error_msg;
        ss << std::endl;
        std::cout << ss.str();
    }

    SimpleHttpClient::Response response;
    response.success = false;
    response.status_code = 0;
    response.body.clear();
    response.error_msg.assign("all retries failed-POST");
    return response;
}

// 同步get
/*
Get的请求报文格式
GET /index.html?name=john&age=25 HTTP/1.1\r\n
Host: www.example.com\r\n
User-Agent: Mozilla/5.0\r\n
Accept: text/html\r\n
\r\n
*/

SimpleHttpClient::Response SimpleHttpClient::Get(const std::string& host, int port, const std::string& path, 
        int timeout_sec /*= 3*/, int retry_times /*= 1*/)
{
    std::string req = std::move(SimpleHttpClient::BuildHttpRequest("GET", path, host, port));
    for(int i = 0; i < retry_times; ++i)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        auto res = SimpleHttpClient::BlockedSendRequest(host, port, req, timeout_sec);
        if(res.success)
        {
            return res;
        }

        std::ostringstream ss;
        ss << "SimpleHttpClient::Get  success:" <<  res.success;
        ss << " status_code:" <<  res.status_code;
        ss << " body:" <<  res.body;
        ss << " error_msg:" <<  res.error_msg;
        ss << std::endl;
        std::cout << ss.str();
    }

    SimpleHttpClient::Response response;
    response.success = false;
    response.status_code = 0;
    response.body.clear();
    response.error_msg.assign("all retries failed-GET");
    return response;
}


std::string SimpleHttpClient::BuildHttpRequest(const std::string& method, const std::string& path, 
    const std::string& host, int port, const std::string& body /*= ""*/)
{
    std::ostringstream req;
    req << method << " " << path << " HTTP/1.1\r\n";
    req << "Host: " << host << ":" << port << "\r\n";
    // 每次请求独立完成，完成该请求后关闭tcp连接。因为低频，所以使用这种用完就关的短连接
    req << "Connection: close\r\n"; 
    
    if(!body.empty())
    {
        req << "Content-Type: application/json\r\n"; //后续也可以扩展为protobuf,但为了兼容浏览器以及wrk还是使用json
        req << "Content-Length: " << body.size() << "\r\n";
    }

    req << "\r\n";
    if(!body.empty())
    {
        req << body;
    }

    return req.str();
}




SimpleHttpClient::Response SimpleHttpClient::BlockedSendRequest(const std::string& host, int port, const std::string& request, int timeout_sec)
{
    SimpleHttpClient::Response res;
    res.success = false;
    res.status_code = 0;

    int sock_fd = ClientSocket::Connect(host, port, false);
    if(sock_fd < 0)
    {
        res.error_msg = "connect failed"+std::string(strerror(errno));
        return res;
    }

    ClientSocket client(sock_fd, true);
    client.SetTimeout(timeout_sec);
    if(false == client.BlockedWriteAll(request))
    {
        res.error_msg = "send failed: " + std::string(strerror(errno));
        return res;
    }

    // 接收
    std::string response = std::move(client.BlockedReadAll());
    if (response.empty()) {
        res.error_msg = "empty response";
        return res;
    }

    HttpContext httContext;
    httContext.Reset();
    if(!httContext.ParseResponse(response))
    {
        res.error_msg = "parse response msg error";
        return res;
    }

    res.status_code = httContext.GetStatusCode();
    res.body = httContext.GetResponseBody();
    res.success = (res.status_code >= 200 && res.status_code < 300);
    if(!res.success)
    {
        res.error_msg = "HTTP error " + std::to_string(res.status_code);
    }

    return res;
}
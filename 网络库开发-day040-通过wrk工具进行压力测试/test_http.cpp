#include "HttpContext.h"
#include <iostream>
#include <string>

int main() {
    // 完整的HTTP GET请求报文
    std::string request = 
        "GET /index.html?name=john&age=25 HTTP/1.1\r\n"
        "Host: www.example.com\r\n"
        "User-Agent: Mozilla/5.0\r\n"
        "Accept: text/html\r\n"
        "\r\n";

    HttpContext ctx;
    size_t consumed = 0;
    bool ok = ctx.PraseRequest(request, consumed);  // 注意你的函数名是 PraseRequest

    if (ok) {
        std::cout << "解析成功！" << std::endl;
        std::cout << "Method: " << ctx.GetMethod() << std::endl;
        std::cout << "Path: " << ctx.GetPath() << std::endl;
        std::cout << "Version: " << ctx.GetVersion() << std::endl;
        std::cout << "Host: " << ctx.GetHeader("Host") << std::endl;
        std::cout << "User-Agent: " << ctx.GetHeader("User-Agent") << std::endl;
        std::cout << "Consumed bytes: " << consumed << std::endl;
    } else {
        std::cout << "解析失败" << std::endl;
    }

    return 0;
}
#include "EventLoop.h"
#include "TcpServer.h"
#include "HttpServer.h"

#define PORT 8888


int main()
{
    EventLoop loop;
    /*
    TcpServer server(&loop, PORT);
    server.SetMessageCallBack(std::bind(&TcpServer::HandleOnMessage, 
        &server, std::placeholders::_1, 
        std::placeholders::_2)
    );
    */

    HttpServer server(&loop, PORT);
    server.Start();
    loop.Loop();
    return 0;
}
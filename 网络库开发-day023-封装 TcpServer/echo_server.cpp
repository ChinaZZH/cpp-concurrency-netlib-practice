#include "EventLoop.h"
#include "TcpServer.h"

#define PORT 8888


int main()
{
    EventLoop loop;
    TcpServer server(&loop, PORT);
    server.Start();
    loop.Loop();
    return 0;
}
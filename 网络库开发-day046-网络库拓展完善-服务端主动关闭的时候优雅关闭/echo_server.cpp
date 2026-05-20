#include "EventLoop.h"
#include "TcpServer.h"
#include "HttpServer.h"
#define PORT 8888


int main()
{
    EventLoop loop;
    TcpServer server(&loop, PORT);
    server.SetMessageCallBack(std::bind(&TcpServer::HandleOnMessage, 
        &server, std::placeholders::_1, 
        std::placeholders::_2)
    );
    server.Start(0);

    // httpSrver
    //HttpServer server(&loop, PORT);
    //server.Start(0);

    // 定时器测试
    /*
    int num = 1;
    loop.RunEvery(std::chrono::seconds(1), [&num](){
        std::cout << "Tick = " << num << std::endl;
        ++num;
    });
    */
   
    loop.Loop();
    return 0;
}




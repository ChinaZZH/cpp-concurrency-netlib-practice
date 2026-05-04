


#include "ListenSocket.h"
#include "ClientSocket.h"
#include "Epoll.h"
#include "Channel.h"
#include "EventLoop.h"
#include "TcpConnection.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <map>
#include <vector>
#include <memory>
#include <cerrno>

#define PORT 8888

ListenSocket g_listenScoket;
std::unique_ptr<Channel> g_listenChannel;
EventLoop g_loop;
std::map<int, std::shared_ptr<TcpConnection>> g_mapTcpConnection;


void HandleNewConnection()
{
    int nClientFd = g_listenScoket.Accept();
    if(nClientFd < 0)
    {
        std::cerr << "accept error \n";
        return;
    }


    {
        // 已经存在该对象了
        auto itr = g_mapTcpConnection.find(nClientFd);
        if(itr != g_mapTcpConnection.end())
        {
            return ;
        }
    }
    

    // 将新连接加入epoll  
    auto new_connection = std::make_shared<TcpConnection>(&g_loop, nClientFd);
    new_connection->ConnectEstablished();
    new_connection->SetMessageCallBack([](const std::shared_ptr<TcpConnection>& connection, std::string& strMsg){
        // Echo 服务：原样发送回去
        connection->Send(strMsg);
    });

    new_connection->SetCloseCallBack([](const std::shared_ptr<TcpConnection>& connection){
        std::cout << "Connection closed, fd=" << connection->GetFd() << std::endl;
        g_mapTcpConnection.erase(connection->GetFd());
    });

    g_mapTcpConnection.insert(std::make_pair(nClientFd, new_connection));
}


int main()
{
    // 1.创建socket
    if(!g_listenScoket.IsValid())
    {
        std::cerr << "listen_Socket is invalid " << std::endl;;
        return 1;
    }

    // 2.设置地址重用
    g_listenScoket.SetReuseAddr();

    // 3.绑定地址
    InetAddress addr(PORT);
    if(false == g_listenScoket.Bind(addr))
    {
         std::cerr << "listen_Socket bind error " << std::endl;;
         return 1;
    }


    // 4.监听
    if(false == g_listenScoket.Listen())
    {
        std::cerr << "listen_Socket listen error  " << std::endl;;
        return 1;
    }

   
    // 将监听socket加入到epoll中去。
    g_listenChannel = std::make_unique<Channel>(g_listenScoket.GetSocketId());
    g_listenChannel->SetReadCallBack(HandleNewConnection);
    g_listenChannel->EnableReading();
    
    g_loop.UpdateChannel(g_listenChannel.get());
    g_loop.Loop();
    return 0;
}
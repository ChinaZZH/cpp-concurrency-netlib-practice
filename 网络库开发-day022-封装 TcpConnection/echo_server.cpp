


#include "ListenSocket.h"
#include "ClientSocket.h"
#include "Epoll.h"
#include "Channel.h"
#include "EventLoop.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <map>
#include <vector>
#include <memory>
#include <cerrno>

#define PORT 8888

ListenSocket g_listenScoket;
EventLoop g_loop;
std::map<int, std::unique_ptr<ClientSocket>> g_clientSocketMap;
std::map<int, std::unique_ptr<Channel>> g_mapChannel;


void HandleCloseConnection(int nClientFd)
{
     auto itr = g_mapChannel.find(nClientFd);
     if(itr != g_mapChannel.end())
     {
        g_loop.RemoveChannel(itr->second.get());
        g_mapChannel.erase(itr);
     }

     g_clientSocketMap.erase(nClientFd);
}


// 读事件处理(边缘触发)
void HandleRead(int fd)
{
    auto itr = g_clientSocketMap.find(fd);
    if(itr == g_clientSocketMap.end())
    {
        std::cerr << "g_clientSocketMap find error fd:" << fd << std::endl;
        return ;
    }

    auto& pClient = (itr->second);
    if(false == pClient->IsValid())
    {
        std::cerr << "g_clientSocketMap find error \n";
        return;
    }
       

    // 6.0 接受并回发
    char buffer[4096] = {0};
    while(true)
    {
        memset(buffer, 0, sizeof(buffer));
        errno = 0;
        ssize_t n = pClient->Read(buffer, sizeof(buffer));
        int err = errno;   // 立即保存 errno
        if(n < 0)
        {
            // EAGAIN 或 EWOULDBLOCK 表示本次数据读完了（非阻塞模式）
            if(err == EAGAIN || err == EWOULDBLOCK) {
                break;
            }
            else{
                // 异常，则断开连接
                HandleCloseConnection(fd);
                break;
            }
        }else if(n == 0){
            // 对方关闭连接
            HandleCloseConnection(fd);
            break;
        }else{
            pClient->Write(buffer, n);
        }
    }
}


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
        auto itr = g_clientSocketMap.find(nClientFd);
        if(itr != g_clientSocketMap.end())
        {
            return ;
        }
    }
    

  
                
    // 将新连接加入epoll  
    auto clientChannel = std::make_unique<Channel>(nClientFd);
    clientChannel->SetReadCallBack(std::bind(HandleRead, nClientFd));
    clientChannel->EnableReading();
    clientChannel->EnableET();
    
    auto ptrClient = std::make_unique<ClientSocket>(nClientFd);
    ptrClient->SetNonBlock();

    g_loop.UpdateChannel(clientChannel.get());

    g_mapChannel.insert(std::make_pair(nClientFd, std::move(clientChannel)));
    g_clientSocketMap.insert(std::make_pair(nClientFd, std::move(ptrClient)));
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
    auto listenChannel = std::make_unique<Channel>(g_listenScoket.GetSocketId());
    listenChannel->SetReadCallBack(HandleNewConnection);
    listenChannel->EnableReading();
    
    
    g_loop.UpdateChannel(listenChannel.get());
    g_mapChannel.insert(std::make_pair(g_listenScoket.GetSocketId(), std::move(listenChannel)));

    g_loop.Loop();

    return 0;
}
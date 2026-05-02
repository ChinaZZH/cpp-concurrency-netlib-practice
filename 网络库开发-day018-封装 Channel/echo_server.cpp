


#include "ListenSocket.h"
#include "ClientSocket.h"
#include "Epoll.h"
#include "Channel.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <map>
#include <vector>
#include <memory>
#include <cerrno>

#define PORT 8888

ListenSocket g_listenScoket;
Epoll g_epoll;
std::map<int, std::unique_ptr<ClientSocket>> g_mapClientSocketList;
std::map<int, std::unique_ptr<Channel>> g_mapChannel;

void HandleCloseConnection(int nClientFd)
{
     g_mapChannel.erase(nClientFd);
     g_epoll.RemoveFd(nClientFd);
     g_mapClientSocketList.erase(nClientFd);
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
        auto itr = g_mapClientSocketList.find(nClientFd);
        if(itr != g_mapClientSocketList.end())
        {
            return ;
        }
    }
    

    auto HandleRead = [nClientFd](){
        auto itr = g_mapClientSocketList.find(nClientFd);
        if(itr == g_mapClientSocketList.end())
        {
            std::cerr << "g_mapClientSocketList find error fd:" << nClientFd << std::endl;
            return ;
        }

        auto& pClient = (itr->second);
        if(false == pClient->IsValid())
        {
            std::cerr << "g_mapClientSocketList find error \n";
            return ;
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
                    HandleCloseConnection(nClientFd);
                    break;
                }
            }else if(n == 0){
                // 对方关闭连接
                HandleCloseConnection(nClientFd);
                break;
            }else{
                pClient->Write(buffer, n);
            }
        }
    };
                
    // 将新连接加入epoll  
    auto clientChannel = std::make_unique<Channel>(nClientFd);
    clientChannel->SetReadCallBack(HandleRead);
    clientChannel->EnableReading();
    clientChannel->EnableET();
    
    auto ptrClient = std::make_unique<ClientSocket>(nClientFd);
    ptrClient->SetNonBlock();
    if(false == g_epoll.AddFd(clientChannel.get()))
    {
        std::cerr << "epoll_ctl add client_fd error client fd:" << nClientFd << std::endl;
        return ;
    }

    g_mapChannel.insert(std::make_pair(nClientFd, std::move(clientChannel)));
    g_mapClientSocketList.insert(std::make_pair(nClientFd, std::move(ptrClient)));
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

    // 创建epoll
    if(!g_epoll.IsValid())
    {
        std::cerr << "epoll_create error " << std::endl;;
        return 1;
    }

    // 将监听socket加入到epoll中去。
    auto listenChannel = std::make_unique<Channel>(g_listenScoket.GetSocketId());
    listenChannel->SetReadCallBack(HandleNewConnection);
    listenChannel->EnableReading();
    

    if(false == g_epoll.AddFd(listenChannel.get()))
    {
        std::cerr << "epoll_ctl add listenFd error listen fd:" << g_listenScoket.GetSocketId() << std::endl;
        return 1;
    }

    g_mapChannel.insert(std::make_pair(g_listenScoket.GetSocketId(), std::move(listenChannel)));

    // 5.接受连接并echo
    while(true)
    {
        std::vector<Channel*> activeChannels;
        errno = 0;
        int nfds = g_epoll.Wait(activeChannels);
        if(nfds < 0)
        {
            if (errno != EINTR) {
                std::cerr << "epoll_wait error\n";
                break;
            }

            continue;
        }

        for(auto& ptrChannel: activeChannels)
        {
            if(!ptrChannel)
            {
                continue;
            }

            ptrChannel->HandleEvent();
        }
    }

    return 0;
}
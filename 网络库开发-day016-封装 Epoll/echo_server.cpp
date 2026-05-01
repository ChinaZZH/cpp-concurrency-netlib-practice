


#include "ListenSocket.h"
#include "ClientSocket.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/epoll.h>
#include <map>
#include <memory>


#define MAX_EVENTS 10
#define PORT 8888

using namespace std;
int main()
{
    // 1.创建socket
    ListenSocket listen;

    if(!listen.IsValid())
    {
        std::cerr << "listen_Socket is invalid \n";
        return 1;
    }

    // 2.设置地址重用
    listen.SetReuseAddr();

    // 3.绑定地址
    InetAddress addr(PORT);
    if(false == listen.Bind(addr))
    {
         std::cerr << "listen_Socket bind error \n";
         return 1;
    }


    // 4.监听
    if(false == listen.Listen())
    {
        std::cerr << "listen_Socket listen error \n";
        return 1;
    }

    // 创建epoll
    int epollFd = epoll_create1(0);
    if(epollFd < 0)
    {
        std::cerr << "epoll_create error \n";
        return 1;
    }

    // 将监听socket加入到epoll中去。
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = listen.GetSocketId();
    if(epoll_ctl(epollFd, EPOLL_CTL_ADD, ev.data.fd, &ev) < 0)
    {
        std::cerr << "epoll_ctl add listenFd error \n";
        close(epollFd);
        return 1;
    }

    struct epoll_event events[MAX_EVENTS];


    // 5.接受连接并echo
    std::map<int, std::unique_ptr<ClientSocket>> m_mapClientSocketList;
    while(true)
    {
        int nfds = epoll_wait(epollFd, events, MAX_EVENTS, -1);
        if(nfds < 0)
        {
            std::cerr << "epoll_wait error" <<std::endl;
            break;
        }

        for(int i = 0; i < nfds; ++i)
        {
            int fd = events[i].data.fd;
            if(fd == listen.GetSocketId())
            {
                int nClientFd = listen.Accept();
                if(nClientFd < 0)
                {
                    std::cerr << "accept error \n";
                    continue;
                }

                auto itr = m_mapClientSocketList.find(nClientFd);
                if(itr != m_mapClientSocketList.end())
                {
                    continue;
                }

                
                // 将新连接加入epoll
                struct epoll_event clientEv;
                clientEv.events = EPOLLIN;      // 监听可读事件
                clientEv.data.fd = nClientFd;
                if(epoll_ctl(epollFd, EPOLL_CTL_ADD, nClientFd, &clientEv) < 0)
                {
                    ClientSocket client(nClientFd);
                    std::cerr << "epoll_ctl add listenFd error \n";
                    continue;
                }
                
                auto ptrClient = std::make_unique<ClientSocket>(nClientFd);
                ptrClient->SetNonBlock();
                m_mapClientSocketList.insert(std::make_pair(nClientFd, std::move(ptrClient)));

                //char ip[INET_ADDRSTRLEN];
                //inet_ntop(AF_INET, &clientAddr.sin_addr, ip, sizeof(ip));
                //std::cout << "New connection from " << ip << ":" << ntohs(clientAddr.sin_port) << std::endl;
            }
            else{

                auto itr = m_mapClientSocketList.find(fd);
                if(itr == m_mapClientSocketList.end())
                {
                   std::cerr << "m_mapClientSocketList find error fd:" << fd << std::endl;
                    continue;
                }

                auto& ptrClient = (itr->second);
                if(false == ptrClient->IsValid())
                {
                    std::cerr << "m_mapClientSocketList find error \n";
                    continue;
                }

                // 6.0 接受并回发
                char buffer[4096] = {0};
                while(true)
                {
                    memset(buffer, 0, sizeof(buffer));
                    ssize_t n = ptrClient->Read(buffer, sizeof(buffer));
                    if(n < 0)
                    {
                        // EAGAIN 或 EWOULDBLOCK 表示本次数据读完了（非阻塞模式）
                        if(errno == EAGAIN || errno == EWOULDBLOCK) {
                            
                        }
                        else{
                            //perror("read");
                            std::cout << "error to close connection n < 0" << std::endl;

                             // 异常，则断开连接
                            m_mapClientSocketList.erase(itr);
                            epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, nullptr);
                        }
                    
                       break;
                    }
                    else if(n == 0)
                    {
                        std::cout << "error to close connection n == 0" << std::endl;

                        // 对方关闭连接
                        m_mapClientSocketList.erase(itr);
                        epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, nullptr);
                        break;
                    }
                    else{
                        ptrClient->Write(buffer, n);
                    }
                }
                
            }
        }

    }

    close(epollFd);
    return 0;
}
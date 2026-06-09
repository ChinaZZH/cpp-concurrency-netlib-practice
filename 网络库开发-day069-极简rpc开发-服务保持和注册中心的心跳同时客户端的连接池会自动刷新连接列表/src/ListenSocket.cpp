#include "ListenSocket.h"
#include "Common/ConfigManager.h"
#include <iostream>
#include <cstring>
#include <netinet/in.h>
#include <arpa/inet.h>



 ListenSocket::ListenSocket()
 :socket_fd_(-1)
 {
    socket_fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if(socket_fd_ < 0){
        std::cerr << "ListenSocket socket error \n";
    }
 }


 ListenSocket::~ListenSocket()
 {
    if(socket_fd_ >= 0)
    {
        Close();
    }
 }


 ListenSocket::ListenSocket(ListenSocket&& other)   noexcept:socket_fd_(other.socket_fd_)
 {
    other.socket_fd_ = -1;
 }
 
 
ListenSocket& ListenSocket::operator=(ListenSocket&& other)   noexcept
{
    if(this != &other)
    {
        if(socket_fd_ > 0)
        {
            Close();
        }

        socket_fd_ = (other.socket_fd_);
        other.socket_fd_ = -1;;
    }

    return (*this);
}



void ListenSocket::SetReuseAddr(bool on /*= true*/)
{
    if(false == IsValid())
    {
        std::cerr << "ListenSocket::SetReuseAddr error \n";
        return;
    }

    int opt = on?1:0;
    setsockopt(socket_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
}
    

void ListenSocket::SetReusePort(bool on /*= true*/)
{
    if(false == IsValid())
    {
        std::cerr << "ListenSocket::SetReusePort error \n";
        return;
    }

    int opt = on?1:0;
    setsockopt(socket_fd_, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
}


bool ListenSocket::Bind(const InetAddress& addr)
{
    if(false == IsValid())
    {
        std::cerr << "ListenSocket::Bind error \n";
        return false;
    }


    if(bind(socket_fd_, (struct sockaddr*)addr.GetSockAddr(), addr.Len()) < 0)
    {
        std::cerr << "ListenSocket::Bind bind error \n";
        Close();
        return false;
    }

    return true;
}

    
bool ListenSocket::Listen()
{
    if(false == IsValid())
    {
        std::cerr << "ListenSocket::Listen error \n";
        return false;
    }

    if(listen(socket_fd_, 32768) < 0)
    {
        std::cerr << "ListenSocket::Listen listen error \n";
        Close();
        return false;
    }

    auto& cfg = ConfigManager::getInstance();
    int port = cfg.getInt("RegisterCenter", "port", 8080);

    std::cout << "Listening on port:" << port << " ....\n";
    return true;
}

int ListenSocket::Accept()
{
    if(false == IsValid())
    {
        std::cerr << "ListenSocket::Accept error \n";
        return -1;
    }

    sockaddr_in addr;
    socklen_t len = sizeof(addr);
    int connFd = ::accept(socket_fd_, (struct sockaddr*)(&addr), &len);
    if(connFd > 0)
    {
        InetAddress peerAddr(addr);
        //std::cout << "Accepted fd:=" << connFd <<  " connection from " << peerAddr.toIp() << ":" << peerAddr.toPort() << "\n";
    }

    return connFd;
}




void ListenSocket::Close()
{
    ::close(socket_fd_);
    socket_fd_ = -1;
}
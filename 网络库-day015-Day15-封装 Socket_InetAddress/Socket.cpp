#include "Socket.h"
#include <iostream>
#include <cstring>
#include <netinet/in.h>
#include <arpa/inet.h>


 Socket::Socket()
 :socket_fd_(-1)
 ,bListenFd(true)
 {
    socket_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if(socket_fd_ < 0){
        std::cerr << "socket error \n";
    }
 }


Socket::Socket(int fd)
 :socket_fd_(fd)
 ,bListenFd(false)
{

}



void Socket::SetNonBlock()
{
   


}
    

void Socket::SetReuseAddr()
{
    if(socket_fd_ < 0 || !bListenFd)
    {
        std::cerr << "Socket::SetNonBlock error \n";
        return;
    }

    int opt = 1;
    setsockopt(socket_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
}
    
bool Socket::Bind(InetAddress& addr)
{
    if(socket_fd_ < 0 || !bListenFd)
    {
        std::cerr << "Socket::Bind error \n";
        return false;
    }


    if(bind(socket_fd_, addr.GetSockAddr(), addr.Len()) < 0)
    {
        std::cerr << "Socket::Bind bind error \n";
        Close();
        return false;
    }

    return true;
}

    
bool Socket::Listen()
{
    if(socket_fd_ < 0 || !bListenFd)
    {
        std::cerr << "Socket::Listen error \n";
        return false;
    }

    if(listen(socket_fd_, 128) < 0)
    {
        std::cerr << "Socket::Listen listen error \n";
        Close();
        return false;
    }

    std::cout << "Listening on port 8888 ....\n";
    return true;
}

ssize_t Socket::Accept()
{
    if(socket_fd_ < 0 || !bListenFd)
    {
        std::cerr << "Socket::Accept error \n";
        return -1;
    }

    InetAddress addr;
    socklen_t len = addr.Len();
    int connFd = accept(socket_fd_, addr.GetSockAddr(), &len);
    if(connFd > 0)
    {
        char ip[INET_ADDRSTRLEN] = {0};
        int nPort = addr.InetToHost(ip);

        std::cout << "Accepted connection from " << ip << ":" << nPort << "\n";
    }

    return connFd;
}


ssize_t Socket::Read(char* szBuffer, int nCount)
{
    if(socket_fd_ < 0 || bListenFd)
    {
        std::cerr << "Socket::Read error \n";
        return -1;
    }

    ssize_t nReadResult = read(socket_fd_, szBuffer, nCount);
    if(nReadResult <= 0)
    {
        Close();
    }

    return nReadResult;
}

bool Socket::Write(char* szBuffer, int nCount)
{
    if(socket_fd_ < 0 || bListenFd)
    {
        std::cerr << "Socket::Write error \n";
        return false;
    }

    write(socket_fd_, szBuffer, nCount);
    return true;
}


void Socket::Close()
{
    close(socket_fd_);
    if(false == bListenFd)
    {
        std::cout << "Connection closed\n";
    }
}
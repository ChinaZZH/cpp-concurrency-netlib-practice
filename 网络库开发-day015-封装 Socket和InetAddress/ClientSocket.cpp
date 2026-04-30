#include "ClientSocket.h"
#include <iostream>
#include <cstring>
#include <netinet/in.h>
#include <arpa/inet.h>




ClientSocket::ClientSocket(int fd)
 :socket_fd_(fd)
{
    if(socket_fd_ < 0){
        std::cerr << "ClientSocket socket error \n";
    }
}

 ClientSocket::~ClientSocket()
 {
    if(socket_fd_ >= 0)
    {
        Close();
    }
 }


 ClientSocket::ClientSocket(ClientSocket&& other)   noexcept:socket_fd_(other.socket_fd_)
 {
    other.socket_fd_ = -1;
 }
 
 
ClientSocket& ClientSocket::operator=(ClientSocket&& other)   noexcept
{
    if(this != &other)
    {
        if(socket_fd_ > 0)
        {
            Close(false);
        }

        socket_fd_ = (other.socket_fd_);

        other.socket_fd_ = -1;
    }

    return (*this);
}


ssize_t ClientSocket::Read(char* buf, size_t len)
{
    if(false == IsValid() || !buf)
    {
        std::cerr << "ClientSocket::Read error \n";
        return -1;
    }

    ssize_t nReadResult = ::read(socket_fd_, buf, len);
    if(nReadResult <= 0)
    {
        Close();
    }

    return nReadResult;
}


ssize_t ClientSocket::Write(const char* buf, size_t len)
{
    if(false == IsValid() || !buf || len <= 0)
    {
        std::cerr << "ClientSocket::Write error \n";
        return -1;
    }

    return ::write(socket_fd_, buf, len);
}


void ClientSocket::Close(bool bConsoleInfo /*= true*/)
{
    close(socket_fd_);
    socket_fd_ = -1;

    if(bConsoleInfo)
    {
        std::cout << "Connection closed\n";
    }
}
#include "ClientSocket.h"
#include <iostream>
#include <cstring>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>



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


void ClientSocket::SetNonBlock()
{
    if(false == IsValid())
    {
        std::cerr << "ClientSocket::SetNonBlock error \n";
        return;
    }

    
    int flags = ::fcntl(socket_fd_, F_GETFL, 0);
    flags |= O_NONBLOCK;
    ::fcntl(socket_fd_, F_SETFL, flags);
}

ssize_t ClientSocket::Read(char* buf, size_t len)
{
    if(false == IsValid() || !buf)
    {
        std::cerr << "ClientSocket::Read error \n";
        return -1;
    }

    return ::read(socket_fd_, buf, len);
}


bool ClientSocket::Write(const char* buf, size_t len)
{
    if(false == IsValid() || !buf || len <= 0)
    {
        std::cerr << "ClientSocket::Write error \n";
        return false;
    }

    size_t total = 0;
    while(total < len)
    {
        ssize_t write_len = ::write(socket_fd_, buf + total, len - total);
        if(write_len < 0)
        {
            if(errno == EAGAIN || errno == EWOULDBLOCK) {
                // 这里需要缓存未发送的数据，并注册 EPOLLOUT 事件（略复杂）
                // 简单处理：返回 false，数据丢失
                return false;
            }

            return false;
        }

        total += write_len;
    }


    return true;
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
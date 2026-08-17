#include "ClientSocket.h"
#include <iostream>
#include <cstring>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/tcp.h>


ClientSocket::ClientSocket(int fd, bool tcpClient /*= false*/)
 :socket_fd_(fd)
 ,bTcpClient_(tcpClient)
{
    if(socket_fd_ < 0){
        std::cerr << "ClientSocket socket error socket_fd:=" << socket_fd_ << std::endl;
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
            Close();
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

// 现在已经没用了屏蔽掉
/*
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
                // 后续会进行优化
                return false;
            }

            return false;
        }

        total += write_len;
    }


    return true;
}
*/

void ClientSocket::Close()
{
    close(socket_fd_);
    socket_fd_ = -1;
}


int ClientSocket::Connect(const std::string& strIp, int nPort, bool bNonBlocked /*= true*/)
{
    int connectFd = -1;
    connectFd = socket(AF_INET, SOCK_STREAM, 0);
    if (connectFd < 0)
    {
        std::cerr << "ClientSocket socket error \n";
        return -1;
    }
    
    // 默认使用非阻塞
    if(bNonBlocked)
    {
        int flags = ::fcntl(connectFd, F_GETFL, 0);
        flags |= O_NONBLOCK;
        ::fcntl(connectFd, F_SETFL, flags);
    }

    InetAddress addr(strIp, nPort);
    int result = ::connect(connectFd, (struct sockaddr*)addr.GetSockAddr(), sizeof(addr));
    if(result < 0 && errno != EINPROGRESS)
    {
        ::close(connectFd);
        std::cerr << "ClientSocket connect error \n";
        return -1;
    }   

    return connectFd;
}

void ClientSocket::DisableNagele()
{
    if(socket_fd_ < 0)
    {
        return;
    }
    
    int flag = 1;
    setsockopt(socket_fd_, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
}


void ClientSocket::SetTimeout(int sec)
{
    if(socket_fd_ < 0)
    {
        return;
    }

    struct timeval tv = {sec, 0};
    setsockopt(socket_fd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(socket_fd_, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
}


bool ClientSocket::BlockedWriteAll(const std::string& request)
{
    if(socket_fd_ < 0)
    {
        return false;
    }

    int flags = ::fcntl(socket_fd_, F_GETFL, 0);
    int flag_result = (flags & O_NONBLOCK);
    if(flag_result > 0)
    {
        std::cerr << "ClientSocket::BlockedWriteAll nonBlcok" << std::endl;
        return false;
    }

    //std::cout << "Sending request:\n" << request << std::endl;
    ssize_t sent = write(socket_fd_, request.c_str(), request.size());
    if(sent != request.size())
    {
        //res.error_msg = "send failed: " + std::string(strerror(errno));
        return false;
    }

    //std::cout << "Sent " << sent << " bytes" << std::endl;
    return true;
}
    
std::string ClientSocket::BlockedReadAll()
{
    std::string response;
     if(socket_fd_ < 0)
    {
        return response;
    }

    int flags = ::fcntl(socket_fd_, F_GETFL, 0);
    int flag_result = (flags & O_NONBLOCK);
    if(flag_result > 0)
    {
        std::cerr << "ClientSocket::BlockedReadAll nonBlcok" << std::endl;
        return response;
    }

    char buf[4096] = {0};
    ssize_t n;
    while ((n = ::read(socket_fd_, buf, sizeof(buf))) > 0) {
        response.append(buf, n);
        // std::cout << "Read " << n << " bytes, total now " << response.size() << std::endl;
    }

    //std::cout << "Read finished, errno=" << errno << std::endl;
    return response;
}
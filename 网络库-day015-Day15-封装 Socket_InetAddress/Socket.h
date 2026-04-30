#pragma once

#include "InetAddress.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>

class Socket
{
public:
    // 创建监听线程
    Socket();

    // 创建连接线程
    explicit Socket(int fd);

    // 监听socket
public:
    void SetNonBlock();
    
    void SetReuseAddr();
    
    bool Bind(InetAddress& addr);

    bool Listen();

    ssize_t Accept();

    // 连接socket 客户端的socket_fd
public:
    ssize_t Read(char* szBuffer, int nCount);

    bool Write(char* szBuffer, int nCount);

    // 两者通用的函数
public:
    void Close();

private:
    int socket_fd_ = -1;
    bool bListenFd = false;
};


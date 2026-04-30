#pragma once

#include "InetAddress.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>

class ListenSocket
{
public:
    // 创建监听线程
    ListenSocket();

    ~ListenSocket();

public:
    ListenSocket(const ListenSocket& socket)                = delete;
    ListenSocket& operator=(const ListenSocket& socket)     = delete;

    ListenSocket(ListenSocket&& socket)              noexcept;
    ListenSocket& operator=(ListenSocket&& socket)   noexcept;

    // 监听socket
public:
    void SetNonBlock();
    
    void SetReuseAddr(bool on = true);

    void SetReusePort(bool on = true);
    
    bool Bind(const InetAddress& addr);

    bool Listen();

    int Accept();

    void Close();

    bool IsValid() { return socket_fd_ >= 0;}

private:
    int socket_fd_ = -1;
};


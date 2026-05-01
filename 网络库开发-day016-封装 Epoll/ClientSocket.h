#pragma once

#include "InetAddress.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>

class ClientSocket
{
public:
    explicit ClientSocket(int fd);

     ~ClientSocket();

public:
    ClientSocket(const ClientSocket& socket)                = delete;
    ClientSocket& operator=(const ClientSocket& socket)     = delete;

    ClientSocket(ClientSocket&& socket)              noexcept;
    ClientSocket& operator=(ClientSocket&& socket)   noexcept;

public:
    ssize_t Read(char* buf, size_t len);

    ssize_t Write(const char* buf, size_t len);

    void Close(bool bShowConsole = true);

    bool IsValid() { return socket_fd_ >= 0;}

    void SetNonBlock();
    
private:
    int socket_fd_ = -1;
};


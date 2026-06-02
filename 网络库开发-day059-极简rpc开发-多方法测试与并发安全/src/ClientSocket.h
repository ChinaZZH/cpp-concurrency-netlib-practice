#pragma once

#include "InetAddress.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <string>

class ClientSocket
{
public:
    explicit ClientSocket(int fd, bool tcpClient = false);

     ~ClientSocket();

public:
    ClientSocket(const ClientSocket& socket)                = delete;
    ClientSocket& operator=(const ClientSocket& socket)     = delete;

    ClientSocket(ClientSocket&& socket)              noexcept;
    ClientSocket& operator=(ClientSocket&& socket)   noexcept;

public:
    ssize_t Read(char* buf, size_t len);

    bool Write(const char* buf, size_t len);

    void Close();

    bool IsValid() { return socket_fd_ >= 0;}

    void SetNonBlock();

    static int Connect(const std::string& strIp, int nPort);

    int GetSocketFd() const { return socket_fd_; }

    bool IsTcpClient() const { return bTcpClient_; }

private:
    int socket_fd_ = -1;
    bool bTcpClient_ = false;
};


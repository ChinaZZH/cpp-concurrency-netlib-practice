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
    // 现在已经没用了屏蔽掉
    //ssize_t Read(char* buf, size_t len);

    //bool Write(const char* buf, size_t len);

    void Close();

    bool IsValid() { return socket_fd_ >= 0;}

    void SetNonBlock();

    static int Connect(const std::string& strIp, int nPort, bool bNonBlocked = true);

    int GetSocketFd() const { return socket_fd_; }

    bool IsTcpClient() const { return bTcpClient_; }

    void DisableNagele();
    
    void SetTimeout(int sec);

    bool BlockedWriteAll(const std::string& request);
    
    std::string BlockedReadAll();

private:
    int socket_fd_ = -1;
    bool bTcpClient_ = false;
};


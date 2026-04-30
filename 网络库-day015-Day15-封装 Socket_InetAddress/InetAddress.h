#pragma once

#include <netinet/in.h>
#include <cstdint>
#include <string>

class InetAddress
{
public:
    InetAddress();
    
    explicit InetAddress(int nPort);

public:
    sockaddr* GetSockAddr() { return (sockaddr*)&addr_;}

    socklen_t Len() { return sizeof(addr_); }

    int InetToHost(char* szIpBuff);

public:
    sockaddr_in addr_;
};

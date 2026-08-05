#pragma once

#include <netinet/in.h>
#include <cstdint>
#include <string>

class InetAddress
{
public:
    explicit InetAddress(int nPort);

    explicit InetAddress(const std::string& ip, int nPort);

    explicit InetAddress(const sockaddr_in& addr);

public:
    const sockaddr_in* GetSockAddr() const { return &addr_;}
    sockaddr_in* GetSockAddr()  { return &addr_;}

    socklen_t Len() const { return sizeof(addr_); }

    std::string toIp() const;
    int toPort() const;

public:
    sockaddr_in addr_;
};

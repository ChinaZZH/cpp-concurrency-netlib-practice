#include "InetAddress.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>


InetAddress::InetAddress(int nPort)
{
    memset(&addr_, 0, sizeof(addr_));
    addr_.sin_family = AF_INET;
    addr_.sin_addr.s_addr = htonl(INADDR_ANY);
    addr_.sin_port = htons(nPort);
}

InetAddress::InetAddress(const sockaddr_in& addr)
:addr_(addr)
{

}

std::string InetAddress::toIp() const
{
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr_.sin_addr, ip, INET_ADDRSTRLEN);
    return std::string(ip);
}
    

int InetAddress::toPort() const
{
    return ntohs(addr_.sin_port);
}


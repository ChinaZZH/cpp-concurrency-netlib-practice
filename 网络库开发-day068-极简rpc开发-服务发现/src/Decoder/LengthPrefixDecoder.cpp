#include "LengthPrefixDecoder.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>

static const size_t MAX_PACKET_SIZE = 10 * 1024 * 1024; // 10MB

bool LengthPrefixDecoder::Decode(Buffer& input, std::string& msg) 
{
    // 需要至少4字节读取包头长度
    if(input.ReadableBytes() < sizeof(uint32_t))
    {
        return false;
    }

    // 读取包头长度，并且将网络序转化为主机序
   uint32_t netLen = 0;
   ::memcpy(&netLen, input.Peek(), sizeof(uint32_t));
   uint32_t msg_len = ntohl(netLen);

    // 合法性检查
    if(0 == msg_len || msg_len > MAX_PACKET_SIZE)
    {
        // 非法连接，关闭
        //HandleClose("Invalid packet length"); // 先屏蔽后期看怎么穿给tcpConnection
        return false;
    }

    // 检查数据是否足够, 不够则继续等待
    if(input.ReadableBytes() < (msg_len + sizeof(uint32_t)))
    {
        return false;
    }

    // 足够，则进行解包，跳过包头
    input.Retrieve(sizeof(uint32_t));

    msg.assign(input.Peek(), msg_len);
    input.Retrieve(msg_len);
    return true;
}
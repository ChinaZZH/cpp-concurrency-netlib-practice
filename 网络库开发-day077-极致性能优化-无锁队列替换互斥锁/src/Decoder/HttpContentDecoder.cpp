#include "HttpContentDecoder.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>

// 当前只解析头部的 head. 没解析body的
bool HttpContentDecoder::Decode(Buffer& input, std::string& msg)
{
    if(input.ReadableBytes() <= 4)
    {
        return false;
    }

    // 1.查找头部结束标志("\r\n\r\n")
    const char* header_end = FindHttpDelimiter(input);
    if(!header_end)
    {
        return false;
    }


    // 2.计算出头部长度和字符串(包括 "\r\n\r\n")
    size_t header_len = (header_end - input.Peek()) + 4;
    std::string header(input.Peek(), header_len);

    // 解析出body的长度
    size_t body_length = 0;
    const char* body_len_start = strstr(header.c_str(), "Content-Length:");
    if(body_len_start)
    {
        body_len_start = body_len_start + 15;
        while(' ' == (*body_len_start) || '\t' == (*body_len_start))
        {
            body_len_start = body_len_start + 1;
        }

        body_length = std::strtoul(body_len_start, nullptr, 10);
    }

    // 总的长度
    size_t total_length = header_len + body_length;
     if(input.ReadableBytes() < total_length)
    {
        return false;
    }

    msg.assign(input.Peek(), total_length);
    input.Retrieve(total_length);
    return true;
}


const char* HttpContentDecoder::FindHttpDelimiter(const Buffer& input)
{
    // 查找 "\r\n\r\n"
    int len = input.ReadableBytes();
    const char* data = input.Peek();
    if(!data || len <= 0)
    {
        return nullptr;
    }

    const char* p = static_cast<const char*>(memchr(data, '\r', len));
    while (p && p + 3 < data + len) {
        if (p[1] == '\n' && p[2] == '\r' && p[3] == '\n')
            return p;

        p = static_cast<const char*>(memchr(p + 1, '\r', data + len - (p + 1)));
    }

    return nullptr;
}
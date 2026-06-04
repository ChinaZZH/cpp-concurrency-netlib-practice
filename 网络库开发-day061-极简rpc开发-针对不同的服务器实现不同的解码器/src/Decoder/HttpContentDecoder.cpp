#include "HttpContentDecoder.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>

// 当前只解析头部的 head. 没解析body的
bool HttpContentDecoder::Decode(Buffer& input, std::string& msg)
{
    const char* crlf = FindHttpDelimiter(input);
    if(!crlf)
    {
        return false;
    }

    size_t len = crlf - input.Peek() + 4;
    msg.assign(input.Peek(), len);
    input.Retrieve(len);
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
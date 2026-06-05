#include "LineDecoder.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>


bool LineDecoder::Decode(Buffer& input, std::string& msg)
{
    const char* crlf = static_cast<const char*>(memchr(input.Peek(), '\n', input.ReadableBytes()));
    if(!crlf)
    {
        return false;
    }

    size_t len = crlf - input.Peek() + 1;
    msg.assign(input.Peek(), len);
    input.Retrieve(len);

    if(!msg.empty() && '\n'== msg.back())
    {
        msg.pop_back();
    }

    if(!msg.empty() && '\r'== msg.back())
    {
        msg.pop_back();
    }

    return true;
}
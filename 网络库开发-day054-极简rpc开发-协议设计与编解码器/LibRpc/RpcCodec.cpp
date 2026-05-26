#include "RpcCodec.h"
#include <arpa/inet.h> // htonl, ntohl
#include <cstring>


void RpcCodec::WriteInt32(Buffer& buffer, int32_t value)
{
    int32_t netValue = htonl(value);

    char strData[4] = {0};
    memcpy(strData, &netValue, sizeof(strData));
    buffer.Append(strData, sizeof(strData));
}


bool RpcCodec::ReadInt32(Buffer& buffer, int32_t& value)
{
    int nIntSize = sizeof(value);
    if(buffer.ReadableBytes() < nIntSize)
    {
        return false;
    }

    int32_t netValue = 0;
    memcpy(&netValue, buffer.Peek(), nIntSize);
    buffer.Retrieve(nIntSize);
    value = ntohl(netValue);
    return true;
}


void RpcCodec::WriteString(Buffer& buffer, const std::string& strValue)
{
    int len = strValue.size();
    WriteInt32(buffer, len);
    if(!strValue.empty())
    {
        buffer.Append(strValue.c_str(), strValue.size());
    }
}


bool RpcCodec::ReadString(Buffer& buffer, std::string& strValue)
{
    int32_t len = 0;
    if(!RpcCodec::ReadInt32(buffer, len))
    {
        return false;
    }

    if(len < 0)
    {
        return false;
    }

    if(len > 0)
    {
        if(buffer.ReadableBytes() < len)
        {
            return false;
        }

        strValue.assign(buffer.Peek(), len);
        buffer.Retrieve(len);
    }

    return true;
}


 // 编码请求
void RpcCodec::EncodeRequest(Buffer& buffer, uint32_t id, const std::string& method, const std::string& params)
{
    RpcCodec::WriteInt32(buffer, id);
    RpcCodec::WriteString(buffer, method);
    RpcCodec::WriteString(buffer, params);
}

// 解码请求， 成功则返回true 并且填充 id, method， params; 失败则返回false (数据不足或者格式错误)
bool RpcCodec::DecodeRequest(Buffer& buffer, uint32_t& id, std::string& method, std::string& params)
{
    int32_t decodeId = 0;
    if(!RpcCodec::ReadInt32(buffer, decodeId))
    {
        return false;
    }

    id = static_cast<uint32_t>(decodeId);
    if(!RpcCodec::ReadString(buffer, method))
    {
        return false;
    }

    if(!RpcCodec::ReadString(buffer, params))
    {
        return false;
    }

    return true;
}


// 编码响应
void RpcCodec::EncodeResponse(Buffer& buffer, uint32_t id, int32_t code, const std::string& result)
{
    RpcCodec::WriteInt32(buffer, id);
    RpcCodec::WriteInt32(buffer, code);
    RpcCodec::WriteString(buffer, result);
}

// 解码响应  成功返回true; 失败则返回false
bool RpcCodec::DecodeResponse(Buffer& buffer, uint32_t& id, int32_t& code, std::string& result)
{
    int32_t decodeId = 0;
    if(!RpcCodec::ReadInt32(buffer, decodeId))
    {
        return false;
    }

    id = static_cast<uint32_t>(decodeId);
    if(!RpcCodec::ReadInt32(buffer, code))
    {
        return false;
    }

    if(!RpcCodec::ReadString(buffer, result))
    {
        return false;
    }

    return true;
}
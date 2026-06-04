#include "RpcCodec.h"
#include <arpa/inet.h> // htonl, ntohl
#include <cstring>
#include <iostream>

const size_t MAX_METHOD_SIZE = 1024 * 1024; // 1MB
const size_t MAX_PARAM_SIZE = 10 * 1024 * 1024; // 10MB
static const size_t MAX_STRING_SIZE = 10 * 1024 * 1024; // 10MB

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


void RpcCodec::WriteInt64(Buffer& buffer, int64_t value)
{
    int64_t netValue = htonl(value);

    char strData[8] = {0};
    memcpy(strData, &netValue, sizeof(strData));
    buffer.Append(strData, sizeof(strData));
}


bool RpcCodec::ReadInt64(Buffer& buffer, int64_t& value)
{
    int nIntSize = sizeof(value);
    if(buffer.ReadableBytes() < nIntSize)
    {
        return false;
    }

    int64_t netValue = 0;
    memcpy(&netValue, buffer.Peek(), nIntSize);
    buffer.Retrieve(nIntSize);
    value = ntohl(netValue);
    return true;
}

void RpcCodec::WriteString(Buffer& buffer, const std::string& strValue)
{
    int32_t len = strValue.size();
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
        if(len >  MAX_STRING_SIZE)
        {
            return false;
        }
        
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
void RpcCodec::EncodeRequest(Buffer& buffer, uint64_t id, const std::string& method, const std::string& params, uint64_t test_client_id)
{
    RpcCodec::WriteInt64(buffer, id);
    RpcCodec::WriteString(buffer, method);
    RpcCodec::WriteString(buffer, params);
}

// 解码请求， 成功则返回true 并且填充 id, method， params; 失败则返回false (数据不足或者格式错误)
bool RpcCodec::DecodeRequest(Buffer& buffer, uint64_t& id, std::string& method, std::string& params)
{
    int64_t tmpId = 0;
    if(!RpcCodec::ReadInt64(buffer, tmpId))
    {
        return false;
    }

    std::string strTmpMethod;
    if(!RpcCodec::ReadString(buffer, strTmpMethod))
    {
        return false;
    }

    if (strTmpMethod.size() > MAX_METHOD_SIZE)
    {
        return false;
    }
    
    std::string strTmpParam;
    if(!RpcCodec::ReadString(buffer, strTmpParam))
    {
        return false;
    }

    if(strTmpParam.size() > MAX_PARAM_SIZE)
    {
        return false;
    }

    id = tmpId;
    method = std::move(strTmpMethod);
    params = std::move(strTmpParam);
    return true;
}


// 编码响应
void RpcCodec::EncodeResponse(Buffer& buffer, uint64_t id, int32_t code, const std::string& result)
{
    RpcCodec::WriteInt64(buffer, id);
    RpcCodec::WriteInt32(buffer, code);
    RpcCodec::WriteString(buffer, result);
}

// 解码响应  成功返回true; 失败则返回false
bool RpcCodec::DecodeResponse(Buffer& buffer, uint64_t& id, int32_t& code, std::string& result)
{
    int64_t tmpId = 0;
    if(!RpcCodec::ReadInt64(buffer, tmpId))
    {
        return false;
    }

    
    int32_t tmpCode = 0;
    if(!RpcCodec::ReadInt32(buffer, tmpCode))
    {
        return false;
    }

    std::string strTmpResult;
    if(!RpcCodec::ReadString(buffer, strTmpResult))
    {
        return false;
    }

    id = tmpId;
    code = tmpCode;
    result = std::move(strTmpResult);
    return true;
}
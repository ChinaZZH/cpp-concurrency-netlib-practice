#include "RpcCodec.h"
#include <arpa/inet.h> // htonl, ntohl
#include <cstring>
#include <iostream>
#include "../../build/proto_gen/rpc.pb.h"

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
void RpcCodec::EncodeRequest_Protobuf(Buffer& buffer, uint64_t id, const std::string& method, const std::string& params)
{
    /*
    Buffer tmpBuff;
    RpcCodec::WriteInt64(tmpBuff, id);
    RpcCodec::WriteString(tmpBuff, method);
    RpcCodec::WriteString(tmpBuff, params);


    uint32_t len = tmpBuff.ReadableBytes();
    */

    RpcRequest request;
    request.set_id(id);
    request.set_method(method);
    request.set_args(params);

    std::string strData;
    request.SerializeToString(&strData);

    uint32_t len = strData.size();
    uint32_t net_len = htonl(len);
    buffer.Append(reinterpret_cast<const char*>(&net_len), sizeof(net_len));
    buffer.Append(strData.c_str(), len);
}




bool RpcCodec::DecodeRequest_Protobuf(const std::string& strBuffData, uint64_t& id, std::string& method, std::string& params)
{
    RpcRequest request;
    request.ParseFromString(strBuffData);
    id = request.id();
    method = std::move(request.method());
    params = std::move(request.args());
    return true;
}

// 编码响应
void RpcCodec::EncodeResponse_Protobuf(Buffer& buffer, uint64_t id, int32_t code, const std::string& result)
{
    /*
    Buffer tmpBuff;
    RpcCodec::WriteInt64(tmpBuff, id);
    RpcCodec::WriteInt32(tmpBuff, code);
    RpcCodec::WriteString(tmpBuff, result);

    uint32_t len = tmpBuff.ReadableBytes();
    */

    RpcResponse response;
    response.set_id(id);
    response.set_code(code);
    response.set_result(result);

    std::string strData;
    response.SerializeToString(&strData);

    uint32_t len = strData.size();
    uint32_t net_len = htonl(len);
    buffer.Append(reinterpret_cast<const char*>(&net_len), sizeof(net_len));
    buffer.Append(strData.c_str(), len);
}



// 解码响应  成功返回true; 失败则返回false
bool RpcCodec::DecodeResponse_Protobuf(const std::string& strBuffData, uint64_t& id, int32_t& code, std::string& result)
{
    RpcResponse response;
    response.ParseFromString(strBuffData);
    id = response.id();
    code = response.code();
    result = std::move(response.result());
    return true;
}


 // 编码请求
void RpcCodec::EncodeRequest(Buffer& buffer, uint64_t id, const std::string& method, const std::string& params)
{
    
    Buffer tmpBuff;
    RpcCodec::WriteInt64(tmpBuff, id);
    RpcCodec::WriteString(tmpBuff, method);
    RpcCodec::WriteString(tmpBuff, params);

    uint32_t len = tmpBuff.ReadableBytes();
    
    /*
    RpcRequest request;
    request.set_id(id);
    request.set_method(method);
    request.set_args(params);

    std::string strData;
    request.SerializeToString(&strData);

    uint32_t len = strData.size();
    */

    uint32_t net_len = htonl(len);
    buffer.Append(reinterpret_cast<const char*>(&net_len), sizeof(net_len));
    buffer.Append(tmpBuff.Peek(), len);
}

// 编码响应
void RpcCodec::EncodeResponse(Buffer& buffer, uint64_t id, int32_t code, const std::string& result)
{
    
    Buffer tmpBuff;
    RpcCodec::WriteInt64(tmpBuff, id);
    RpcCodec::WriteInt32(tmpBuff, code);
    RpcCodec::WriteString(tmpBuff, result);

    uint32_t len = tmpBuff.ReadableBytes();
    
    /*
    RpcResponse response;
    response.set_id(id);
    response.set_code(code);
    response.set_result(result);

    std::string strData;
    response.SerializeToString(&strData);

    uint32_t len = strData.size();
    */

    uint32_t net_len = htonl(len);
    buffer.Append(reinterpret_cast<const char*>(&net_len), sizeof(net_len));
    buffer.Append(tmpBuff.Peek(), len);
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
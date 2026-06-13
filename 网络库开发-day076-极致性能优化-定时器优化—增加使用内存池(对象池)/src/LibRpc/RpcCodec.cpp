#include "RpcCodec.h"
#include "RpcErrorCodeDef.h"
#include <arpa/inet.h> // htonl, ntohl
#include <cstring>
#include <iostream>
#include <sstream>
#include "../../build/proto_gen/rpc.pb.h"
#include "../Common/ConfigManager.h"
#include "../Common/LogFile.h"

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
std::string RpcCodec::Protobuf_EncodeRequest(uint64_t id, const std::string& method, const std::string& params, const std::string& trace_id)
{
    // 开启日志链路追踪
    /*
    {
        std::ostringstream file_stream;
        file_stream << "[trace_id=" << trace_id << "] Seq_id=" << id << " RpcClient Send method=" << method << std::endl;

        auto& cfg = ConfigManager::getInstance();
        std::string log_file_name = cfg.getString("Trace", "trace_log_name", "trace_log");
    
        auto& logfile = LogFile::getInstance();
        logfile.AppendContent(log_file_name, file_stream.str());
    }
    */

    RpcRequest request;
    request.set_id(id);
    request.set_method(method);
    request.set_args(params);
    request.set_trace_id(trace_id);

    std::string strData;
    request.SerializeToString(&strData);

    uint32_t len = strData.size();
    uint32_t net_len = htonl(len);
    
    std::string strEncodeResult;
    strEncodeResult.reserve(len + sizeof(net_len));
    strEncodeResult.append(reinterpret_cast<const char*>(&net_len), sizeof(net_len));
    strEncodeResult.append(strData);
    return strEncodeResult;
}




bool RpcCodec::Protobuf_DecodeRequest(const std::string& strBuffData, uint64_t& id, std::string& method, std::string& params,  std::string& trace_id)
{
    RpcRequest request;
    request.ParseFromString(strBuffData);
    id = request.id();
    method = std::move(request.method());
    params = std::move(request.args());
    trace_id = std::move(request.trace_id());

     // 开启日志链路追踪
     /*
    {
        std::ostringstream file_stream;
        file_stream << "[trace_id=" << trace_id << "] Seq_id=" << id << " RpcServer receive method=" << method << std::endl;

        auto& cfg = ConfigManager::getInstance();
        std::string log_file_name = cfg.getString("Trace", "trace_log_name", "trace_log");
    
        auto& logfile = LogFile::getInstance();
        logfile.AppendContent(log_file_name, file_stream.str());
    }
    */

    return true;
}

// 编码响应
std::string RpcCodec::Protobuf_EncodeResponse(uint64_t id, int32_t code, const std::string& result, const std::string& trace_id)
{
    // 开启日志链路追踪
    /*
    {
        std::ostringstream file_stream;
        file_stream << "[trace_id=" << trace_id << "]  Seq_id=" << id << " RpcServer Send code= " << code << std::endl;

        auto& cfg = ConfigManager::getInstance();
        std::string log_file_name = cfg.getString("Trace", "trace_log_name", "trace_log");
    
        auto& logfile = LogFile::getInstance();
        logfile.AppendContent(log_file_name, file_stream.str());
    }
    */

    RpcResponse response;
    response.set_id(id);
    response.set_code(code);
    response.set_result(result);
    response.set_trace_id(trace_id);

    std::string strData;
    response.SerializeToString(&strData);

    uint32_t len = strData.size();
    uint32_t net_len = htonl(len);

    std::string strEncodeResult;
    strEncodeResult.reserve(len + sizeof(net_len));
    strEncodeResult.append(reinterpret_cast<const char*>(&net_len), sizeof(net_len));
    strEncodeResult.append(strData);
    return strEncodeResult;
}



// 解码响应  成功返回true; 失败则返回false
bool RpcCodec::Protobuf_DecodeResponse(const std::string& strBuffData, uint64_t& id, int32_t& code, std::string& result, std::string& trace_id)
{
    RpcResponse response;
    response.ParseFromString(strBuffData);
    id = response.id();
    code = response.code();
    result = std::move(response.result());
    trace_id = std::move(response.trace_id());

     // 开启日志链路追踪
    /*
    {
        std::ostringstream file_stream;
        file_stream << "[trace_id=" << trace_id << "]  Seq_id=" << id << " RpcClient receive code= " << code << std::endl;

        auto& cfg = ConfigManager::getInstance();
        std::string log_file_name = cfg.getString("Trace", "trace_log_name", "trace_log");
    
        auto& logfile = LogFile::getInstance();
        logfile.AppendContent(log_file_name, file_stream.str());
    }
    */

    return true;
}


 // 编码请求
void RpcCodec::EncodeRequest(Buffer& buffer, uint64_t id, const std::string& method, const std::string& params, const std::string& trace_id)
{
    
    Buffer tmpBuff;
    RpcCodec::WriteInt64(tmpBuff, id);
    RpcCodec::WriteString(tmpBuff, method);
    RpcCodec::WriteString(tmpBuff, params);
    RpcCodec::WriteString(tmpBuff, trace_id);

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
void RpcCodec::EncodeResponse(Buffer& buffer, uint64_t id, int32_t code, const std::string& result, const std::string& trace_id)
{
    
    Buffer tmpBuff;
    RpcCodec::WriteInt64(tmpBuff, id);
    RpcCodec::WriteInt32(tmpBuff, code);
    RpcCodec::WriteString(tmpBuff, result);
    RpcCodec::WriteString(tmpBuff, trace_id);

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
bool RpcCodec::DecodeResponse(Buffer& buffer, uint64_t& id, int32_t& code, std::string& result, std::string& trace_id)
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

    std::string strTmpTraceId;
    if(!RpcCodec::ReadString(buffer, strTmpTraceId))
    {
        return false;
    }


    id = tmpId;
    code = tmpCode;
    result = std::move(strTmpResult);
    trace_id = std::move(strTmpTraceId);
    return true;
}


// 解码请求， 成功则返回true 并且填充 id, method， params; 失败则返回false (数据不足或者格式错误)
bool RpcCodec::DecodeRequest(Buffer& buffer, uint64_t& id, std::string& method, std::string& params, std::string& trace_id)
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

    std::string strTmpTraceId;
    if(!RpcCodec::ReadString(buffer, strTmpTraceId))
    {
        return false;
    }


    id = tmpId;
    method = std::move(strTmpMethod);
    params = std::move(strTmpParam);
    trace_id = std::move(strTmpTraceId);
    return true;
}
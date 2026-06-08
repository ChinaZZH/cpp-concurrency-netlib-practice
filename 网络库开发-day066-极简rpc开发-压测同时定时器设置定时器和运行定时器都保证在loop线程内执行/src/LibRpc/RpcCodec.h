#pragma once

#include <string>
#include <cstdint>
#include "../Buffer.h"

enum eRpcCode
{
    eRpcCode_Success        = 0,
    eRpcCode_NotGetMethod   = 1,
    eRpcCode_ParamError     = 2,
    eRpcCode_ServerError    = 3,
    eRpcCode_TimeOut        = 4,
};

class RpcCodec
{
public:
    // 编码请求
    static std::string Protobuf_EncodeRequest(uint64_t id, const std::string& method, const std::string& params);

    static bool Protobuf_DecodeRequest(const std::string& strBuffData, uint64_t& id, std::string& method, std::string& params);

    // 编码响应
    static std::string Protobuf_EncodeResponse(uint64_t id, int32_t code, const std::string& result);

    static bool Protobuf_DecodeResponse(const std::string& strBuffData, uint64_t& id, int32_t& code, std::string& result);

    // 四个辅助函数
    static void WriteInt64(Buffer& buffer, int64_t value);

    static bool ReadInt64(Buffer& buffer, int64_t& value);

    static void WriteInt32(Buffer& buffer, int32_t value);

    static bool ReadInt32(Buffer& buffer, int32_t& value);

    static void WriteString(Buffer& buffer, const std::string& strValue);

    static bool ReadString(Buffer& buffer, std::string& strValue);

// 传统的方式禁用，后面统一使用protobuf来做rpc的传输方式
private:
    // 暂时禁用，不用传统的方式统一走protobuf
    static void EncodeRequest(Buffer& buffer, uint64_t id, const std::string& method, const std::string& params);

     // 解码响应  成功返回true; 失败则返回false
    static void EncodeResponse(Buffer& buffer, uint64_t id, int32_t code, const std::string& result);

    static bool DecodeRequest(Buffer& buffer, uint64_t& id, std::string& method, std::string& params);

    static bool DecodeResponse(Buffer& buffer, uint64_t& id, int32_t& code, std::string& result);
};
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
};

class RpcCodec
{
public:
    // 编码请求
    static void EncodeRequest(Buffer& buffer, uint64_t id, const std::string& method, const std::string& params);

    // 解码请求， 成功则返回true 并且填充 id, method， params; 失败则返回false (数据不足或者格式错误)
    static bool DecodeRequest(Buffer& buffer, uint64_t& id, std::string& method, std::string& params);

    // 编码响应
    static void EncodeResponse(Buffer& buffer, uint64_t id, int32_t code, const std::string& result);

    // 解码响应  成功返回true; 失败则返回false
    static bool DecodeResponse(Buffer& buffer, uint64_t& id, int32_t& code, std::string& result);

    // 四个辅助函数
    static void WriteInt64(Buffer& buffer, int64_t value);

    static bool ReadInt64(Buffer& buffer, int64_t& value);

    static void WriteInt32(Buffer& buffer, int32_t value);

    static bool ReadInt32(Buffer& buffer, int32_t& value);

    static void WriteString(Buffer& buffer, const std::string& strValue);

    static bool ReadString(Buffer& buffer, std::string& strValue);
};
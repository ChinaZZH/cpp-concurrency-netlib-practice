#include "../Buffer.h"
#include "RpcCodec.h"
#include <cassert>
#include <iostream>

int main() {
    // 测试请求编码/解码
    Buffer reqBuf;
    uint32_t reqId = 12345;
    std::string method = "add";
    std::string params = "{\"a\":1,\"b\":2}";
    RpcCodec::EncodeRequest(reqBuf, reqId, method, params);

    uint32_t decodedId;
    std::string decodedMethod, decodedParams;
    bool ok = RpcCodec::DecodeRequest(reqBuf, decodedId, decodedMethod, decodedParams);
    assert(ok);
    assert(decodedId == reqId);
    assert(decodedMethod == method);
    assert(decodedParams == params);

    // 测试响应编码/解码
    Buffer respBuf;
    uint32_t respId = 12345;
    int32_t code = 0;
    std::string result = "{\"result\":3}";
    RpcCodec::EncodeResponse(respBuf, respId, code, result);

    uint32_t decodedRespId;
    int32_t decodedCode;
    std::string decodedResult;
    ok = RpcCodec::DecodeResponse(respBuf, decodedRespId, decodedCode, decodedResult);
    assert(ok);
    assert(decodedRespId == respId);
    assert(decodedCode == code);
    assert(decodedResult == result);

    std::cout << "All RpcCodec tests passed!" << std::endl;
    return 0;
}
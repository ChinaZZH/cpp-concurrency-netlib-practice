#include "../Buffer.h"
#include "RpcCodec.h"
#include "RpcClient.h"
#include "../TcpClient.h"
#include "../Buffer.h"
#include <cassert>
#include <iostream>
#include <chrono>
#include <arpa/inet.h> // htonl, ntohl

void testNormal()
{
     // 测试请求编码/解码
    Buffer reqBuf;
    uint32_t reqId = 12345;
    std::string method = "add";
    std::string params = "{\"a\":1,\"b\":2}";
    RpcCodec::EncodeRequest(reqBuf, reqId, method, params);

    uint64_t decodedId;
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

    uint64_t decodedRespId;
    int32_t decodedCode;
    std::string decodedResult;
    ok = RpcCodec::DecodeResponse(respBuf, decodedRespId, decodedCode, decodedResult);
    assert(ok);
    assert(decodedRespId == respId);
    assert(decodedCode == code);
    assert(decodedResult == result);
    std::cout << "testNormal passed" << std::endl;
}


void testEmptyString()
{
    Buffer buf;
    RpcCodec::EncodeRequest(buf, 1, "", "");

    uint64_t id;
    std::string strMethod, strParams;
    bool ok = RpcCodec::DecodeRequest(buf, id, strMethod, strParams);
    assert(ok);
    assert(id == 1);
    assert(strMethod.empty());
    assert(strParams.empty());
    std::cout << "testEmptyString passed" << std::endl;
}


void testLongMethodName()
{
    Buffer buf;
    std::string longMethod(1024*1024, 'X');
    RpcCodec::EncodeRequest(buf, 1, longMethod, "{}");

    uint64_t id;
    std::string strMethod, strParams;
    bool ok = RpcCodec::DecodeRequest(buf, id, strMethod, strParams);
    assert(ok);
    assert(id == 1);
    assert(strMethod == longMethod);
    std::cout << "testLongMethodName passed" << std::endl;
}

void testLargeParam()
{
    Buffer buf;
    std::string largetParam(10*1024*1024, 'Y');
    RpcCodec::EncodeRequest(buf, 1, "foo", largetParam);

    uint64_t id;
    std::string strMethod, strParams;
    bool ok = RpcCodec::DecodeRequest(buf, id, strMethod, strParams);
    assert(ok);
    assert(id == 1);
    assert(strParams == largetParam);
    std::cout << "testLargeParam passed" << std::endl;
}


void testIncompleteData()
{
    Buffer buf;
    // 只写入id,不写入后续
    int32_t id = 123;
    int32_t netId = htonl(id);
    buf.Append(reinterpret_cast<const char*>(&netId), sizeof(netId));

    // 尝试解码
    uint64_t outId = 0;
    std::string strMethod = "old", strParams = "old";
    bool ok = RpcCodec::DecodeRequest(buf, outId, strMethod, strParams);
    assert(!ok);
    assert(outId == 0 && strMethod == "old" && strParams == "old");
    std::cout << "testIncompleteData passed" << std::endl;
}

void testNegativeLength()
{
    // 构造恶意请求，方法名长度为负数
    Buffer buf;
    int32_t id = 123;
    int32_t netId = htonl(id);
    buf.Append(reinterpret_cast<const char*>(&netId), sizeof(netId));

    int32_t len = -1;
    int32_t netLen = htonl(len);
    buf.Append(reinterpret_cast<const char*>(&netLen), sizeof(netLen));

    // 没有方法名，直接尝试解码
    uint64_t outId = 0;
    std::string strMethod, strParams;
    bool ok = RpcCodec::DecodeRequest(buf, outId, strMethod, strParams);
    assert(!ok);
    std::cout << "testNegativeLength passed" << std::endl;
}


void testPerformance()
{
    const int iterations = 100000;
    Buffer buf;
    auto start_time = std::chrono::high_resolution_clock::now();
    for(int i = 0; i < iterations; ++i)
    {
        buf.Clear();
        RpcCodec::EncodeRequest(buf, i, "add", "{\"a\":1,\"b\":2}");

        uint64_t id = 0;
        std::string strMethod, strParams;
        if(!RpcCodec::DecodeRequest(buf, id, strMethod, strParams))
        {
            std::cerr << "decode failed at iteration " << i << std::endl;
            break;
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    double elapsed = std::chrono::duration<double>(end_time-start_time).count();
    std::cout << "Performance: " << iterations << " encode/decode cycles in "
              << elapsed << " s -> " << (iterations / elapsed) << " ops/s" << std::endl;
}

/*
int main() 
{
    testNormal();
    testEmptyString();
    testLongMethodName();
    testLargeParam();
    testIncompleteData();
    testNegativeLength();
    testPerformance();
    std::cout << "All RpcCodec tests passed!" << std::endl;
    return 0;
}
*/
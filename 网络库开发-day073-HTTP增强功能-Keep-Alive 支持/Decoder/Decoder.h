#pragma once

#include <string>
#include "../Buffer.h"

class Decoder {
public:
    virtual ~Decoder() = default;

    // 从 input 中解析出一条完整消息，存入 msg。
    // 成功返回 true，并从 input 中移除已解析的数据；
    // 失败（数据不足或格式错误）返回 false，input 保持不变。
    virtual bool Decode(Buffer& input, std::string& msg) = 0;
};
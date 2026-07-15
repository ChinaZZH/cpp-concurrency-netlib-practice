// LengthPrefixDecoder.h
#pragma once

#include "Decoder.h"
#include <unistd.h>

class LengthAndTypePrefixDecoder : public Decoder {
public:
    virtual ~LengthAndTypePrefixDecoder() = default;

    virtual bool Decode(Buffer& input, std::string& msg, uint32_t& msgType) override;

    static std::string MakeRequestString(const std::string& strContent, uint32_t msgType);
};


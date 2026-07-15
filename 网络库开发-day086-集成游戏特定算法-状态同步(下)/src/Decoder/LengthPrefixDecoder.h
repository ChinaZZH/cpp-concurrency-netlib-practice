// LengthPrefixDecoder.h
#pragma once

#include "Decoder.h"

class LengthPrefixDecoder : public Decoder {
public:
    virtual ~LengthPrefixDecoder() = default;

    virtual bool Decode(Buffer& input, std::string& msg, uint32_t& msgType) override;
};
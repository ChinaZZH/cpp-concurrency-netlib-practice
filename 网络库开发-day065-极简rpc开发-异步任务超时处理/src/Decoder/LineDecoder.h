// LineDecoder.h
#pragma once

#include "Decoder.h"

class LineDecoder : public Decoder {
public:
    virtual ~LineDecoder() = default;

    virtual bool Decode(Buffer& input, std::string& msg) override;
};
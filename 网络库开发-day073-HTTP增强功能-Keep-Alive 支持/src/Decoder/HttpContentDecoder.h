// HttpContentDecoder.h
#pragma once

#include "Decoder.h"

class HttpContentDecoder : public Decoder {
public:
    virtual ~HttpContentDecoder() = default;
    
    virtual bool Decode(Buffer& input, std::string& msg) override;

private:
    const char* FindHttpDelimiter(const Buffer& input);
};
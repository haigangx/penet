#pragma once

#include "slice.h"
#include "buffer.h"

class CodecBase
{
public:
    virtual int tryDecode(Slice data, Slice &msg) = 0;
    virtual void encode(Slice msg, Buffer &buf) = 0;
    virtual CodecBase *clone() = 0;
};

//以\r\n结尾的消息
class LineCodec : public CodecBase
{
public:
    int tryDecode(Slice data, Slice &msg) override;
    void encode(Slice msg, Buffer &buf) override;
    CodecBase *clone() override { return new LineCodec(); }
};

//给出长度的消息
class LengthCodec : public CodecBase
{
public:
    int tryDecode(Slice data, Slice &msg) override;
    void encode(Slice msg, Buffer &buf) override;
    CodecBase *clone() override { return new LengthCodec(); }
};


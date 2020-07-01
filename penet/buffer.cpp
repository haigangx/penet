#include "buffer.h"

#include <algorithm>

char *Buffer::makeRoom(size_t len)
{
    if (e_ + len <= cap_)
    { }
    //实际剩余空间可容纳，但是尾部长度不够容纳，将占用空间集体向头部移动
    else if (size() + len < cap_ / 2)
    {
        moveHead(); 
    }
    else
    {
        expand(len);
    }
    return end();
}

Buffer &Buffer::absorb(Buffer &buf)
{
    if (&buf != this)
    {
        if (size() == 0)
        {
            char b[sizeof(buf)];
            memcpy(b, this, sizeof(b));
            memcpy(this, &buf, sizeof(b));
            memcpy(&buf, b, sizeof(b));
            //仅仅交换数据部分，不交换exp_
            std::swap(exp_, buf.exp_);
        }
        else
        {
            append(buf.begin(), buf.size());
            buf.clear();
        }
    }
    return *this;
}

void Buffer::expand(size_t len)
{
    size_t ncap = std::max(exp_, std::max(2 * cap_, size() + len));
    char *p = new char[ncap];
    std::copy(begin(), end(), p);
    e_ -= b_;
    b_ = 0;
    delete[] buf_;
    buf_ = p;
    cap_ = ncap;
}

void Buffer::copyFrom(const Buffer &b)
{
    memcpy(this, &b, sizeof(b));
    if (b.buf_) 
    {
        buf_ = new char[cap_];
        memcpy(data(), b.begin(), b.size());
    }
}
#pragma once

#include "slice.h"

#include <string.h>

class Buffer
{
public:
    Buffer(): buf_(NULL), b_(0), e_(0), cap_(0), exp_(512) {}
    ~Buffer() { delete[] buf_; }

    void clear()
    {
        delete[] buf_;
        buf_ = NULL;
        cap_ = 0; 
        b_ = e_ = 0;
    }

    size_t size() const { return e_ - b_; }
    bool empty() const { return e_ == b_; }
    char *data() const { return buf_ + b_; }
    char *begin() const { return buf_ + b_; }
    char *end() const { return buf_ + e_; }

    //makeRoom拓展空间
    char *makeRoom(size_t len);
    void makeRoom()
    {
        if (space() < exp_)
            expand(0);
    }

    //返回剩余空间
    size_t space() const { return cap_ - e_; }
    void addSize(size_t len) { e_ += len; }
    char *allocRoom(size_t len)
    {
        char *p = makeRoom(len);
        addSize(len);
        return p;
    }

    //append 向buffer追加数据
    Buffer &append(const char *p, size_t len)
    {
        memcpy(allocRoom(len), p, len);
        return *this;
    }
    Buffer &append(Slice slice) { return append(slice.data(), slice.size()); }
    Buffer &append(const char *p) { return append(p, strlen(p)); }

    //appendValue 向buffer中追加任意类型的数据
    template<class T>
    Buffer &appendValue(const T &v)
    {
        append((const char *)&v, sizeof(v));
        return *this;
    }

    Buffer &consume(size_t len)
    {
        b_ += len;
        if (size() == 0)
            clear();
        return *this;
    }

    Buffer &absorb(Buffer &buf);
    void setSuggestSize(size_t sz) { exp_ == sz; }

    //拷贝构造函数
    Buffer(const Buffer &b) { copyFrom(b); }
    //赋值运算符
    Buffer &operator=(const Buffer &b)
    {
        if (this == &b)
            return *this;
        delete [] buf_;
        buf_ = NULL;
        copyFrom(b);
        return *this;
    }

    //类型转换
    operator Slice() { return Slice(data(), size()); }



private:
    //buffer空间的起始坐标
    char *buf_;
    //b_:存储内容的起始位置, e_:存储内容的终止位置, cap_:最大容量位置， exp_:最大容量大小
    size_t b_, e_, cap_, exp_;

    void moveHead() {
        std::copy(begin(), end(), buf_);
        e_ -= b_;
        b_ = 0;
    }
    void expand(size_t len);
    void copyFrom(const Buffer &b);
};
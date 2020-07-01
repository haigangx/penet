#pragma once

#include "define.h"

#include <list>
#include <memory>

using IdleId = std::unique_ptr<IdleIdImp>;
using TimerId = std::pair<int64_t, int64_t>;

struct TimerRepeatable
{
    int64_t at;
    int64_t interval;
    TimerId timerid;
    Task cb;
};


struct IdleNode
{
    TcpConnPtr con_;
    int64_t updated_;
    TcpCallBack cb_;
};


class IdleIdImp
{
public:
    typedef std::list<IdleNode>::iterator Iter;

    IdleIdImp() {} 
    IdleIdImp(std::list<IdleNode> *lst, Iter iter) : lst_(lst), iter_(iter) {}

public:
    std::list<IdleNode> *lst;
    Iter iter_;
};


//管理类的声明周期
class AutoContext
{
public:
    AutoContext() : ctx(0) {}
    ~AutoContext() 
    {
        if (ctx)
            ctxDel();
    }

    template <class T>
    T &context()
    {
        if (ctx == NULL) 
        {
            ctx = new T();
            ctxDel = [this] { delete (T *)ctx; };
        }
        return *(T *)ctx;
    }

public:
    void *ctx;
    Task ctxDel;
}
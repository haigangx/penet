#pragma once

#include "safe_queue.h"
#include "define.h"

#include <memory>
#include <atomic>

class PollerBase;
class EventImp;
class EventBase;
class EventBases
{
public:
    virtual EventBase *allocBase() = 0;
};


class EventBase : public EventBases
{
public:
    EventBase(int taskCapacity = 0);
    ~EventBase();

    void exit();
    void loop();

    virtual EventBase *allocBase() { return this; }

public:
    std::unique_ptr<EventImp> imp_;
};

struct IdleNode
{
    TcpConnPtr con_;
    int64_t updated_;
    TcpCallBack cb_;
};


class EventImp
{
public:
    EventImp(EventBase *base, int taskCap);
    ~EventImp();

    void init();
    void callIdleles();
    Idle

public:
    EventBase *base_;
    PollerBase *poller_;
    std::atomic<bool> exit_;
    int wakeupFds_[2];
    SafeQueue<Task> tasks_;
};
#pragma once

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
    EventBase();
    ~EventBase();

    void init();
    void exit();
    void loop();

    virtual EventBase *allocBase() { return this; }

public:
    std::unique_ptr<EventImp> imp_;
};


class EventImp
{
public:
    EventImp(EventBase *base, int taskCap);
    ~EventImp();

    void init();

public:
    EventBases *base_;
    PollerBase *poller_;
    std::atomic<bool> exit_;
    int wakeupFds_[2];
};
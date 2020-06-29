#pragma once

#include <set>
#include <poll.h>
#include <sys/epoll.h>

const int kMaxEvents = 2000;
const int kReadEvent = POLLIN;
const int kWriteEvent = POLLOUT;

class Channel;
class PollerBase
{
public:
    PollerBase();
    virtual ~PollerBase(){};

    virtual void addChannel(Channel *ch) = 0;
    virtual void removeChannel(Channel *ch) = 0;
    virtual void updateChannel(Channel *ch) = 0;
    virtual void loop_once(int waitMs) = 0;

public:
    int64_t id_;
    int lastActive_;
};

PollerBase *createPoller();

class PollerEpoll : public PollerBase
{
public:
    PollerEpoll();
    ~PollerEpoll();

    void addChannel(Channel *ch) override;
    void removeChannel(Channel *ch) override;
    void updateChannel(Channel *ch) override;
    void loop_once(int waitMs) override;

public:
    int fd_;
    std::set<Channel *> liveChannels_;
    struct epoll_event activeEvs_[kMaxEvents];
};
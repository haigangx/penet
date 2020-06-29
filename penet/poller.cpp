#include "poller.h"

#include <atomic>
#include <string.h>
#include <unistd.h>

#include "logger.h"
#include "channel.h"
#include "util.h"

PollerBase::PollerBase()
    : lastActive_(-1)
{
    static std::atomic<int64_t> id(0);
    id_ = ++id;
}

PollerBase *createPoller()
{
    return new PollerEpoll();
}

PollerEpoll::PollerEpoll()
{
    fd_ = epoll_create1(EPOLL_CLOEXEC);
    fatalif(fd_ < 0, "epoll_create error %d %s", errno, strerror(errno));
    info("poller epoll %d created", fd_);
}

PollerEpoll::~PollerEpoll()
{
    info("destroying poller %d", fd_);
    while (liveChannels_.size())
    {
        (*liveChannels_.begin())->close();
    }
    ::close(fd_);
    info("poller %d destroyed", fd_);
}

void PollerEpoll::addChannel(Channel *ch)
{
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = ch->events();
    ev.data.ptr = ch;
    trace("adding channel %lld fd %d events %d epoll %d", 
            (long long)ch->id(), ch->fd(), ev.events, fd_);
    int r = epoll_ctl(fd_, EPOLL_CTL_ADD, ch->fd(), &ev);
    fatalif(r, "epoll_ctl add failed %d %s", errno, strerror(errno));
    liveChannels_.insert(ch);
}

void PollerEpoll::updateChannel(Channel *ch)
{
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = ch->events();
    ev.data.ptr = ch;
    trace("modifying channel %lld fd %d events read %d write %d epoll %d", 
            (long long)ch->id(), ch->fd(), ev.events, fd_);
    int r = epoll_ctl(fd_, EPOLL_CTL_MOD, ch->fd(), &ev);
    fatalif(r, "epoll_ctl mod failed %d %s", errno, strerror(errno));
}

void PollerEpoll::removeChannel(Channel *ch)
{
    trace("deleting channel %lld fd %d epoll %d", (long long)ch->id(), ch->fd(), fd_);
    liveChannels_.erase(ch);
    for (int i = lastActive_; i >= 0; i--)
    {
        if (ch == activeEvs_[i].data.ptr)
        {
            activeEvs_[i].data.ptr = nullptr;
            break;
        }
    }
}

void PollerEpoll::loop_once(int waitMs)
{
    int64_t ticks = Util::timeMilli();
}
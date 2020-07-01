#include "channel.h"

#include <atomic>
#include <unistd.h>

#include "event_base.h"
#include "poller.h"
#include "logger.h"
#include "net.h"

Channel::Channel(EventBase *base, int fd, int events)
    : base_(base), fd_(fd), events_(events)
{
    fatalif(Net::setNonBlock(fd_) < 0, "channel set nonblock failed");
    static std::atomic<int64_t> id(0);
    id_ = ++id;
    poller_ = base_->imp_->poller_;
    poller_->addChannel(this);
}

Channel::~Channel()
{
}

void Channel::close()
{
    if (fd_ >= 0)
    {
        trace("close channel %ld fd %d", (long)id_, fd_);
        poller_->removeChannel(this);
        ::close(fd_);
        fd_ = -1;
        handleRead();
    }
}

void Channel::enableRead(bool enable)
{

}

void Channel::enableWrite(bool enable)
{

}

void Channel::enableReadWrite(bool readable, bool writeable)
{

}

bool Channel::readEnabled()
{
    return true;
}

bool Channel::writeEnabled()
{
    return true;
}

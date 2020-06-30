#include "event_base.h"

#include "poller.h"
#include "logger.h"
#include "util.h"
#include "channel.h"

#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


EventBase::EventBase(int taskCapacity)
{
    imp_.reset(new EventImp(this, taskCapacity));
    imp_->init();
}

EventBase::~EventBase()
{
}

void EventBase::exit()
{
    //return imp_->exited();
}

void EventBase::loop()
{}


EventImp::EventImp(EventBase* base, int taskCap)
    : base_(base)
    , poller_(createPoller())
    , exit_(false)
    //, nextTimeout_(1 << 30)
    //, timerSeq_(0)
    //, idleEnalbed(false)
{}

EventImp::~EventImp()
{}

void EventImp::init()
{
    int r = pipe(wakeupFds_);
    fatalif(r, "pipe failed %d %s", errno, strerror(errno));
    r = Util::addFdFlag(wakeupFds_[0], FD_CLOEXEC);
    fatalif(r, "addFdFlag failed %d %s", errno, strerror(errno));
    r = Util::addFdFlag(wakeupFds_[1], FD_CLOEXEC);
    fatalif(r, "addFdFlag failed %d %s", errno, strerror(errno));
    trace("wakeup pipe created %d %d", wakeupFds_[0], wakeupFds_[1]);

    Channel *ch = new Channel(base_, wakeupFds_[0], kReadEvent);
    std::function<void()> pipe_read_func = [=]{
        char buf[1024];
        int r = ch->fd() >= 0 ? ::read(ch->fd(), buf, sizeof(buf)) : 0;
        if ( r > 0)
        {
            Task task;
            while (tasks_.pop_wait(&task, 0))
            {
                task();
            }
        }
        else if (r == 0)
        {
            delete ch;
        }
        else if (errno == EINTR)
        { }
        else
        {
          fatal("wakeup channel read error %d %d %s", r, errno, strerror(errno));
        }
    };
    //调用onRead(const Task &)
    ch->onRead(pipe_read_func);
    //如果使用匿名lambda表达式，调用onRead(Task &&)
}
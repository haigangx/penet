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

void EventBase::loop_once(int waitMs)
{
    imp_->loop_once(waitMs);
}

void EventBase::loop()
{
    imp_->loop();
}

bool EventBase::cancel(TimerId timerid)
{
    return imp_ && imp_->cancel(timerid);
}

TimerId EventBase::runAt(int64_t milli, Task &&task, int64_t interval)
{
    return imp_->runAt(milli, std::move(task), interval);
}

EventBase &EventBase::exit()
{
    imp_->exit();
}

bool EventBase::exited()
{
    imp_->exited();
}

void EventBase::wakeup()
{
    imp_->wakeup();
}

void EventBase::safeCall(Task &&task)
{
    imp_->safeCall(std::move(task));
}


EventImp::EventImp(EventBase* base, int taskCap)
    : base_(base)
    , poller_(createPoller())
    , exit_(false)
    , nextTimeout_(1 << 30)
    , tasks_(taskCap)
    , timerSeq_(0)
    , idleEnalbed(false)
{}

EventImp::~EventImp()
{
    delete poller_;
    ::close(wakeupFds_[1]);
}

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
    //不使用std::move调用onRead(const Task &)
    //使用std::move调用onRead(Task &&)
    //如果直接传递匿名lambda表达式，调用onRead(Task &&)
    ch->onRead(std::move(pipe_read_func));
}

void EventImp::callIdleles()
{
    int64_t now = Util::timeMilli() / 1000;
    for (auto &l : idleConns_)
    {
        int idle = l.first;
        auto lst = l.second;
        while (lst.size())
        {
            IdleNode &node = lst.front();
            if (node.updated_ + idle > now)
            {
                break;
            }
            node.updated_ = now;
            //将链表头部元素重新插入到链表尾部
            lst.splice(lst.end(), lst, lst.begin());
            node.cb_(node.con_);
        }
    }
}

IdleId EventImp::registerIdle(int idle, const TcpConnPtr &con, const TcpCallBack &cb)
{
    if (!idleEnabled)
    {
        base_->runAfter(1000, [this]{ callIdleles(); }, 1000);
        idleEnabled = true;
    }
    auto &lst = idleConns_[idle];
    lst.push_back(IdleNode{con, Util::timeMilli() / 1000}, move(cb));
    trace("register idle");
    return IdleId(new IdleIdImp(&lst, --lst.end()));
}

void EventImp::unregisterIdle(const IdleId &id)
{
    trace("unregister idle");
    id->lst_->erase(id->iter_);
}

void EventImp::updateIdle(const IdleId &id)
{
    trace("update idle");
    id->iter_->update_ = Util::timeMilli() / 1000;
    id->lst_->splice(id->lst_->end(), *id->lst_, id->iter_);
}

void EventImp::handleTimeouts()
{
    //处理已经到达时间的超时事件
    int64_t now = Util::timeMilli();
    TimerId tid{now, 1L << 62};
    while (timers_.size() && timers_.begin()->first < tid)
    {
        Task task = move(timers_.begin()->second);
        timers_.erase(timers_.begin());
        task();
    }
    refreshNearest();
}

void EventImp::refreshNearest(const TimerId *tid = NULL)
{
    //将nextTimeout_设置为下一个超时事件到达的时间
    if (timers_.empty())
    {
        nextTimeout_ = 1 << 30;
    }
    else
    {
        const TimerId &t = timers_.begin()->first;
        nextTimeout_ = t.first - Util::timeMilli();
        nextTimeout_ = nextTimeout_ < 0 ? 0 : nextTimeout_;
    }
    
}

void EventImp::repeatableTimeout(TimerRepeatable *tr)
{
    //每隔interval时间执行一次该函数
    tr->at += tr->interval;
    tr->timerid = {tr->at, ++timerSeq_};
    timers_[tr->timerid] = [this, tr] { repeatableTimeout(tr); };
    refreshNearest(&tr->timerid);
    tr->cb();
}

void EventImp::loop()
{
    while (!exit_)
    {
        loop_once(10000);
    }
    timerReps_.clear();
    timers_.clear();
    idleConns_.clear();
    for (auto recon : reconnectConns_)
    {   //重连的连接无法通过channel清理，因此单独清理
        recon->cleanup(recon);
    }
    loop_once(0);
}

bool EventImp::cancel(TimerId timerid)
{
    if (timerid.first < 0)
    {
        auto p = timerReps_.find(timerid);
        auto ptimer = timers_.find(p->second.timerid);
        if (ptimer != timers_.end())
        {
            timers_.erase(ptimer);
        }
        timerReps_.erase(p);
        return true;
    }
    else
    {
        auto p = timers_.find(timerid);
        if (p != timers_.end())
        {
            timers_.erase(p);
            return true;
        }
        return false;
    }
}

TimerId EventImp::runAt(int64_t milli, Task &&task, int64_t interval)
{
    if (exit_)
    {
        return TimerId();
    }
    if (interval)
    {
        TimerId tid{-milli,  ++timerSeq_};
        TimerRepeatable &rtr = timerReps_[tid];
        rtr = {milli, interval, {milli, ++timerSeq_}, std::move(task)};
        TimerRepeatable *tr = &rtr;
        timers_[tr->timerid] = [this, tr] { repeatableTimeout(tr); };
        refreshNearest(&tr->timerid);
        return tid;
    }
    else
    {
        TimerId tid{milli, ++timerSeq_};
        timers_.insert({tid, move(task)});
        refreshNearest(&tid);
        return tid;
    }
}
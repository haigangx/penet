#pragma once

#include "safe_queue.h"
#include "define.h"
#include "tcp_conn.h"
#include "logger.h"

#include <memory>
#include <atomic>
#include <map>
#include <set>

class PollerBase;
class EventImp;
class EventBase;
class EventBases
{
public:
    virtual EventBase *allocBase() = 0;
};


//时间派发器，可管理定时器，连接，超时连接
class EventBase : public EventBases
{
public:
    //taskCapacity指定任务队列的大小，0无限制
    EventBase(int taskCapacity = 0);
    ~EventBase();
    //处理已到期的事件，waitMs表示着无当前需要处理的任务，需要等待的时间
    void loop_once(int waitMs);
    //进入事件处理循环
    void loop();
    //取消定时任务，若timer已经过期，则忽略
    bool cancel(TimerId timerid);
    //添加定时任务，interval=0表示一次性任务，否则为重复任务，时间为毫秒
    TimerId runAt(int64_t milli, const Task &task, int64_t interval = 0)
        { return runAt(milli, Task(task), interval); }
    TimerId runAt(int64_t milli, Task &&task, int64_t interval = 0);
    TimerId runAfter(int64_t milli, const Task &task, int64_t interval = 0)
        { return runAt(Util::timeMilli() + milli, Task(task), interval); }
    TimerId runAfter(int64_t milli, Task &&task, int64_t interval = 0)
        { return runAt(Util::timeMilli() + milli, std::move(task), interval); }

    //下列函数为线程安全的

    //退出事件循环
    EventBase &exit();
    //是否已退出
    bool exited();
    //唤醒事件处理
    void wakeup();
    //添加任务
    void safeCall(Task &&task);
    void safeCall(const Task &task) { safeCall(Task(task)); }
    //分配一个事件派发器
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
    IdleId registerIdle(int idle, const TcpConnPtr &con, const TcpCallBack &cb);
    void unregisterIdle(const IdleId &id);
    void updateIdle(const IdleId &id);
    void handleTimeouts();
    void refreshNearest(const TimerId *tid = NULL);
    void repeatableTimeout(TimerRepeatable *tr);

    //eventbase functions
    EventBase &exit()
    {
        exit_ = true;
        wakeup();
        return *base_;
    }
    bool exited() { return exit_; }
    void safeCall(Task &&task)
    {
        tasks_.push(move(task));
        wakeup();
    }
    void loop();
    void loop_once(int waitMs)
    {
        poller_->loop_once(std::min(waitMs, nextTimeout_));
        handleTimeouts();
    }
    void wakeup()
    {
        int r = write(wakeupFds_[1], "", 1);
        fatalif(r <= 0, "write error wd %d %d %s", r, errno, strerror(errno));
    }
    bool cancel(TimerId timerid);
    TimerId runAt(int64_t milli, Task &&task, int64_t interval);

public:
    EventBase *base_;
    PollerBase *poller_;
    std::atomic<bool> exit_;
    int wakeupFds_[2];
    int nextTimeout_;
    SafeQueue<Task> tasks_;

    std::map<TimerId, TimerRepeatable> timerReps_;
    std::map<TimerId, Task> timers_;
    std::atomic<int64_t> timerSeq_;
    //记录每个idle时间(单位秒)下所有的连接，链表中的所有连接，最新的插入到链表末尾
    //连接着有活动，会把连接从链表中移到链表尾部，做法参考memcache
    std::map<int, std::list<IdleNode>> idleConns_;
    std::set<TcpConnPtr> reconnectConns_;
    bool idleEnabled;
};
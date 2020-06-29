#pragma once

#include "define.h"

class EventBase;
class PollerBase;
class Channel
{
public:
    Channel(EventBase *base, int fd, int events);
    ~Channel();

    //获取成员变量的值
    EventBase *getBase() { return base_; }
    int fd() { return fd_; }
    int64_t id() { return id_; }
    short events() { return events_; }

    //关闭该通道
    void close();

    //分别设置读写事件处理器
    //TODO:为什么要分 const 和非 const 两种定义？
    void onRead(const Task &readcb) { readcb_ = readcb; }
    void onWrite(const Task &writecb) { writecb_ = writecb; }
    //参数传入 lambda 表达式时使用
    //std::move 用于将 readcb 中保存的右值释放出来
    void onRead(Task &&readcb) { readcb_ = std::move(readcb); }
    void onWrite(Task &&writecb) { writecb_ = std::move(writecb); }

    //设置和返回读写监听状态
    void enableRead(bool enable);
    void enableWrite(bool enable);
    void enableReadWrite(bool readable, bool writeable);
    bool readEnabled();
    bool writeEnabled();

    //调用读写处理事件
    void handleRead() { readcb_(); }
    void handleWrite() { writecb_(); }

protected:
    EventBase* base_;
    PollerBase *poller_;
    int fd_;
    short events_;
    int64_t id_;    //通道号
    CHANNEL_FUNC readcb_, writecb_, errorcb_;
};
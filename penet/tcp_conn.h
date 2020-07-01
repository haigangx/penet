#pragma once

#include "buffer.h"
#include "ip4addr.h"
#include "codec.h"
#include "idleid.h"
#include "channel.h"

#include <memory>
#include <functional>
#include <list>
#include <assert.h>
#include <unistd.h>

using TcpConnPtr = std::shared_ptr<TcpConn> ;
using TcpCallBack = std::function<void(const TcpConnPtr &)>;
using MsgCallBack = std::function<void(const TcpConnPtr &, Slice msg)>;

class EventBase;
class Channel;
//继承 enable_shared_from_this 使 TcpConn 拥有 shared_from_this() 函数
class TcpConn : public std::enable_shared_from_this<TcpConn>
{
public:

    //Tcp连接的五个状态
    enum State
    {
        Invalid = 1,
        Handshaking,
        Connected,
        Closed,
        Failed,
    };

    //Tcp 构造函数，实际可用的连接应当通过 createConnection 创建
    TcpConn();
    virtual ~TcpConn();

    //可传入连接类型，返回智能指针
    template <class C = TcpConn>
    static TcpConnPtr createConnection(EventBase *base, const std::string &host, unsigned short port, int timeout = 0, const std::string &localip = "")
    {
        TcpConnPtr con(new C);
        con->connect(base, host, port, timeout, localip);
        return con;
    }
    template <class C = TcpConn>
    static TcpConnPtr createConnection(EventBase *base, int fd, Ip4Addr local, Ip4Addr perr)
    {
        TcpConnPtr con(new C);
        con->attach(base, fd, local, peer);
        return con;
    } 

    bool isClient() { return destPort_ > 0; }
    template <class T>
    T &context() 
    {
        return ctx_.context<T>();
    }

    EventBase *getBase() { return base_; }
    State getState() { return state_; }
    //TcpConn的输入输出缓冲区
    Buffer &getInput(){ return input_; };
    Buffer &getOutput() { return output_; }

    Channel *getChannel() { return channel_; }
    bool writable() { return channel_ ? channel_->writeEnabled() : false; }

    //发送数据
    void sendOutput() { send(output_); }
    void send(Buffer &msg);
    void send(const char *buf, size_t len);
    void send(const std::string &s) { send(s.data(), s.size()); }
    void send(const char *s) { send(s, strlen(s)); }

    //数据到达时回调
    void onRead(const TcpCallBack &cb)
    {
        assert(!readcb_);
        readcb_ = cb;
    }
    //当tcp缓冲区可写时回调
    void onWriteable(const TcpCallBack &cb) { writablecb_ = cb; }
    //tcp状态改变时回调
    void onState(const TcpCallBack &cb) { statecb_ = cb; }
    //tcp空闲回调
    void addIdleCB(int idle, const TcpCallBack &cb);

    //消息回调，此回调与onRead回调冲突，只能狗调用一个
    //codec所有权交给onMsg
    void onMsg(CodecBase *codec, const MsgCallBack &cb);
    //发送消息
    void sendMsg(Slice msg);

    //conn会在下个事件周期进行处理
    void close();
    //设置重连时间间隔 -1：不重连 0：立即重连 其他：等待毫秒数后重连 未设置：不重连
    void setReconnectInterval(int milli) { reconnectInterval_ = milli; }

    //立即关闭连接，清理相关资源，可能导致该连接的引用计数变为0，从而使当前调用者引用的连接被析构
    void closeNow() 
    {
        if (channel_)
            channel_->close();
    }

    std::string str() { return peer_.toString(); }



private:
    void handleRead(const TcpConnPtr &con);
    void handleWrite(const TcpConnPtr &con);
    ssize_t isend(const char *buf, size_t len);
    void cleanup(const TcpConnPtr &con);
    void connect(EventBase *base, const std::string &host, unsigned short port, int timeout, const std::string &localip);
    void reconnect();
    void attach(EventBase *base, int fd, Ip4Addr local, Ip4Addr peer);
    virtual int readImp(int fd, void *buf, size_t bytes) { return ::read(fd, buf, bytes); }
    virtual int writeImp(int fd, const void *buf, size_t bytes) { return ::write(fd, buf, bytes); }
    virtual int handleHandshake(const TcpConnPtr &conn);

private:
    EventBase *base_;
    Channel *channel_;
    Buffer input_, output_;
    Ip4Addr local_, peer_;
    State state_;
    TcpCallBack readcb_, writablecb_, statecb_;
    std::list<IdleId> idleIds_;
    TimerId timeoutId_;
    AutoContext ctx_, internalCtx_;
    std::string destHost_, localIp_;
    int destPort_, connectTimeout_, reconnectInterval_;
    int64_t connectedTime_;
    std::unique_ptr<CodecBase> codec_;
};

#pragma once

#include "tcp_conn.h"

#include <string>
#include <functional>
#include <memory>


class EventBase;
class TcpServer
{
public:
    TcpServer(EventBase *base);
    ~TcpServer() { delete listen_channel_; }

    static TcpServerPtr* startServer(EventBase *base, const std::string &host, unsigned short port);
    int bind(const std::string &host, unsigned short port, bool reusePort = false);

    Ip4Addr getAddr() { return addr_; }
    EventBase *getBase() { return base_; }
    void onConnCreate(const std::function<TcpConnPtr()> &cb) { createcb_ = cb; }
    void onConnState(const TcpCallBack &cb) { statecb_ = cb; }
    void onConnRead(const TcpCallBack &cb)
    {
        readcb_ = cb;
        assert(!msgcb_);
    }
    //消息处理与Read回调冲突，只能调用一个
    void onConnMsg(CodecBase *codec, const MsgCallBack &cb)
    {
        codec_.reset(codec);
        msgcb_ = cb;
        assert(!readcb_);
    }

    void onConnRead(const TcpCallBack &cb);

private:
    EventBase *base_;
    EventBases *bases_;
    Ip4Addr addr_;
    Channel *listen_channel_;
    TcpCallBack statecb_, readcb_;
    MsgCallBack msgcb_;
    std::function<TcpConnPtr()> createcb_;
    std::unique_ptr<CodecBase> codec_;
    void handleAccept();
};

using TcpServerPtr = std::shared_ptr<TcpServer>;
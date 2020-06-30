#pragma once

#include "buffer.h"

#include <memory>

using TcpConnPtr = std::shared_ptr<TcpConn> ;
using TcpCallBack = std::function<void(const TcpConnPtr &)>;

class EventBase;
class Channel;
//继承 enable_shared_from_this 使 TcpConn 拥有 shared_from_this() 函数
class TcpConn : public std::enable_shared_from_this<TcpConn>
{
public:
    //Tcp 构造函数，实际可用的连接应当通过 createConnection 创建
    TcpConn();
    virtual ~TcpConn();

    Buffer getInput(){ return Buffer(); };
    void send(Buffer msg);


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

private:
    EventBase *base_;
    Channel *channel_;
    Buffer input_, output_;
    Ip4Addr local_, perr_;
};

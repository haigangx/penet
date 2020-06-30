#pragma once

#include <string>
#include <functional>
#include "tcp_conn.h"

class EventBase;
class TcpServer
{
public:
    TcpServer(EventBase *base);
    ~TcpServer();

    static TcpServer* startServer(EventBase *base, const std::string &host, unsigned short port);

    void onConnRead(const TcpCallBack &cb);

private:
};

using TcpServerPtr = TcpServer *;
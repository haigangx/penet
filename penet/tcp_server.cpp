#include "tcp_server.h"
#include "event_base.h"

TcpServer::TcpServer(EventBase *base)
{}

TcpServer::~TcpServer()
{}

TcpServer* TcpServer::startServer(EventBase *base, const std::string &host, unsigned short port)
{
    return new TcpServer(base);
}

void TcpServer::onConnRead(const TcpCallBack &cb)
{}
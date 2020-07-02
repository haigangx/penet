#include "tcp_server.h"

#include "event_base.h"
#include "net.h"
#include "logger.h"
#include "util.h"
#include "channel.h"
#include "tcp_conn.h"

#include <fcntl.h>

TcpServer::TcpServer(EventBase *base)
{}

TcpServerPtr* TcpServer::startServer(EventBase *base, const std::string &host, unsigned short port)
{
    TcpServerPtr p(new TcpServer(base));
    int r = p->bind(host, port, reusePort);
    if (r)
    {
        error("bind to %s:%d failed %d %s", host.c_str(), port, errno, strerror(errno));
    }
}

int TcpServer::bind(const std::string &host, unsigned short port, bool reusePort = false)
{
    addr_ = Ip4Addr(host, port);
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    int r = Net::setReuseAddr(fd);
    fatalif(r, "set socket reuse port option failed");
    r = Net::setReusePort(fd, reusePort);
    fatalif(r, "set socket reuse port option failed");
    r = Util::addFdFlag(fd, FD_CLOEXEC);
    fatalif(r, "addFdFlag FD_CLOEXEC failed");
    r = ::bind(fd, (struct sockaddr *)&addr_.getAddr(), sizeof(struct sockaddr));
    if (r)
    {
        close(fd);
        error("bind to %s failed %d %s", addr_.toString().c_str(), errno, strerror(errno));
        return errno;
    }
    r = listen(fd, 20);
    fatalif(r, "listen failed %d %s", errno, strerror(errno));
    info("fd %d listening at %s", fd, addr_.toString().c_str());
    listen_channel_ = new Channel(base_, fd, kReadEvent);
    listen_channel_->onRead([this]{ handleAccept(); });
    return 0;
}

void TcpServer::handleAccept()
{
    struct sockaddr_in raddr;
    socklen_t rsz = sizeof(raddr);
    int lfd = listen_channel_->fd();
    int cfd;
    while (lfd >= 0 && (cfd = accept(lfd, (struct sockaddr *)&raddr, &rsz)) >= 0)
    {
        sockaddr_in peer, local;
        socklen_t alen = sizeof(peer);
        int r = getpeername(cfd, (sockaddr *)&peer, &alen);
        if (r < 0)
        {
            error("get peer name failed %d %s", errno, strerror(errno));
            continue;
        }
        r = getsockname(cfd, (sockaddr *)&local, &alen);
        if (r < 0)
        {
            error("getsockanme failed %d %s", errno, strerror(errno));
            continue;
        }
        r = Util::addFdFlag(cfd, FD_CLOEXEC);
        fatalif(r, "addFdFlag FD_CLOEXEC failed");
        EventBase *b = bases_->allocBase();
        auto addcon = [=] {
            TcpConnPtr con = createcb_();
            con->attach(b, cfd, local, peer);
            if (statecb_)
            {
                con->onState(statecb_);
            }
            if (readcb_)
            {
                con->onRead(readcb_);
            }
            if (msgcb_)
            {
                con->onMsg(codec_->clone(), msgcb_);
            }
        };
        if (b == base_)
        {
            addcon();
        }
        else
        {
            b->safeCall(std::move(addcon));
        }
    }
    if (lfd >= 0 && errno != EAGAIN && errno != EINTR)
    {
        warn("accept return %d %d %s", cfd, errno, strerror(errno));
    }
}
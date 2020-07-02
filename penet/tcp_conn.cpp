#include "tcp_conn.h"

#include "util.h"
#include "logger.h"
#include "event_base.h"
#include "net.h"

#include <poll.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

void handyUnregisterIdle(EventBase *base, const IdleId &idle)
{
    base->imp_->unregisterIdle(idle);
}

void handyUpdateIdle(EventBase *base, const IdleId &idle)
{
    base->imp_->updateIdle(idle);
}

TcpConn::TcpConn()
    : base_(NULL)
    , channel_(NULL)
    , state_(State::Invalid)
    , destPort_(-1)
    , connectTimeout_(0)
    , reconnectInterval_(-1)
    , connectedTime_(Util::timeMilli())
{}

TcpConn::~TcpConn()
{
    trace("tcp destroyed %s - %s", local_.toString().c_str(), peer_.toString().c_str());
    delete channel_;
}

void TcpConn::send(Buffer &buf)
{
    if (channel_)
    {
        if (channel_->writeEnabled())
        {
            output_.absorb(buf);
        }
        if (buf.size())
        {
            ssize_t sended = isend(buf.begin(), buf.size());
            buf.consume(sended);
        }
        if (buf.size())
        {
            output_.absorb(buf);
            if (!channel_->writeEnabled())
                channel_->enableWrite(true);
        }
    }
    else
    {
        warn("connection %s - %s closed, but still writing %lu bytes",
                local_.toString().c_str(), peer_.toString().c_str(), buf.size());
    }
    
}

void TcpConn::send(const char *buf, size_t len)
{
    if (channel_)
    {
        if (output_.empty())
        {
            ssize_t sended = isend(buf, len);
            buf += sended;
            len -= sended;
        }
        if (len)
        {
            output_.append(buf, len);
        }
    }
    else
    {
        warn("connecting %s - %s closed, but still writing %lu bytes", 
                local_.toString().c_str(), peer_.toString().c_str(), buf.size());
    }
}


void TcpConn::addIdleCB(int idle, const TcpCallBack &cb)
{
    if (channel_)
    {
        idleIds_.push_back(getBase()->imp_->registerIdle(idle, shared_from_this(), cb));
    }
}

void TcpConn::onMsg(CodecBase *codec, const MsgCallBack &cb)
{
    assert(!readcb_);
    codec_.reset(codec);
    std::function<void(const TcpConnPtr &con)> fn = [cb](const TcpConnPtr &con) {
        int r = 1;
        while (r)
        {
            Slice msg;
            r = con->codec_->tryDecode(con->getInput(), msg);
            if (r < 0)
            {
                con->channel_->close();
                break;
            }
            else if (r > 0)
            {
                trace("a msg decoded. origin len %d msg len %ld", r, msg.size());
                cb(con, msg);
                con->getInput().consume(r);
            }
        }
    };
    onRead(fn);
}

void TcpConn::sendMsg(Slice msg)
{
    //将msg按照协议要求封装到output_中并发送
    codec_->encode(msg, getOutput());
    sendOutput();
}

void TcpConn::close()
{
    if (channel_)
    {
        TcpConnPtr con = shared_from_this();
        //关闭交由事件派发器EventBase处理
        getBase()->SafeCall([con]{
            if (con->channel_)
                con->channel_->close();
        });
    }
}

void TcpConn::handleRead(const TcpConnPtr &con)
{
    if (state_ == State::Handshaking && handleHandshake(con))
        return ;

    while (state_ == State::Connected)
    {
        input_.makeRoom();
        int rd = 0;
        if (channel_->fd() >= 0)
        {
            rd = readImp(channel_->fd(), input_.end(), input_.space());
            trace("channel %lld fd %d readed %d bytes", (long long)channel_->id(),
                    channel_->fd(), rd);
        }
        if (rd == -1 && errno == EINTR)
        {
            continue;
        }
        else if (rd == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
        {
            for (auto &idle : idleIds_)
                handyUpdateIdle(getBase(), idle);
            if (readcb_ && input_.size())
                readcb_(con);
            break;
        }
        else if (channel_->fd() == -1 || rd == 0 || rd == -1)
        {
            cleanup(con);
            break;
        }
        else    //rd > 0
        {
            //如果读到数据，暂时放入_input中，然后继续读
            input_.addSize(rd);
        }
    }
}

void TcpConn::handleWrite(const TcpConnPtr &con)
{
    if (state_ == State::Handshaking)
    {
        handleHandshake(con);
    }
    else if (state_ == State::Connected)
    {
        ssize_t sended = isend(output_.begin(), output_.size());
        output_.consume(sended);
        if (output_.empty() && writablecb_)
            writablecb_(con);
        // writablecb_ may write something
        if (output_.empty() && channel_->writeEnabled())
            channel_->enableWrite(false);
    }
    else
    {
        error("handle write unexpected");
    }
}

ssize_t TcpConn::isend(const char *buf, size_t len)
{
    size_t sended = 0;
    while (len > sended)
    {
        ssize_t wd = writeImp(channel_->fd(), buf + sended, len - sended);
        trace("channel %lld fd %d write %ld bytes", 
                (long long)channel_->id(), channel_->fd(), wd);
        if (wd > 0)
        {
            sended += wd;
            continue;
        }
        //此调用被信号中断
        else if (wd == -1 && errno == EINTR)
        {
            continue;
        }
        else if (wd == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
        {
            if (!channel_->writeEnabled())
                channel_->enableWrite(true);
            break;
        }
        else
        {
            error("write error: channel %lld fd %d wd %ld %d %s",
                    (long long)channel_->id(), channel_->fd(), wd, errno, strerror(errno));
            break;
        }
    }
    return sended;
}

void TcpConn::cleanup(const TcpConnPtr &con)
{
    if (readcb_ && input_.size())
    {
        readcb_(con);
    }
    if (state_ == State::Handshaking)
        state_ = State::Failed;
    else
        state_ = State::Closed;
    
    trace("tcp closing %s - %s fd %d %d", local_.toString().c_str(), 
        peer_.toString().c_str(), channel_ ? channel_->fd() : -1, errno);

    getBase()->cancel(timeoutId_);
    if (statecb_)
        statecb_(con);
    if (reconnectInterval_ >= 0 && !getBase()->exited())
    {
        reconnect();
        return ;
    }
    for (auto &idle : idleIds_)
        handyUnregisterIdle(getBase(), idle);
    //channel may have hold TcpConnPtr, set channel_ to NULL before delete
    readcb_ = writablecb_ = statecb_ = nullptr;
    Channel *ch = channel_;
    channel_ = NULL;
    delete ch;
    
}

void TcpConn::connect(EventBase *base, const std::string &host, 
                    unsigned short port, int timeout, const std::string &localip)
{
    fatalif(state_ != State::Invalid && state_ != State::Closed && state_ != State::Failed,
        "current state is bad state to connect. state : %d", state_);
    
    destHost_ = host;
    destPort_ = port;
    connectTimeout_ = timeout;
    connectedTime_ = Util::timeMilli();
    localIp_ = localip;
    Ip4Addr addr(host, port);
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    fatalif(fd < 0, "socket failed %d %s", errno, strerror(errno));
    Net::setNonBlock(fd);
    int t = Util::addFdFlag(fd, FD_CLOEXEC);
    fatalif(t, "addFdFlag FD_CLOEXEC failed %d %s", t, strerror(t));
    int r = 0;
    if (localip.size())
    {
        Ip4Addr addr(localip, 0);
        r = ::bind(fd, (struct sockaddr *)&addr.getAddr(), sizeof(struct sockaddr));
        error("bind to %s failed error %d %s", addr.toString().c_str(), errno, strerror(errno));
    }
    if (r == 0)
    {
        r = ::connect(fd, (sockaddr *)&addr.getAddr(), sizeof(sockaddr_in));
        if (r != 0 && errno != EINPROGRESS)
            error("connect to %s error %d %s", addr.toString().c_str(), errno, strerror(errno));
    }

    sockaddr_in local;
    socklen_t alen = sizeof(local);
    if (r == 0)
    {
        r = getsockname(fd, (sockaddr *)&local, &alen);
        if (r < 0)
            error("getsockname failed %d %s", errno, strerror(errno));
    }
    state_ = State::Handshaking;
    attach(base, fd, Ip4Addr(local), addr);
    if (timeout)
    {
        TcpConnPtr con = shared_from_this();
        timeoutId_ = base->runAfter(timeout, [con]{
            if (con->getState() == Handshaking)
                con->channel_->close();
        });
    }
}

void TcpConn::reconnect()
{
    auto con = shared_from_this();
    getBase()->imp_->reconnectConns_.insert(con);
    long long interval = reconnectInterval_ - (Util::timeMilli() - connectedTime_);
    interval = interval > 0 ? interval : 0;
    info("reconnect interval : %d will reconnect after %lld ms", reconnectInterval_, interval);
    getBase()->runAfter(interval, [this, con](){
        getBase()->imp_->reconnectConns_.erase(con);
        connect(getBase(), destHost_, (unsigned short)destPort_, connectTimeout_, localIp_);
    });
    delete channel_;
    channel_ = NULL;
}

void TcpConn::attach(EventBase *base, int fd, Ip4Addr local, Ip4Addr peer)
{
    fatalif( (destPort_ <= 0 && state_ != State::Invalid) 
            || (destPort_ >= 0 && state_ != State::Handshaking), 
            "you should use a new TcpConn to attach. state: %d", state_);
    base_ = base;
    state_ = State::Handshaking;
    local_ = local;
    peer_ = peer;
    delete channel_;
    channel_ = new Channel(base, fd, kWriteEvent | kReadEvent);
    trace("tcp constructed %s - %s fd: %d", local_.toString().c_str(), peer_.toString().c_str(), fd);
    TcpConnPtr con = shared_from_this();
    con->channel_->onRead( [=]{ con->handleRead(con); } );
    con->channel_->onWrite( [=]{ con->handleWrite(con); } );
}

int TcpConn::handleHandshake(const TcpConnPtr &con)
{
    fatalif(state_ != Handshaking, "handleHandshaking called when state_=%d", state_);
    struct pollfd pfd;
    pfd.fd = channel_->fd();
    pfd.events = POLLOUT | POLLERR;
    //用poll监控channel_上有可写事件到达
    int r = poll(&pfd, 1, 0);
    if (r == 1 && pfd.revents == POLLOUT)
    {
        channel_->enableReadWrite(true, false);
        state_ = State::Connected;
        if (state_ == State::Connected)
        {
            connectedTime_ = Util::timeMilli();
            trace("tcp connected %s - %s fd %d", local_.toString().c_str(), 
                    peer_.toString().c_str(), channel_->fd());
            if (statecb_)
                statecb_(con);
        }
    }
    else
    {
        trace("poll fd %d return %d revents %d", channel_->fd(), r, pfd.revents);
        cleanup(con);
        return -1;
    }
    return 0;
    
}
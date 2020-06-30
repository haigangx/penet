#pragma once

#include <string>
#include <sys/socket.h>
#include <netinet/in.h>

class Ip4Addr
{
public:
    Ip4Addr(const std::string &host, unsigned short port);
    Ip4Addr(unsigned short port = 0) : Ip4Addr("", port) {}
    Ip4Addr(const struct sockaddr_in &addr) : addr_(addr) {}
    ~Ip4Addr();

    std::string toString() const;
    std::string ip() const;
    unsigned short port() const;
    unsigned int ipInt() const;
    bool isIpValid() const;
    struct sockaddr_in &getAddr() { return addr_; }
    static std::string hostToIp(const std::string &host);

private:
    struct sockaddr_in addr_;
};
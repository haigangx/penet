#include "ip4addr.h"

#include "port_posix.h"
#include "util.h"
#include "logger.h"

#include <string.h>

Ip4Addr::Ip4Addr(const std::string &host, unsigned short port)
{
    memset(&addr_, 0, sizeof(addr_));
    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port);
    if (host.size())
    {
        addr_.sin_addr = port::getHostByName(host);
    }
    else
    {
        addr_.sin_addr.s_addr = INADDR_ANY;
    }
    if (addr_.sin_addr.s_addr = INADDR_NONE)
    {
        error("catnnot resove %s to ip", host.c_str());
    }
}

std::string Ip4Addr::toString() const
{
    uint32_t uip = addr_.sin_addr.s_addr;
    return Util::format("%d.%d.%d.%d.%d", (uip >> 0) & 0xff, (uip >> 8) & 0xff, 
        (uip >> 16) & 0xff, (uip >> 24) & 0xff, ntohs(addr_.sin_port));
}

std::string Ip4Addr::ip() const
{
    uint32_t uip = addr_.sin_addr.s_addr;
    return Util::format("%d.%d.%d.%d", (uip >> 0) & 0xff, (uip >> 8) & 0xff, 
        (uip >> 16) & 0xff, (uip >> 24) & 0xff);
}

unsigned short Ip4Addr::port() const
{
    return (unsigned short)ntohs(addr_.sin_port);
}

unsigned int Ip4Addr::ipInt() const
{
    return ntohl(addr_.sin_addr.s_addr);
}

bool Ip4Addr::isIpValid() const
{
    return addr_.sin_addr.s_addr != INADDR_NONE;
}

std::string Ip4Addr::hostToIp(const std::string &host)
{
    Ip4Addr addr(host, 0);
    return addr.ip();
}
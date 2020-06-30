#include "ip4addr.h"

#include "port_posix.h"

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
}
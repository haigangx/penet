#include "net.h"
#include <fcntl.h>
#include <errno.h>

int Net::setNonBlock(int fd, bool value)
{
    int old_flags = fcntl(fd, F_GETFL);
    if (old_flags < 0)
        return errno;

    if (value)
    {
        return fcntl(fd, F_SETFL, old_flags | O_NONBLOCK);
    }
    else
    {
        return fcntl(fd, F_SETFL, old_flags | ~O_NONBLOCK);
    }
}

int Net::setReuseAddr(int fd, bool value)
{
    return 0;
}

int Net::setReusePort(int fd, bool value)
{
    return 0;
}

int Net::setNoDelay(int fd, bool value)
{
    return 0;
}
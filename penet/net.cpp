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
#pragma once

#include "port_posix.h"

class Net
{
public:

    template <class T>
    static T hton(T v) { return port::htobe(v); }
    template <class T>
    static T ntoh(T v) { return port::htobe(v); }
    //为文件描述符设置非阻塞标志
    static int setNonBlock(int fd, bool value = true);
    static int setReuseAddr(int fd, bool value = true);
    static int setReusePort(int fd, bool value = true);
    static int setNoDelay(int fd, bool value = true);
};
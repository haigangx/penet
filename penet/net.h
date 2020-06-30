#pragma once

class Net
{
public:
    //为文件描述符设置非阻塞标志
    static int setNonBlock(int fd, bool value = true);
    static int setReuseAddr(int fd, bool value = true);
    static int setReusePort(int fd, bool value = true);
    static int setNoDelay(int fd, bool value = true);
};
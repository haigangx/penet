#pragma once

class Net
{
public:
    //为文件描述符设置非阻塞标志
    static int setNonBlock(int fd, bool value = true);
};
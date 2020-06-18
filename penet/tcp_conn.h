#pragma once

#include "buffer.h"

class TcpConn
{
public:
    TcpConn(){};
    ~TcpConn(){};

    Buffer getInput(){return Buffer();};
    void send(Buffer msg){};
};

#pragma once

#include "define.h"
#include <signal.h>

class Signal
{
public:
    static void signal(int sig, SIGNAL_FUNC handler);
};

#pragma once

#include <string>
#include <string.h>

class Util
{
public:
    static std::string format(const char *fmt, ...);
    static int64_t timeMicro();
    static int64_t timeMilli() { return timeMicro() / 1000; }
    static int64_t steadyMicro();
    static int64_t steadyMilli() { return steadyMicro() / 1000; }
    static std::string readableTime(time_t t);
    static int64_t atoi(const char *b, const char *e) 
        { return strtol(b, (char **)&e, 10); }
    static int64_t atoi2(const char *b, const char *e);
    static int64_t atoi(const char *b)
        { return atoi(b, b + strlen(b)); }
    static int addFdFlag(int fd, int flag);
};
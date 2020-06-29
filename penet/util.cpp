#include "util.h"
#include <fcntl.h>
#include <chrono>

using namespace std;

std::string Util::format(const char *fmt, ...)
{

}

int64_t Util::timeMicro()
{
    chrono::time_point<chrono::system_clock> p = chrono::system_clock::now();
    return chrono::duration_cast<chrono::microseconds>(p.time_since_epoch()).count();
}

int64_t Util::steadyMicro()
{
    chrono::time_point<chrono::steady_clock> p = chrono::steady_clock::now();
    return chrono::duration_cast<chrono::microseconds>(p.time_since_epoch()).count();
}

std::string Util::readableTime(time_t t)
{
    struct tm tm1;
    localtime_r(&t, &tm1);
    return format("%04d-%02d-%02d %02d:%02d:%02d", tm1.tm_year + 1900, 
                 tm1.tm_mon + 1, tm1.tm_mday, tm1.tm_hour, tm1.tm_min, tm1.tm_sec);
}

int64_t Util::atoi2(const char *b, const char *e)
{
    char **ne = (char **)&e;
    int64_t v = strtol(b, ne, 10);
    return ne == (char **)&e ? v : -1;
}

int Util::addFdFlag(int fd, int flag)
{
    int old_flag = fcntl(fd, F_GETFL);
    return fcntl(fd, F_SETFL, old_flag | flag);
}
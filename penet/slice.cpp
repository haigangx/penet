#include "slice.h"

Slice Slice::eatWord()
{
    const char *b = pb_;
    while (b < pe_ && isspace(*b))
    {
        b++;
    }
    const char *e = b;
    while (e < pe_ && !isspace(*e))
    {
        e++;
    }
    pb_ = e;
    return Slice(b, e - b);
}

Slice Slice::eatLine()
{
    const char *p = pb_;
    while (pb_ < pe_ && *pb_ != '\n' && *pb_ != '\r')
    {
        pb_++;
    }
    return Slice(p, pb_ - p);
}

Slice &Slice::trimSpace()
{
    while (pb_ < pe_ && isspace(*pb_))
    {
        pb_++;
    }
    while (pb_ < pe_ && isspace(pe_[-1]))
    {
        pe_--;
    }
    return *this;
}

int Slice::compare(const Slice &b) const
{
    size_t sz = size(), bsz = b.size();
    const int min_len = (sz < bsz) ? sz : bsz;
    int r = memcmp(pb_, b.pb_, min_len);
    if (r == 0)
    {
        if (sz < bsz)
            r = -1;
        else if (sz > bsz)
            r = 1;
    }
    return r;
}

std::vector<Slice> Slice::split(char ch) const
{
    std::vector<Slice> r;
    const char *pb = pb_;
    for (const char *p = pb_; p < pe_; p++)
    {
        if (*p == ch)
        {
            r.push_back(Slice(pb, p));
            pb = p + 1;
        }
    }
}

#pragma once

#include <string>
#include <string.h>
#include <vector>

class Slice
{
public:
    Slice() : pb_("") { pe_ = pb_; }
    Slice(const char *b, const char *e)  : pb_(b), pe_(e) {}
    Slice(const char *d, size_t n) : pb_(d), pe_(d + n) {}
    Slice(const std::string &s) : pb_(s.data()), pe_(s.data() + s.size()) {}
    Slice(const char *s) : pb_(s), pe_(s + strlen(s)) {}

    const char *data() const { return pb_; }
    const char *begin() const { return pb_; }
    const char *end() const { return pe_; }

    char front() { return *pb_; }
    char back() { return pe_[-1]; }
    size_t size() const { return pe_ - pb_; }
    void resize(size_t sz) { pe_ = pb_ + sz; }
    inline bool empty() const { return pe_ == pb_; }
    void clear() { pe_ = pb_ = ""; }

    //返回eat掉的数据
    Slice eatWord();
    Slice eatLine();
    Slice eat(int sz)
    {
        Slice s(pb_, sz);
        pb_ += sz;
        return s;
    }
    Slice sub(int boff, int eoff = 0) const
    {
        Slice s(*this);
        s.pb_ += boff;
        s.pe_ += eoff;
        return s;
    }
    Slice &trimSpace();

    inline char operator[](size_t n) const { return pb_[n]; }

    std::string toString() const { return std::string(pb_, pe_); }
    int compare(const Slice &b) const;

    bool starts_with(const Slice &x) const 
        { return (size() >= x.size()  && memcmp(pb_, x.pb_, x.size()) == 0);}
    bool end_with(const Slice &x) const
        { return (size() >= x.size() && memcmp(pe_-x.size(), x.pb_, x.size()) == 0); }
    operator std::string() const
        { return std::string(pb_, pe_); }

    std::vector<Slice> split(char ch) const;


private:
    const char *pb_;
    const char *pe_;
};

bool operator<(const Slice &x, const Slice &y)
{
    return x.compare(y) < 0;
}

bool operator==(const Slice &x, const Slice &y)
{
    return ((x.size() == y.size()) && (memcmp(x.data(), y.data(), x.size()) == 0));
}

bool operator!=(const Slice &x, const Slice &y)
{
    return !(x == y);
}

#pragma once

#include <netinet/in.h>
#include <string>

namespace port
{

static const int kLittleEndian = LITTLE_ENDIAN;

inline uint16_t htobe(uint16_t v);

inline uint32_t htobe(uint32_t v);

inline uint64_t htobe(uint64_t v);

inline int16_t htobe(int16_t v)
{ return (int16_t)htobe((uint16_t)v); }

inline int32_t htobe(int32_t v)
{ return (int32_t)htobe((uint32_t)v); }

inline int64_t htobe(int64_t v)
{ return (int64_t)htobe((uint64_t)v); }

struct in_addr getHostByName(const std::string &host);

uint64_t gettid();

};
#include "randXorShift.h"

unsigned long RandXorshift128()
{ // 2^128-1
    static unsigned long x = 123456789, y = 362436069, z = 521288629, w = 88675123;
    unsigned long t;
    t = (x ^ (x << 11)) & 0xffffffff;
    x = y;
    y = z;
    z = w;
    return (w = (w ^ (w >> 19)) ^ (t ^ (t >> 8)));
}

uint64 Rand64()
{
    unsigned long r1 = RandXorshift128();
    unsigned long r2 = RandXorshift128();
    uint64 r = ((uint64)r1 << 32) | r2;
    return r;
}

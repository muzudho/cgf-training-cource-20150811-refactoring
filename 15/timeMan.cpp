#include <time.h>
#include "timeMan.h"

double TimeMan::GetClock()
{
#if defined(_MSC_VER)
    return clock();
#else
    struct timeval val;
    struct timezone zone;
    if (gettimeofday(&val, &zone) == -1)
    {
        Prt("time err\n");
        exit(0);
    }
    double t = val.tv_sec * 1000.0 + (val.tv_usec / 1000.0);
    return t;
#endif
}

double TimeMan::GetSpendTime(double ct)
{
    //int div = CLOCKS_PER_SEC;	// 1000 ...VC, 1000000 ...gcc
    int div = 1000;
    return (double)(GetClock() + 1 - ct) / div;
}

int TimeMan::IsTimeOver()
{
    if (GetSpendTime(start_time) >= time_limit_sec)
        return 1;

    return 0;
}

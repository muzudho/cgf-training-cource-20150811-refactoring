#include <time.h>

/// <summary>
/// ミリ秒を取得します。
/// get mill second time.
///
/// clock()はLinuxならプロセスのCPU時間を返しますが、マルチスレッドでは適切ではありません。
/// clock() returns process CPU times on Linux, not proper when multi thread.
/// </summary>
/// <returns></returns>
double GetClock()
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

/// <summary>
/// 秒を取得します。
/// get sec time.
/// </summary>
/// <param name="ct"></param>
/// <returns></returns>
double GetSpendTime(double ct)
{
    //int div = CLOCKS_PER_SEC;	// 1000 ...VC, 1000000 ...gcc
    int div = 1000;
    return (double)(GetClock() + 1 - ct) / div;
}

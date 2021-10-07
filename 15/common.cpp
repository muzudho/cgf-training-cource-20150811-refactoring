#include "common.h"
#include "position.h"

int GetZ(int x, int y)
{
    return y * kWidth + x;
}

int Get81(int z)
{
    // 段
    int y = z / kWidth;

    // 筋
    // 106 = 9*11 + 7 = (x,y)=(7,9) -> 79
    int x = z - y * kWidth;

    // パスなら0を返します
    if (z == 0)
        return 0;

    // 人が読める形の座標
    // x*100+y for 19x19
    return x * 10 + y;
}

char *GetCharZ(int z)
{
    // 筋
    int x;

    // 段
    int y;

    // 筋アルファベット
    int ax;

    // 文字列バッファー
    static char buf[16];

    // パス
    sprintf(buf, "pass");
    if (z == 0)
        return buf;

    y = z / kWidth;
    x = z - y * kWidth;
    ax = x - 1 + 'A';

    // 筋に I列は無いので詰めます
    if (ax >= 'I')
        ax++; // from 'A' to 'T', excluding 'I'

    sprintf(buf, "%c%d", ax, kBoardSize + 1 - y);

    return buf;
}

void Prt(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    //{ FILE *fp = fopen("out.txt","a"); if ( fp ) { vfprt( fp, fmt, ap ); fclose(fp); } }
    vfprintf(stderr, fmt, ap);
    va_end(ap);
}

void SendGtp(const char* fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vfprintf(stdout, fmt, ap);
    va_end(ap);
}

#pragma once

/// <summary>
/// 時間管理（Time Manager）
/// </summary>
class TimeMan
{
public:
    /// <summary>
    /// 開始時刻
    /// </summary>
    double start_time;

    /// <summary>
    /// 時間制限秒
    /// </summary>
    double time_limit_sec = 3.0;

    /// <summary>
    /// ミリ秒を取得します。
    /// get mill second time.
    ///
    /// clock()はLinuxならプロセスのCPU時間を返しますが、マルチスレッドでは適切ではありません。
    /// clock() returns process CPU times on Linux, not proper when multi thread.
    /// </summary>
    /// <returns></returns>
    double GetClock();

    /// <summary>
    /// 秒を取得します。
    /// get sec time.
    /// </summary>
    /// <param name="ct"></param>
    /// <returns></returns>
    double GetSpendTime(double ct);

    /// <summary>
    /// 時間切れかどうか
    /// </summary>
    /// <returns>時間切れなら1を、そうでなければ0</returns>
    int IsTimeOver();
};

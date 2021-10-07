#pragma once

#include "go15.h"
#include "position.h"

/// <summary>
/// 最大の子数。9路なら82個。+1 for PASS
/// </summary>
const int kChildSize = (kBoardSize * kBoardSize + 1);

/// <summary>
/// 手を保存するための構造体
/// </summary>
class Child
{
public:
    /// <summary>
    /// 手の場所（move position）
    /// </summary>
    int z;

    /// <summary>
    /// 試した回数（number of games）
    /// </summary>
    int games;

    /// <summary>
    /// 勝率（winrate）
    /// </summary>
    double rate;

    /// <summary>
    /// (RAVE) number of games
    /// </summary>
    int rave_games;

    /// <summary>
    /// (RAVE) winrate
    /// </summary>
    double rave_rate;

    /// <summary>
    /// ノードのリストのインデックス。次のノード（next node）を指す
    /// </summary>
    int next;

    /// <summary>
    /// 人間的に盤面上の3x3のパターンの形を考えると悪手なので、着手の確率を下げるための割引率 0.0～1.0（shape bonus）
    /// </summary>
    double bonus;
};

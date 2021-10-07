#pragma once

#include "child.h"

/// <summary>
/// no next node
/// </summary>
const int NODE_EMPTY = -1;

/// <summary>
/// 局面を保存するものです
/// </summary>
class Node
{
public:
    /// <summary>
    /// 実際の子どもの数
    /// </summary>
    int child_num;

    Child children[kChildSize];

    /// <summary>
    /// 何回の対局でこのノードに来たか（子の合計）
    /// </summary>
    int child_games_sum;

    /// <summary>
    /// レーブ？な対局数？（子の合計）
    /// </summary>
    int child_rave_games_sum;

    void AddChild(int z, double bonus);
};

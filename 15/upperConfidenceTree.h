#pragma once

#include "node.h"

/// <summary>
/// 最大10000局面まで
/// </summary>
const int kNodeMax = 10000;

/// <summary>
/// `UCT` - 探索と知識利用のバランスを取る手法
/// </summary>
class UpperConfidenceTree
{
public:

    /// <summary>
    /// number of uct loop
    /// </summary>
    int loop = 1000000;

    /// <summary>
    /// ノードのリスト
    /// </summary>
    Node nodeList[kNodeMax];

    /// <summary>
    /// ノードのリストのサイズ。登録局面数
    /// </summary>
    int node_num = 0;

    int CreateNode(int prev_z);
    int SelectBestUcb(int node_n, int color);
    void UpdateRave(Node *pN, int color, int current_depth, double win);
    int SearchUct(int color, int node_n);
    int GetBestUct(int color);
};

#pragma once

#include "node.h"

/// <summary>
/// `UCT` - 探索と知識利用のバランスを取る手法
/// </summary>
class UpperConfidenceTree
{
public:
    int CreateNode(int prev_z);
    int SelectBestUcb(int node_n, int color);
    void UpdateRave(Node *pN, int color, int current_depth, double win);
    int SearchUct(int color, int node_n);
    int GetBestUct(int color);
};

#include "node.h"

/// <summary>
/// リストの末尾に要素を追加。手を追加。
/// この手を打った後のノードは、なし
/// </summary>
/// <param name="pN">局面</param>
/// <param name="z">手の座標</param>
/// <param name="bonus">
/// 人間的に考えて悪手なので、着手の確率を下げるための割引率 0.0～1.0
/// from 0 to 10, good move has big bonus
/// </param>
void Node::AddChild(int z, double bonus)
{
    // 新しい要素のインデックス
    int n = this->child_num;
    this->children[n].z = z;
    this->children[n].games = 0;
    this->children[n].rate = 0;
    this->children[n].rave_games = 0;
    this->children[n].rave_rate = 0;
    this->children[n].next = NODE_EMPTY;
    this->children[n].bonus = bonus;
    // ノードのリストのサイズ更新
    this->child_num++;
}

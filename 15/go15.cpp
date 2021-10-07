// sprintf is Err in VC++
#define _CRT_SECURE_NO_WARNINGS

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>
#include "go15.h"
#include "timeMan.h"
#include "child.h"
#include "node.h"
#include "upperConfidenceTree.h"
#include "position.h"
#include "common.h"
#include "randXorShift.h"

Position position = Position();

/// <summary>
/// UCTで使われる経路（パス）
/// </summary>
int path[kDMax];

/// <summary>
/// UCTで使われる経路（パス）の先頭がらn番目(0開始)
/// </summary>
int depth;

/// <summary>
/// 1...black, 2...white, 3...ko
/// </summary>
const int kHashKinds = 4;

/// <summary>
/// コウ？
/// </summary>
const int kHashKo = 3;

/// <summary>
/// 盤の各交点の、種類別のハッシュ？
/// </summary>
uint64 hashboard[kBoardMax][kHashKinds];

/// <summary>
/// ハッシュ コード？
/// </summary>
uint64 hashcode = 0;

/// <summary>
/// GTPプロトコルとして送信せずに 引数r を表示？
/// </summary>
/// <param name="r"></param>
void PrtCode64(uint64 r)
{
    //Prt("%016" PRIx64,r);
    Prt("%08x%08x", (int)(r >> 32), (int)r);
};

/// <summary>
/// 盤の各交点に乱数を割り振っている？
/// </summary>
void MakeHashboard()
{
    int z, i;
    for (z = 0; z < kBoardMax; z++)
    {
        //  Prt("[%3d]=",z);
        for (i = 0; i < kHashKinds; i++)
        {
            hashboard[z][i] = Rand64();
            //    PrtCode64(hashboard[z][i]); Prt(",");
        }
        //  Prt("\n");
    }
}

/// <summary>
/// ハッシュコードの 2進数の 0と1 をひっくり返す？
/// </summary>
void HashPass()
{
    hashcode = ~hashcode;
}

/// <summary>
/// XOR演算する？
/// </summary>
/// <param name="z">着手点</param>
/// <param name="color">手番の色</param>
void HashXor(int z, int color)
{
    // 指定座標、指定色のハッシュの 2進数の 0と1 を反転させたものを ハッシュコードに記憶させている？
    hashcode ^= hashboard[z][color];
}

/// <summary>
/// 石の色を反転
/// </summary>
/// <param name="col">石の色</param>
/// <returns>反転した石の色</returns>
int FlipColor(int col)
{
    return 3 - col;
}

/// <summary>
/// 盤領域を 標準エラー出力へ出力？
/// </summary>
void Position::PrintBoardArea()
{
    int x, y;
    int all = all_playouts;
    if (all == 0)
        all = 1;

    Prt("board_area_sum\n   ");
    for (x = 0; x < kBoardSize; x++)
        Prt("   %c", 'A' + x + (x > 7));
    Prt("\n");
    for (y = 0; y < kBoardSize; y++)
    {
        Prt("%2d ", kBoardSize - y);
        for (x = 0; x < kBoardSize; x++)
        {
            int sum = board_area_sum[GetZ(x + 1, y + 1)];
            double per = 100.0 * sum / all;
            Prt("%4.0f", per);
        }

        Prt("\n");
    }
}

/// <summary>
/// 致命さ？
/// </summary>
/// <param name="z">着手点</param>
/// <returns>致命さを表す数？</returns>
double Position::GetCriticality(int z)
{
    double all = all_playouts + 1;
    double v = board_winner[0][z] + board_winner[1][z];
    double per = v / all;
    double bp = (double)board_winner[0][z] * winner_count[0] / (all * all);
    double wp = (double)board_winner[1][z] * winner_count[1] / (all * all);
    double criticality = (per - (bp + wp));
    return criticality;
}

/// <summary>
/// 致命さをエラー出力へ出力？
/// </summary>
void Position::PrintCriticality()
{
    int x, y;

    Prt("criticality\n  ");
    for (x = 0; x < kBoardSize; x++)
        Prt("    %c", 'A' + x + (x > 7));
    Prt("\n");
    for (y = 0; y < kBoardSize; y++)
    {
        Prt("%2d ", kBoardSize - y);
        for (x = 0; x < kBoardSize; x++)
        {
            double crt = GetCriticality(GetZ(x + 1, y + 1));
            Prt("%5.2f", crt);
        }
        Prt("\n");
    }
}

/// <summary>
/// count_liberty関数の中で呼び出されます。再帰
/// </summary>
/// <param name="tz">着手（開始）座標</param>
/// <param name="color">連の色</param>
/// <param name="p_liberty">呼吸点の数</param>
/// <param name="p_stone">連の石の数</param>
void Position::CountLibertySub(int tz, int color, int* p_liberty, int* p_stone)
{
    // 着手点
    int z;

    // ループ カウンター
    int i;

    check_board[tz] = 1; // search flag
    (*p_stone)++;        // number of stone
    for (i = 0; i < 4; i++)
    {
        z = tz + kDir4[i];
        if (check_board[z])
            continue;
        if (Board[z] == 0)
        {
            check_board[z] = 1;
            (*p_liberty)++; // number of liberty
        }
        if (Board[z] == color)
            CountLibertySub(z, color, p_liberty, p_stone);
    }
}

/// <summary>
/// 呼吸点の数
/// </summary>
/// <param name="tz">着手座標</param>
/// <param name="p_liberty">呼吸点の数</param>
/// <param name="p_stone">連の石の数</param>
void Position::CountLiberty(int tz, int* p_liberty, int* p_stone)
{
    int i;
    *p_liberty = *p_stone = 0;
    for (i = 0; i < kBoardMax; i++)
        check_board[i] = 0;
    CountLibertySub(tz, Board[tz], p_liberty, p_stone);
}

/// <summary>
/// 石を取り上げます
/// </summary>
/// <param name="tz">着手座標</param>
/// <param name="color">石の色</param>
void Position::TakeStone(int tz, int color)
{
    int z, i;

    HashXor(tz, color);
    Board[tz] = 0;
    for (i = 0; i < 4; i++)
    {
        z = tz + kDir4[i];
        if (Board[z] == color)
            TakeStone(z, color);
    }
}

/// <summary>
/// 目潰しをエラーとするなら
/// </summary>
const int kFillEyeErr = 1;

/// <summary>
/// 目潰しを合法手とするなら（囲碁のルールでは合法手）
/// </summary>
const int kFillEyeOk = 0;

/// <summary>
/// put stone.
/// </summary>
/// <param name="tz">着手座標。0ならパス</param>
/// <param name="color">石の色</param>
/// <param name="fill_eye_err">目潰しをエラーとするなら1、そうでないなら0</param>
/// <returns>エラーコード。success returns 0. in playout, fill_eye_err = 1</returns>
int Position::PutStone(int tz, int color, int fill_eye_err)
{
    // 検索情報を覚えておく配列
    int around[4][3];

    // 相手の石の色
    int un_col = FlipColor(color);

    // 空白に石を置いたら1
    int space = 0;

    // 壁に石を置いたら1
    int wall = 0;

    // 自殺手になってしまうとき1
    int mycol_safe = 0;

    // 取り上げた石の数
    int capture_sum = 0;

    // コウかもしれないとき1
    int ko_maybe = 0;

    // 呼吸点の数
    int liberty;

    // 連の石の数
    int stone;

    // ループ・カウンタ
    int i;

    // pass
    if (tz == 0)
    {
        if (ko_z != 0)
            HashXor(ko_z, kHashKo);

        ko_z = 0;
        HashPass();
        return 0;
    }

    // count 4 neighbor's liberty and stones.
    for (i = 0; i < 4; i++)
    {
        int z, c, liberty, stone;
        around[i][0] = around[i][1] = around[i][2] = 0;

        // 隣の座標
        z = tz + kDir4[i];
        c = Board[z]; // color

        // もし、隣が空点なら
        if (c == 0)
            space++;

        // もし、隣が壁なら
        if (c == 3)
            wall++;

        // もし、隣が空点または壁なら
        if (c == 0 || c == 3)
            continue;

        // 呼吸点の数と、連の石の数を数えます
        CountLiberty(z, &liberty, &stone);

        // 隣の石が相手の色で、呼吸点が1なら、その石を取れます
        around[i][0] = liberty;
        around[i][1] = stone;
        around[i][2] = c;

        // 隣の石が相手の色で、呼吸点が1なら、その石を取れます
        if (c == un_col && liberty == 1)
        {
            capture_sum += stone;
            ko_maybe = z;
        }

        // もし隣に自分の色の石があっても、その石の呼吸点が２以上あればセーフ
        if (c == color && liberty >= 2)
            mycol_safe++;
    }

    // 石を取っておらず、隣に空点がなく、隣に呼吸点が２つ以上空いている自分の石もないなら、自殺手
    if (capture_sum == 0 && space == 0 && mycol_safe == 0)
        return 1; // suicide

    // もし、コウの座標に石を置こうとしたら、コウ
    if (tz == ko_z)
        return 2; // ko

    // もし、目の座標に石を置こうとしたら、目潰し
    if (wall + mycol_safe == 4 && fill_eye_err)
        return 3; // eye

    // もし、石の上に石を置こうとしたら、反則手
    if (Board[tz] != 0)
        return 4;

    // 取れる相手の石を取ります
    for (i = 0; i < 4; i++)
    {
        int lib = around[i][0];
        int c = around[i][2];
        if (c == un_col && lib == 1 && Board[tz + kDir4[i]] != 0)
        {
            TakeStone(tz + kDir4[i], un_col);
        }
    }

    // 石を置きます
    Board[tz] = color;

    // 着手点のビット列を、XOR演算でひっくり返します
    HashXor(tz, color);

    // ハッシュコードのビット列の 0,1 をひっくり返している？
    HashPass();

    // コウであれば、コウの場所のビット列の 0,1 をひっくり返している？
    if (ko_z != 0)
        HashXor(ko_z, kHashKo);

    // 着手点を含む連の呼吸点の数を数えます
    CountLiberty(tz, &liberty, &stone);

    // 石を1個取ったらコウかも知れない
    if (capture_sum == 1 && stone == 1 && liberty == 1)
    {
        ko_z = ko_maybe;
        HashXor(ko_z, kHashKo);
    }
    else
    {
        ko_z = 0;
    }
    return 0;
}

/// <summary>
/// 地の簡易計算（これが厳密に計算できるようなら囲碁は完全解明されている）を表示し、勝敗を返します。
/// スコアにはコミは含みませんが、勝敗にはコミを含んでいます。
/// </summary>
/// <param name="turn_color">手番の色</param>
/// <returns>黒の勝ちなら1、負けなら0</returns>
int Position::CountScore(int turn_color)
{
    // 筋
    int x;

    // 段
    int y;

    // ループ カウンター
    int i;

    // 黒のスコア
    int score = 0;

    // 手番の勝ちなら1、負けなら0
    int win;

    // 黒石の数、白石の数
    int black_area = 0, white_area = 0;

    // 石の数＋地の数
    int black_sum, white_sum;

    // 4方向にある石について、[ゴミ値, 盤上の黒石の数, 盤上の白石の数]
    int mk[4];

    // [空点の数, 黒石の数, 白石の数]
    int kind[3];

    kind[0] = kind[1] = kind[2] = 0;
    for (y = 0; y < kBoardSize; y++)
        for (x = 0; x < kBoardSize; x++)
        {
            int z = GetZ(x + 1, y + 1);
            int c = Board[z];
            kind[c]++;

            // 石が置いてある座標なら以降は無視
            if (c != 0)
            {
                if (c == 1)
                    board_area_sum[z]++;
                if (c == 2)
                    board_area_sum[z]--;
                continue;
            }

            // 大雑把に さくっと地計算。
            // 4方向にある黒石、白石の数を数えます
            mk[1] = mk[2] = 0;
            for (i = 0; i < 4; i++)
                mk[Board[z + kDir4[i]]]++;

            // 黒石だけがあるなら黒の地
            if (mk[1] && mk[2] == 0)
            {
                black_area++;
                board_area_sum[z]++;
            }

            // 白石だけがあるなら白の地
            if (mk[2] && mk[1] == 0)
            {
                white_area++;
                board_area_sum[z]--;
            }
        }

    // スコア計算（コミ含まず）
    black_sum = kind[1] + black_area;
    white_sum = kind[2] + white_area;
    score = black_sum - white_sum;

    // 黒の勝敗判定
    win = 0;
    if (score - komi > 0)
        win = 1;

    if (win == 1)
    {
        // 黒番の勝ちなら
        // black win
        for (y = 0; y < kBoardSize; y++)
            for (x = 0; x < kBoardSize; x++)
            {
                // 着手点
                int z = GetZ(x + 1, y + 1);

                // 同色
                if (Board[z] == 1)
                    // 集計
                    board_winner[0][z]++;
            }

        // 黒勝ちのカウント
        winner_count[0]++;
    }
    else
    {
        // 白番の勝ちなら
        for (y = 0; y < kBoardSize; y++)
            for (x = 0; x < kBoardSize; x++)
            {
                // 着手点
                int z = GetZ(x + 1, y + 1);

                // 同色
                if (Board[z] == 2)
                    // 集計
                    board_winner[1][z]++;
            }

        // 白勝ちのカウント
        winner_count[1]++;
    }

    // 白番なら勝敗を反転
    if (turn_color == 2)
        win = -win;

    //Prt("black_sum=%2d, (stones=%2d, area=%2d)\n",black_sum, kind[1], black_area);
    //Prt("white_sum=%2d, (stones=%2d, area=%2d)\n",white_sum, kind[2], white_area);
    //Prt("score=%d, win=%d\n",score, win);
    return win;
}

/// <summary>
/// プレイアウトします
/// </summary>
/// <param name="turn_color">手番の石の色</param>
/// <returns>黒の勝ちなら1、負けなら0</returns>
int Position::Playout(int turn_color)
{
    // 手番の色
    int color = turn_color;

    // １つ前の着手の座標
    int previous_z = 0;

    // ループ・カウンタ
    int loop;
    int loop_max = kBoardSize * kBoardSize + 200; // for triple ko

    // プレイアウト回数のカウント
    all_playouts++;

    for (loop = 0; loop < loop_max; loop++)
    {
        // all empty points are candidates.
        // [0]...z, [1]...probability
        int empty[kBoardMax][2];

        // 配列のインデックス
        int empty_num = 0;

        // 空点を選ぶ確率？
        int prob_sum = 0;
        // 壁を除く盤上の全ての空点の(座標,確率)を empty配列にセットします
        int x, y, z, err, pr;
        for (y = 0; y < kBoardSize; y++)
            for (x = 0; x < kBoardSize; x++)
            {
                int z = GetZ(x + 1, y + 1);
                if (Board[z] != 0)
                    continue;
                empty[empty_num][0] = z;
                // 空点を選ぶ確率？
                pr = 100;
                //    pr = get_prob(z, previous_z, color);
                empty[empty_num][1] = pr;
                prob_sum += pr;
                empty_num++;
            }

        for (;;)
        {
            int i = 0;
            // もし空点がなければ、パス
            if (empty_num == 0)
            {
                z = 0;
            }
            else
            {
                // 確率的に空点を選んでいる？
                int r = rand() % prob_sum;
                int sum = 0;
                for (i = 0; i < empty_num; i++)
                {
                    sum += empty[i][1]; // 0,1,2   [0]=1, [1]=1, [2]=1
                    if (sum > r)
                        break;
                }
                if (i == empty_num)
                {
                    Prt("Err! prob_sum=%d,sum=%d,r=%d,r=%d\n", prob_sum, sum, r, i);
                    exit(0);
                }
                z = empty[i][0];
            }
            err = PutStone(z, color, kFillEyeErr);
            if (err == 0)
                break; // pass is ok.

            // もし空点に石を置くと正常終了しなかったなら、残りの座標で続行します
            prob_sum -= empty[i][1];
            empty[i][0] = empty[empty_num - 1][0]; // err, delete
            empty[i][1] = empty[empty_num - 1][1];
            empty_num--;
        }

        // テストでプレイアウトするのなら、棋譜に手を記録します
        if (flag_test_playout)
            record[moves++] = z;

        // 経路（パス）の深さに配列サイズ上まだ余裕があれば、着手点を記憶します
        if (depth < kDMax)
            path[depth++] = z;

        // もしパスが連続したら対局終了
        if (z == 0 && previous_z == 0)
            break; // continuous pass

        // そうでなければ盤を表示して手番を変えて続行
        previous_z = z;
        //  Prt("loop=%d,z=%s,c=%d,empty_num=%d,ko_z=%d\n",loop,GetCharZ(z),color,empty_num,ko_z);

        // 手番を反転
        color = FlipColor(color);
    }

    return CountScore(turn_color);
}

/// <summary>
/// 原始モンテカルロ（ネガマックス）
/// </summary>
/// <param name="color">手番の色</param>
/// <returns>最善手の座標</returns>
int Position::PrimitiveMonteCalro(int color)
{
    // number of playout
    int try_num = 30;

    // 最善手の着手点
    int best_z = 0;

    // 最善の価値
    double best_value;

    // 勝率
    double win_rate;

    // 筋
    int x;

    // 段
    int y;

    // エラーコード
    int err;

    // ループ カウンター
    int i;

    // プレイアウトして勝った回数
    int win_sum;

    // 手番が勝ったら1、負けたら0
    int win;

    // コウの座標のコピー
    int ko_z_copy;

    // 盤のコピー
    // keep current board
    int board_copy[kBoardMax];
    ko_z_copy = ko_z;
    memcpy(board_copy, Board, sizeof(Board));

    // 根ノードでは最低-100点から
    best_value = -100;

    // try all empty point
    for (y = 0; y < kBoardSize; y++)
        for (x = 0; x < kBoardSize; x++)
        {
            // 石を置く座標
            int z = GetZ(x + 1, y + 1);

            // 空点でなければ無視
            if (Board[z] != 0)
                continue;

            // 目潰ししないように石を置く
            err = PutStone(z, color, kFillEyeErr);
            if (err != 0)
                continue;

            // 勝った回数
            win_sum = 0;
            for (i = 0; i < try_num; i++)
            {
                // 現局面を退避
                int board_copy2[kBoardMax];
                int ko_z_copy2 = ko_z;
                memcpy(board_copy2, Board, sizeof(Board));

                // プレイアウト
                win = -Playout(FlipColor(color));
                win_sum += win;

                // 現局面に復元
                ko_z = ko_z_copy2;
                memcpy(Board, board_copy2, sizeof(Board));
            }

            // 勝率
            win_rate = (double)win_sum / try_num;
            //  PrintBoard();
            //  Prt("z=%d,win=%5.3f\n",Get81(z),win_rate);

            // 最善手の更新
            if (win_rate > best_value)
            {
                best_value = win_rate;
                best_z = z;
                //    Prt("best_z=%d,color=%d,v=%5.3f,try_num=%d\n",Get81(best_z),color,best_value,try_num);
            }

            // コウの復元
            ko_z = ko_z_copy;
            // 盤の復元
            memcpy(Board, board_copy, sizeof(Board)); // resume board
        }

    return best_z;
}

// following are for UCT(Upper Confidence Tree)
// `UCT` - 探索と知識利用のバランスを取る手法

// 以下、探索木全体を保存
UpperConfidenceTree uct = UpperConfidenceTree();

/// <summary>
/// illegal move
/// </summary>
const int ILLEGAL_Z = -1;

/// <summary>
/// create new node.
/// 空点を全部追加。
/// PASSも追加。
/// </summary>
/// <param name="prev_z">前回の着手の座標</param>
/// <returns>ノードのリストのインデックス。作られたノードを指す。最初は0から</returns>
int UpperConfidenceTree::CreateNode(int prev_z)
{
    // 筋
    int x;

    // 段
    int y;

    // 着手点
    int z;

    // ループ カウンター
    int i, j;

    // この局面
    Node* pN;

    // これ以上増やせません
    if (node_num == kNodeMax)
    {
        Prt("nodeList over Err\n");
        exit(0);
    }

    // 末尾の未使用の要素
    pN = &nodeList[node_num];
    pN->child_num = 0;
    pN->child_games_sum = 0;
    pN->child_rave_games_sum = 0;

    // 空点をリストの末尾に追加
    for (y = 0; y < kBoardSize; y++)
        for (x = 0; x < kBoardSize; x++)
        {
            z = GetZ(x + 1, y + 1);
            if (position.Board[z] != 0)
                continue;
            pN->AddChild(z, 0);
        }
    pN->AddChild(0, 0); // add PASS

    // sort children
    for (i = 0; i < pN->child_num - 1; i++)
    {
        double max_b = pN->children[i].bonus;
        int max_i = i;
        Child tmp;
        for (j = i + 1; j < pN->child_num; j++)
        {
            Child* c = &pN->children[j];
            if (max_b >= c->bonus)
                continue;
            max_b = c->bonus;
            max_i = j;
        }
        if (max_i == i)
            continue;
        tmp = pN->children[i];
        pN->children[i] = pN->children[max_i];
        pN->children[max_i] = tmp;
    }

    // 末尾に１つ追加した分、リストのサイズ１つ追加
    node_num++;

    // 最後の要素を指すインデックスを返します
    return node_num - 1;
}

/// <summary>
/// UCBが最大の手を返します。
/// 一度も試していない手は優先的に選びます。
/// 定数 Ｃ は実験で決めてください。
/// PASS があるので、すべての手がエラーはありえません。
/// </summary>
/// <param name="node_n">ノードのリストのインデックス</param>
/// <param name="color">手番の色</param>
/// <returns>ノードのリストのインデックス。選択した子ノードを指します</returns>
int UpperConfidenceTree::SelectBestUcb(int node_n, int color)
{
    // この局面
    Node* pN = &nodeList[node_n];

    // 1手でも選ばれたか否か
    int select = -1;
    double max_ucb = -999;
    double ucb = 0, ucb_rave = 0, beta;

    // ループ カウンター
    int i;

    for (i = 0; i < pN->child_num; i++)
    {
        // この子ノード
        Child* c = &pN->children[i];

        // 非合法手の座標なら無視
        if (c->z == ILLEGAL_Z)
            continue;

        // 試した回数が 0 のとき
        if (c->games == 0)
        {
            ucb_rave = 10000 + (rand() & 0x7fff); // try once
        }
        else
        {
            const double C = 0.30; // depends on program

            // 何らかのレーブの計算？
            const double RAVE_D = 3000;
            double moveCount = c->games;
            double raveCount = c->rave_games;
            double rave = c->rave_rate;
            if (c->z == 0)
            { // dont select pass
                rave = 1 - color;
                raveCount = pN->child_games_sum;
            }

            beta = raveCount / (raveCount + moveCount + raveCount * moveCount / RAVE_D);

            // UCBの計算
            ucb = c->rate + C * sqrt(log((double)pN->child_games_sum) / c->games);

            ucb_rave = beta * rave + (1 - beta) * ucb;
            //    if ( depth==0 ) Prt("%2d:z=%2d,rate=%6.3f,games=%4d, rave_r=%6.3f,g=%4d, beta=%f,ucb_rave=%f\n", i, Get81(c->z), c->rate, c->games, c->rave_rate, c->rave_games,beta,ucb_rave);
        }

        // UCB値の最大を更新
        if (ucb_rave > max_ucb)
        {
            max_ucb = ucb_rave;
            select = i;
        }
    }

    // 1手も選ばれなかったらエラーなので強制終了
    if (select == -1)
    {
        Prt("Err! select\n");
        exit(0);
    }

    return select;
}

/// <summary>
/// レーブの更新？
/// </summary>
/// <param name="pN">ノード？</param>
/// <param name="color">手番の色</param>
/// <param name="current_depth">経路の深さ</param>
/// <param name="win">勝率</param>
void UpperConfidenceTree::UpdateRave(Node* pN, int color, int current_depth, double win)
{
    // 盤面の各交点の手番の色？
    int played_color[kBoardMax];

    // ループ カウンター
    int i;

    // 着手した座標
    int z;

    // 手番の色
    int c = color;

    // ゼロ クリアーした？
    memset(played_color, 0, sizeof(played_color));

    // 経路の残りの負荷さについて
    for (i = current_depth; i < depth; i++)
    {
        // 着手点
        z = path[i];

        // 記録がなければ上書き
        if (played_color[z] == 0)
            played_color[z] = c;

        // 手番の色交代
        c = FlipColor(c);
    }

    // パスはゼロ クリアー
    // ignore pass
    played_color[0] = 0;

    // すべての子ノードについて
    for (i = 0; i < pN->child_num; i++)
    {
        // 子ノード
        Child* c = &pN->children[i];

        // 非合法手は無視
        if (c->z == ILLEGAL_Z)
            continue;

        // 相手の色なら無視
        if (played_color[c->z] != color)
            continue;

        // レーブ率の再計算？
        c->rave_rate = (c->rave_games * c->rave_rate + win) / (c->rave_games + 1);

        // レーブの対局数？をカウント
        c->rave_games++;

        // 子レーブの対局数？をカウント
        pN->child_rave_games_sum++;
    }
}

/// <summary>
/// ゲームをプレイします（再帰関数）
/// UCTという探索の手法で行います
/// </summary>
/// <param name="color">手番の色。最初は考えているプレイヤーの色</param>
/// <param name="node_n">ノードのリストのインデックス。最初は0</param>
/// <returns>手番の勝率</returns>
int UpperConfidenceTree::SearchUct(int color, int node_n)
{
    // この局面
    Node* pN = &nodeList[node_n];

    // 最善の一手（子ノード）
    Child* c = NULL;
    int select, z, err, win, current_depth;

    // とりあえず打ってみる
    for (;;)
    {
        // 最善の一手（子ノード）のインデックス
        select = SelectBestUcb(node_n, color);

        // 最善の一手（子ノード）
        c = &pN->children[select];

        // 最善の一手（子ノード）の座標
        z = c->z;

        // 石を置く
        err = position.PutStone(z, color, kFillEyeErr);

        // 合法手ならループを抜けます
        if (err == 0)
            break;

        // 非合法手なら、 ILLEGAL_Z をセットして ループをやり直し
        // select other move
        c->z = ILLEGAL_Z;
    }

    // 現在の深さを更新
    current_depth = depth;

    // 経路に着手点を格納
    path[depth++] = c->z;

    // この一手が１度も試行されていなければ、プレイアウトします
    // c->games <= 10 とかにすればメモリを節約できます。
    // c->games <= 0 より強くなる場合もあります。
    // 経路の深さが配列のサイズを超えているときもプレイアウトします。
    // 自分がパスを連続しているときもプレイアウトします。
    // playout in first time. <= 10 can reduce node.
    if (c->games <= 0 || depth == kDMax || (c->z == 0 && depth >= 2 && path[depth - 2] == 0))
    {
        // 手番をひっくり返してプレイアウト
        win = -position.Playout(FlipColor(color));
    }
    else
    {
        // 子ノードが葉なら、さらに延長
        if (c->next == NODE_EMPTY)
            c->next = uct.CreateNode(c->z);

        // 手番をひっくり返して UCT探索（ネガマックス形式）。勝率はひっくり返して格納
        win = -SearchUct(FlipColor(color), c->next);
    }

    // レーブの更新
    UpdateRave(pN, color, current_depth, win);

    // 勝率の更新（update winrate）
    c->rate = (c->rate * c->games + win) / (c->games + 1);

    // 対局数カウントアップ
    c->games++;

    // 子の対局数？をカウントアップ
    pN->child_games_sum++;

    return win;
}

TimeMan timeMan = TimeMan();

/// <summary>
/// 時間計
/// </summary>
/// <returns></returns>
double Position::CountTotalTime()
{
    int i;
    double total_time[2];

    total_time[0] = 0; // black time
    total_time[1] = 0; // white

    for (i = 0; i < moves; i++)
    {
        total_time[i & 1] += record_time[i];
    }

    return total_time[moves & 1];
}

/// <summary>
/// 一番良く打たれた一手の座標を返します
/// </summary>
/// <param name="color">手番の色</param>
/// <returns>一番良く打たれた一手の座標</returns>
int Position::GetBestUct(int color)
{
    int next, i, best_z, best_i = -1;

    // 手が打たれた対局数の最大数？
    int max = -999;

    // この局面
    Node* pN;

    // 前回の着手座標？
    int prev_z = 0;

    // 初手以外は前回の着手点を記憶
    if (moves > 0)
        prev_z = record[moves - 1];

    // ノードリストの要素数
    uct.node_num = 0;

    // 次のノードのインデックス。ここでは0。現図を作成しています
    next = uct.CreateNode(prev_z);

    // とりあえず UCT探索（search_uct）を、uct.loop回繰り返します
    for (i = 0; i < uct.loop; i++)
    {
        // 現図を退避
        int board_copy[kBoardMax];
        int ko_z_copy = ko_z;
        memcpy(board_copy, Board, sizeof(Board));

        // 経路の深さを初期化
        depth = 0;

        // UCT探索
        uct.SearchUct(color, next);

        // 現図を復元
        ko_z = ko_z_copy;
        memcpy(Board, board_copy, sizeof(Board));

        // 時間切れなら抜けます
        if (timeMan.IsTimeOver())
            break;
    }

    // 次のノード
    pN = &uct.nodeList[next];

    // 子ノード全部確認
    for (i = 0; i < pN->child_num; i++)
    {
        // 子ノード
        Child* c = &pN->children[i];

        // 最大対局数（一番打たれた手ということ）の更新
        if (c->games > max)
        {
            best_i = i;
            max = c->games;
        }
        Prt("%2d:z=%2d,rate=%6.3f,games=%4d, rave_r=%6.3f,g=%4d\n",
            i, Get81(c->z), c->rate, c->games, c->rave_rate, c->rave_games);
    }

    // ベストなノードの座標
    best_z = pN->children[best_i].z;
    Prt("best_z=%d,rate=%6.3f,games=%4d,playouts=%d,nodes=%d\n",
        Get81(best_z), pN->children[best_i].rate, max, all_playouts, uct.node_num);

    return best_z;
}

/// <summary>
/// 盤の初期化
/// </summary>
void Position::InitBoard()
{
    int i, x, y;

    // 盤の枠を整えます
    for (i = 0; i < kBoardMax; i++)
        Board[i] = 3;

    // 盤上の石をすべて取り除きます
    for (y = 0; y < kBoardSize; y++)
        for (x = 0; x < kBoardSize; x++)
            Board[GetZ(x + 1, y + 1)] = 0;

    // 手数とコウとハッシュコードをクリアーします
    moves = 0;
    ko_z = 0;
    hashcode = 0;
}

/// <summary>
/// 指し手を棋譜に記入
/// </summary>
/// <param name="z">座標</param>
/// <param name="color">手番の色</param>
void Position::AddMoves(int z, int color, double sec)
{
    // 石を置きます
    int err = PutStone(z, color, kFillEyeOk);

    // 非合法手なら強制終了
    if (err != 0)
    {
        Prt("PutStone err=%d\n", err);
        exit(0);
    }

    // 棋譜の末尾に記入
    record[moves] = z;
    record_time[moves] = sec;

    // 棋譜の書くところを１つ進めます
    moves++;

    // 盤表示
    PrintBoard();

    // ハッシュコード表示
    Prt("hashcode=");
    PrtCode64(hashcode);
    Prt("\n");
}

/// <summary>
/// 原始モンテカルロ探索
/// </summary>
const int SEARCH_PRIMITIVE = 0;

/// <summary>
/// UCT探索
/// </summary>
const int SEARCH_UCT = 1;

/// <summary>
/// コンピューターの指し手
/// </summary>
/// <param name="color">手番の色</param>
/// <param name="search">探索方法。SEARCH_PRIMITIVE または SEARCH_UCT</param>
/// <returns>座標</returns>
int Position::PlayComputerMove(int color, int search)
{
    // 秒
    double sec;

    // 着手点
    int z;

    // 累計時間
    double total_time = CountTotalTime();

    double base_time = 60 * 10; // 10 minutes
    double left_time = base_time - total_time;
    int div = 12; // 40 ... 13x13, 70 ... 19x19
    timeMan.time_limit_sec = left_time / div;
    if (left_time < 60)
        timeMan.time_limit_sec = 1.0;
    if (left_time < 20)
        timeMan.time_limit_sec = 0.2;
    Prt("time_limit_sec=%.1f, total=%.1f, left=%.1f\n", timeMan.time_limit_sec, total_time, left_time);

    // 開始時刻
    timeMan.start_time = timeMan.GetClock();

    // プレイアウト回数
    all_playouts = 0;

    // 盤領域をゼロ クリアー？
    memset(board_area_sum, 0, sizeof(board_area_sum));

    // 盤上の勝ち数をゼロ クリアー？
    memset(board_winner, 0, sizeof(board_winner));

    // 勝ち数をクリアー？
    memset(winner_count, 0, sizeof(winner_count));

    if (search == SEARCH_UCT)
    {
        // UCTを使ったゲームプレイ
        z = GetBestUct(color);
    }
    else
    {
        // 原始モンテカルロでゲームプレイ
        z = PrimitiveMonteCalro(color);
    }

    // 盤領域を表示？
    //PrintBoardArea();

    // クリティカルさを表示？
    //PrintCriticality();

    // 消費時間（秒）？
    sec = timeMan.GetSpendTime(timeMan.start_time);

    // 情報表示
    Prt("z=%s,color=%d,moves=%d,playouts=%d, %.1f sec(%.0f po/sec),depth=%d\n",
        GetCharZ(z), color, moves, all_playouts, sec, all_playouts / sec, depth);

    // 指し手を棋譜に記憶します
    AddMoves(z, color, sec);

    return z;
}

/// <summary>
/// print SGF game record
/// </summary>
void Position::PrintSgf()
{
    int i;

    // ヘッダー出力
    Prt("(;GM[1]SZ[%d]KM[%.1f]PB[]PW[]\n", kBoardSize, komi);

    // 指し手出力
    for (i = 0; i < moves; i++)
    {
        // 座標
        int z = record[i];

        // 段
        int y = z / kWidth;

        // 筋
        int x = z - y * kWidth;

        // 色
        const char* sStone[2] = { "B", "W" };
        Prt(";%s", sStone[i & 1]);

        // パス
        if (z == 0)
        {
            Prt("[]");
        }
        else
        {
            Prt("[%c%c]", x + 'a' - 1, y + 'a' - 1);
        }

        // 改行
        if (((i + 1) % 10) == 0)
            Prt("\n");
    }

    // 終端
    Prt(")\n");
}

/// <summary>
/// 黒番は原始モンテカルロ、白番はUCTで自己対戦
/// </summary>
void Position::Selfplay()
{
    // 黒の手番
    int color = 1;

    // 座標
    int z;

    // 探索方法フラグ
    int search;

    for (;;)
    {
        // 黒番、白番ともにUCT探索
        if (color == 1)
        {
            search = SEARCH_UCT; //SEARCH_PRIMITIVE;
        }
        else
        {
            search = SEARCH_UCT;
        }

        // 次の一手
        z = PlayComputerMove(color, search);

        // パスパスなら終局
        if (z == 0 && moves > 1 && record[moves - 2] == 0)
            break;

        // 300手を超えても終局
        if (moves > 300)
            break; // too long

        // 手番の色反転
        color = FlipColor(color);
    }

    // SGF形式で棋譜表示
    PrintSgf();
}

/// <summary>
/// 黒手番でプレイアウトのテスト
/// </summary>
void Position::TestPlayout()
{
    // テストプレイであるというフラグを立てます
    flag_test_playout = 1;

    // 黒手番でプレイアウト
    Playout(1);

    // 盤表示
    PrintBoard();

    // SGF形式の棋譜を出力
    PrintSgf();
}

// GTPプロトコルの1行を読込むのに十分な文字列サイズ
const int kStrMax = 256;

// スペース区切りで3つ分まで読込む？
const int kTokenMax = 3;

/// <summary>
/// GTPプロトコルのループ
/// </summary>
void GtpLoop()
{
    // 入力文字列バッファー
    char str[kStrMax];

    // スプリットされた文字列のリスト
    char sa[kTokenMax][kStrMax];

    // コマンドはスペース区切り
    char seps[] = " ";

    // スプリットされた文字列１つ
    char* token;
    int x, y, z, ax, count;

    // 標準出力
    setbuf(stdout, NULL);

    // 標準エラー出力
    setbuf(stderr, NULL);

    for (;;)
    {
        // 標準入力から文字列読取
        if (fgets(str, kStrMax, stdin) == NULL)
            break;

        //  Prt("gtp<-%s",str);

        // 文字列のスプリットをして、結果を sa配列に格納しています
        count = 0;
        token = strtok(str, seps);
        while (token != NULL)
        {
            strcpy(sa[count], token);
            count++;
            if (count == kTokenMax)
                break;
            token = strtok(NULL, seps);
        }

        // 盤のサイズを n にしてください
        if (strstr(sa[0], "boardsize"))
        {
            // 無視
            //    int new_board_size = atoi( sa[1] );
            SendGtp("= \n\n");
        }
        // 盤を初期化してください
        else if (strstr(sa[0], "clear_board"))
        {
            position.InitBoard();
            SendGtp("= \n\n");
        }
        // アプリケーションを終了してください
        else if (strstr(sa[0], "quit"))
        {
            break;
        }
        // GTPプロトコルのバージョンを返してください
        else if (strstr(sa[0], "protocol_version"))
        {
            SendGtp("= 2\n\n");
        }
        // 思考エンジンの名前を返してください
        else if (strstr(sa[0], "name"))
        {
            SendGtp("= your_program_name\n\n");
        }
        // 思考エンジンのバージョンを返してください
        else if (strstr(sa[0], "version"))
        {
            SendGtp("= 0.0.1\n\n");
        }
        // 対応しているコマンドの改行区切りリストを返してください
        else if (strstr(sa[0], "list_commands"))
        {
            SendGtp("= boardsize\nclear_board\nquit\nprotocol_version\n"
                "name\nversion\nlist_commands\nkomi\ngenmove\nplay\n\n");
        }
        // コミを n にしてください
        else if (strstr(sa[0], "komi"))
        {
            position.komi = atof(sa[1]);
            SendGtp("= \n\n");
        }
        // 着手の座標を返してください
        else if (strstr(sa[0], "genmove"))
        {
            int color = 1;
            if (tolower(sa[1][0]) == 'w')
                color = 2;

            z = position.PlayComputerMove(color, SEARCH_UCT);
            SendGtp("= %s\n\n", GetCharZ(z));
        }
        // 指定の色の石を、指定の座標に置いてください
        else if (strstr(sa[0], "play"))
        { // "play b c4", "play w d17"
            int color = 1;
            if (tolower(sa[1][0]) == 'w')
                color = 2;
            ax = tolower(sa[2][0]);
            x = ax - 'a' + 1;
            if (ax >= 'i')
                x--;
            y = atoi(&sa[2][1]);
            z = GetZ(x, kBoardSize - y + 1);
            if (tolower(sa[2][0]) == 'p')
                z = 0; // pass
            position.AddMoves(z, color, 0);
            SendGtp("= \n\n");
        }
        // その他のコマンドには未対応です
        else
        {
            SendGtp("? unknown_command\n\n");
        }
    }
}

/// <summary>
/// プログラムはここから始まります
/// </summary>
/// <returns>エラーコード。正常時は0</returns>
int main()
{
    // 乱数の種を設定
    //srand( (unsigned)time( NULL ) );

    // 盤の初期化
    position.InitBoard();

    // ハッシュボードの初期化
    MakeHashboard();

    // 自己対戦
    if (0)
    {
        position.Selfplay();
        return 0;
    }

    // プレイアウトのテスト
    if (0)
    {
        position.TestPlayout();
        return 0;
    }

    // GTPプロトコルのループ
    GtpLoop();

    return 0;
}

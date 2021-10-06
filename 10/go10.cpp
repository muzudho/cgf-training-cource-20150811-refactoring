// sprintf is Err in VC++
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <stdarg.h>
#include <limits.h>
#include <ctype.h>

/// <summary>
/// コミ
/// </summary>
double komi = 6.5;

/// <summary>
/// n路盤
/// </summary>
const int kBoardSize = 9;

/// <summary>
/// 両端に番兵込みの幅
/// </summary>
const int kWidth = (kBoardSize + 2);

/// <summary>
/// 番兵込みの盤の面積
/// </summary>
const int kBoardMax = (kWidth * kWidth);

/// <summary>
/// 右、下、左、上
/// </summary>
const int kDir4[4] = { +1, +kWidth, -1, -kWidth };

/// <summary>
/// 右、下、左、上、右下、左下、左上、右上
/// </summary>
const int kDir8[8] = { +1, +kWidth, -1, -kWidth, +1 + kWidth, +kWidth - 1, -1 - kWidth, -kWidth + 1 };

/// <summary>
/// 指し手の要素数
/// </summary>
const int kMaxMoves = 1000;

// UCTで使われる経路（パス）の最大長
const int kDMax = 1000;

/// <summary>
/// 目潰しをエラーとするなら
/// </summary>
const int kFillEyeErr = 1;

/// <summary>
/// 目潰しを合法手とするなら（囲碁のルールでは合法手）
/// </summary>
const int kFillEyeOk = 0;

/// <summary>
/// 左上、上、右上、左、基準点、右、左下、下、右下
/// </summary>
const int kDir3x3[9] = { -kWidth - 1, -kWidth, -kWidth + 1, -1, 0, +1, +kWidth - 1, +kWidth, +kWidth + 1 };

/// <summary>
/// 3x3パターンを生成している？
/// </summary>
const int kExpandPatternMax = (8 * 100);

/// <summary>
/// x, y を z（座標；配列のインデックス） に変換
/// </summary>
/// <param name="x">is (1 &lt;= x &lt;= 9)</param>
/// <param name="y">is (1 &lt;= y &lt;= 9)</param>
/// <returns></returns>
int GetZ(int x, int y)
{
    return y * kWidth + x;
}

/// <summary>
/// for display only
/// </summary>
/// <param name="z">座標</param>
/// <returns>人が読める形の座標</returns>
int Get81(int z)
{
    int y = z / kWidth;
    int x = z - y * kWidth; // 106 = 9*11 + 7 = (x,y)=(7,9) -> 79
    if (z == 0)
        return 0;
    return x * 10 + y; // x*100+y for 19x19
}

/// <summary>
/// 同じ命令文で2回呼び出さないでください。
/// don't call twice in same sentence.
///
/// like Prt("z0=%s,z1=%s\n",GetCharZ(z0),GetCharZ(z1));
/// </summary>
/// <param name="z">座標</param>
/// <returns>人が読める形の座標（文字列）</returns>
char* GetCharZ(int z)
{
    int x, y, ax;
    static char buf[16];

    // パス
    sprintf(buf, "pass");
    if (z == 0)
        return buf;

    y = z / kWidth;
    x = z - y * kWidth;
    ax = x - 1 + 'A';

    // I列は無いので詰めます
    if (ax >= 'I')
        ax++; // from 'A' to 'T', excluding 'I'

    sprintf(buf, "%c%d", ax, kBoardSize + 1 - y);
    return buf;
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
/// プリントなんだけれども 標準エラー出力に出してる？
/// サーバーに出力したくない文字列を表示したいときに使う？
/// </summary>
/// <param name="fmt">書式か？</param>
/// <param name="">可変長引数</param>
void Prt(const char* fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    //{ FILE *fp = fopen("out.txt","a"); if ( fp ) { vfprt( fp, fmt, ap ); fclose(fp); } }
    vfprintf(stderr, fmt, ap);
    va_end(ap);
}

/// <summary>
/// Go Text Protocol の何か？
/// </summary>
/// <param name="fmt">書式か？</param>
/// <param name="">可変長引数</param>
void SendGtp(const char* fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vfprintf(stdout, fmt, ap);
    va_end(ap);
}

/// <summary>
/// 局面
/// </summary>
class Position {
public:
    /// <summary>
    /// 盤
    /// </summary>
    int Board[kBoardMax] = {
        3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
        3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3,
        3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3,
        3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3,
        3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3,
        3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3,
        3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3,
        3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3,
        3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3,
        3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3,
        3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 };

    /// <summary>
    /// コウの座標
    /// </summary>
    int ko_z = 0;

    /// <summary>
    /// 棋譜
    /// </summary>
    int record[kMaxMoves] = { 0 };

    /// <summary>
    /// n手目。1手目が0
    /// </summary>
    int moves = 0;

    /// <summary>
    /// プレイアウト回数
    /// </summary>
    int all_playouts = 0;

    /// <summary>
    /// テストでプレイアウトするなら1
    /// </summary>
    int flag_test_playout = 0;

    /// <summary>
    /// 呼吸点を探索するアルゴリズムで使用
    /// </summary>
    int check_board[kBoardMax];

    void CountLibertySub(int tz, int color, int* p_liberty, int* p_stone);
    void CountLiberty(int tz, int* p_liberty, int* p_stone);
    void TakeStone(int tz, int color);
    int PutStone(int tz, int color, int fill_eye_err);
    void PrintBoard();
    int CountScore(int turn_color);
    int Playout(int turn_color);
    int PrimitiveMonteCalro(int color);
    void InitBoard();
    void AddMoves(int z, int color);
    void PrintSgf();
    void TestPlayout();

} position;

/// <summary>
/// count_liberty関数の中で呼び出されます。再帰
/// </summary>
/// <param name="tz">着手（開始）座標</param>
/// <param name="color">連の色</param>
/// <param name="p_liberty">呼吸点の数</param>
/// <param name="p_stone">連の石の数</param>
void Position::CountLibertySub(int tz, int color, int* p_liberty, int* p_stone)
{
    int z, i;

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

    Board[tz] = 0;
    for (i = 0; i < 4; i++)
    {
        z = tz + kDir4[i];
        if (Board[z] == color)
            TakeStone(z, color);
    }
}

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
        ko_z = 0;
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
        if (c == un_col && lib == 1 && Board[tz + kDir4[i]])
        {
            TakeStone(tz + kDir4[i], un_col);
        }
    }

    // 石を置きます
    Board[tz] = color;

    // 着手点を含む連の呼吸点の数を数えます
    CountLiberty(tz, &liberty, &stone);
    // 石を1個取ったらコウかも知れない
    if (capture_sum == 1 && stone == 1 && liberty == 1)
        ko_z = ko_maybe;
    else
        ko_z = 0;

    return 0;
}

/// <summary>
/// 盤の描画
/// </summary>
void Position::PrintBoard()
{
    int x, y;
    const char* str[4] = { ".", "X", "O", "#" };

    // 筋の符号の表示
    Prt("   ");
    //for (x=0;x<kBoardSize;x++) Prt("%d",x+1);
    for (x = 0; x < kBoardSize; x++)
        Prt("%c", 'A' + x + (x > 7));
    Prt("\n");

    // 盤の各行の表示
    for (y = 0; y < kBoardSize; y++)
    {
        //  Prt("%2d ",y+1);
        Prt("%2d ", kBoardSize - y);
        for (x = 0; x < kBoardSize; x++)
        {
            Prt("%s", str[Board[GetZ(x + 1, y + 1)]]);
        }
        if (y == 4)
            Prt("  ko_z=%d,moves=%d", Get81(ko_z), moves);
        Prt("\n");
    }
}

/// <summary>
/// 地の簡易計算（これが厳密に計算できるようなら囲碁は完全解明されている）を表示し、勝敗を返します。
/// スコアにはコミは含みませんが、勝敗にはコミを含んでいます。
/// </summary>
/// <param name="turn_color">手番の色</param>
/// <returns>黒の勝ちなら1、負けなら0</returns>
int Position::CountScore(int turn_color)
{
    int x, y, i;
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
                continue;

            // 大雑把に さくっと地計算。
            // 4方向にある黒石、白石の数を数えます
            mk[1] = mk[2] = 0;
            for (i = 0; i < 4; i++)
                mk[Board[z + kDir4[i]]]++;

            // 黒石だけがあるなら黒の地
            if (mk[1] && mk[2] == 0)
                black_area++;
            // 白石だけがあるなら白の地
            if (mk[2] && mk[1] == 0)
                white_area++;
        }

    // スコア計算（コミ含まず）
    black_sum = kind[1] + black_area;
    white_sum = kind[2] + white_area;
    score = black_sum - white_sum;

    // 黒の勝敗判定
    win = 0;
    if (score - komi > 0)
        win = 1;

    // 白番なら勝敗を反転
    if (turn_color == 2)
        win = -win;

    //Prt("black_sum=%2d, (stones=%2d, area=%2d)\n",black_sum, kind[1], black_area);
    //Prt("white_sum=%2d, (stones=%2d, area=%2d)\n",white_sum, kind[2], white_area);
    //Prt("score=%d, win=%d\n",score, win);
    return win;
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

    // 最善手の座標
    int best_z = 0;

    // 最善の価値
    double best_value;

    // 勝率
    double win_rate;
    int x, y, err, i;
    // プレイアウトして勝った回数
    int win_sum;
    // 手番が勝ったら1、負けたら0
    int win;

    // コウの座標のコピー
    int ko_z_copy;

    // 盤のコピー
    int board_copy[kBoardMax]; // keep current board
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

            // もし石を置けなかったら、次の交点へ
            if (err != 0)
                continue;

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

    // 手数とコウをクリアーします
    moves = 0;
    ko_z = 0;
}

/// <summary>
/// 指し手を棋譜に記入
/// </summary>
/// <param name="z">座標</param>
/// <param name="color">手番の色</param>
void Position::AddMoves(int z, int color)
{
    // 石を置きます
    int err = PutStone(z, color, kFillEyeOk);
    // 非合法手なら強制終了
    if (err != 0)
    {
        Prt("Err!\n");
        exit(0);
    }
    // 棋譜の末尾に記入
    record[moves] = z;
    // 棋譜のサイズを伸ばします
    moves++;
    // 盤表示
    PrintBoard();
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
/// 黒手番でプレイアウトのテスト
/// </summary>
void Position::TestPlayout()
{
    flag_test_playout = 1;
    // 黒手番でプレイアウト
    Playout(1);
    // 盤表示
    PrintBoard();
    // SGF形式の棋譜を出力
    PrintSgf();
}

/// <summary>
/// 盤上の3x3のパターン
/// </summary>
class BoardPattern3x3 {
public:
    /// <summary>
    /// 3x3パターンの数
    /// </summary>
    int e_pat_num = 0;

    /// <summary>
    /// rotate and flip pattern
    /// </summary>
    int e_pat[kExpandPatternMax][9];

    /// <summary>
    /// [0] ...pattern, [1]...mask
    /// </summary>
    int e_pat_bit[kExpandPatternMax][2];

    void ExpandPattern3x3();
    int MatchPattern3x3(int z, int col);
    int GetProb(int z, int prev_z, int col);

}boardPattern3x3;

/// <summary>
/// 空点を確率的に選ぶための山下さんの工夫。3x3 のパターン
/// </summary>
char* pattern3x3[] = {
    "XO?"  // "X" ... black,  black "X" to play at the center.
    "..."  // "O" ... white
    "???", // "." ... empty
           // "#" ... out of board
    "OX."  // "?" ... dont care
    "..."
    "?.?",

    "XO?"
    "..X"
    "?.?",

    "X.."
    "O.."
    "###",

    NULL };

/// <summary>
/// 3x3パターンを作ってる？
/// 1つのパターンを回転させて、種類を増やしている？
/// </summary>
void BoardPattern3x3::ExpandPattern3x3()
{
    int n, i, j;

    // パターン番号？
    e_pat_num = 0;
    // n は パターン番号？
    for (n = 0;; n++)
    {
        int i, j;
        char* p = pattern3x3[n];

        // パターンが格納されていなければ終了
        if (p == NULL)
            break;

        // 容量を超えたら強制終了
        if (e_pat_num > kExpandPatternMax - 8)
        {
            Prt("e_pat_num over Err\n");
            exit(0);
        }

        // 3x3 の盤上に置かれた文字列を、数に変換？
        for (i = 0; i < 9; i++)
        {
            int m = 0;
            char c = *(p + i);
            if (c == '.')
                m = 0;
            if (c == 'X')
                m = 1;
            if (c == 'O')
                m = 2;
            if (c == '#')
                m = 3;
            if (c == '?')
                m = 4;
            e_pat[e_pat_num][i] = m;
        }

        e_pat_num++;

        for (i = 0; i < 2; i++)
        {
            int* p;
            int* q;
            for (j = 0; j < 3; j++)
            {
                p = e_pat[e_pat_num - 1];
                q = e_pat[e_pat_num];
                // roteta 90
                //  "012"      "630"
                //  "345"  ->  "741"
                //  "678"      "852"
                q[0] = p[6];
                q[1] = p[3];
                q[2] = p[0];
                q[3] = p[7];
                q[4] = p[4];
                q[5] = p[1];
                q[6] = p[8];
                q[7] = p[5];
                q[8] = p[2];
                e_pat_num++;
            }

            if (i == 1)
                break;

            p = e_pat[e_pat_num - 1];
            q = e_pat[e_pat_num];

            // vertical flip
            q[0] = p[6];
            q[1] = p[7];
            q[2] = p[8];
            q[3] = p[3];
            q[4] = p[4];
            q[5] = p[5];
            q[6] = p[0];
            q[7] = p[1];
            q[8] = p[2];
            e_pat_num++;
        }
    }

    for (i = 0; i < e_pat_num; i++)
    {
        e_pat_bit[i][0] = 0;
        e_pat_bit[i][1] = 0;
        //  Prt("%4d\n",i);
        for (j = 0; j < 9; j++)
        {
            int c = e_pat[i][j];
            int mask = 3;
            //    Prt("%d",c);
            //    if ((j+1)%3==0) Prt("\n");
            if (c == 4)
            {
                mask = 0;
                c = 0;
            }
            e_pat_bit[i][0] = e_pat_bit[i][0] << 2;
            e_pat_bit[i][1] = e_pat_bit[i][1] << 2;
            e_pat_bit[i][0] |= c;
            e_pat_bit[i][1] |= mask;
        }
        //  Prt("bit=%08x,mask=%08x\n",e_pat_bit[i][0],e_pat_bit[i][1]);
    }
    Prt("pattern3x3 num=%d, e_pat_num=%d\n", n, e_pat_num);
}

/// <summary>
/// return pattern number, -1 ... not found
/// </summary>
/// <param name="z">座標</param>
/// <param name="col">手番の色</param>
/// <returns>マッチしたパターン番号？ -1ならマッチしなかった</returns>
int BoardPattern3x3::MatchPattern3x3(int z, int col)
{
#if 1 // 413 playouts/sec
    int pat_bit = 0;
    int i, j;
    for (j = 0; j < 9; j++)
    {
        int c = position.Board[z + kDir3x3[j]];
        if (col == 2 && (c == 1 || c == 2))
            c = 3 - c;
        pat_bit = pat_bit << 2;
        pat_bit |= c;
    }
    for (i = 0; i < e_pat_num; i++)
    {
        int e_bit = e_pat_bit[i][0];
        int e_mask = e_pat_bit[i][1];
        if (e_bit == (pat_bit & e_mask))
            return i;
    }
    return -1;
#else // 353 playouts/sec
    int i, j;
    for (i = 0; i < e_pat_num; i++)
    {
        for (j = 0; j < 9; j++)
        {
            int c = position.Board[z + kDir3x3[j]];
            int e = e_pat[i][j];
            if (col == 2 && e == 1)
                e = 2;
            else if (col == 2 && e == 2)
                e = 1;
            if (e == 4)
                continue;
            if (c != e)
                break;
        }
        if (j == 9)
            return i;
    }
    return -1;
#endif
}

/// <summary>
/// 空点を選ぶ確率？
/// </summary>
/// <param name="z">座標</param>
/// <param name="prev_z">前回の座標</param>
/// <param name="col">手番の色</param>
/// <returns>空点を選ぶ確率？</returns>
int BoardPattern3x3::GetProb(int z, int prev_z, int col)
{
    const int kMaxProb = INT_MAX / kBoardMax;
    int pr = 100;
    int un_col = FlipColor(col);
    int y = z / kWidth;
    int x = z - y * kWidth;
    int prev_y = prev_z / kWidth;
    int prev_x = prev_z - prev_y * kWidth;
    int dx = abs(x - prev_x);
    int dy = abs(y - prev_y);
    int m;
    int i;
    int sc[4];

    // add your code, start -->

    sc[0] = sc[1] = sc[2] = sc[3] = 0;
    for (i = 0; i < 8; i++)
    {
        sc[position.Board[z + kDir8[i]]]++;
    }
    if (sc[un_col] >= 3 && sc[col] == 0)
        pr /= 2;
    if (sc[col] >= 3 && sc[un_col] == 0)
        pr *= 2;

    m = -1;
    if (prev_z != 0 && ((dx + dy) == 1 || (dx * dy) == 1))
    {
        m = MatchPattern3x3(z, col);
    }
    if (m >= 0)
    {
        int n = m / 8; // pattern number
                       //  Prt("match=%3d,z=%s,col=%d\n",m,GetCharZ(z),col);
        pr *= 1000;
        if (n == 0)
            pr *= 2;
    }

    // add your code, end <--

    if (pr < 1)
        pr = 1;
    if (pr > kMaxProb)
        pr = kMaxProb;
    return pr;
}

// following are for UCT(Upper Confidence Tree)
// `UCT` - 探索と知識利用のバランスを取る手法

/// <summary>
/// 最大の子数。9路なら82個。+1 for PASS
/// </summary>
const int kChildSize = (kBoardSize * kBoardSize + 1);

// 最大10000局面まで
const int kNodeMax = 10000;

/// <summary>
/// no next node
/// </summary>
const int kNodeEmpty = -1;

/// <summary>
/// illegal move
/// </summary>
const int kIllegalZ = -1;

/// <summary>
/// 人間的に盤面上の3x3のパターンの形を考えると悪手なので、着手の確率を下げるための割引率 0.0～1.0
/// </summary>
/// <param name="z">座標</param>
/// <param name="prev_z">前回の座標</param>
/// <returns>人間的に盤面上の3x3のパターンの形を考えると悪手なので、着手の確率を下げるための割引率 0.0～1.0</returns>
double GetBonus(int z, int prev_z)
{
    // 段
    int y = z / kWidth;
    // 筋
    int x = z - y * kWidth;
    // 前回の着手の段
    int prev_y = prev_z / kWidth;
    // 前回の着手の筋
    int prev_x = prev_z - prev_y * kWidth;
    // 筋の水平距離
    int dx = abs(x - prev_x);
    // 段の垂直距離
    int dy = abs(y - prev_y);
    // ボーナス？
    double b = 1.0;

    // 両端の筋なら 5割
    if (x == 1 || x == kBoardSize)
        b *= 0.5;

    // 両端の段なら 5割
    if (y == 1 || y == kBoardSize)
        b *= 0.5;

    // 両端から１つ離れた筋なら 8割
    if (x == 2 || x == kBoardSize - 1)
        b *= 0.8;

    // 両端から１つ離れた段なら8割
    if (y == 2 || y == kBoardSize - 1)
        b *= 0.8;

    // 前回の着手点がパスではなく、上下左右に隣接して置くようなら2割
    if (prev_z != 0 && (dx + dy) == 1)
        b *= 2.0;

    return b;
}

/// <summary>
/// 手を保存するための構造体
/// </summary>
class Child {
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
    /// ノードのリストのインデックス。次のノード（next node）を指す
    /// </summary>
    int next;

    /// <summary>
    /// 人間的に盤面上の3x3のパターンの形を考えると悪手なので、着手の確率を下げるための割引率 0.0～1.0（shape bonus）
    /// </summary>
    double bonus;

};

/// <summary>
/// 局面を保存する構造体
/// </summary>
class Node {
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

    void AddChild(int z, double bonus);
};

// 以下、探索木全体を保存

/// <summary>
/// リストの末尾に要素を追加。手を追加。
/// この手を打った後のノードは、なし
/// </summary>
/// <param name="z">手の座標</param>
/// <param name="bonus">
/// 人間的に盤面上の3x3のパターンの形を考えると悪手なので、着手の確率を下げるための割引率 0.0～1.0
/// from 0 to 10, good move has big bonus
/// </param>
void Node::AddChild(int z, double bonus)
{
    // 新しい要素のインデックス
    int n = this->child_num;
    this->children[n].z = z;
    this->children[n].games = 0;
    this->children[n].rate = 0;
    this->children[n].next = kNodeEmpty;
    this->children[n].bonus = bonus;
    // ノードのリストのサイズ更新
    this->child_num++;
}

/// <summary>
/// number of uct loop
/// </summary>
int uct_loop = 1000;

/// <summary>
/// 原始モンテカルロ探索
/// </summary>
const int kSearchPrimitive = 0;

/// <summary>
/// UCT探索
/// </summary>
const int kSearchUct = 1;

/// <summary>
/// `UCT` - 探索と知識利用のバランスを取る手法
/// </summary>
class UpperConfidenceTree {
public:

    /// <summary>
    /// ノードのリスト
    /// </summary>
    Node nodeList[kNodeMax];

    /// <summary>
    /// ノードのリストのサイズ。登録局面数
    /// </summary>
    int node_num = 0;

    /// <summary>
    /// UCTで使われる経路（パス）
    /// </summary>
    int path[kDMax];

    /// <summary>
    /// UCTで使われる経路（パス）の先頭がらn番目(0開始)
    /// </summary>
    int depth;

    int CreateNode(int prev_z);
    int SelectBestUcb(int node_n);
    int SearchUct(int color, int node_n);
    int GetBestUct(int color);
    int GetComputerMove(int color, int search);
    void Selfplay();

}uct;


/// <summary>
/// プレイアウトします
/// </summary>
/// <param name="turn_color">手番の石の色</param>
/// <returns>黒の勝ちなら1、負けなら0</returns>
int Position::Playout(int turn_color)
{
    int color = turn_color;

    // １つ前の着手の座標
    int previous_z = 0;

    // ループ・カウンタ
    int loop;
    int loop_max = kBoardSize * kBoardSize + 200; // for triple ko

    all_playouts++;

    for (loop = 0; loop < loop_max; loop++)
    {
        // all empty points are candidates.
        // 確率的に空点を選ぶ仕組み？
        // [0]...z, [1]...probability
        int empty[kBoardMax][2];

        // 配列のインデックス
        int empty_num = 0;
        // 空点を選ぶ確率？
        int prob_sum = 0;

        int x, y, z, err, pr;
        // 壁を除く盤上の全ての空点の(座標,確率)を empty配列にセットします
        for (y = 0; y < kBoardSize; y++)
            for (x = 0; x < kBoardSize; x++)
            {
                int z = GetZ(x + 1, y + 1);
                if (Board[z] != 0)
                    continue;
                empty[empty_num][0] = z;
                // 空点を選ぶ確率？
                pr = boardPattern3x3.GetProb(z, previous_z, color);
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
        if (uct.depth < kDMax)
            uct.path[uct.depth++] = z;

        // もしパスが連続したら対局終了
        if (z == 0 && previous_z == 0)
            break; // continuous pass

        // そうでなければ盤を表示して手番を変えて続行
        previous_z = z;
        //  Prt("loop=%d,z=%s,c=%d,empty_num=%d,ko_z=%d\n",loop,GetCharZ(z),color,empty_num,ko_z);
        color = FlipColor(color);
    }
    return CountScore(turn_color);
}

/// <summary>
/// create new node.
/// 空点を全部追加。
/// PASSも追加。
/// </summary>
/// <param name="prev_z"></param>
/// <returns>ノードのリストのインデックス。作られたノードを指す。最初は0から</returns>
int UpperConfidenceTree::CreateNode(int prev_z)
{
    int x, y, z, i, j;
    // 人間的に盤面上の3x3のパターンの形を考えると悪手なので、着手の確率を下げるための割引率 0.0～1.0
    double bonus;
    Node* pN;

    // これ以上増やせません
    if (node_num == kNodeMax)
    {
        Prt("node over Err\n");
        exit(0);
    }

    // 末尾の未使用の要素
    pN = &nodeList[node_num];
    pN->child_num = 0;
    pN->child_games_sum = 0;

    // 空点をリストの末尾に追加
    for (y = 0; y < kBoardSize; y++)
        for (x = 0; x < kBoardSize; x++)
        {
            z = GetZ(x + 1, y + 1);
            if (position.Board[z] != 0)
                continue;

            // 人間的に盤面上の3x3のパターンの形を考えると悪手なので、着手の確率を下げるための割引率 0.0～1.0
            bonus = GetBonus(z, prev_z);

            pN->AddChild(z, bonus);
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
/// <returns>ノードのリストのインデックス。選択した子ノードを指します</returns>
int UpperConfidenceTree::SelectBestUcb(int node_n)
{
    Node* pN = &nodeList[node_n];
    int select = -1;
    double max_ucb = -999;
    double ucb = 0;
    int i;
    int legal_num = 0;
    int pw_num = (int)(1.0 + log(pN->child_games_sum + 1.0) / log(1.8));
    if (pw_num < 1)
        pw_num = 1;

    // 子要素の数だけ繰り返します
    for (i = 0; i < pN->child_num; i++)
    {
        Child* c = &pN->children[i];

        // 非合法手の座標なら無視
        if (c->z == kIllegalZ)
            continue;

        legal_num++;
        if (legal_num > pw_num)
            break;

        if (c->games == 0)
        {
            ucb = 10000 + (rand() & 0x7fff); // try once
        }
        else
        {
            const double C = 1; // depends on program

            // 何らかのボーナスの計算？
            const double B0 = 0.1;
            const double B1 = 100;
            double plus = B0 * log(1.0 + c->bonus) * sqrt(B1 / (B1 + c->games));

            // ucbの計算にボーナスを加えます
            ucb = c->rate + C * sqrt(log((double)pN->child_games_sum) / c->games) + plus;
        }

        // UCB値の最大を更新
        if (ucb > max_ucb)
        {
            max_ucb = ucb;
            select = i;
        }
    }
    if (select == -1)
    {
        Prt("Err! select\n");
        exit(0);
    }
    return select;
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
    int select, z, err, win;

    // とりあえず打ってみる
    for (;;)
    {
        // 最善の一手（子ノード）のインデックス
        select = SelectBestUcb(node_n);

        // 最善の一手（子ノード）
        c = &pN->children[select];

        // 最善の一手（子ノード）の座標
        z = c->z;

        // 石を置く
        err = position.PutStone(z, color, kFillEyeErr);

        // 合法手ならループを抜けます
        if (err == 0)
            break;

        // 非合法手なら、 kIllegalZ をセットして ループをやり直し
        c->z = kIllegalZ; // select other move
    }

    // 経路に格納
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
        if (c->next == kNodeEmpty)
            c->next = CreateNode(c->z);

        // 手番をひっくり返して UCT探索（ネガマックス形式）。勝率はひっくり返して格納
        win = -SearchUct(FlipColor(color), c->next);
    }

    // 勝率の更新（update winrate）
    c->rate = (c->rate * c->games + win) / (c->games + 1);

    // 対局数カウントアップ
    c->games++;
    pN->child_games_sum++;

    return win;
}

/// <summary>
/// 一番良く打たれた一手の座標を返します
/// </summary>
/// <param name="color">手番の色</param>
/// <returns>一番良く打たれた一手の座標</returns>
int UpperConfidenceTree::GetBestUct(int color)
{
    int next, i, best_z, best_i = -1;
    int max = -999;
    Node* pN;

    // 前回の着手座標？
    int prev_z = 0;

    // 初手以外は前回の着手点を記憶
    if (position.moves > 0)
        prev_z = position.record[position.moves - 1];

    // ノードリストの要素数
    node_num = 0;
    // 次のノードのインデックス。ここでは0。現図を作成しています
    next = CreateNode(prev_z);

    // とりあえず UCT探索（search_uct）を、uct_loop回繰り返します
    for (i = 0; i < uct_loop; i++)
    {
        // 現図を退避
        int board_copy[kBoardMax];
        int ko_z_copy = position.ko_z;
        memcpy(board_copy, position.Board, sizeof(position.Board));

        // 経路の深さを初期化
        depth = 0;

        // UCT探索
        SearchUct(color, next);

        // 現図を復元
        position.ko_z = ko_z_copy;
        memcpy(position.Board, board_copy, sizeof(position.Board));
    }
    // 次のノード
    pN = &nodeList[next];
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
        Prt("%2d:z=%2d,rate=%.4f,games=%3d,bonus=%.4f\n", i, Get81(c->z), c->rate, c->games, c->bonus);
    }
    // ベストなノードの座標
    best_z = pN->children[best_i].z;
    Prt("best_z=%d,rate=%.4f,games=%d,playouts=%d,nodes=%d\n",
        Get81(best_z), pN->children[best_i].rate, max, position.all_playouts, node_num);

    return best_z;
}

/// <summary>
/// コンピューターの指し手
/// </summary>
/// <param name="color">手番の色</param>
/// <param name="search">探索方法。SEARCH_PRIMITIVE または kSearchUct</param>
/// <returns>座標</returns>
int UpperConfidenceTree::GetComputerMove(int color, int search)
{
    // 現在時刻
    clock_t st = clock();
    // 思考時間（秒）
    double t;
    // 座標
    int z;

    // プレイアウト回数
    position.all_playouts = 0;
    if (search == kSearchUct)
    {
        // UCTを使ったゲームプレイ
        z = GetBestUct(color);
    }
    else
    {
        // 原始モンテカルロでゲームプレイ
        z = position.PrimitiveMonteCalro(color);
    }
    // 消費時間（秒）
    t = (double)(clock() + 1 - st) / CLOCKS_PER_SEC;
    // 情報表示
    Prt("z=%s,color=%d,moves=%d,playouts=%d, %.1f sec(%.0f playout/sec)\n",
        GetCharZ(z), color, position.moves, position.all_playouts, t, position.all_playouts / t);

    return z;
}

/// <summary>
/// 黒番は原始モンテカルロ、白番はUCTで自己対戦
/// </summary>
void UpperConfidenceTree::Selfplay()
{
    // 黒の手番
    int color = 1;
    // 座標
    int z;
    // 探索方法フラグ
    int search;

    for (;;)
    {
        if (color == 1)
        {
            // 黒番は原始モンテカルロ探索
            search = kSearchPrimitive;
        }
        else
        {
            // 白番はUCT探索
            search = kSearchUct;
        }

        // 次の一手
        z = GetComputerMove(color, search);
        // 棋譜に書込み
        position.AddMoves(z, color);
        // パスパスなら終局
        if (z == 0 && position.moves > 1 && position.record[position.moves - 2] == 0)
            break;
        // 300手を超えても終局
        if (position.moves > 300)
            break; // too long
        // 手番の色反転
        color = FlipColor(color);
    }

    // SGF形式の棋譜を出力
    position.PrintSgf();
}

/// <summary>
/// GTPプロトコルの1行を読込むのに十分な文字列サイズ
/// </summary>
const int kStrMax = 256;

/// <summary>
/// スペース区切りで3つ分まで読込む？
/// </summary>
const int kTokenMax = 3;

/// <summary>
/// プログラムはここから始まります
/// </summary>
/// <returns>エラーコード。正常時は0</returns>
int main()
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

    // 乱数の種を設定
    srand((unsigned)time(NULL));
    //srand( 0 );  // rand() generates same random number.

    // 盤の初期化
    position.InitBoard();

    // 確率的に空点を選ぶための初期設定？
    boardPattern3x3.ExpandPattern3x3();

    // 自己対戦
    if (0)
    {
        uct.Selfplay();
        return 0;
    }

    // プレイアウトのテスト
    if (0)
    {
        position.TestPlayout();
        return 0;
    }

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
            komi = atof(sa[1]);
            SendGtp("= \n\n");
        }
        // 着手の座標を返してください
        else if (strstr(sa[0], "genmove"))
        {
            int color = 1;
            if (tolower(sa[1][0]) == 'w')
                // 白手番なら
                color = 2;

            // 着手の座標
            z = uct.GetComputerMove(color, kSearchUct);
            // 棋譜に追加
            position.AddMoves(z, color);
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
            position.AddMoves(z, color);
            SendGtp("= \n\n");
        }
        // その他のコマンドには未対応です
        else
        {
            SendGtp("? unknown_command\n\n");
        }
    }

    return 0;
}

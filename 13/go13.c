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

// n路盤
#define B_SIZE 9

// 両端に番兵込みの幅
#define WIDTH (B_SIZE + 2)

// 番兵込みの盤の面積
#define BOARD_MAX (WIDTH * WIDTH)

/// <summary>
/// 盤
/// </summary>
int board[BOARD_MAX] = {
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
/// 右、下、左、上
/// </summary>
int dir4[4] = { +1, +WIDTH, -1, -WIDTH };

/// <summary>
/// 右、下、左、上、右下、左下、左上、右上
/// </summary>
int dir8[8] = { +1, +WIDTH, -1, -WIDTH, +1 + WIDTH, +WIDTH - 1, -1 - WIDTH, -WIDTH + 1 };

/// <summary>
/// コウの座標
/// </summary>
int ko_z;

// 指し手の要素数
#define MAX_MOVES 1000

/// <summary>
/// 棋譜
/// </summary>
int record[MAX_MOVES];

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

// UCTで使われる経路（パス）の最大長
#define D_MAX 1000

/// <summary>
/// UCTで使われる経路（パス）
/// </summary>
int path[D_MAX];

/// <summary>
/// UCTで使われる経路（パス）の先頭がらn番目(0開始)
/// </summary>
int depth;

/// <summary>
/// 盤エリアの何らかの累計？
/// </summary>
int board_area_sum[BOARD_MAX];

/// <summary>
/// プリントなんだけれども 標準エラー出力に出してる？
/// サーバーに出力したくない文字列を表示したいときに使う？
/// </summary>
/// <param name="fmt">書式か？</param>
/// <param name="">可変長引数</param>
void prt(const char* fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    //{ FILE *fp = fopen("out.txt","a"); if ( fp ) { vfprt( fp, fmt, ap ); fclose(fp); } }
    vfprintf(stderr, fmt, ap);
    va_end(ap);
}

/// <summary>
/// Go Text Protocol のコマンドを標準出力に出力します
/// </summary>
/// <param name="fmt">書式か？</param>
/// <param name="">可変長引数</param>
void send_gtp(const char* fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vfprintf(stdout, fmt, ap);
    va_end(ap);
}

// 64ビット符号なし整数型の定義
#if defined(_MSC_VER)
typedef unsigned __int64 uint64;
#define PRIx64 "I64x"
#else
#include <stdint.h>
#include <sys/time.h>
#include <unistd.h>
typedef uint64_t uint64; // Linux
#define PRIx64 "llx"
#endif

// 1...black, 2...white, 3...ko
#define HASH_KINDS 4

// コウ？
#define HASH_KO 3

// 盤の各交点の、種類別のハッシュ？
uint64 hashboard[BOARD_MAX][HASH_KINDS];

// ハッシュ コード？
uint64 hashcode = 0;

/// <summary>
/// 何らかの数？ static変数を使って ずらしてる？？
/// </summary>
/// <returns></returns>
unsigned long rand_xorshift128()
{ // 2^128-1
    static unsigned long x = 123456789, y = 362436069, z = 521288629, w = 88675123;
    unsigned long t;
    t = (x ^ (x << 11)) & 0xffffffff;
    x = y;
    y = z;
    z = w;
    return (w = (w ^ (w >> 19)) ^ (t ^ (t >> 8)));
}

/// <summary>
/// 何らかの数？
/// </summary>
/// <returns></returns>
uint64 rand64()
{
    unsigned long r1 = rand_xorshift128();
    unsigned long r2 = rand_xorshift128();
    uint64 r = ((uint64)r1 << 32) | r2;
    return r;
}

/// <summary>
/// GTPプロトコルとして送信せずに 引数r を表示？
/// </summary>
/// <param name="r"></param>
void prt_code64(uint64 r)
{
    //prt("%016" PRIx64,r);
    prt("%08x%08x", (int)(r >> 32), (int)r);
};

/// <summary>
/// 盤の各交点に乱数を割り振っている？
/// </summary>
void make_hashboard()
{
    int z, i;
    for (z = 0; z < BOARD_MAX; z++)
    {
        //  prt("[%3d]=",z);
        for (i = 0; i < HASH_KINDS; i++)
        {
            hashboard[z][i] = rand64();
            //    prt_code64(hashboard[z][i]); prt(",");
        }
        //  prt("\n");
    }
}

/// <summary>
/// ハッシュコードの 2進数の 0と1 をひっくり返す？
/// </summary>
void hash_pass()
{
    hashcode = ~hashcode;
}

/// <summary>
/// XOR演算する？
/// </summary>
/// <param name="z">着手点</param>
/// <param name="color">手番の色</param>
void hash_xor(int z, int color)
{
    // 指定座標、指定色のハッシュの 2進数の 0と1 を反転させたものを ハッシュコードに記憶させている？
    hashcode ^= hashboard[z][color];
}

/// <summary>
/// x, y を z（座標；配列のインデックス） に変換
/// </summary>
/// <param name="x">is (1 &lt;= x &lt;= 9)</param>
/// <param name="y">is (1 &lt;= y &lt;= 9)</param>
/// <returns></returns>
int get_z(int x, int y)
{
    return y * WIDTH + x;
}

/// <summary>
/// for display only
/// </summary>
/// <param name="z">座標</param>
/// <returns>人が読める形の座標</returns>
int get81(int z)
{
    // 段
    int y = z / WIDTH;

    // 筋
    // 106 = 9*11 + 7 = (x,y)=(7,9) -> 79
    int x = z - y * WIDTH;

    // パスなら0を返します
    if (z == 0)
        return 0;

    // 人が読める形の座標
    return x * 10 + y; // x*100+y for 19x19
}

/// <summary>
/// 同じ命令文で2回呼び出さないでください。
/// don't call twice in same sentence.
///
/// like prt("z0=%s,z1=%s\n",get_char_z(z0),get_char_z(z1));
/// </summary>
/// <param name="z">座標</param>
/// <returns>人が読める形の座標（文字列）</returns>
char* get_char_z(int z)
{
    int x, y, ax;
    static char buf[16];
    sprintf(buf, "pass");
    if (z == 0)
        return buf;
    y = z / WIDTH;
    x = z - y * WIDTH;
    ax = x - 1 + 'A';
    if (ax >= 'I')
        ax++; // from 'A' to 'T', excluding 'I'
    sprintf(buf, "%c%d", ax, B_SIZE + 1 - y);
    return buf;
}

/// <summary>
/// 石の色を反転
/// </summary>
/// <param name="col">石の色</param>
/// <returns>反転した石の色</returns>
int flip_color(int col)
{
    return 3 - col;
}

/// <summary>
/// 盤の描画
/// </summary>
void print_board()
{
    int x, y;
    const char* str[4] = { ".", "X", "O", "#" };
    int played_z = 0;
    int color = 0;
    if (moves > 0)
    {
        played_z = record[moves - 1];
        color = board[played_z];
    }

    prt("   ");
    //for (x=0;x<B_SIZE;x++) prt("%d",x+1);
    for (x = 0; x < B_SIZE; x++)
        prt("%c", 'A' + x + (x > 7));
    prt("\n");
    for (y = 0; y < B_SIZE; y++)
    {
        //  prt("%2d ",y+1);
        prt("%2d ", B_SIZE - y);
        for (x = 0; x < B_SIZE; x++)
        {
            prt("%s", str[board[get_z(x + 1, y + 1)]]);
        }
        if (y == 4)
            prt("  ko_z=%s,moves=%d", get_char_z(ko_z), moves);
        if (y == 7)
            prt("  play_z=%s, color=%d", get_char_z(played_z), color);

        prt("\n");
    }
}

/// <summary>
/// 盤領域を 標準エラー出力へ出力？
/// </summary>
void print_board_area()
{
    int x, y;
    int all = all_playouts;
    if (all == 0)
        all = 1;

    prt("board_area_sum\n   ");
    for (x = 0; x < B_SIZE; x++)
        prt("   %c", 'A' + x + (x > 7));
    prt("\n");

    for (y = 0; y < B_SIZE; y++)
    {
        prt("%2d ", B_SIZE - y);
        for (x = 0; x < B_SIZE; x++)
        {
            int sum = board_area_sum[get_z(x + 1, y + 1)];
            double per = 100.0 * sum / all;
            prt("%4.0f", per);
        }

        prt("\n");
    }
}

/// <summary>
/// 呼吸点を探索するアルゴリズムで使用
/// </summary>
int check_board[BOARD_MAX];

/// <summary>
/// count_liberty関数の中で呼び出されます。再帰
/// </summary>
/// <param name="tz">着手（開始）座標</param>
/// <param name="color">連の色</param>
/// <param name="p_liberty">呼吸点の数</param>
/// <param name="p_stone">連の石の数</param>
void count_liberty_sub(int tz, int color, int* p_liberty, int* p_stone)
{
    int z, i;

    check_board[tz] = 1; // search flag
    (*p_stone)++;        // number of stone
    for (i = 0; i < 4; i++)
    {
        z = tz + dir4[i];
        if (check_board[z])
            continue;
        if (board[z] == 0)
        {
            check_board[z] = 1;
            (*p_liberty)++; // number of liberty
        }
        if (board[z] == color)
            count_liberty_sub(z, color, p_liberty, p_stone);
    }
}

/// <summary>
/// 呼吸点の数
/// </summary>
/// <param name="tz">着手座標</param>
/// <param name="p_liberty">呼吸点の数</param>
/// <param name="p_stone">連の石の数</param>
void count_liberty(int tz, int* p_liberty, int* p_stone)
{
    int i;
    *p_liberty = *p_stone = 0;
    for (i = 0; i < BOARD_MAX; i++)
        check_board[i] = 0;
    count_liberty_sub(tz, board[tz], p_liberty, p_stone);
}

/// <summary>
/// 石を取り上げます
/// </summary>
/// <param name="tz">着手座標</param>
/// <param name="color">石の色</param>
void take_stone(int tz, int color)
{
    int z, i;

    hash_xor(tz, color);
    board[tz] = 0;
    for (i = 0; i < 4; i++)
    {
        z = tz + dir4[i];
        if (board[z] == color)
            take_stone(z, color);
    }
}

/// <summary>
/// 目潰しをエラーとするなら
/// </summary>
const int FILL_EYE_ERR = 1;

/// <summary>
/// 目潰しを合法手とするなら（囲碁のルールでは合法手）
/// </summary>
const int FILL_EYE_OK = 0;

/// <summary>
/// put stone.
/// </summary>
/// <param name="tz">着手座標。0ならパス</param>
/// <param name="color">石の色</param>
/// <param name="fill_eye_err">目潰しをエラーとするなら1、そうでないなら0</param>
/// <returns>エラーコード。success returns 0. in playout, fill_eye_err = 1</returns>
int put_stone(int tz, int color, int fill_eye_err)
{
    // 検索情報を覚えておく配列
    int around[4][3];

    // 相手の石の色
    int un_col = flip_color(color);

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
            hash_xor(ko_z, HASH_KO);

        ko_z = 0;
        hash_pass();
        return 0;
    }

    // count 4 neighbor's liberty and stones.
    for (i = 0; i < 4; i++)
    {
        int z, c, liberty, stone;
        around[i][0] = around[i][1] = around[i][2] = 0;

        // 隣の座標
        z = tz + dir4[i];
        c = board[z]; // color

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
        count_liberty(z, &liberty, &stone);

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
    if (board[tz] != 0)
        return 4;

    // 取れる相手の石を取ります
    for (i = 0; i < 4; i++)
    {
        int lib = around[i][0];
        int c = around[i][2];
        if (c == un_col && lib == 1 && board[tz + dir4[i]] != 0)
        {
            take_stone(tz + dir4[i], un_col);
        }
    }

    // 石を置きます
    board[tz] = color;

    // 着手点のビット列を、XOR演算でひっくり返します
    hash_xor(tz, color);

    // ハッシュコードのビット列の 0,1 をひっくり返している？
    hash_pass();

    // コウであれば、コウの場所のビット列の 0,1 をひっくり返している？
    if (ko_z != 0)
        hash_xor(ko_z, HASH_KO);

    // 着手点を含む連の呼吸点の数を数えます
    count_liberty(tz, &liberty, &stone);

    // 石を1個取ったらコウかも知れない
    if (capture_sum == 1 && stone == 1 && liberty == 1)
    {
        ko_z = ko_maybe;
        hash_xor(ko_z, HASH_KO);
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
int count_score(int turn_color)
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
    for (y = 0; y < B_SIZE; y++)
        for (x = 0; x < B_SIZE; x++)
        {
            int z = get_z(x + 1, y + 1);
            int c = board[z];
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
                mk[board[z + dir4[i]]]++;

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

    // 白番なら勝敗を反転
    if (turn_color == 2)
        win = -win;

    //prt("black_sum=%2d, (stones=%2d, area=%2d)\n",black_sum, kind[1], black_area);
    //prt("white_sum=%2d, (stones=%2d, area=%2d)\n",white_sum, kind[2], white_area);
    //prt("score=%d, win=%d\n",score, win);
    return win;
}

/// <summary>
/// プレイアウトします
/// </summary>
/// <param name="turn_color">手番の石の色</param>
/// <returns>黒の勝ちなら1、負けなら0</returns>
int playout(int turn_color)
{
    // 手番の色
    int color = turn_color;

    // １つ前の着手の座標
    int previous_z = 0;

    // ループ・カウンタ
    int loop;
    int loop_max = B_SIZE * B_SIZE + 200; // for triple ko

    // プレイアウト回数のカウント
    all_playouts++;

    for (loop = 0; loop < loop_max; loop++)
    {
        // all empty points are candidates.
        // [0]...z, [1]...probability
        int empty[BOARD_MAX][2];

        // 配列のインデックス
        int empty_num = 0;

        // 空点を選ぶ確率？
        int prob_sum = 0;

        int x, y, z, err, pr;
        // 壁を除く盤上の全ての空点の(座標,確率)を empty配列にセットします
        for (y = 0; y < B_SIZE; y++)
            for (x = 0; x < B_SIZE; x++)
            {
                int z = get_z(x + 1, y + 1);
                if (board[z] != 0)
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
                    prt("Err! prob_sum=%d,sum=%d,r=%d,r=%d\n", prob_sum, sum, r, i);
                    exit(0);
                }
                z = empty[i][0];
            }
            err = put_stone(z, color, FILL_EYE_ERR);
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
        if (depth < D_MAX)
            path[depth++] = z;

        // もしパスが連続したら対局終了
        if (z == 0 && previous_z == 0)
            break; // continuous pass

        // そうでなければ盤を表示して手番を変えて続行
        previous_z = z;
        //  prt("loop=%d,z=%s,c=%d,empty_num=%d,ko_z=%d\n",loop,get_char_z(z),color,empty_num,ko_z);

        // 手番を反転
        color = flip_color(color);
    }

    return count_score(turn_color);
}

/// <summary>
/// 原始モンテカルロ（ネガマックス）
/// </summary>
/// <param name="color">手番の色</param>
/// <returns>最善手の座標</returns>
int primitive_monte_calro(int color)
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
    int board_copy[BOARD_MAX];
    ko_z_copy = ko_z;
    memcpy(board_copy, board, sizeof(board));

    // 根ノードでは最低-100点から
    best_value = -100;

    // try all empty point
    for (y = 0; y < B_SIZE; y++)
        for (x = 0; x < B_SIZE; x++)
        {
            // 石を置く座標
            int z = get_z(x + 1, y + 1);

            // 空点でなければ無視
            if (board[z] != 0)
                continue;

            // 目潰ししないように石を置く
            err = put_stone(z, color, FILL_EYE_ERR);

            // もし石を置けなかったら、次の交点へ
            if (err != 0)
                continue;

            // 勝った回数
            win_sum = 0;
            for (i = 0; i < try_num; i++)
            {
                // 現局面を退避
                int board_copy2[BOARD_MAX];
                int ko_z_copy2 = ko_z;
                memcpy(board_copy2, board, sizeof(board));

                // プレイアウト
                win = -playout(flip_color(color));
                win_sum += win;

                // 現局面に復元
                ko_z = ko_z_copy2;
                memcpy(board, board_copy2, sizeof(board));
            }

            // 勝率
            win_rate = (double)win_sum / try_num;
            //  print_board();
            //  prt("z=%d,win=%5.3f\n",get81(z),win_rate);

            // 最善手の更新
            if (win_rate > best_value)
            {
                best_value = win_rate;
                best_z = z;
                //    prt("best_z=%d,color=%d,v=%5.3f,try_num=%d\n",get81(best_z),color,best_value,try_num);
            }

            // コウの復元
            ko_z = ko_z_copy;
            // 盤の復元
            memcpy(board, board_copy, sizeof(board)); // resume board
        }

    return best_z;
}

// following are for UCT(Upper Confidence Tree)
// `UCT` - 探索と知識利用のバランスを取る手法

/// <summary>
/// 手を保存するための構造体
/// </summary>
typedef struct
{
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
} CHILD;

// 最大の子数。9路なら82個。+1 for PASS
#define CHILD_SIZE (B_SIZE * B_SIZE + 1)

/// <summary>
/// 局面を保存する構造体
/// </summary>
typedef struct
{
    /// <summary>
    /// 実際の子どもの数
    /// </summary>
    int child_num;
    CHILD child[CHILD_SIZE];
    /// <summary>
    /// 何回の対局でこのノードに来たか（子の合計）
    /// </summary>
    int child_games_sum;
    /// <summary>
    /// レーブ？な対局数？（子の合計）
    /// </summary>
    int child_rave_games_sum;
} NODE;

// 以下、探索木全体を保存

// 最大10000局面まで
#define NODE_MAX 10000

/// <summary>
/// ノードのリスト
/// </summary>
NODE node[NODE_MAX];

/// <summary>
/// ノードのリストのサイズ。登録局面数
/// </summary>
int node_num = 0;

/// <summary>
/// no next node
/// </summary>
const int NODE_EMPTY = -1;

/// <summary>
/// illegal move
/// </summary>
const int ILLEGAL_Z = -1;

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
void add_child(NODE* pN, int z, double bonus)
{
    // 新しい要素のインデックス
    int n = pN->child_num;
    pN->child[n].z = z;
    pN->child[n].games = 0;
    pN->child[n].rate = 0;
    pN->child[n].rave_games = 0;
    pN->child[n].rave_rate = 0;
    pN->child[n].next = NODE_EMPTY;
    pN->child[n].bonus = bonus;
    // ノードのリストのサイズ更新
    pN->child_num++;
}

/// <summary>
/// create new node.
/// 空点を全部追加。
/// PASSも追加。
/// </summary>
/// <param name="prev_z">前回の着手の座標</param>
/// <returns>ノードのリストのインデックス。作られたノードを指す。最初は0から</returns>
int create_node(int prev_z)
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
    NODE* pN;

    // これ以上増やせません
    if (node_num == NODE_MAX)
    {
        prt("node over Err\n");
        exit(0);
    }

    // 末尾の未使用の要素
    pN = &node[node_num];
    pN->child_num = 0;
    pN->child_games_sum = 0;
    pN->child_rave_games_sum = 0;

    // 空点をリストの末尾に追加
    for (y = 0; y < B_SIZE; y++)
        for (x = 0; x < B_SIZE; x++)
        {
            z = get_z(x + 1, y + 1);

            if (board[z] != 0)
                continue;

            add_child(pN, z, 0);
        }
    add_child(pN, 0, 0); // add PASS

    // sort children
    for (i = 0; i < pN->child_num - 1; i++)
    {
        double max_b = pN->child[i].bonus;
        int max_i = i;
        CHILD tmp;
        for (j = i + 1; j < pN->child_num; j++)
        {
            CHILD* c = &pN->child[j];
            if (max_b >= c->bonus)
                continue;
            max_b = c->bonus;
            max_i = j;
        }
        if (max_i == i)
            continue;
        tmp = pN->child[i];
        pN->child[i] = pN->child[max_i];
        pN->child[max_i] = tmp;
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
int select_best_ucb(int node_n, int color)
{
    // この局面
    NODE* pN = &node[node_n];

    // 1手でも選ばれたか否か
    int select = -1;

    double max_ucb = -999;
    double ucb = 0, ucb_rave = 0, beta;

    // ループ カウンター
    int i;

    // 子要素の数だけ繰り返します
    for (i = 0; i < pN->child_num; i++)
    {
        // この子ノード
        CHILD* c = &pN->child[i];

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
            //    if ( depth==0 ) prt("%2d:z=%2d,rate=%6.3f,games=%4d, rave_r=%6.3f,g=%4d, beta=%f,ucb_rave=%f\n", i, get81(c->z), c->rate, c->games, c->rave_rate, c->rave_games,beta,ucb_rave);
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
        prt("Err! select\n");
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
void update_rave(NODE* pN, int color, int current_depth, double win)
{
    // 盤面の各交点の手番の色？
    int played_color[BOARD_MAX];

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
        c = flip_color(c);
    }

    // パスはゼロ クリアー
    // ignore pass
    played_color[0] = 0;

    // すべての子ノードについて
    for (i = 0; i < pN->child_num; i++)
    {
        // 子ノード
        CHILD* c = &pN->child[i];

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
int search_uct(int color, int node_n)
{
    // この局面
    NODE* pN = &node[node_n];

    // 最善の一手（子ノード）
    CHILD* c = NULL;
    int select, z, err, win, current_depth;

    // とりあえず打ってみる
    for (;;)
    {
        // 最善の一手（子ノード）のインデックス
        select = select_best_ucb(node_n, color);

        // 最善の一手（子ノード）
        c = &pN->child[select];

        // 最善の一手（子ノード）の座標
        z = c->z;

        // 石を置く
        err = put_stone(z, color, FILL_EYE_ERR);

        // 合法手ならループを抜けます
        if (err == 0)
            break;

        // 非合法手なら、 ILLEGAL_Z をセットして ループをやり直し
        c->z = ILLEGAL_Z; // select other move
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
    if (c->games <= 0 || depth == D_MAX || (c->z == 0 && depth >= 2 && path[depth - 2] == 0))
    {
        // 手番をひっくり返してプレイアウト
        win = -playout(flip_color(color));
    }
    else
    {
        // 子ノードが葉なら、さらに延長
        if (c->next == NODE_EMPTY)
            c->next = create_node(c->z);

        // 手番をひっくり返して UCT探索（ネガマックス形式）。勝率はひっくり返して格納
        win = -search_uct(flip_color(color), c->next);
    }

    // レーブの更新
    update_rave(pN, color, current_depth, win);

    // 勝率の更新（update winrate）
    c->rate = (c->rate * c->games + win) / (c->games + 1);

    // 対局数カウントアップ
    c->games++;

    // 子の対局数？をカウントアップ
    pN->child_games_sum++;

    return win;
}

/// <summary>
/// number of uct loop
/// </summary>
int uct_loop = 1000;

/// <summary>
/// 一番良く打たれた一手の座標を返します
/// </summary>
/// <param name="color">手番の色</param>
/// <returns>一番良く打たれた一手の座標</returns>
int get_best_uct(int color)
{
    int next, i, best_z, best_i = -1;

    // 手が打たれた対局数の最大数？
    int max = -999;

    // この局面
    NODE* pN;

    // 前回の着手座標？
    int prev_z = 0;

    // 初手以外は前回の着手点を記憶
    if (moves > 0)
        prev_z = record[moves - 1];

    // ノードリストの要素数
    node_num = 0;

    // 次のノードのインデックス。ここでは0。現図を作成しています
    next = create_node(prev_z);

    // とりあえず UCT探索（search_uct）を、uct_loop回繰り返します
    for (i = 0; i < uct_loop; i++)
    {
        // 現図を退避
        int board_copy[BOARD_MAX];
        int ko_z_copy = ko_z;
        memcpy(board_copy, board, sizeof(board));

        // 経路の深さを初期化
        depth = 0;

        // UCT探索
        search_uct(color, next);

        // 現図を復元
        ko_z = ko_z_copy;
        memcpy(board, board_copy, sizeof(board));
    }

    // 次のノード
    pN = &node[next];

    // 子ノード全部確認
    for (i = 0; i < pN->child_num; i++)
    {
        // 子ノード
        CHILD* c = &pN->child[i];

        // 最大対局数（一番打たれた手ということ）の更新
        if (c->games > max)
        {
            best_i = i;
            max = c->games;
        }
        prt("%2d:z=%2d,rate=%6.3f,games=%4d, rave_r=%6.3f,g=%4d\n",
            i, get81(c->z), c->rate, c->games, c->rave_rate, c->rave_games);
    }

    // ベストなノードの座標
    best_z = pN->child[best_i].z;
    prt("best_z=%d,rate=%6.3f,games=%4d,playouts=%d,nodes=%d\n",
        get81(best_z), pN->child[best_i].rate, max, all_playouts, node_num);

    return best_z;
}

/// <summary>
/// 盤の初期化
/// </summary>
void init_board()
{
    int i, x, y;

    // 盤の枠を整えます
    for (i = 0; i < BOARD_MAX; i++)
        board[i] = 3;

    // 盤上の石をすべて取り除きます
    for (y = 0; y < B_SIZE; y++)
        for (x = 0; x < B_SIZE; x++)
            board[get_z(x + 1, y + 1)] = 0;

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
void add_moves(int z, int color)
{
    // 石を置きます
    int err = put_stone(z, color, FILL_EYE_OK);

    // 非合法手なら強制終了
    if (err != 0)
    {
        prt("put_stone err=%d\n", err);
        exit(0);
    }

    // 棋譜の末尾に記入
    record[moves] = z;

    // 棋譜のサイズを伸ばします
    moves++;

    // 盤表示
    print_board();

    // ハッシュコード表示
    prt("hashcode=");
    prt_code64(hashcode);
    prt("\n");
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
int get_computer_move(int color, int search)
{
    // 現在時刻
    clock_t st = clock();

    // 思考時間（秒）
    double t;

    // 座標
    int z;

    // プレイアウト回数
    all_playouts = 0;

    // 盤領域をゼロ クリアー？
    memset(board_area_sum, 0, sizeof(board_area_sum));

    if (search == SEARCH_UCT)
    {
        // UCTを使ったゲームプレイ
        z = get_best_uct(color);
    }
    else
    {
        // 原始モンテカルロでゲームプレイ
        z = primitive_monte_calro(color);
    }

    // 盤領域を表示？
    print_board_area();

    // 消費時間（秒）
    t = (double)(clock() + 1 - st) / CLOCKS_PER_SEC;

    // 情報表示
    prt("z=%s,color=%d,moves=%d,playouts=%d, %.1f sec(%.0f po/sec),depth=%d\n",
        get_char_z(z), color, moves, all_playouts, t, all_playouts / t, depth);

    return z;
}

/// <summary>
/// print SGF game record
/// </summary>
void print_sgf()
{
    int i;

    // ヘッダー出力
    prt("(;GM[1]SZ[%d]KM[%.1f]PB[]PW[]\n", B_SIZE, komi);

    // 指し手出力
    for (i = 0; i < moves; i++)
    {
        // 座標
        int z = record[i];

        // 段
        int y = z / WIDTH;

        // 筋
        int x = z - y * WIDTH;

        // 色
        const char* sStone[2] = { "B", "W" };
        prt(";%s", sStone[i & 1]);

        // パス
        if (z == 0)
        {
            prt("[]");
        }
        else
        {
            prt("[%c%c]", x + 'a' - 1, y + 'a' - 1);
        }

        // 改行
        if (((i + 1) % 10) == 0)
            prt("\n");
    }

    // 終端
    prt(")\n");
}

/// <summary>
/// 黒番は原始モンテカルロ、白番はUCTで自己対戦
/// </summary>
void selfplay()
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
        z = get_computer_move(color, search);

        // 棋譜に書込み
        add_moves(z, color);

        // パスパスなら終局
        if (z == 0 && moves > 1 && record[moves - 2] == 0)
            break;

        // 300手を超えても終局
        if (moves > 300)
            break; // too long

        // 手番の色反転
        color = flip_color(color);
    }

    print_sgf();
}

/// <summary>
/// 黒手番でプレイアウトのテスト
/// </summary>
void test_playout()
{
    // テストプレイであるというフラグを立てます
    flag_test_playout = 1;

    // 黒手番でプレイアウト
    playout(1);

    // 盤表示
    print_board();

    // SGF形式の棋譜を出力
    print_sgf();
}

// GTPプロトコルの1行を読込むのに十分な文字列サイズ
#define STR_MAX 256

// スペース区切りで3つ分まで読込む？
#define TOKEN_MAX 3

/// <summary>
/// GTPプロトコルのループ
/// </summary>
void gtp_loop()
{
    // 入力文字列バッファー
    char str[STR_MAX];

    // スプリットされた文字列のリスト
    char sa[TOKEN_MAX][STR_MAX];

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
        if (fgets(str, STR_MAX, stdin) == NULL)
            break;

        //  prt("gtp<-%s",str);

        // 文字列のスプリットをして、結果を sa配列に格納しています
        count = 0;
        token = strtok(str, seps);
        while (token != NULL)
        {
            strcpy(sa[count], token);
            count++;
            if (count == TOKEN_MAX)
                break;
            token = strtok(NULL, seps);
        }

        // 盤のサイズを n にしてください
        if (strstr(sa[0], "boardsize"))
        {
            // 無視
            //    int new_board_size = atoi( sa[1] );
            send_gtp("= \n\n");
        }
        // 盤を初期化してください
        else if (strstr(sa[0], "clear_board"))
        {
            init_board();
            send_gtp("= \n\n");
        }
        // アプリケーションを終了してください
        else if (strstr(sa[0], "quit"))
        {
            break;
        }
        // GTPプロトコルのバージョンを返してください
        else if (strstr(sa[0], "protocol_version"))
        {
            send_gtp("= 2\n\n");
        }
        // 思考エンジンの名前を返してください
        else if (strstr(sa[0], "name"))
        {
            send_gtp("= your_program_name\n\n");
        }
        // 思考エンジンのバージョンを返してください
        else if (strstr(sa[0], "version"))
        {
            send_gtp("= 0.0.1\n\n");
        }
        // 対応しているコマンドの改行区切りリストを返してください
        else if (strstr(sa[0], "list_commands"))
        {
            send_gtp("= boardsize\nclear_board\nquit\nprotocol_version\n"
                "name\nversion\nlist_commands\nkomi\ngenmove\nplay\n\n");
        }
        // コミを n にしてください
        else if (strstr(sa[0], "komi"))
        {
            komi = atof(sa[1]);
            send_gtp("= \n\n");
        }
        // 着手の座標を返してください
        else if (strstr(sa[0], "genmove"))
        {
            int color = 1;
            if (tolower(sa[1][0]) == 'w')
                color = 2;

            z = get_computer_move(color, SEARCH_UCT);
            add_moves(z, color);
            send_gtp("= %s\n\n", get_char_z(z));
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
            z = get_z(x, B_SIZE - y + 1);
            if (tolower(sa[2][0]) == 'p')
                z = 0; // pass
            add_moves(z, color);
            send_gtp("= \n\n");
        }
        // その他のコマンドには未対応です
        else
        {
            send_gtp("? unknown_command\n\n");
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
    init_board();

    // ハッシュボードの初期化
    make_hashboard();

    // 自己対戦
    if (0)
    {
        selfplay();
        return 0;
    }

    // プレイアウトのテスト
    if (0)
    {
        test_playout();
        return 0;
    }

    // GTPプロトコルのループ
    gtp_loop();

    return 0;
}

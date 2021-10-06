#include <stdio.h>
#include <stdlib.h>

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
const int kBoardMax = (kWidth* kWidth);

/// <summary>
/// 右、下、左、上
/// </summary>
const int kDir4[4] = { +1, +kWidth, -1, -kWidth };

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
/// 石の色を反転
/// </summary>
/// <param name="col">石の色</param>
/// <returns>反転した石の色</returns>
int FlipColor(int col)
{
    return 3 - col;
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
        3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, //    1 2 3 4 5 6 7 8 9
        3, 0, 0, 0, 0, 0, 2, 0, 0, 0, 3, // 1 ┌┬┬┬┬○┬┬┐
        3, 0, 0, 0, 0, 2, 1, 2, 2, 2, 3, // 2 ├┼┼┼○●○○○
        3, 0, 0, 0, 0, 2, 1, 1, 1, 1, 3, // 3 ├┼┼┼○●●●●
        3, 0, 0, 0, 0, 0, 2, 1, 2, 2, 3, // 4 ├┼┼┼┼○●○○
        3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, // 5 ├┼┼┼┼┼┼┼┤
        3, 0, 1, 2, 0, 0, 0, 0, 0, 0, 3, // 6 ├●○┼┼┼┼┼┤
        3, 1, 2, 0, 2, 0, 0, 0, 0, 0, 3, // 7 ●○┼○┼┼┼┼┤
        3, 0, 1, 2, 0, 2, 2, 1, 1, 0, 3, // 8 ├●○┼○○●●┤
        3, 0, 0, 0, 0, 2, 1, 0, 2, 1, 3, // 9 └┴┴┴○●┴○●
        3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 };

    /// <summary>
    /// コウの座標
    /// </summary>
    int ko_z;

    /// <summary>
    /// 呼吸点を探索するアルゴリズムで使用
    /// </summary>
    int check_board[kBoardMax];

    void CountLibertySub(int tz, int color, int* p_liberty, int* p_stone);
    void CountLiberty(int tz, int* p_liberty, int* p_stone);
    void TakeStone(int tz, int color);
    int PutStone(int tz, int color);
    void PrintBoard();
};

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
        // 隣の座標
        z = tz + kDir4[i];

        // もし、チェック済みの交点なら、無視します
        if (check_board[z])
            continue;

        // もし、空点なら、チェック済みにし、呼吸点を１増やします
        if (Board[z] == 0)
        {
            check_board[z] = 1;
            (*p_liberty)++; // number of liberty
        }

        // もし、着手した石と同じ色なら、再帰します
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

    // チェックボードのクリア
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
/// <returns>エラーコード。success returns 0</returns>
int Position::PutStone(int tz, int color)
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
      //if ( wall + mycol_safe == 4 ) return 3; // eye

      // もし、石の上に石を置こうとしたら、反則手
    if (Board[tz] != 0)
        return 4;

    // 取れる相手の石を取ります
    for (i = 0; i < 4; i++)
    {
        int lib = around[i][0];
        int c = around[i][2];
        if (c == un_col && lib == 0 && Board[tz + kDir4[i]])
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
    printf("   ");
    for (x = 0; x < kBoardSize; x++)
        printf("%d", x + 1);

    // 盤の各行の表示
    printf("\n");
    for (y = 0; y < kBoardSize; y++)
    {
        printf("%2d ", y + 1);
        for (x = 0; x < kBoardSize; x++)
        {
            printf("%s", str[Board[GetZ(x + 1, y + 1)]]);
        }
        if (y == 4)
            printf("  ko_z=%d", Get81(ko_z));
        printf("\n");
    }
}

/// <summary>
/// プログラムはここから始まります
/// </summary>
/// <returns>エラーコード。正常時は0</returns>
int main()
{
    int err;

    Position position = Position();

    position.PrintBoard();

    err = position.PutStone(GetZ(7, 5), 2); // take black 6 stones.
    printf("err=%d\n", err);
    position.PrintBoard();
    //  err = PutStone(GetZ(3,7), 1);	// take white one stone.
    //  printf("err=%d\n",err);  print_board();
    //  err = PutStone(GetZ(2,7), 2);	// retake black, ko, illegal
    //  printf("err=%d\n",err);  print_board();
    //  err = PutStone(GetZ(7,9), 2);	// take black one stone. not ko.
    //  printf("err=%d\n",err);  print_board();

    return 0;
}

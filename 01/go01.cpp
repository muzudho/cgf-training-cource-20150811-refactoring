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
const int kBoardMax = (kWidth * kWidth);

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
        3, 2, 1, 1, 0, 1, 0, 0, 0, 0, 3,
        3, 2, 2, 1, 1, 0, 1, 2, 0, 0, 3,
        3, 2, 0, 2, 1, 2, 2, 1, 1, 0, 3,
        3, 0, 2, 2, 2, 1, 1, 1, 0, 0, 3,
        3, 0, 0, 0, 2, 1, 2, 1, 0, 0, 3,
        3, 0, 0, 2, 0, 2, 2, 1, 0, 0, 3,
        3, 0, 0, 0, 0, 2, 1, 1, 0, 0, 3,
        3, 0, 0, 0, 0, 2, 2, 1, 0, 0, 3,
        3, 0, 0, 0, 0, 0, 2, 1, 0, 0, 3,
        3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 };

    void PrintBoard();
};


/// <summary>
/// 盤の描画
/// </summary>
void Position::PrintBoard()
{
    int x, y;
    const char* str[4] = { ".", "X", "O", "#" };

    // 筋の表示
    printf("   ");
    for (x = 0; x < kBoardSize; x++)
        printf("%d", x + 1);
    printf("\n");

    // 盤部の表示
    for (y = 0; y < kBoardSize; y++)
    {
        printf("%2d ", y + 1);
        for (x = 0; x <= kBoardSize; x++)
        {
            int c = this->Board[(y + 1) * kWidth + (x + 1)];
            printf("%s", str[c]);
        }
        printf("\n");
    }
}

/// <summary>
/// プログラムはここから始まります
/// </summary>
/// <returns>エラーコード。正常時は0</returns>
int main()
{
    Position position = Position();

    position.PrintBoard();
    return 0;
}

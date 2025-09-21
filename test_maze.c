#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <windows.h> // 色付けのために追加

// --- データ構造 (変更なし) ---
typedef struct {
    int x;
    int y;
    int direction;
} WallCandidate;

// --- 迷路生成 (変更なし) ---
void GenerateMaze(int* mazeMap, int width, int height) {
    int* groupIDs = (int*)malloc(width * height * sizeof(int));
    WallCandidate* candidates = (WallCandidate*)malloc(width * height * 2 * sizeof(WallCandidate));
    if (groupIDs == NULL || candidates == NULL) { return; }
    for (int i = 0; i < width * height; i++) {
        mazeMap[i] = 3;
        groupIDs[i] = i;
    }
    int totalGroups = width * height;
    while (totalGroups > 1) {
        int candidateCount = 0;
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int idx = y * width + x;
                if (x < width - 1 && groupIDs[idx] != groupIDs[idx + 1])
                    candidates[candidateCount++] = (WallCandidate){x, y, 1};
                if (y < height - 1 && groupIDs[idx] != groupIDs[idx + width])
                    candidates[candidateCount++] = (WallCandidate){x, y, 2};
            }
        }
        if (candidateCount == 0) break;
        int randIdx = rand() % candidateCount;
        WallCandidate wall = candidates[randIdx];
        int wallIdx = wall.y * width + wall.x;
        int groupToMerge;
        int targetGroup = groupIDs[wallIdx];
        if (wall.direction == 1) {
            mazeMap[wallIdx] -= 1;
            groupToMerge = groupIDs[wallIdx + 1];
        } else {
            mazeMap[wallIdx] -= 2;
            groupToMerge = groupIDs[wallIdx + width];
        }
        for (int i = 0; i < width * height; i++) {
            if (groupIDs[i] == groupToMerge) groupIDs[i] = targetGroup;
        }
        totalGroups--;
    }
    free(groupIDs);
    free(candidates);
}


// --- ここからが変更・追加された関数 ---

/**
 * @brief 迷路の壁情報マップを、色情報付きの2値マップに変換する
 * @param binaryMap 変換結果。0:通路, 1:外周壁, 2以上:色付き壁
 */
void ConvertMazeToBinaryWithColor(const int* mazeMap, int width, int height, int* binaryMap) {
    int bWidth = width * 2 + 1;
    int bHeight = height * 2 + 1;

    // 1. まず、マップ全体を外周の壁(1)で埋める
    for (int i = 0; i < bWidth * bHeight; i++) {
        binaryMap[i] = 1;
    }

    // 2. 壁情報マップを元に、通路(0)と色付きの壁(2以上)を配置していく
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int mazeIndex = y * width + x;
            
            // このセルが元々属していた初期グループIDに基づいて色を決定
            // (例: 6色で循環させる。+2は通路(0)と外周壁(1)を避けるため)
            int colorID = (mazeIndex % 6) + 2;

            int bX = x * 2 + 1;
            int bY = y * 2 + 1;

            // マスの中心は常通路
            binaryMap[bY * bWidth + bX] = 0;

            // 右に壁があるかどうか
            if ((mazeMap[mazeIndex] & 1) != 0) {
                binaryMap[bY * bWidth + (bX + 1)] = colorID;
            } else if (x < width - 1) { // 壁がなく通路の場合
                binaryMap[bY * bWidth + (bX + 1)] = 0;
            }

            // 下に壁があるかどうか
            if ((mazeMap[mazeIndex] & 2) != 0) {
                 binaryMap[(bY + 1) * bWidth + bX] = colorID;
            } else if (y < height - 1) { // 壁がなく通路の場合
                binaryMap[(bY + 1) * bWidth + bX] = 0;
            }
        }
    }
}


/**
 * @brief 生成された色情報付き2値マップをコンソールにカラーで描画する
 */
void printColoredBinaryMap(const int* binaryMap, int bWidth, int bHeight) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    const WORD colors[] = {
        FOREGROUND_RED | FOREGROUND_INTENSITY,   // Color ID 2
        FOREGROUND_GREEN | FOREGROUND_INTENSITY, // Color ID 3
        FOREGROUND_BLUE | FOREGROUND_INTENSITY,  // Color ID 4
        FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY, // Color ID 5 (Yellow)
        FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY,  // Color ID 6 (Magenta)
        FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY // Color ID 7 (Cyan)
    };
    
    for (int y = 0; y < bHeight; y++) {
        for (int x = 0; x < bWidth; x++) {
            int cellValue = binaryMap[y * bWidth + x];
            if (cellValue == 0) { // 通路
                printf(" ");
            } else { // 壁
                // 壁の値(1:外周, 2-7:色付き)に応じて色を設定
                if (cellValue >= 2 && cellValue <= 7) {
                    SetConsoleTextAttribute(hConsole, colors[cellValue - 2]);
                } else {
                    // デフォルトの色 (白)
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                }
                printf("@");
            }
        }
        printf("\n");
    }
    // 最後に色を元に戻す
    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
}


// --- メイン関数 (テスト用) ---
int main() {
    srand((unsigned int)time(NULL));

    int mazeWidth = 30;
    int mazeHeight = 15;

    int binaryMapWidth = mazeWidth * 2 + 1;
    int binaryMapHeight = mazeHeight * 2 + 1;

    int* mazeData = (int*)malloc(mazeWidth * mazeHeight * sizeof(int));
    int* binaryMap = (int*)malloc(binaryMapWidth * binaryMapHeight * sizeof(int));

    if (mazeData == NULL || binaryMap == NULL) { return 1; }

    // 1. 迷路の構造を生成
    GenerateMaze(mazeData, mazeWidth, mazeHeight);

    // 2. 迷路構造を元に、色情報付きの2値マップに変換
    ConvertMazeToBinaryWithColor(mazeData, mazeWidth, mazeHeight, binaryMap);

    // 3. 色付きマップを画面に表示して確認
    printf("Generated Colored Binary Maze Map:\n");
    printColoredBinaryMap(binaryMap, binaryMapWidth, binaryMapHeight);

    // 4. メモリを解放
    free(mazeData);
    free(binaryMap);

    return 0;
}
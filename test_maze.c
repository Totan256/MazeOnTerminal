#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

// --- 定数定義 ---
// 迷路のサイズをここで定義
#define MAZE_WIDTH 30
#define MAZE_HEIGHT 16

// 壁の方向を示す定数 (可読性向上のため)
#define WALL_DIR_HORIZONTAL 1
#define WALL_DIR_VERTICAL 2

// --- データ構造の定義 ---

// 「壊せる壁の候補」を管理するための構造体
// h_x, h_y, h_n のようなバラバラの配列を一つにまとめる
typedef struct {
    int x;
    int y;
    int direction; // WALL_DIR_HORIZONTAL または WALL_DIR_VERTICAL
} WallCandidate;


// --- 関数のプロトタイプ宣言 ---

void initializeMaze(int groupIDs[MAZE_WIDTH][MAZE_HEIGHT], int mazeWalls[MAZE_WIDTH][MAZE_HEIGHT]);
void generateMaze(int groupIDs[MAZE_WIDTH][MAZE_HEIGHT], int mazeWalls[MAZE_WIDTH][MAZE_HEIGHT]);
void prepareDrawBuffer(int mazeWalls[MAZE_WIDTH][MAZE_HEIGHT], bool drawBuffer[MAZE_WIDTH * 2][MAZE_HEIGHT * 2]);
void drawMaze(bool drawBuffer[MAZE_WIDTH * 2][MAZE_HEIGHT * 2]);


// --- メイン関数 ---

int main() {
    // 乱数の初期化 (プログラムの最初で一度だけ呼び出すのが定石)
    srand((unsigned int)time(NULL));

    // 迷路データを格納する2次元配列
    int groupIDs[MAZE_WIDTH][MAZE_HEIGHT];  // 各マスのグループIDを管理
    int mazeWalls[MAZE_WIDTH][MAZE_HEIGHT]; // 壁の状態を管理 (0:通路, 1:右壁, 2:下壁, 3:右下壁)

    // 描画用のバッファ
    bool drawBuffer[MAZE_WIDTH * 2][MAZE_HEIGHT * 2];

    // 処理を関数として呼び出す
    initializeMaze(groupIDs, mazeWalls);
    generateMaze(groupIDs, mazeWalls);
    prepareDrawBuffer(mazeWalls, drawBuffer);
    drawMaze(drawBuffer);

    return 0;
}


// --- 各関数の実装 ---

/**
 * @brief 迷路を初期状態にする（すべてのマスを壁で区切り、個別のグループIDを割り当てる）
 */
void initializeMaze(int groupIDs[MAZE_WIDTH][MAZE_HEIGHT], int mazeWalls[MAZE_WIDTH][MAZE_HEIGHT]) {
    int currentID = 0;
    for (int y = 0; y < MAZE_HEIGHT; y++) {
        for (int x = 0; x < MAZE_WIDTH; x++) {
            mazeWalls[x][y] = 3; // 3は「右と下に壁がある」という意味
            groupIDs[x][y] = currentID;
            currentID++;
        }
    }
}

/**
 * @brief クラスカル法を応用して迷路を生成する
 */
void generateMaze(int groupIDs[MAZE_WIDTH][MAZE_HEIGHT], int mazeWalls[MAZE_WIDTH][MAZE_HEIGHT]) {
    // 最大の壁候補数 (多めに確保)
    WallCandidate wallCandidates[MAZE_WIDTH * MAZE_HEIGHT * 2];
    int totalGroups = MAZE_WIDTH * MAZE_HEIGHT;

    // グループが一つになるまでループ
    while (totalGroups > 1) {
        int candidateCount = 0;

        // 1. 壊せる壁の候補をリストアップする
        for (int y = 0; y < MAZE_HEIGHT; y++) {
            for (int x = 0; x < MAZE_WIDTH; x++) {
                // 右隣のマスとグループが違うなら、右の壁を候補に追加
                if (x < MAZE_WIDTH - 1 && groupIDs[x][y] != groupIDs[x + 1][y]) {
                    wallCandidates[candidateCount++] = (WallCandidate){x, y, WALL_DIR_HORIZONTAL};
                }
                // 下隣のマスとグループが違うなら、下の壁を候補に追加
                if (y < MAZE_HEIGHT - 1 && groupIDs[x][y] != groupIDs[x][y + 1]) {
                    wallCandidates[candidateCount++] = (WallCandidate){x, y, WALL_DIR_VERTICAL};
                }
            }
        }
        
        // 2. 候補の中からランダムに壁を1つ選んで壊す
        int randomIndex = rand() % candidateCount;
        WallCandidate chosenWall = wallCandidates[randomIndex];
        
        int x = chosenWall.x;
        int y = chosenWall.y;
        
        int groupToMerge;
        int targetGroup = groupIDs[x][y];

        if (chosenWall.direction == WALL_DIR_HORIZONTAL) {
            mazeWalls[x][y] -= WALL_DIR_HORIZONTAL; // 右の壁を壊す
            groupToMerge = groupIDs[x + 1][y];
        } else { // WALL_DIR_VERTICAL
            mazeWalls[x][y] -= WALL_DIR_VERTICAL; // 下の壁を壊す
            groupToMerge = groupIDs[x][y + 1];
        }

        // 3. 2つのグループを統合する (マップ全体をスキャンしてIDを書き換える)
        for (int row = 0; row < MAZE_HEIGHT; row++) {
            for (int col = 0; col < MAZE_WIDTH; col++) {
                if (groupIDs[col][row] == groupToMerge) {
                    groupIDs[col][row] = targetGroup;
                }
            }
        }

        totalGroups--; // グループの総数を1つ減らす
    }
}


/**
 * @brief 迷路の壁データを、描画用のバッファに変換する
 */
void prepareDrawBuffer(int mazeWalls[MAZE_WIDTH][MAZE_HEIGHT], bool drawBuffer[MAZE_WIDTH * 2][MAZE_HEIGHT * 2]) {
    for (int y = 0; y < MAZE_HEIGHT; y++) {
        for (int x = 0; x < MAZE_WIDTH; x++) {
            int drawX = x * 2;
            int drawY = y * 2;
            drawBuffer[drawX][drawY] = false; // マスの中は常通路
            drawBuffer[drawX + 1][drawY] = (mazeWalls[x][y] % 2 == 1); // 右の壁
            drawBuffer[drawX][drawY + 1] = (mazeWalls[x][y] >= 2);   // 下の壁
            drawBuffer[drawX + 1][drawY + 1] = true;              // マスの角は常に壁
        }
    }
}

/**
 * @brief 描画用バッファの内容をコンソールに出力する
 */
void drawMaze(bool drawBuffer[MAZE_WIDTH * 2][MAZE_HEIGHT * 2]) {
    // 上の枠線
    for (int x = 0; x < MAZE_WIDTH * 2 + 1; x++) {
        printf("@");
    }
    printf("\n");

    // 迷路本体
    for (int y = 0; y < MAZE_HEIGHT * 2; y++) {
        printf("@"); // 左の枠線
        for (int x = 0; x < MAZE_WIDTH * 2; x++) {
            if (drawBuffer[x][y] == false) {
                printf(" ");
            } else {
                printf("@");
            }
        }
        printf("\n");
    }

    
    printf("\n");
}
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <windows.h> // 色付けのために追加

// --- データ構造---
typedef struct {
    int x;
    int y;
    int direction;
    int score;
} WallCandidate;
int compareCandidates(const void *a, const void *b) {
    WallCandidate *candidateA = (WallCandidate *)a;
    WallCandidate *candidateB = (WallCandidate *)b;
    return (candidateA->score - candidateB->score);
}
#define ADJ(id1, id2) adjacencyMatrix[(id1) * maxIDs + (id2)]
// --- 迷路生成---
void GenerateMaze(int* mazeMap, int* areaIDs, int width, int height) {
    int aveAreaSize = width*height/20;//マップを12分割するイメージ

    int* groupIDs = (int*)malloc(width * height * sizeof(int));
    int* areaSizes = (int*)malloc(width * height * sizeof(int));
    WallCandidate* candidates = (WallCandidate*)malloc(width * height * 2 * sizeof(WallCandidate));
    if (groupIDs == NULL || candidates == NULL || areaSizes == NULL || areaIDs == NULL) { return; }
    for (int i = 0; i < width * height; i++) {
        mazeMap[i] = 3;
        areaIDs[i] = groupIDs[i] = i;
        areaSizes[i] = 1;
    }
    int totalGroups = width * height;
    while (totalGroups > 1) {
        //壊せる壁をリストアップ
        int candidateCount = 0;
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int idx = y * width + x;
                //壁を壊せる場合，壁を挟んで存在する二つのエリアのサイズからスコアを
                if (x < width - 1 && groupIDs[idx] != groupIDs[idx + 1]){
                    int score = areaSizes[idx] + areaSizes[idx + 1];//To do ちゃんと決める
                    candidates[candidateCount++] = (WallCandidate){x, y, 1, score};
                }
                if (y < height - 1 && groupIDs[idx] != groupIDs[idx + width]){
                    int score = areaSizes[idx] + areaSizes[idx + width];//To do
                    candidates[candidateCount++] = (WallCandidate){x, y, 2, score};
                }
            }
        }
        if (candidateCount == 0) break;
        qsort(candidates, candidateCount, sizeof(WallCandidate), compareCandidates);//低い順にソート
        double r = (rand()%1000) / 1000.;
        r*=r;
        int randIdx = (rand() % candidateCount)*r*0.7;//スコア低い順に選ばれやすいように後で調整
        WallCandidate wall = candidates[randIdx];
        int wallIdx = wall.y * width + wall.x;
        int groupToMerge;
        int targetGroupID = groupIDs[wallIdx];
        int targetAreaSize = areaSizes[wallIdx];
        int AreaSizeToMerge;
        //右と下どちらかを壊す
        if (wall.direction == 1) {
            mazeMap[wallIdx] -= 1;
            groupToMerge = groupIDs[wallIdx + 1];
            AreaSizeToMerge = areaSizes[wallIdx + 1];
        } else {
            mazeMap[wallIdx] -= 2;
            groupToMerge = groupIDs[wallIdx + width];
            AreaSizeToMerge = areaSizes[wallIdx + width];
        }
        //部屋番号を統合
        for (int i = 0; i < width * height; i++) {
            if (groupIDs[i] == groupToMerge){
                groupIDs[i] = targetGroupID;
            } 
        }
        //エリアIDを統合，両方のエリアサイズが大きい場合はエリアidの統合はしない
        if(aveAreaSize*1.3>AreaSizeToMerge+targetAreaSize){
            int size = AreaSizeToMerge + targetAreaSize;
            for (int i = 0; i < width * height; i++) {
                if (groupIDs[i] == groupToMerge || groupIDs[i] == targetGroupID){
                    areaIDs[i]=areaIDs[wallIdx];
                    areaSizes[i]=size;
                } 
            }
        }
        totalGroups--;
    }
    free(groupIDs);
    free(candidates);
}

// 新しい関数を追加
void BuildAdjacency(const int* mazeMap, const int* areaIDs, int width, int height, bool* adjacencyMatrix, int maxIDs) {
    // 最初にマトリックスをクリアしておく
    for (int i = 0; i < maxIDs * maxIDs; i++) {
        adjacencyMatrix[i] = false;
    }

    // 全てのセルをスキャン
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = y * width + x;
            int id1 = areaIDs[idx];

            // 右のセルとの間に壁がないかチェック
            if (x < width - 1 && (mazeMap[idx] & 1) == 0) {
                int id2 = areaIDs[idx + 1];
                if (id1 != id2) {
                    ADJ(id1, id2) = true;
                    ADJ(id2, id1) = true;
                }
            }

            // 下のセルとの間に壁がないかチェック
            if (y < height - 1 && (mazeMap[idx] & 2) == 0) {
                int id2 = areaIDs[idx + width];
                if (id1 != id2) {
                    ADJ(id1, id2) = true;
                    ADJ(id2, id1) = true;
                }
            }
        }
    }
}

//無向グラフから色付け
void AssignTerritoryColors(int* territoryColors, int maxIDs, const bool* adjacencyMatrix, const int* uniqueAreaIDs, int uniqueCount) {
    for (int i = 0; i < maxIDs; i++) {
        territoryColors[i] = -1;
    }

    #define numColors 6 // 使う色の数
    int rn = 0;//同じ色を何度も使わないようにする

    // 2. 全ての領土IDを順番に処理
    for (int i = 0; i < uniqueCount; i++) {
        int currentID = uniqueAreaIDs[i]; // ユニークIDリストからIDを取得
        bool usedColors[numColors] = { false };

        // 3. 隣接する領土が使っている色をチェック
        for (int neighborID = 0; neighborID < maxIDs; neighborID++) {
            if (adjacencyMatrix[currentID * maxIDs + neighborID]) {
                int neighborColor = territoryColors[neighborID];
                if (neighborColor != -1) {
                    usedColors[neighborColor] = true;
                }
            }
        }

        // 4. 使われていない一番若い色を探して割り当てる
        int chosenColor = rn%numColors;
        for (int c = rn; c < numColors+rn; c++) {
            if (!usedColors[c%numColors]) {
                chosenColor = c%numColors;
                break;
            }
        }
        rn++;
        territoryColors[currentID] = chosenColor;
    }
    printf("maxID=%d, rn=%d\n",maxIDs, rn);
}

//あとでこれも壁の色じゃなくて通路の色を変えるようにする
/**
 * @brief 迷路の壁情報マップを、色情報付きの2値マップに変換する
 * @param binaryMap 変換結果。0:通路, 1:外周壁, 2以上:色付き壁
 */
void ConvertMazeToBinaryWithColor(const int* mazeMap, const int* areaIDs, const int* territoryColors, int width, int height, int* binaryMap) {
    int bWidth = width * 2 + 1;
    int bHeight = height * 2 + 1;

    // 1. まず、マップ全体を外周の壁(-1)で埋める
    for (int i = 0; i < bWidth * bHeight; i++) {
        binaryMap[i] = -1;
    }

    // 2. 壁情報マップを元に、通路(0以上)を配置していく
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int mazeIndex = y * width + x;
            int currentAreaID = areaIDs[mazeIndex];
            int wayColor = territoryColors[currentAreaID];

            int bX = x * 2 + 1;
            int bY = y * 2 + 1;

            // マスの中心は常通路
            binaryMap[bY * bWidth + bX] = wayColor;

            // 右に壁があるかどうか
            if ((mazeMap[mazeIndex] & 1) != 0) {
                binaryMap[bY * bWidth + (bX + 1)] = -1;
            } else if (x < width - 1) { // 壁がなく通路の場合
                binaryMap[bY * bWidth + (bX + 1)] = wayColor;
            }

            // 下に壁があるかどうか
            if ((mazeMap[mazeIndex] & 2) != 0) {
                 binaryMap[(bY + 1) * bWidth + bX] = -1;
            } else if (y < height - 1) { // 壁がなく通路の場合
                binaryMap[(bY + 1) * bWidth + bX] = wayColor;
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
        BACKGROUND_RED | BACKGROUND_INTENSITY,
        BACKGROUND_GREEN | BACKGROUND_INTENSITY,
        BACKGROUND_BLUE | BACKGROUND_INTENSITY,
        BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_INTENSITY,
        BACKGROUND_RED | BACKGROUND_BLUE | BACKGROUND_INTENSITY,
        BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_INTENSITY
    };
    
    for (int y = 0; y < bHeight; y++) {
        for (int x = 0; x < bWidth; x++) {
            int cellValue = binaryMap[y * bWidth + x];
            if (cellValue == -1) { // 壁
                // デフォルトの色 (白)
                SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                printf("@");
            } else {//通路
                SetConsoleTextAttribute(hConsole, colors[cellValue]);
                printf(" ");
            }
        }
        SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
        printf("\n");
    }
    // 最後に色を元に戻す
    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
}


// --- メイン関数 (テスト用) ---
int main() {
    srand((unsigned int)time(NULL));

    int mazeWidth = 5;
    int mazeHeight = 5;

    int binaryMapWidth = mazeWidth * 2 + 1;
    int binaryMapHeight = mazeHeight * 2 + 1;

    int* mazeData = (int*)malloc(mazeWidth * mazeHeight * sizeof(int));
    int* areaIDs = (int*)malloc(mazeWidth * mazeHeight * sizeof(int));
    int* binaryMap = (int*)malloc(binaryMapWidth * binaryMapHeight * sizeof(int));
    int maxIDs = mazeWidth * mazeHeight;
    bool* adjacencyMatrix = (bool*)calloc(maxIDs * maxIDs, sizeof(bool));
    int* territoryColors = (int*)malloc(maxIDs * sizeof(int));
    if (mazeData == NULL || binaryMap == NULL ||adjacencyMatrix == NULL || territoryColors == NULL) { return 1; }

    GenerateMaze(mazeData, areaIDs, mazeWidth, mazeHeight);

    // 2. 完成した迷路から正確な隣接関係を構築
    BuildAdjacency(mazeData, areaIDs, mazeWidth, mazeHeight, adjacencyMatrix, maxIDs);
    int* uniqueAreaIDs = (int*)malloc(maxIDs * sizeof(int));
    //領土のリストアップ
    int uniqueCount = 0;
    bool* seen = (bool*)calloc(maxIDs, sizeof(bool));
    for (int i = 0; i < mazeWidth * mazeHeight; i++) {
        int id = areaIDs[i];
        if (!seen[id]) {
            seen[id] = true;
            uniqueAreaIDs[uniqueCount++] = id;
        }
    }
    free(seen);
    // 2. 貪欲法で色を決定
    AssignTerritoryColors(territoryColors, maxIDs, adjacencyMatrix, uniqueAreaIDs, uniqueCount);

    // 3. 決定した色を使って2値マップに変換
    ConvertMazeToBinaryWithColor(mazeData, areaIDs, territoryColors, mazeWidth, mazeHeight, binaryMap);
    // 3. 色付きマップを画面に表示して確認
    printf("Generated Colored Binary Maze Map:\n");
    printColoredBinaryMap(binaryMap, binaryMapWidth, binaryMapHeight);

    // 4. メモリを解放
    free(mazeData);
    free(binaryMap);
    free(areaIDs);
    free(adjacencyMatrix);
    free(territoryColors);
    return 0;
}
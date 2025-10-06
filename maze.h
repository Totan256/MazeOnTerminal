#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

// 「壊せる壁の候補」を管理するための構造体
typedef struct {
    int x;
    int y;
    int direction; // 1: 水平(右), 2: 垂直(下)
} WallCandidate;

typedef struct{
    int width;
    int height;
    int *Data;
}Map;

int maze_getNum(Map *map, int x, int y){
    int id = map->width*y + x;
    return map->Data[id];
}

/**
 * @brief 指定された幅と高さで迷路を生成し、結果を mazeMap 配列に格納する
 * @param mazeMap 生成された迷路データが格納されるポインタ（1次元配列）
 * @param width   迷路の幅
 * @param height  迷路の高さ
 */
void GenerateMaze(int* mazeMap, int width, int height) {
    // 1. 内部的なデータ構造を動的に確保
    int* groupIDs = (int*)malloc(width * height * sizeof(int));
    WallCandidate* candidates = (WallCandidate*)malloc(width * height * 2 * sizeof(WallCandidate));

    if (groupIDs == NULL || candidates == NULL) {
        printf("Error: Failed to allocate memory for maze generation.\n");
        free(groupIDs);
        free(candidates);
        return;
    }

    // 2. 迷路の初期化
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int index = y * width + x;
            mazeMap[index] = 3;  // 3は「右と下に壁がある」
            groupIDs[index] = index; // 各セルがユニークなグループIDを持つ
        }
    }

    // 3. 迷路生成のメインループ
    int totalGroups = width * height;
    while (totalGroups > 1) {
        int candidateCount = 0;

        // 3a. 壊せる壁の候補をリストアップ
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int currentIndex = y * width + x;
                // 右隣のセルとグループが違うかチェック
                if (x < width - 1 && groupIDs[currentIndex] != groupIDs[currentIndex + 1]) {
                    candidates[candidateCount++] = (WallCandidate){x, y, 1};
                }
                // 下隣のセルとグループが違うかチェック
                if (y < height - 1 && groupIDs[currentIndex] != groupIDs[currentIndex + width]) {
                    candidates[candidateCount++] = (WallCandidate){x, y, 2};
                }
            }
        }

        if (candidateCount == 0){
            printf("not wall\n"); 
            break;
        } // 念のため

        // 3b. ランダムに壁を選んで壊す
        int randomIndex = rand() % candidateCount;
        WallCandidate chosenWall = candidates[randomIndex];
        
        int wallX = chosenWall.x;
        int wallY = chosenWall.y;
        int wallIndex = wallY * width + wallX;

        int groupToMerge;
        int targetGroup = groupIDs[wallIndex];

        if (chosenWall.direction == 1) { // 水平の壁
            mazeMap[wallIndex] -= 1; // 右の壁を壊す
            groupToMerge = groupIDs[wallIndex + 1];
        } else { // 垂直の壁
            mazeMap[wallIndex] -= 2; // 下の壁を壊す
            groupToMerge = groupIDs[wallIndex + width];
        }

        // 3c. グループを統合
        for (int i = 0; i < width * height; i++) {
            if (groupIDs[i] == groupToMerge) {
                groupIDs[i] = targetGroup;
            }
        }
        totalGroups--;
    }

    // 4. 内部で使ったメモリを解放
    free(groupIDs);
    free(candidates);
}

/**
 * @brief 迷路の壁情報マップを、1(壁)と0(通路)の2値マップに変換する
 * @param mazeMap     GenerateMazeによって生成された壁情報マップ
 * @param width       迷路の幅
 * @param height      迷路の高さ
 * @param binaryMap   変換結果が格納されるポインタ (サイズは (width*2+1) * (height*2+1) が必要)
 */
void ConvertMazeToBinary(const int* mazeMap, int width, int height, int* binaryMap) {
    int bWidth = width * 2 + 1;
    int bHeight = height * 2 + 1;

    //マップ全体を壁(1)で埋める
    for (int i = 0; i < bWidth * bHeight; i++) {
        binaryMap[i] = 1;
    }

    //壁情報マップを元に通路(0)を削る
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int mazeIndex = y * width + x;
            int bX = x * 2 + 1;
            int bY = y * 2 + 1;

            //各マスの中心は常に通路
            binaryMap[bY * bWidth + bX] = 0;

            //右に壁がない場合右の通路を削る
            if ((mazeMap[mazeIndex] & 1) == 0 && x < width - 1) {
                binaryMap[bY * bWidth + (bX + 1)] = 0;
            }
            //下に壁がない場合下の通路を削る
            if ((mazeMap[mazeIndex] & 2) == 0 && y < height - 1) {
                binaryMap[(bY + 1) * bWidth + bX] = 0;
            }
        }
    }
}

//生成された迷路描画するテスト用
void printBinaryMap(Map *map, int bWidth, int bHeight) {
    for (int y = 0; y < bHeight; y++) {
        for (int x = 0; x < bWidth; x++) {
            printf("%s", maze_getNum(map, x,y) == 0 ? " " : "@");
        }
        printf("\n");
    }
}
void maze_generate(Map *map, int width, int height){
    srand((unsigned int)time(NULL));

    //2値マップのサイズ計算
    map->width = width * 2 + 1;
    map->height = height * 2 + 1;

    // 必要なメモリを確保
    int* wallData = (int*)malloc(width * height * sizeof(int));
    map->Data = (int*)malloc(map->width * map->height * sizeof(int));

    if (wallData == NULL || map == NULL) {
        printf("Memory allocation failed.\n");
        exit(1);
    }

    //迷路生成
    GenerateMaze(wallData, width, height);

    //マス調に変換
    ConvertMazeToBinary(wallData, width, height, map->Data);

    // 3. 2値マップを画面に表示して確認
    //printf("Generated Binary Maze Map (1=Wall, 0=Path):\n");
    //printBinaryMap(map, binaryMapWidth, binaryMapHeight);
}

/*
int main() {
    srand((unsigned int)time(NULL));

    int mazeWidth = 30;
    int mazeHeight = 15;

    // 2値マップのサイズを計算
    int binaryMapWidth = mazeWidth * 2 + 1;
    int binaryMapHeight = mazeHeight * 2 + 1;

    // 必要なメモリを確保
    int* mazeData = (int*)malloc(mazeWidth * mazeHeight * sizeof(int));
    int* binaryMap = (int*)malloc(binaryMapWidth * binaryMapHeight * sizeof(int));

    if (mazeData == NULL || binaryMap == NULL) {
        printf("Memory allocation failed.\n");
        return 1;
    }

    // 1. 迷路を生成
    GenerateMaze(mazeData, mazeWidth, mazeHeight);

    // 2. 壁情報マップを2値マップに変換
    ConvertMazeToBinary(mazeData, mazeWidth, mazeHeight, binaryMap);

    // 3. 2値マップを画面に表示して確認
    printf("Generated Binary Maze Map (1=Wall, 0=Path):\n");
    printBinaryMap(binaryMap, binaryMapWidth, binaryMapHeight);

    // 4. メモリを解放
    free(mazeData);
    free(binaryMap);

    return 0;
}
*/
//テスト用
void testMaze(Map *map){
    map->width = 24;
    map->height = 24;
    map->Data = (int*)malloc(map->width * map->height * sizeof(int));
    const int initialMapData[] = {
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
        1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
        1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
        1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
        1,0,0,0,0,0,2,2,2,2,2,0,0,0,0,3,0,3,0,3,0,0,0,1,
        1,0,0,0,0,0,2,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,1,
        1,0,0,0,0,0,2,0,0,0,2,0,0,0,0,3,0,0,0,3,0,0,0,1,
        1,0,0,0,0,0,2,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,1,
        1,0,0,0,0,0,2,2,0,2,2,0,0,0,0,3,0,3,0,3,0,0,0,1,
        1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
        1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
        1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
        1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
        1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
        1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
        1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
        1,4,4,4,4,4,4,4,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
        1,4,0,4,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
        1,4,0,0,0,0,5,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
        1,4,0,4,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
        1,4,0,4,4,4,4,4,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
        1,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
        1,4,4,4,4,4,4,4,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
    };
    map->Data = (int*)malloc(map->width * map->height * sizeof(int));
    if (map->Data == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(3);
    }

    // 1次元配列から1次元配列へ、memcpyでデータを一括コピー
    memcpy(map->Data, initialMapData, sizeof(initialMapData));
    
}
#include "Maze.hpp"
#include <time.h>
#include <stdio.h>
#include <stdlib.h> // rand, srand

namespace maze {

// 「壊せる壁の候補」構造体は、このファイル内でしか使わないのでここに定義
struct WallCandidate {
    int x;
    int y;
    int direction; // 1: 水平(右), 2: 垂直(下)
};


void Maze::generate(int cellWidth, int cellHeight) {
    srand((unsigned int)time(NULL));

    // 最終的なバイナリマップのサイズを計算・設定
    width = cellWidth * 2 + 1;
    height = cellHeight * 2 + 1;

    // 壁情報（どっちの壁を壊すか）を格納する一時的なベクター
    std::vector<int> wallData(cellWidth * cellHeight);

    // 1. 棒倒し法で壁情報を生成する
    generateWallData(wallData, cellWidth, cellHeight);

    // 2. 壁情報をもとに、最終的なバイナリマップを作成する
    convertToBinaryMap(wallData, cellWidth, cellHeight);
}

// 壁情報を生成するヘルパー関数 (旧internalGenerate)
void Maze::generateWallData(std::vector<int>& wallData, int cellWidth, int cellHeight) {
    std::vector<int> groupIDs(cellWidth * cellHeight);
    std::vector<WallCandidate> candidates;
    candidates.reserve(cellWidth * cellHeight * 2); // 事前にメモリを確保しておくと効率的

    for (int i = 0; i < cellWidth * cellHeight; ++i) {
        wallData[i] = 3; // 3は「右と下に壁がある」
        groupIDs[i] = i; // 各セルがユニークなグループIDを持つ
    }

    int totalGroups = cellWidth * cellHeight;
    while (totalGroups > 1) {
        candidates.clear();
        for (int y = 0; y < cellHeight; ++y) {
            for (int x = 0; x < cellWidth; ++x) {
                int currentIndex = y * cellWidth + x;
                if (x < cellWidth - 1 && groupIDs[currentIndex] != groupIDs[currentIndex + 1]) {
                    candidates.push_back({x, y, 1});
                }
                if (y < cellHeight - 1 && groupIDs[currentIndex] != groupIDs[currentIndex + cellWidth]) {
                    candidates.push_back({x, y, 2});
                }
            }
        }

        if (candidates.empty()) break;

        int randomIndex = rand() % candidates.size();
        WallCandidate chosenWall = candidates[randomIndex];
        
        int wallIndex = chosenWall.y * cellWidth + chosenWall.x;
        int groupToMerge;
        int targetGroup = groupIDs[wallIndex];

        if (chosenWall.direction == 1) { // 水平の壁
            wallData[wallIndex] -= 1; // 右の壁を壊す (3 -> 2)
            groupToMerge = groupIDs[wallIndex + 1];
        } else { // 垂直の壁
            wallData[wallIndex] -= 2; // 下の壁を壊す (3 -> 1)
            groupToMerge = groupIDs[wallIndex + cellWidth];
        }

        for (int i = 0; i < cellWidth * cellHeight; ++i) {
            if (groupIDs[i] == groupToMerge) {
                groupIDs[i] = targetGroup;
            }
        }
        totalGroups--;
    }
}


// 壁情報マップを、1(壁)と0(通路)の2値マップに変換する
void Maze::convertToBinaryMap(const std::vector<int>& wallData, int cellWidth, int cellHeight) {
    mapData.assign(width * height, 1); // マップ全体を壁(1)で埋める

    for (int y = 0; y < cellHeight; y++) {
        for (int x = 0; x < cellWidth; x++) {
            // wallDataのインデックスはcellWidth/Heightで計算
            int mazeIndex = y * cellWidth + x;
            // mapDataのインデックスは最終的なマップサイズで計算
            int bX = x * 2 + 1;
            int bY = y * 2 + 1;

            mapData[bY * width + bX] = 0; // 各マスの中心は常に通路

            // 右に壁がない場合(=ビット0が0)、右の通路を削る
            if ((wallData[mazeIndex] & 1) == 0) {
                mapData[bY * width + (bX + 1)] = 0;
            }
            // 下に壁がない場合(=ビット1が0)、下の通路を削る
            if ((wallData[mazeIndex] & 2) == 0) {
                mapData[(bY + 1) * width + bX] = 0;
            }
        }
    }
}

int Maze::getNum(int x, int y) const {
    if (x < 0 || x >= width || y < 0 || y >= height) return 1; // 範囲外は壁扱い
    return mapData[y * width + x];
}

void Maze::print() const {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if(x == 1 && y == 1) printf("S");
            else if(x == width - 2 && y == height - 2) printf("G");
            else printf("%s", getNum(x, y) == 0 ? " " : "■");
        }
        printf("\n");
    }
}

} // namespace maze
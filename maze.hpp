#pragma once
#include <vector>

// Mazeクラスをmaze名前空間に入れる
namespace maze {

class Maze {
public:
    // コンストラクタ
    Maze() = default;

    // 迷路を生成する
    void generate(int cellWidth, int cellHeight);

    // 指定座標のデータを取得する (constを追加)
    int getNum(int x, int y) const;
    
    // デバッグ用に迷路を描画する (constを追加)
    void print() const;
    
    // マップの幅と高さを取得するゲッター
    int getWidth() const { return width; }
    int getHeight() const { return height; }

private:
    // ヘルパー関数 (外部から隠蔽)
    void generateWallData(std::vector<int>& wallData, int cellWidth, int cellHeight);
    void convertToBinaryMap(const std::vector<int>& wallData, int cellWidth, int cellHeight);

    int width = 0;
    int height = 0;
    std::vector<int> mapData;
};

} // namespace maze
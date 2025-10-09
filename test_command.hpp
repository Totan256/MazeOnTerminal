#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <memory>   // std::unique_ptr, std::make_unique
#include <cstdint>  // uint8_t
#include <map>      // std::map
#include <functional> // std::function
#include <sstream>  // std::stringstream

// --- クラスの前方宣言 ---
class Directory;
static const uint8_t PERM_READ = 0b100; // 4
static const uint8_t PERM_WRITE = 0b010; // 2
static const uint8_t PERM_EXECUTE = 0b001; // 1
//==============================================================================
// ## 1. ファイルシステムの基底クラス (全てのモノの原型)
//==============================================================================
class FileSystemNode {
public:
    // 権限を管理するためのビットフラグ
    

    // コンストラクタ
    FileSystemNode(std::string name, Directory* parent, uint8_t permissions, int size)
        : name(std::move(name)), parent(parent), permissions(permissions), size(size) {}

    // 派生クラスを安全に破棄するためのお作法
    virtual ~FileSystemNode() = default;

    // 権限を文字列で表示するヘルパー関数
    std::string getPermissionsString() const;

    // メンバ変数
    std::string name;
    Directory* parent;
    uint8_t permissions;
    int size;
};

//==============================================================================
// ## 2. FileSystemNodeを継承した具象クラス
//==============================================================================

// 通常のテキストファイル
class File : public FileSystemNode {
public:
    File(const std::string& name, Directory* parent, std::string content, uint8_t permissions, int size)
        : FileSystemNode(name, parent, permissions, size), content(std::move(content)) {}
    
    std::string content;
};

// 実行可能なコマンドファイル
class Executable : public FileSystemNode {
public:
    Executable(const std::string& name, Directory* parent, uint8_t permissions, int size)
        : FileSystemNode(name, parent, permissions, size) {}
};

// ファイルやディレクトリを格納するコンテナ
class Directory : public FileSystemNode {
public:
    // コンストラクタ (ルートディレクトリの場合、親はnullptr)
    Directory(const std::string& name, Directory* parent, uint8_t permissions);

    // 子ノードを追加・検索するメソッド
    void addChild(std::unique_ptr<FileSystemNode> child);
    FileSystemNode* findChild(const std::string& childName);
    
    // 中身を一覧表示する
    void listContents(bool show_details) const;

private:
    std::vector<std::unique_ptr<FileSystemNode>> children;
};

//==============================================================================
// ## 3. ゲーム全体の状態を管理するクラス
//==============================================================================
class Game {
public:
    Game(); // コンストラクタでファイルシステムを構築
    void run(); // メインループを実行

    Directory* getCurrentDirectory() const { return current_directory; }
    void setCurrentDirectory(Directory* dir) { current_directory = dir; }

private:
    std::unique_ptr<Directory> root; // ファイルシステムの起点
    Directory* current_directory;    // プレイヤーの現在地
};

//==============================================================================
// ## 4. コマンドを処理するクラス
//==============================================================================
class CommandProcessor {
public:
    CommandProcessor(Game& game); // Gameの状態を操作するため参照を受け取る
    void execute(const std::string& input_line);

private:
    Game& game; // ゲーム本体への参照
    
    // コマンド名と、それに対応する処理(関数)を結びつけるMap
    std::map<std::string, std::function<void(const std::vector<std::string>&)>> commands;

    // 各コマンドの処理を実装するメンバ関数
    void cmd_ls(const std::vector<std::string>& args);
    void cmd_cd(const std::vector<std::string>& args);
    void cmd_cat(const std::vector<std::string>& args);
    void cmd_pwd(const std::vector<std::string>& args);
    void cmd_chmod(const std::vector<std::string>& args);
};
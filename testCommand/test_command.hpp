#pragma once

#include <iostream>
#include <string>
#include <set>
#include <vector>
#include <memory>   // std::unique_ptr, std::make_unique
#include <cstdint>  // uint8_t
#include <map>      // std::map
#include <functional> // std::function
#include <sstream>  // std::stringstream

class Directory;
class Game;

// --- クラスの前方宣言 ---
class Directory;
static const uint8_t PERM_NONE = 0b000; // 0
static const uint8_t PERM_READ = 0b100; // 4
static const uint8_t PERM_WRITE = 0b010; // 2
static const uint8_t PERM_EXECUTE = 0b001; // 1
//==============================================================================
// ## 1. ファイルシステムの基底クラス (全てのモノの原型)
//==============================================================================
class FileSystemNode {
public:
    // 権限を管理するためのビットフラグ
    
    enum class Owner { PLAYER, ROOT };
    std::string getOwnerString() const {
        return (owner == Owner::PLAYER) ? "player" : "root";
    }
    // コンストラクタ
    FileSystemNode(std::string name, Directory* parent, uint8_t permissions, int size, Owner owner)
        : name(std::move(name)), parent(parent), permissions(permissions), size(size), owner(owner) {}

    // 派生クラスを安全に破棄するためのお作法
    virtual ~FileSystemNode() = default;

    // 権限を文字列で表示するヘルパー関数
    std::string getPermissionsString() const;

    virtual int getSize() const { return size; }

    // メンバ変数
    std::string name;
    Directory* parent;
    uint8_t permissions;
    int size;
    Owner owner;
    bool checkPerm(bool isSuperUser, uint8_t perm) const{
        if (!this) return false;
        if (isSuperUser) return true;
        if (this->owner == FileSystemNode::Owner::ROOT) return false;
        if (!(this->permissions & perm)) return false;
        return true;
    }
};

//==============================================================================
// ## 2. FileSystemNodeを継承した具象クラス
//==============================================================================

// 通常のテキストファイル
class File : public FileSystemNode {
public:
    File(const std::string& name, Directory* parent, std::string content, uint8_t permissions, int size, Owner owner)
        : FileSystemNode(name, parent, permissions, size, owner), content(std::move(content)) {}
    
    std::string content;
    bool isLarge = false;//catを使えないようにする
    void appendText(const std::string& text) {
        content += text;
        size += text.length();
    }
    int getSize() const override {
        return this->size;
    }
};

//実行妨害ノード
class TrapNode : public FileSystemNode {
public:
    TrapNode(const std::string& name, Directory* parent, int size,Owner owner)
        // サイズ0, 権限なし
        : FileSystemNode(name, parent, PERM_NONE, size, owner) {}
    
    int getSize() const override { return this->size; }
};

// 実行可能なコマンドファイル
class Executable : public FileSystemNode {
public:
    Executable(const std::string& name, Directory* parent, uint8_t permissions, int size, Owner owner)
        : FileSystemNode(name, parent, permissions, size, owner) {}
    int getSize() const override {
        return this->size;
    }
};
class SymbolicLink : public FileSystemNode {
public:
    SymbolicLink(const std::string& name, Directory* parent, std::string target_path, uint8_t permissions, int size, Owner owner)
        : FileSystemNode(name, parent, permissions, size, owner), target_path(std::move(target_path)) {}
    
    int getSize() const override {
        // リンク自体のサイズ (パス文字列の長さ)
        return this->size; 
    }

    std::string target_path; // リンク先のパス文字列 (例: "/bin/make_maze")
};
// ファイルやディレクトリを格納するコンテナ
class Directory : public FileSystemNode {
public:
    // コンストラクタ (ルートディレクトリの場合、親はnullptr)
    Directory(const std::string& name, Directory* parent, uint8_t permissions, Owner owner);

    int getSize() const override {
        int total_size = size; // ディレクトリ自体のサイズは通常0
        for (const auto& child : children) {
            total_size += child->getSize(); // 子ノ-ドのgetSize()を再帰的に呼び出す
        }
        return total_size;
    }

    // 子ノードを追加・検索するメソッド
    void addChild(std::unique_ptr<FileSystemNode> child);
    std::vector<FileSystemNode*> findChildren(const std::string& childName);
    std::vector<FileSystemNode*> getChildren(){
        std::vector<FileSystemNode*> v;
        for (const auto& child : children) {
            v.push_back(child.get());
        }
        return v;
    }
    void removeChild(const std::string& childName);
    std::unique_ptr<FileSystemNode> releaseChild(const std::string& childName);

    // 中身を一覧表示する
    void listContents(bool show_details, bool show_all) const;

private:
    std::vector<std::unique_ptr<FileSystemNode>> children;
};



//==============================================================================
// ## 4. コマンドを処理するクラス
//==============================================================================
class CommandProcessor {
public:
    CommandProcessor(Game& game); // Gameの状態を操作するため参照を受け取る
    void execute(const std::string& input_line);
    struct ShortOptsArgs {
        std::set<char> options;
        std::vector<std::string> paths;
    };

    struct LongOptsArgs {
        std::map<std::string, std::string> options_with_value; // -name "file.txt"
        std::set<std::string> flags; // -delete
        std::vector<std::string> paths;
    };

    ShortOptsArgs parseShortOptions(const std::vector<std::string>& args);
    LongOptsArgs parseLongOptions(const std::vector<std::string>& args);

private:
    Game& game; // ゲーム本体への参照
    bool executeInternal(const std::vector<std::string>& args);
    
    std::vector<std::string> containOptions;
    std::vector<std::string> paths;
    
    // コマンド名と、それに対応する処理(関数)を結びつけるMap
    std::map<std::string, std::function<void(const std::vector<std::string>&)>> commands;
    std::string getNodeFullPath(FileSystemNode* node);
    void findHelper(Directory* dir, const std::string& name_pattern, const std::string& type_filter);

    // 各コマンドの処理を実装するメンバ関数
    void cmd_ls(const std::vector<std::string>& args);
    void cmd_cd(const std::vector<std::string>& args);
    void cmd_cat(const std::vector<std::string>& args);
    void cmd_pwd(const std::vector<std::string>& args);
    void cmd_chmod(const std::vector<std::string>& args);
    void cmd_sudo(const std::vector<std::string>& args);
    void cmd_rm(const std::vector<std::string>& args);
    void cmd_find(const std::vector<std::string>& args);
    void cmd_grep(const std::vector<std::string>& args);
    void cmd_mv(const std::vector<std::string>& args);
    void cmd_ps(const std::vector<std::string>& args);
    void cmd_kill(const std::vector<std::string>& args);
};

class Process{
public:
    Game* game;
    Process(Game& _game, int _id){
        id = _id;
        game = &_game;
        isOperating = true;
    }
    virtual void update() = 0;
    int id;
    bool isOperating;
    std::string name;
};

class MazeMaster : public Process{
public:
    MazeMaster(Game& game, int id) : Process(game, id) {
        name = "MazeMaster";
    }
    void update() override{

    }
};

class Trader : public Process{
public:
    Trader(Game& game, int id) : Process(game, id) {
        name = "trader";
    }
    void update() override{

    }
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

    void setSuperUser(bool status) { is_superuser = status; }
    bool isSuperUser() const { return is_superuser; }
    std::string getSudoPassword() const { return sudo_password; }

    std::vector<FileSystemNode*> resolvePaths(const std::string& path);

    Directory* getTrashDirectory() const { return trash_directory_; }

    std::vector<std::string> outputStrings;//出力を一個にまとめて行う用

    int getMaxDiskSize() const { return maxDiskSize; }
    int getCurrentDiskSize() const {
        if (root) {
            return root->getSize();
        }
        return 0;
    }

    File* logText = nullptr;
    std::map<std::string, bool> usableOptions;
    std::string aliasConvert(std::string arg){
        auto it = aliasesList.find(arg);
        if(it != aliasesList.end()){
            return it->second;
        }
        return arg;
    }
    std::map<int, std::unique_ptr<Process>> processList;
private:
    void resolvePathsRecursive(const std::vector<std::string>& parts, size_t index, std::vector<FileSystemNode*>& current_nodes, std::vector<FileSystemNode*>& results);
    std::unique_ptr<Directory> root; // ファイルシステムの起点
    Directory* current_directory;    // プレイヤーの現在地
    Directory* trash_directory_ = nullptr; // trash
    bool is_superuser = false;
    std::string sudo_password = "1234";
    int maxDiskSize = 256;
    std::map<std::string, std::string> aliasesList;
};
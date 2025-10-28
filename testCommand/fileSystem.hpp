#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <set>
#include <vector>
#include <memory>   // std::unique_ptr
#include <cstdint>  // uint8_t
#include <algorithm>

class Directory;

// --- クラスの前方宣言 ---
class Directory;
static const uint8_t PERM_NONE = 0b000; // 0
static const uint8_t PERM_READ = 0b100; // 4
static const uint8_t PERM_WRITE = 0b010; // 2
static const uint8_t PERM_EXECUTE = 0b001; // 1

bool matchWildCard(const char* str, const char* pattern);

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
    if (this->owner == FileSystemNode::Owner::ROOT) {
        if (perm == PERM_WRITE) {
             return false;
        }
    }
    if (!(this->permissions & perm)) return false;
    
    return true;
}
};

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
    std::stringstream listContents(bool show_details, bool show_all) const;

private:
    std::vector<std::unique_ptr<FileSystemNode>> children;
};
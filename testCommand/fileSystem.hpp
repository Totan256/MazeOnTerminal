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
    File(const std::string& name, Directory* parent, std::vector<std::string> content, uint8_t permissions, int size, Owner owner, bool isLarge)
        : FileSystemNode(name, parent, permissions, size, owner), content(std::move(content)) {
        this->isLarge = isLarge;
    }
    
    std::vector<std::string> content;
    bool isLarge = false;//catを使えないようにする
    void appendText(const std::string& text) {
        content.push_back(text);
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
    Executable(const std::string& name, Directory* parent, uint8_t permissions, int size, Owner owner, std::string internal_name)
        : FileSystemNode(name, parent, permissions, size, owner), commandName(std::move(internal_name)) {}
    int getSize() const override {
        return this->size;
    }
    const std::string commandName;
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

template<typename TBuilder, typename TNode>
class NodeBuilder {
public:
    // ビルダーは親ディレクトリと名前を持って初期化される
    NodeBuilder(Directory* parent, std::string name)
        : parent_(parent), name_(name) {}

    // オプション設定: メソッドチェーン (.with...()) を可能にするため、*this の参照を返す
    TBuilder& withPermissions(uint8_t perms) {
        perms_ = perms;
        return static_cast<TBuilder&>(*this);
    }

    TBuilder& withOwner(FileSystemNode::Owner owner) {
        owner_ = owner;
        return static_cast<TBuilder&>(*this);
    }
    
    // 派生クラスで build() を実装する
protected:
    Directory* parent_;
    std::string name_;

    // --- デフォルト値 ---
    uint8_t perms_ = PERM_READ | PERM_WRITE; // デフォルトは "rw-"
    FileSystemNode::Owner owner_ = FileSystemNode::Owner::PLAYER;
};

// --- File 専用ビルダー ---
class FileBuilder : public NodeBuilder<FileBuilder, File> {
public:
    FileBuilder(Directory* parent, std::string name)
        : NodeBuilder(parent, name) {}

    FileBuilder& withContent(std::vector<std::string> content) {
        content_ = std::move(content);
        return *this;
    }
    FileBuilder& withContent(std::string content) {
        content_.push_back(content);
        return *this;
    }
    
    FileBuilder& isLarge(bool large) {
        isLarge_ = large;
        return *this;
    }

    FileBuilder& withSize(int size) {
        size_ = size;
        return *this;
    }

    // 最終的なオブジェクトを構築し、親に追加する
    File* build();
private:
    // --- Fileのデフォルト値 ---
    std::vector<std::string> content_ = {""};
    bool isLarge_ = false;
    int size_ = 4;
};

class TrapNodeBuilder : public NodeBuilder<TrapNodeBuilder, TrapNode> {
public:
    TrapNodeBuilder(Directory* parent, std::string name)
        : NodeBuilder(parent, name) {}

    TrapNodeBuilder& withSize(int size) {
        size_ = size;
        return *this;
    }

    // 最終的なオブジェクトを構築し、親に追加する
    TrapNode* build();
private:
    int size_ = 100;
};


// --- Executable 専用ビルダー ---
class ExecutableBuilder : public NodeBuilder<ExecutableBuilder, Executable> {
public:
    // Executable の場合、内部コマンド名も必須とする
    ExecutableBuilder(Directory* parent, std::string name, std::string internal_name)
        : NodeBuilder(parent, name), internal_name_(std::move(internal_name)) {
        perms_ = PERM_EXECUTE; // Executable のデフォルト権限を "x" に上書き
    }
    
    ExecutableBuilder& withSize(int size) { // サイズは固定値が多いので
        size_ = size;
        return *this;
    }

    Executable* build();
private:
    std::string internal_name_;
    int size_ = 4; // Executable のデフォルトサイズ
};

// --- Directory 専用ビルダー ---
class DirectoryBuilder : public NodeBuilder<DirectoryBuilder, Directory> {
public:
    DirectoryBuilder(Directory* parent, std::string name)
        : NodeBuilder(parent, name) {
        perms_ = PERM_READ | PERM_EXECUTE; // Directory のデフォルト権限
    }

    Directory* build();
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
    std::vector<std::string> listContents(bool show_details, bool show_all) const;

    FileBuilder buildFile(const std::string& name) {
        return FileBuilder(this, name);
    }
    ExecutableBuilder buildExecutable(const std::string& name, const std::string& internal_name) {
        return ExecutableBuilder(this, name, internal_name);
    }
    DirectoryBuilder buildDirectory(const std::string& name) {
        return DirectoryBuilder(this, name);
    }
    TrapNodeBuilder buildTrapNode(const std::string& name){
        return TrapNodeBuilder(this,name);
    }

private:
    std::vector<std::unique_ptr<FileSystemNode>> children;
};
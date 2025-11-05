#include "fileSystem.hpp"


std::string FileSystemNode::getPermissionsString() const {
    std::string p;
    p += (permissions & PERM_READ) ? 'r' : '-';
    p += (permissions & PERM_WRITE) ? 'w' : '-';
    p += (permissions & PERM_EXECUTE) ? 'x' : '-';
    return p;
}


Directory::Directory(const std::string& name, Directory* parent, uint8_t permissions, Owner owner, bool has_wildcard_trap)
    : FileSystemNode(name, parent, permissions, 0, owner) {
    this->has_wildcard_trap = has_wildcard_trap;
}

void Directory::addChild(std::unique_ptr<FileSystemNode> child) {
    children.push_back(std::move(child));
}
// fileSystem.cpp

bool matchWildCard(const char* str, const char* pattern) {
    const char* s = str;
    const char* p = pattern;
    const char* star = nullptr; // * の位置を記憶
    const char* s_star = nullptr; // * に対応する文字列の位置を記憶

    while (*s != '\0') {
        // 1. パターンと文字列が一致する場合 (または ? の場合)
        // (現在の実装には '?' はありませんが、将来の拡張用)
        if (*p == *s) {
            s++;
            p++;
            continue;
        }

        // 2. パターンが '*' の場合
        if (*p == '*') {
            star = p;     // * の位置を記憶
            p++;          // パターンを次に進める
            s_star = s;   // その時の文字列の位置を記憶
            continue;
        }

        // 3. 一致せず、'*' のバックトラックが可能な場合
        if (star != nullptr) {
            p = star + 1; // パターンを * の次に戻す
            s_star++;     // 文字列側を1文字進める
            s = s_star;   // 文字列ポインタを更新
            continue;
        }

        // 4. 一致せず、バックトラックも不可
        return false;
    }

    // 文字列の終端に達した後、残りのパターンが '*' だけならOK
    while (*p == '*') {
        p++;
    }

    // パターンも終端に達していればマッチ成功
    return *p == '\0';
}

std::vector<FileSystemNode*> Directory::findChildren(const std::string& childName) {
    std::vector<FileSystemNode*> result;
    if (childName.find('*') != std::string::npos && this->has_wildcard_trap) {
        throw WildcardTrapException();
    }
    for (const auto& child : children) {
        //隠しファイルチェック
        if (childName.front() != '.' && child->name.front() == '.') {
            continue;
        }
        if (matchWildCard(child->name.c_str(), childName.c_str())) {
            result.push_back(child.get());
        }
    }
    return result;
}
void Directory::removeChild(const std::string& childName) {
    auto new_end = std::remove_if(children.begin(), children.end(), 
        [&](const std::unique_ptr<FileSystemNode>& child) {
            return child->name == childName;
        });
    children.erase(new_end, children.end());
}
std::unique_ptr<FileSystemNode> Directory::releaseChild(const std::string& childName) {
    auto it = std::find_if(children.begin(), children.end(), 
        [&](const auto& child) { return child->name == childName; });

    if (it != children.end()) {
        std::unique_ptr<FileSystemNode> ptr = std::move(*it);
        children.erase(it);
        return ptr;
    }
    return nullptr;
}

std::vector<std::string> Directory::listContents(bool show_details, bool show_all) const {
    std::vector<std::string> result;
    std::string s;
    for (const auto& child : children) {
        if(!show_all && child->name.front() == '.'){
            continue;
        }
        if (show_details) {
            result.push_back( child->getPermissionsString() + " " + std::to_string(child->size) + "KB "
                      + child->name );
        } else {
            s += child->name + " ";
        }
    }
    if(!show_details) result.push_back(s);

    return result;
}


File* FileBuilder::build() {
    // サイズをコンテントから自動計算
    int size = 0;
    for(const auto& line : content_) size += (line.length() + 1); // +1 は改行分 (概算)

    auto file = std::make_unique<File>(name_, parent_, content_, perms_, size, owner_, isLarge_);
    File* file_ptr = file.get(); // 親に追加する前にポインタを保持
    parent_->addChild(std::move(file)); // 自動で親に追加
    return file_ptr; // 作成したオブジェクトのポインタを返す
}

Executable* ExecutableBuilder::build() {
    auto exec = std::make_unique<Executable>(name_, parent_, perms_, size_, owner_, internal_name_);
    Executable* exec_ptr = exec.get();
    parent_->addChild(std::move(exec));
    return exec_ptr;
}

Directory* DirectoryBuilder::build() {
    auto dir = std::make_unique<Directory>(name_, parent_, perms_, owner_, has_wildcard_trap_);
    Directory* dir_ptr = dir.get();
    parent_->addChild(std::move(dir));
    return dir_ptr;
}

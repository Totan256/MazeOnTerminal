#include "fileSystem.hpp"


std::string FileSystemNode::getPermissionsString() const {
    std::string p;
    p += (permissions & PERM_READ) ? 'r' : '-';
    p += (permissions & PERM_WRITE) ? 'w' : '-';
    p += (permissions & PERM_EXECUTE) ? 'x' : '-';
    return p;
}


Directory::Directory(const std::string& name, Directory* parent, uint8_t permissions, Owner owner)
    : FileSystemNode(name, parent, permissions, 0, owner) {} // ディレクトリは常にrwx

void Directory::addChild(std::unique_ptr<FileSystemNode> child) {
    children.push_back(std::move(child));
}
bool matchWildCard(const char* str, const char* pattern) {
    // 1. パターンが終端に達した場合
    if (*pattern == '\0') {
        // 文字列も終端ならマッチ成功
        return *str == '\0';
    }
    // 2. パターンの現在の文字が'*'の場合
    if (*pattern == '*') {
        // 連続する'*'を1つにまとめる (例: t**t -> t*t)
        while (*(pattern + 1) == '*') {
            pattern++;
        }        
        // '*'が0文字にマッチする場合 (パターンだけ進める)
        // または
        // '*'が1文字以上にマッチする場合 (文字列だけ進め、パターンはそのまま)(文字列が終端に達していないことが条件)
        return matchWildCard(str, pattern + 1) || (*str != '\0' && matchWildCard(str + 1, pattern));
    }
    // 3. パターンの現在の文字が'*'でない場合,文字列が終端でなく、かつ両方の文字が一致すれば、残りの部分を比較する
    if (*str != '\0' && *str == *pattern) {
        return matchWildCard(str + 1, pattern + 1);
    }
    // 4. 上記のいずれにも当てはまらない場合は不一致
    return false;
}

std::vector<FileSystemNode*> Directory::findChildren(const std::string& childName) {
    std::vector<FileSystemNode*> v;
    for (const auto& child : children) {
        if (matchWildCard(child->name.c_str(), childName.c_str())) {
            v.push_back(child.get());
        }
    }
    return v;
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

std::stringstream Directory::listContents(bool show_details, bool show_all) const {
    std::stringstream result;
    for (const auto& child : children) {
        if(!show_all && child->name.front() == '.'){
            continue;
        }
        if (show_details) {
            result << child->getPermissionsString() << " "
                      << child->size << "KB "
                      << child->name << std::endl;
        } else {
            result << child->name << " ";
        }
    }
    if(!show_details) result << std::endl;

    return result;
}

#pragma once
#include "fileSystem.hpp" 
#include "Process.hpp"

#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <memory> // std::unique_ptr

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
    std::vector<FileSystemNode*> resolvePaths(const std::string& path, Directory* baseDir);
    std::vector<FileSystemNode*> resolvePathsWithSymbolic(const std::string& path);

    Directory* getTrashDirectory() const { return trash_directory_; }

    std::stringstream outputString;//出力を一個にまとめて行う用

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
#pragma once

#include <iostream>
#include <string>
#include <set>
#include <vector>
#include <map>
#include <functional> // std::function
#include <sstream>    // std::stringstream


class ShellGame;
class FileSystemNode;
class Directory;

class CommandProcessor {
public:
    CommandProcessor(ShellGame& game); // Gameの状態を操作するため参照を受け取る
    void execute(const std::string& input_line, const std::string& execterName);
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
    ShellGame& game; // ゲーム本体への参照
    bool executeInternal(const std::vector<std::string>& args);
    
    std::vector<std::string> containOptions;
    std::vector<std::string> paths;
    std::string executorName;
    
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
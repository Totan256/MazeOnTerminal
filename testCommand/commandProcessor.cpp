#include <string>
#include <algorithm>
#include "commandProcessor.hpp"
#include "shellGame.hpp"
#include "fileSystem.hpp"

CommandProcessor::CommandProcessor(ShellGame& game) : game(game){
    
    // コマンド名と、実行するラムダ式を結びつける//組み込みは除外
    commands["ls"] = [this](const auto& args){ this->cmd_ls(args); };
    //commands["cd"] = [this](const auto& args){ this->cmd_cd(args); };
    commands["cat"] = [this](const auto& args){ this->cmd_cat(args); };
    commands["pwd"] = [this](const auto& args){ this->cmd_pwd(args); };
    commands["chmod"] = [this](const auto& args){ this->cmd_chmod(args); };
    //commands["sudo"] = [this](const auto& args){ this->cmd_sudo(args); };
    commands["rm"] = [this](const auto& args){ this->cmd_rm(args); };
    commands["find"] = [this](const auto& args){ this->cmd_find(args); };
    commands["grep"] = [this](const auto& args){ this->cmd_grep(args); };
    commands["mv"] = [this](const auto& args){ this->cmd_mv(args); };
    commands["ps"] = [this](const auto& args){ this->cmd_ps(args); };
    commands["kill"] = [this](const auto& args){ this->cmd_kill(args); };
}
CommandProcessor::ShortOptsArgs CommandProcessor::parseShortOptions(const std::vector<std::string>& args) {
    ShortOptsArgs result;
    for (size_t i = 1; i < args.size(); ++i) {
        const std::string& arg = args[i];
        if (arg.rfind("-", 0) == 0 && arg.length() > 1) {
            for (size_t j = 1; j < arg.length(); ++j) {
                char short_opt = arg[j];
                std::string opt_str(1, short_opt);
                if(game.usableOptions.count(opt_str) && game.usableOptions[opt_str]){ // 将来的な有効化チェック
                    result.options.insert(short_opt);
                }
            }
        } else {
            result.paths.push_back(arg);
        }
    }
    return result;
}
CommandProcessor::LongOptsArgs CommandProcessor::parseLongOptions(const std::vector<std::string>& args) {
    LongOptsArgs result;
    for (size_t i = 1; i < args.size(); ++i) {
        const std::string& arg = args[i];
        if (arg.rfind("-", 0) == 0) {
            std::string opt_name = arg.substr(1); // '-' を取り除く
            if(game.usableOptions.count(opt_name) && game.usableOptions[opt_name]){
                // -name や -type のように、次の引数を値として取るオプション
                if ((opt_name == "name" || opt_name == "type") && i + 1 < args.size()) {
                    result.options_with_value[opt_name] = args[i + 1];
                    i++; // 次の引数は消費したので、ループを1つ進める
                } else {
                // -delete のように、引数を取らないオプション（フラグ）
                    result.flags.insert(opt_name);
                }
            }else{
                result.paths.push_back(arg);
            }
        }else{
            result.paths.push_back(arg);
        }
    }
    return result;
}

void CommandProcessor::execute(const std::string& input_line, const std::string& execterName) {
    game.outputString.clear();
    game.outputString << "Log : Name=" << execterName << std::endl;
    std::vector<std::string> args;
    {
        std::stringstream ss(input_line);
        std::string arg;
        while(ss >> arg) {
            args.push_back(arg);
        }
    }

    if (args.empty()) return;

    if(this->executeInternal(args)){
        game.logText->appendText(input_line + "\n");

        //実行後の容量処理
        if (game.getCurrentDiskSize() > game.getMaxDiskSize()) { // game.getCurrentDiskSize()は再帰計算を行う
            game.outputString << "DISK FULL. FILESYSTEM CORRUPTED. SHUTTING DOWN..." << std::endl;
            exit(1);
        }
    }    
}
bool CommandProcessor::executeInternal(const std::vector<std::string>& args){
    std::string command = args.front();
    //最初のコマンド名自体にワイルドカードが含まれていたら実行しない
    if (command.find('*') != std::string::npos) {
        game.outputString << "Command not found: " << command << std::endl;
        return false;
    }
    //エイリアス
    command = game.aliasConvert(command);
    //組み込みコマンドの実行
    if(command=="cd"){
        cmd_cd(args);
        return true;
    }else if(command=="sudo"){
        cmd_sudo(args);
        return true;
    }else if (command == "cd_locked" || command == "sudo_locked") {
        game.outputString << "Command '" << args.front() << "' is locked." << std::endl;
        return true;
    }

    //パス解析（シンボリックリンク）
    auto nodes = game.resolvePathsWithSymbolic(command);
    if (nodes.empty()) {
        game.outputString << "Command not found: " << command << std::endl;
        return false;
    }
    if(nodes.size()!=1){
        game.outputString << "cant use this type"<< std::endl;
        return false;
    }
    FileSystemNode* command_node = nodes.front();
    //確認
    Executable* exec_file = dynamic_cast<Executable*>(command_node);
    if (!exec_file) {//実行ファイルかどうか確認
        game.outputString << command << ": is not an executable file." << std::endl;
        return false;
    }
    if (!exec_file->checkPerm(game.isSuperUser(), PERM_EXECUTE)) {//権限チェック
        game.outputString << "Permission denied: " << command << std::endl;
        return false;
    }    

    //実行
    if (commands.count(command_node->name)) {
        commands[command_node->name](args); // Mapから関数を呼び出す
        //ログテキストのサイズ増加
    } else {// ゲーム内にファイルはあるが、C++側に実装がない場合
        game.outputString << "Execution failed: internal error for command '" << command << "'" << std::endl;
    }
    return true;
}

// --- 各コマンドの処理 ---
void CommandProcessor::cmd_ls(const std::vector<std::string>& args) {
    ShortOptsArgs parsed_args = parseShortOptions(args);

    bool long_format = parsed_args.options.count('l');
    bool all_files = parsed_args.options.count('a');
    std::vector<std::string> paths = parsed_args.paths;

    if(paths.empty()) 
        paths.push_back("."); // デフォルトはカレントディレクトリ

    for(auto path : paths){
        // パスを解決して対象ノードを取得
        //FileSystemNode* target_node = game.resolvePath(path);
        auto target_nodes = game.resolvePaths(path);

        for(auto node : target_nodes){
            // ノードがディレクトリか確認
            Directory* target_dir = dynamic_cast<Directory*>(node);
            if (target_dir) { // まず、ディレクトリかどうかを確認
                if(!target_dir->checkPerm(game.isSuperUser(), PERM_READ)){
                    game.outputString << "ls: cannot open directory '" << node->name << "': Permission denied" << std::endl;
                    continue;
                }
                bool trap_found = false;
                for (auto child : target_dir->getChildren()) {
                    if (dynamic_cast<TrapNode*>(child)) {
                        trap_found = true;
                        break;
                    }
                }
                if (trap_found) {
                    game.outputString << "ls: cannot open directory '" << node->name << "': too large" << std::endl;
                    continue; // このディレクトリの ls をスキップ
                }
                game.outputString << target_dir->listContents(long_format, all_files).str();
            } else if (node) { // ディレクトリではないが、ファイルとして存在する場合
                if(!node->checkPerm(game.isSuperUser(), PERM_READ)){
                     game.outputString << "ls: cannot access '" << node->name << "': Permission denied" << std::endl;
                     continue;
                }
                game.outputString << node->name << std::endl;
            }
        }
    }
}

void CommandProcessor::cmd_cd(const std::vector<std::string>& args) {
    if (args.size() != 2) return;
    const std::string& path = args[1];

    auto nodes = game.resolvePaths(path);
    if(nodes.size() != 1)
    {
        game.outputString<<"cd : cant "<<std::endl;
    }
    FileSystemNode* target_node = nodes.front();
    
    if (!target_node) {
        game.outputString << "cd: No such file or directory: " << path << std::endl;
        return;
    }
    
    Directory* target_dir = dynamic_cast<Directory*>(target_node);
    if(!target_dir){
        game.outputString<<"cd : cant cd such file"<<std::endl;
        return;
    }
    if(!target_dir->checkPerm(game.isSuperUser(), PERM_EXECUTE)){//権限チェック
        return;
    }
    if (target_dir) {
        // 権限チェックはここに集約できる
        if (path == ".." && !(game.getCurrentDirectory()->permissions & PERM_EXECUTE)) {
            game.outputString << "cd: ..: Permission denied." << std::endl;
            return;
        }
        game.setCurrentDirectory(target_dir);
    } else {
        game.outputString << "cd: Not a directory: " << path << std::endl;
    }
}

void CommandProcessor::cmd_cat(const std::vector<std::string>& args) {
    if (args.size() < 2) { game.outputString << "cat: missing operand" << std::endl; return; }
    const std::string& filename = args[1];
    auto nodes = game.resolvePaths(args[1]);

    for(FileSystemNode* node : nodes){
        File* file = dynamic_cast<File*>(node);
        if(!file){
            game.outputString<<"cat : cant cd such file"<<std::endl;
            return;
        }
        if(!file->checkPerm(game.isSuperUser(), PERM_READ)){//権限チェック
            return;
        }
        if(file) {
            if(file->isLarge){
               game.outputString << "cat: " << filename << " is too large" << std::endl;
            }
            game.outputString << file->content << std::endl;
        } else {
            game.outputString << "cat: " << filename << " is not a regular file." << std::endl;
        }
    }    
}
std::string CommandProcessor::getNodeFullPath(FileSystemNode* node){
    if (!node) return "";
    if (node->parent == nullptr) return "/";//ルート

    std::string s = node->name;
    FileSystemNode* current = node->parent;
    while (current != nullptr){
        if (current->parent == nullptr){//ルートディレクトリ
            s = "/" + s;
            break;
        }
        s = current->name + "/" + s;
        current = current->parent;
    }
    return s;
}

void CommandProcessor::cmd_pwd(const std::vector<std::string>& args) {
    if(args.size() != 1) return;
    game.outputString << getNodeFullPath(game.getCurrentDirectory()) << std::endl;
}

void CommandProcessor::cmd_chmod(const std::vector<std::string>& args) {
    if (args.size() != 3) { game.outputString << "chmod: missing operand" << std::endl; return; }
    const std::string& mode = args[1];
    const std::string& filename = args[2];
    auto nodes = game.resolvePaths(args[2]);

    for(FileSystemNode* node :nodes){
        //if (!node) { game.outputString << "chmod: No such file: " << filename << std::endl; return; }
        
        if(!node->checkPerm(game.isSuperUser(), PERM_WRITE)){//権限チェック
            return;
        }
        if (mode == "+x") {
            node->permissions |= PERM_EXECUTE;
            game.outputString << "Set execute permission on " << filename << std::endl;
        } else if (mode == "+r") {
            node->permissions |= PERM_READ;
            game.outputString << "Set read permission on " << filename << std::endl;
        } // 他の権限も同様に追加可能
        else {
            game.outputString << "chmod: Invalid mode: " << mode << std::endl;
        }
    }
}

// CommandProcessorクラスにパスワードを保持するメンバ変数を追加
// std::string sudo_password;

void CommandProcessor::cmd_sudo(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        game.outputString << "sudo: missing command" << std::endl;
        return;
    }

    game.outputString << "[sudo] password for player: ";
    std::string input_password;
    std::getline(std::cin, input_password);

    // 設定されたパスワードと一致するかチェック
    if (input_password == game.getSudoPassword()) {
        // --- 権限昇格 ---
        game.setSuperUser(true); // 管理者モードフラグをON

        // sudoに続くコマンドを再実行
        std::vector<std::string> new_args(args.begin() + 1, args.end());
        // std::string command_name = new_args[0];
        // if(commands.count(command_name)) {
        //     commands[command_name](new_args);
        // }
        this->executeInternal(new_args);
        
        game.setSuperUser(false); // コマンド実行後すぐに権限を戻す
    } else {
        game.outputString << "sudo: incorrect password" << std::endl;
    }
}

void CommandProcessor::cmd_rm(const std::vector<std::string>& args){
    if (args.size() < 2) {
        game.outputString << "rm: missing command" << std::endl;
        return;
    }
    Directory* trash_dir = game.getTrashDirectory();
    if (!trash_dir) return; // ゴミ箱がなければ何もしない
    for(int i=1; i<args.size(); i++){
        auto nodes = game.resolvePaths(args[i]);
        for(FileSystemNode* node : nodes){
            if(!node->checkPerm(game.isSuperUser(), PERM_WRITE)){//権限チェック
                return;
            }
            Directory* parent = node->parent;
            if (parent) {
                std::unique_ptr<FileSystemNode> ptr = parent->releaseChild(node->name);
                if (ptr) {
                    ptr->parent = trash_dir;
                    trash_dir->addChild(std::move(ptr));
                    // to do 容量処理追加
                }
            }
        }
    }
}
void CommandProcessor::findHelper(Directory* dir, const std::string& name_pattern, const std::string& type_filter) {
    // 権限チェック
    if (!dir->checkPerm(game.isSuperUser(), PERM_READ)) {
        game.outputString << "find: '" << getNodeFullPath(dir) << "': Permission denied" << std::endl;
        return;
    }

    // ディレクトリ内の全子要素をチェック
    for (FileSystemNode* child : dir->getChildren()) {
        bool name_match = matchWildCard(child->name.c_str(), name_pattern.c_str());
        bool type_match = true; // デフォルトはOK

        // タイプチェック
        if (type_filter == "f") { // 'f' (File)
            type_match = (dynamic_cast<File*>(child) != nullptr);
        } else if (type_filter == "d") { // 'd' (Directory)
            type_match = (dynamic_cast<Directory*>(child) != nullptr);
        }

        // 両方マッチしたらパスを出力
        if (name_match && type_match) {
            game.outputString << getNodeFullPath(child) << std::endl;
        }

        // 子がディレクトリなら再帰
        Directory* child_dir = dynamic_cast<Directory*>(child);
        if (child_dir) {
            findHelper(child_dir, name_pattern, type_filter);
        }
    }
}

void CommandProcessor::cmd_find(const std::vector<std::string>& args){
    if (args.size() < 2) {
        game.outputString << "find: missing path operand" << std::endl;
        return;
    }
    
    // 既存のパーサーを利用
    LongOptsArgs parsed_args = parseLongOptions(args);
    
    std::string search_path = ".";
    if (!parsed_args.paths.empty()) {
        search_path = parsed_args.paths[0]; // 最初のパスを探索起点とする
    }

    // -name オプションの解析
    std::string name_pattern = "*"; // デフォルトは全一致
    if (parsed_args.options_with_value.count("name")) {
        name_pattern = parsed_args.options_with_value["name"];
    }
    
    // -type オプションの解析 (f: file, d: directory)
    std::string type_filter = ""; // デフォルトはタイプ問わず
    if (parsed_args.options_with_value.count("type")) {
        type_filter = parsed_args.options_with_value["type"];
    }

    // 探索開始
    auto start_nodes = game.resolvePaths(search_path);
    if (start_nodes.empty()) {
        game.outputString << "find: '" << search_path << "': No such file or directory" << std::endl;
        return;
    }

    for (FileSystemNode* node : start_nodes) {
        Directory* start_dir = dynamic_cast<Directory*>(node);
        if (start_dir) {
            // 起点がディレクトリなら再帰探索開始
            findHelper(start_dir, name_pattern, type_filter);
        } else {
            // 起点がファイルなら、それ自身をチェック
            bool name_match = matchWildCard(node->name.c_str(), name_pattern.c_str());
            // (ファイルなので "d" フィルタには絶対一致しない)
            bool type_match = (type_filter != "d"); 
            
            if (name_match && type_match) {
                game.outputString << getNodeFullPath(node) << std::endl;
            }
        }
    }
}

// test_command.cpp (CommandProcessor の実装内)

// ★ 新規
void CommandProcessor::cmd_grep(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        game.outputString << "grep: missing pattern" << std::endl;
        return;
    }
    if (args.size() < 3) {
        game.outputString << "grep: missing file operand" << std::endl;
        return;
    }

    std::string pattern = args[1];
    std::vector<std::string> file_paths;
    for (size_t i = 2; i < args.size(); ++i) {
        file_paths.push_back(args[i]);
    }
    
    bool print_filename = (file_paths.size() > 1); // 検索対象が複数ならファイル名表示

    for (const std::string& path : file_paths) {
        auto nodes = game.resolvePaths(path);
        if (nodes.empty()) {
            game.outputString << "grep: " << path << ": No such file or directory" << std::endl;
            continue;
        }

        for (FileSystemNode* node : nodes) {
            if (dynamic_cast<Directory*>(node)) {
                game.outputString << "grep: " << node->name << ": is a directory" << std::endl;
                continue;
            }
            
            File* file = dynamic_cast<File*>(node);
            if (!file) {
                // Executable など、File ではないノードはスキップ
                continue; 
            }
            
            if (!file->checkPerm(game.isSuperUser(), PERM_READ)) {
                game.outputString << "grep: " << node->name << ": Permission denied" << std::endl;
                continue;
            }

            // ファイル内容を行ごとに検索
            std::stringstream ss(file->content);
            std::string line;
            while (std::getline(ss, line)) {
                // std::string::find でパターンが含まれるかチェック
                if (line.find(pattern) != std::string::npos) {
                    if (print_filename) {
                        game.outputString << node->name << ":";
                    }
                    game.outputString << line << std::endl;
                }
            }
        }
    }
}

void CommandProcessor::cmd_mv(const std::vector<std::string>& args) {
    // 引数が3未満（mv, src, dest）の場合はエラー
    if (args.size() < 3) {
        game.outputString << "mv: missing destination file operand" << std::endl;
        return;
    }

    // 最後の引数を移動先パスとして取得
    std::string dest_path = args.back();
    auto dest_nodes = game.resolvePaths(dest_path);

    // 移動先が存在しない、または複数見つかった場合はエラー
    if (dest_nodes.size() != 1) {
        game.outputString << "mv: target '" << dest_path << "' is not a directory or does not exist." << std::endl;
        return;
    }
    // 移動先がディレクトリでない場合はエラー
    Directory* dest_dir = dynamic_cast<Directory*>(dest_nodes.front());
    if (!dest_dir) {
        game.outputString << "mv: target '" << dest_path << "' is not a directory" << std::endl;
        return;
    }
    // (ここに移動先の権限チェックを追加)
    if(!dest_dir->checkPerm(game.isSuperUser(), PERM_WRITE)){ return; }

    // 最初(args[1])から最後の一つ手前までをループで処理
    for (size_t i = 1; i < args.size() - 1; ++i) {
        const std::string& src_path = args[i];
        auto src_nodes = game.resolvePaths(src_path);

        for (FileSystemNode* src_node : src_nodes) {
            // (ここに移動元の権限チェックなどを追加)
            if (!src_node || !src_node->parent) continue;
            if(!src_node->checkPerm(game.isSuperUser(), PERM_WRITE)){ continue; }
            if(!src_node->parent->checkPerm(game.isSuperUser(), PERM_WRITE)){ continue; }
            
            // 実際の移動処理
            std::unique_ptr<FileSystemNode> ptr = src_node->parent->releaseChild(src_node->name);
            if (ptr) {
                src_node->parent = dest_dir;
                dest_dir->addChild(std::move(ptr));
            }
        }
    }
}
void CommandProcessor::cmd_ps(const std::vector<std::string>& args) {
    if (args.size() != 1) {
        game.outputString << "ps: missing command" << std::endl;
        return;
    }
    for(auto const& [pid, process] : game.processList){
        if (process) {
            game.outputString<<"id="<< process->id<<" : name="<<process->name << std::endl;
        }
    }
}
void CommandProcessor::cmd_kill(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        game.outputString << "kill: missing command" << std::endl;
        return;
    }
    
    for(int i=1; i<args.size(); i++){
        try {
            int id = stoi(args.at(i));
            if(game.processList.count(id)){
                game.processList.erase(id);
            }else{
                game.outputString<<"no process : id = "<< id <<std::endl;
            }
        } catch (const std::invalid_argument& e) {
            game.outputString << "kill: argument must be a process ID" << std::endl;
            continue; // 次の引数へ
        } catch (const std::out_of_range& e) {
            game.outputString << "kill: process ID out of range" << std::endl;
            continue; // 次の引数へ
        }
        
    }
}
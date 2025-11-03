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
    game.outputString.push_back("[Log : Name=" + execterName+"]");
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
            game.outputString.push_back("DISK FULL. FILESYSTEM CORRUPTED. SHUTTING DOWN...");
            //exit(1);
        }
    }    
}
bool CommandProcessor::executeInternal(const std::vector<std::string>& args){
    std::string command = args.front();
    //最初のコマンド名自体にワイルドカードが含まれていたら実行しない
    if (command.find('*') != std::string::npos) {
        game.outputString.push_back("Command not found: " + command);
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
        game.outputString.push_back("Command '" + args.front() + "' is locked.");
        return true;
    }

    //パス解析（シンボリックリンク）
    auto nodes = game.resolvePathsWithSymbolic(command);
    if (nodes.empty()) {
        game.outputString.push_back("Command not found: " + command);
        return false;
    }
    if(nodes.size()!=1){
        game.outputString.push_back("cant use this type");
        return false;
    }
    FileSystemNode* command_node = nodes.front();
    //確認
    Executable* exec_file = dynamic_cast<Executable*>(command_node);
    if (!exec_file) {//実行ファイルかどうか確認
        game.outputString.push_back(command + ": is not an executable file.");
        return false;
    }
    if (!exec_file->checkPerm(game.isSuperUser(), PERM_EXECUTE)) {//権限チェック
        game.outputString.push_back("Permission denied: " + command);
        return false;
    }    

    //実行
    if (commands.count(exec_file->commandName)) {
        commands[exec_file->commandName](args); // Mapから関数を呼び出す
        //ログテキストのサイズ増加
    } else {// ゲーム内にファイルはあるが、C++側に実装がない場合
        game.outputString.push_back("Execution failed: internal error for command '" + command + "'");
    }
    return true;
}

// --- 各コマンドの処理 ---
void CommandProcessor::cmd_ls(const std::vector<std::string>& args) {
    ShortOptsArgs parsed_args = parseShortOptions(args);

    bool long_format = parsed_args.options.count('l');
    bool all_files = parsed_args.options.count('a');
    std::vector<std::string> paths = parsed_args.paths;

    std::vector<std::string> output; //trap nodeがあった時に出力させないためにいったんバッファに書く

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
                bool trap_found = false;
                for (auto child : target_dir->getChildren()) {
                    if (dynamic_cast<TrapNode*>(child)) {
                        trap_found = true;
                        break;
                    }
                }
                if (trap_found) {
                    game.outputString.push_back("ls: Too much content to display");
                    return;
                }
                if(!target_dir->checkPerm(game.isSuperUser(), PERM_READ)){
                    output.push_back("ls: cannot open directory '" + node->name + "': Permission denied");
                    continue;
                }
                std::vector<std::string> result = target_dir->listContents(long_format, all_files);
                for(auto s : result)
                    output.push_back(s);
            } else if (node) { // ディレクトリではないが、ファイルとして存在する場合
                if(!node->checkPerm(game.isSuperUser(), PERM_READ)){
                     output.push_back("ls: cannot access '" + node->name + "': Permission denied");
                     continue;
                }
                output.push_back(node->name);
            }
        }
    }
    for(auto s : output){
        game.outputString.push_back(s);
    }
}

void CommandProcessor::cmd_cd(const std::vector<std::string>& args) {
    if (args.size() != 2) return;
    const std::string& path = args[1];

    auto nodes = game.resolvePaths(path);
    if(nodes.size() != 1)
    {
        game.outputString.push_back("cd : cant ");
    }
    FileSystemNode* target_node = nodes.front();
    
    if (!target_node) {
        game.outputString.push_back("cd: No such file or directory: " + path);
        return;
    }
    
    Directory* target_dir = dynamic_cast<Directory*>(target_node);
    if(!target_dir){
        game.outputString.push_back("cd : cant cd such file");
        return;
    }
    if(!target_dir->checkPerm(game.isSuperUser(), PERM_EXECUTE)){//権限チェック
        game.outputString.push_back("cd: " + path + ": Permission denied");
        return;
    }
    if (target_dir) {
        // 権限チェックはここに集約できる
        if (path == ".." && !(game.getCurrentDirectory()->permissions & PERM_EXECUTE)) {
            game.outputString.push_back("cd: ..: Permission denied.");
            return;
        }
        game.setCurrentDirectory(target_dir);
    } else {
        game.outputString.push_back("cd: Not a directory: " + path);
    }
}

void CommandProcessor::cmd_cat(const std::vector<std::string>& args) {
    if (args.size() < 2) { 
        game.outputString.push_back("cat: missing operand"); 
        return; 
    }
    std::vector<std::string> output; //trap nodeまたはlarge fileがあった時に出力させないためにいったんバッファに書く
    
    // cat *, cat a b* のように複数の引数やワイルドカードを処理するため、
    // args[1] から最後までループします。
    for (size_t i = 1; i < args.size(); ++i) {
        const std::string& path_pattern = args[i];
        auto nodes = game.resolvePaths(path_pattern); // ワイルドカードを解決

        if (nodes.empty()) {
            output.push_back("cat: " + path_pattern + ": No such file or directory");
            continue;
        }

        for(FileSystemNode* node : nodes){
            File* file = dynamic_cast<File*>(node);
            // FileでもDirectoryでもない場合 (Executableなど)
            if(!file){
                if(dynamic_cast<TrapNode*>(node)){
                    game.outputString.push_back("cat: Too much content to display");
                    return;
                }
                output.push_back("cat: " + node->name + ": Is not a regular file");
                continue; // ★ return せずに次のノードへ
            } 
            if(file->isLarge){
               game.outputString.push_back("cat: Too much content to display");
               return;
            }
            // ディレクトリの場合
            if (dynamic_cast<Directory*>(node)) {
                output.push_back("cat: " + node->name + ": Is a directory");
                continue; // ★ return せずに次のノードへ
            }
                       
            // 権限チェック（エラーメッセージ追加）
            if(!file->checkPerm(game.isSuperUser(), PERM_READ)){
                output.push_back("cat: " + node->name + ": Permission denied");
                continue; // ★ return せずに次のノードへ
            }            
            output.push_back("["+file->name+"]");
            for(auto line : file->content)
                output.push_back(line);
        }
    }
    for(auto s : output){
        game.outputString.push_back(s);
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
    game.outputString.push_back(getNodeFullPath(game.getCurrentDirectory()));
}

void CommandProcessor::cmd_chmod(const std::vector<std::string>& args) {
    if (args.size() != 3) { game.outputString.push_back("chmod: missing operand"); return; }
    const std::string& mode_str = args[1];
    const std::string& filename = args[2];
    auto nodes = game.resolvePaths(args[2]);

    for(FileSystemNode* node :nodes)
        if(dynamic_cast<TrapNode*>(node)){
            game.outputString.push_back("chmod: Too many files");
            return;
        }

    for(FileSystemNode* node :nodes){
        if(!node->checkPerm(game.isSuperUser(), PERM_WRITE)){//権限チェック
            game.outputString.push_back("chmod: cannot access '" + node->name + "': Permission denied");
            continue;
        }
        // モード指定の解析
        if (mode_str.find_first_not_of("01234567") == std::string::npos && mode_str.length() == 1) {
            try {
                uint8_t new_perms = static_cast<uint8_t>(std::stoul(mode_str, nullptr, 8));
                node->permissions = new_perms;
            } catch (...) {
                game.outputString.push_back("chmod: Invalid mode: " + mode_str);
            }
        } else {
            if (mode_str == "+r") {
                node->permissions |= PERM_READ;
            } else if (mode_str == "-r") {
                node->permissions &= ~PERM_READ;
            } else if (mode_str == "+w") {
                node->permissions |= PERM_WRITE;
            } else if (mode_str == "-w") {
                node->permissions &= ~PERM_WRITE;
            } else if (mode_str == "+x") {
                node->permissions |= PERM_EXECUTE;
            } else if (mode_str == "-x") {
                node->permissions &= ~PERM_EXECUTE;
            } else {
                game.outputString.push_back("chmod: Invalid mode: " + mode_str);
            }
        }
    }
}

void CommandProcessor::cmd_sudo(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        game.outputString.push_back("sudo: missing command");
        return;
    }
    game.currentState = ShellState::WAITING_PASSWORD;

    // sudoに続くコマンドを保存
    std::vector<std::string> new_args(args.begin() + 1, args.end());
    game.setSudoCommand(new_args);
}

void CommandProcessor::cmd_rm(const std::vector<std::string>& args){
    if (args.size() < 2) {
        game.outputString.push_back("rm: missing command");
        return;
    }
    Directory* trash_dir = game.getTrashDirectory();
    if (!trash_dir) return; // ゴミ箱がなければ何もしない
    for(int i=1; i<args.size(); i++){
        auto nodes = game.resolvePaths(args[i]);
        for(FileSystemNode* node :nodes)
            if(dynamic_cast<TrapNode*>(node)){
                game.outputString.push_back("rm: "+ args[i] + ": Too many files");
                return;
            }
        for(FileSystemNode* node : nodes){
            if(!node->checkPerm(game.isSuperUser(), PERM_WRITE)){//権限チェック
                game.outputString.push_back("rm: cannot remove '" + node->name + "': Permission denied");
                continue;
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
        game.outputString.push_back("find: '" + getNodeFullPath(dir) + "': Permission denied");
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
            game.outputString.push_back(getNodeFullPath(child));
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
        game.outputString.push_back("find: missing path operand");
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
        game.outputString.push_back("find: '" + search_path + "': No such file or directory");
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
                game.outputString.push_back(getNodeFullPath(node));
            }
        }
    }
}

// test_command.cpp (CommandProcessor の実装内)

// ★ 新規
void CommandProcessor::cmd_grep(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        game.outputString.push_back("grep: missing pattern");
        return;
    }
    if (args.size() < 3) {
        game.outputString.push_back("grep: missing file operand");
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
            game.outputString.push_back("grep: " + path + ": No such file or directory");
            continue;
        }

        for (FileSystemNode* node : nodes) {
            if (dynamic_cast<Directory*>(node)) {
                game.outputString.push_back("grep: " + node->name + ": is a directory");
                continue;
            }
            
            File* file = dynamic_cast<File*>(node);
            if (!file) {
                // Executable など、File ではないノードはスキップ
                continue; 
            }
            
            if (!file->checkPerm(game.isSuperUser(), PERM_READ)) {
                game.outputString.push_back("grep: " + node->name + ": Permission denied");
                continue;
            }

            // ファイル内容を行ごとに検索
            for(auto line : file->content){
                // std::string::find でパターンが含まれるかチェック
                if (line.find(pattern) != std::string::npos) {
                    if (print_filename) {
                        game.outputString.push_back(node->name + ":");
                    }
                    game.outputString.push_back(line);
                }
            }
        }
    }
}

// commandProcessor.cpp

void CommandProcessor::cmd_mv(const std::vector<std::string>& args) {
    if (args.size() < 3) {
        game.outputString.push_back("mv: missing destination file operand");
        return;
    }

    std::string dest_path_str = args.back();
    auto dest_nodes = game.resolvePaths(dest_path_str);

    // ケース1: 移動先 (dest) がディレクトリの場合
    Directory* dest_dir = nullptr;
    if (dest_nodes.size() == 1) { // 移動先が一意に決まる
        dest_dir = dynamic_cast<Directory*>(dest_nodes.front());
    }

    if (dest_dir) {
        // dest がディレクトリの場合 (mv src1 src2... dest_dir)
        if (!dest_dir->checkPerm(game.isSuperUser(), PERM_WRITE)) {
            game.outputString.push_back("mv: cannot move to '" + dest_path_str + "': Permission denied");
            return;
        }

        for (size_t i = 1; i < args.size() - 1; ++i) {
            const std::string& src_path = args[i];
            auto src_nodes = game.resolvePaths(src_path);

            for (FileSystemNode* src_node : src_nodes) {
                if (!src_node || !src_node->parent) continue;
                if (!src_node->checkPerm(game.isSuperUser(), PERM_WRITE) || !src_node->parent->checkPerm(game.isSuperUser(), PERM_WRITE)) {
                    game.outputString.push_back("mv: cannot move '" + src_node->name + "': Permission denied");
                    continue;
                }
                
                // 実際の移動処理
                std::unique_ptr<FileSystemNode> ptr = src_node->parent->releaseChild(src_node->name);
                if (ptr) {
                    ptr->parent = dest_dir; // 親を更新
                    dest_dir->addChild(std::move(ptr));
                }
            }
        }
        return;
    }

    // ケース2: リネームまたはファイル上書き (mv src dest) (ここに来た時点で、dest はディレクトリではない)
    // ソースが複数あるのに dest がディレクトリでない場合はエラー
    if (args.size() != 3) {
        game.outputString.push_back("mv: target '" + dest_path_str + "' is not a directory");
        return;
    }

    // ソース取得
    auto src_nodes = game.resolvePaths(args[1]);
    if (src_nodes.empty()) {
        game.outputString.push_back("mv: cannot stat '" + args[1] + "': No such file or directory");
        return;
    }
    if (src_nodes.size() > 1) {
        game.outputString.push_back("mv: cannot rename multiple files to '" + dest_path_str + "'");
        return;
    }
    FileSystemNode* src_node = src_nodes.front();

    // 権限チェック
    if (!src_node || !src_node->parent) return;
    if (!src_node->checkPerm(game.isSuperUser(), PERM_WRITE) || !src_node->parent->checkPerm(game.isSuperUser(), PERM_WRITE)) {
        game.outputString.push_back("mv: cannot move '" + src_node->name + "': Permission denied");
        return;
    }

    // dest が存在する場合 (ファイル上書き)
    if (!dest_nodes.empty()) {
        FileSystemNode* dest_node = dest_nodes.front();
        if (dynamic_cast<Directory*>(dest_node)) {
             game.outputString.push_back("mv: cannot overwrite directory '" + dest_path_str + "' with non-directory");
             return;
        }
        if (!dest_node->checkPerm(game.isSuperUser(), PERM_WRITE)) {
             game.outputString.push_back("mv: cannot overwrite '" + dest_path_str + "': Permission denied");
             return;
        }
        
        // dest_node をゴミ箱に移動 (rm のロジック)
        Directory* trash_dir = game.getTrashDirectory();
        if (!trash_dir || !dest_node->parent) {
            game.outputString.push_back("mv: internal error (cannot find trash)");
            return;
        }
        std::unique_ptr<FileSystemNode> ptr = dest_node->parent->releaseChild(dest_node->name);
        if (ptr) {
            ptr->parent = trash_dir;
            trash_dir->addChild(std::move(ptr));
        }
    }

    // dest が存在しない (リネームまたは移動＋リネーム)
    // dest の親ディレクトリと、新しいファイル名を決定
    std::string dest_parent_path = ".";
    std::string dest_filename = dest_path_str;
    
    size_t last_slash = dest_path_str.find_last_of('/');
    if (last_slash != std::string::npos) {
        dest_parent_path = dest_path_str.substr(0, last_slash);
        if (dest_parent_path.empty()) dest_parent_path = "/"; // ルートの場合
        dest_filename = dest_path_str.substr(last_slash + 1);
    }
    // dest の親ディレクトリを解決
    auto dest_parent_nodes = game.resolvePaths(dest_parent_path);
    if (dest_parent_nodes.size() != 1 || !dynamic_cast<Directory*>(dest_parent_nodes[0])) {
        game.outputString.push_back("mv: cannot move to '" + dest_path_str + "': No such file or directory");
        return;
    }
    Directory* dest_parent_dir = dynamic_cast<Directory*>(dest_parent_nodes[0]);
    
    if (!dest_parent_dir->checkPerm(game.isSuperUser(), PERM_WRITE)) {
        game.outputString.push_back("mv: cannot move to '" + dest_path_str + "': Permission denied");
        return;
    }

    // 新しい名前が不正でないかチェック
    if (dest_filename.empty() || dest_filename == "." || dest_filename == "..") {
         game.outputString.push_back("mv: invalid destination name '" + dest_path_str + "'");
         return;
    }

    // 実行:
    if (src_node->parent == dest_parent_dir) {
        // 親が同じ (単純なリネーム)
        src_node->name = dest_filename;
    } else {
        // 親が違う (移動 + リネーム)
        std::unique_ptr<FileSystemNode> ptr = src_node->parent->releaseChild(src_node->name);
        if (ptr) {
            ptr->name = dest_filename; // 名前を変更
            ptr->parent = dest_parent_dir; // 親を変更
            dest_parent_dir->addChild(std::move(ptr));
        }
    }
}

void CommandProcessor::cmd_ps(const std::vector<std::string>& args) {
    if (args.size() != 1) {
        game.outputString.push_back("ps: missing command");
        return;
    }
    for(auto const& [pid, process] : game.processList){
        if (process) {
            game.outputString.push_back("id=" + std::to_string(process->id) + " : name=" + process->name);
        }
    }
}
void CommandProcessor::cmd_kill(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        game.outputString.push_back("kill: missing command");
        return;
    }
    
    for(int i=1; i<args.size(); i++){
        try {
            int id = stoi(args.at(i));
            if(game.processList.count(id)){
                game.processList.erase(id);
            }else{
                game.outputString.push_back("no process : id = " + id);
            }
        } catch (const std::invalid_argument& e) {
            game.outputString.push_back("kill: argument must be a process ID");
            continue; // 次の引数へ
        } catch (const std::out_of_range& e) {
            game.outputString.push_back("kill: process ID out of range");
            continue; // 次の引数へ
        }
        
    }
}
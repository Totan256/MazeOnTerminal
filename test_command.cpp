#include "test_command.hpp"
#include <string>
#include <algorithm>

//==============================================================================
// FileSystemNodeクラスの実装
//==============================================================================
std::string FileSystemNode::getPermissionsString() const {
    std::string p;
    p += (permissions & PERM_READ) ? 'r' : '-';
    p += (permissions & PERM_WRITE) ? 'w' : '-';
    p += (permissions & PERM_EXECUTE) ? 'x' : '-';
    return p;
}

//==============================================================================
// Directoryクラスの実装
//==============================================================================
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

void Directory::listContents(bool show_details, bool show_all) const {
    for (const auto& child : children) {
        if(!show_all && child->name.front() == '.'){
            continue;
        }
        if (show_details) {
            std::cout << child->getPermissionsString() << " "
                      << child->size << "KB "
                      << child->name << std::endl;
        } else {
            std::cout << child->name << " ";
        }
    }
    if(!show_details) std::cout << std::endl;
}


//==============================================================================
// Gameクラスの実装
//==============================================================================
Game::Game() {
    // --- ① 起点：「ルートディレクトリ」の準備 ---
    root = std::make_unique<Directory>("/", nullptr, PERM_READ | PERM_WRITE | PERM_EXECUTE, FileSystemNode::Owner::ROOT);
    current_directory = root.get();

    // --- ② 初期設定：ファイルとディレクトリの配置 ---
    auto floor1 = std::make_unique<Directory>("floor1", root.get(), PERM_READ, FileSystemNode::Owner::PLAYER);
    // trashの用意　メンバ変数にしてアクセスできるように
    auto trash = std::make_unique<Directory>(".trash", root.get(), PERM_READ | PERM_WRITE | PERM_EXECUTE, FileSystemNode::Owner::ROOT);
    this->trash_directory_ = trash.get();
    root->addChild(std::move(trash));
    //ログファイル
    auto log = std::make_unique<File>("log.txt", root.get(), "", 0, 4, FileSystemNode::Owner::ROOT); // 権限なし
    this->logText = log.get();
    root->addChild(std::move(log));
    
    // ファイルを作成してfloor1に追加
    auto readme = std::make_unique<File>("readme.txt", floor1.get(),
        "Escape from this maze.\nUse 'ls -l' to see details.\nUse 'chmod +x' to run commands.",
        PERM_READ | PERM_WRITE, 8, FileSystemNode::Owner::PLAYER);
    
    auto secret = std::make_unique<File>("secret.txt", floor1.get(),
        "The password is '1234'.", 0, 4, FileSystemNode::Owner::PLAYER); // 権限なし

    auto run_cmd = std::make_unique<Executable>("run.sh", floor1.get(), PERM_READ, 16, FileSystemNode::Owner::PLAYER); // 実行権限なし

    floor1->addChild(std::move(readme));
    floor1->addChild(std::move(secret));
    floor1->addChild(std::move(run_cmd));

    root->addChild(std::move(floor1));
    
    // ゲーム開始時の現在地を設定
    current_directory = static_cast<Directory*>(root->findChildren("floor1").front());
}

void Game::run() {
    CommandProcessor processor(*this);
    std::string line;
    std::cout << "Welcome to Command Maze. Type 'exit' to quit." << std::endl;

    while (true) {
        std::cout << current_directory->name << "> ";
        if (!std::getline(std::cin, line)) break;
        if (line == "exit") break;

        processor.execute(line);
    }
}

void Game::resolvePathsRecursive(const std::vector<std::string>& parts, size_t index, std::vector<FileSystemNode*>& current_nodes, std::vector<FileSystemNode*>& results) {
    if (index == parts.size()) {
        results.insert(results.end(), current_nodes.begin(), current_nodes.end());
        return;
    }
    std::vector<FileSystemNode*> next_nodes;
    const std::string& part = parts[index];
    for (FileSystemNode* node : current_nodes) {
        if (part == ".") { next_nodes.push_back(node); continue; }
        if (part == "..") { if (node->parent) next_nodes.push_back(node->parent); continue; }
        Directory* dir = dynamic_cast<Directory*>(node);
        if (dir && dir->checkPerm(is_superuser, PERM_EXECUTE)) {
            std::vector<FileSystemNode*> found = dir->findChildren(part);
            next_nodes.insert(next_nodes.end(), found.begin(), found.end());
        }
    }
    if (!next_nodes.empty()) {
        resolvePathsRecursive(parts, index + 1, next_nodes, results);
    }
}
std::vector<FileSystemNode*> Game::resolvePaths(const std::string& path) {
    Directory* start_dir = (path.front() == '/') ? root.get() : current_directory;
    if (path == "/" || path == ".") return {start_dir};
    if (path == "..") return {start_dir->parent ? start_dir->parent : start_dir};

    std::vector<std::string> parts;
    std::stringstream ss(path);
    std::string part;
    if (path.front() == '/') std::getline(ss, part, '/');
    while (std::getline(ss, part, '/')) if (!part.empty()) parts.push_back(part);

    std::vector<FileSystemNode*> start_nodes = { start_dir };
    std::vector<FileSystemNode*> results;
    resolvePathsRecursive(parts, 0, start_nodes, results);
    return results;
}
//==============================================================================
// CommandProcessorクラスの実装
//==============================================================================
CommandProcessor::CommandProcessor(Game& game) : game(game) {
    // コマンド名と、実行するラムダ式を結びつける
    commands["ls"] = [this](const auto& args){ this->cmd_ls(args); };
    commands["cd"] = [this](const auto& args){ this->cmd_cd(args); };
    commands["cat"] = [this](const auto& args){ this->cmd_cat(args); };
    commands["pwd"] = [this](const auto& args){ this->cmd_pwd(args); };
    commands["chmod"] = [this](const auto& args){ this->cmd_chmod(args); };
    commands["sudo"] = [this](const auto& args){ this->cmd_sudo(args); };
    commands["rm"] = [this](const auto& args){ this->cmd_rm(args); };
    commands["find"] = [this](const auto& args){ this->cmd_find(args); };
    commands["mv"] = [this](const auto& args){ this->cmd_mv(args); };
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

void CommandProcessor::execute(const std::string& input_line) {
    game.outputStrings = {0};
    std::vector<std::string> args;
    {
        std::stringstream ss(input_line);
        std::string arg;
        while(ss >> arg) {
            args.push_back(arg);
        }
    }

    if(this->executeInternal(args)){
        game.logText->appendText(input_line + "\n");

        //実行後の容量処理
        if (game.getCurrentDiskSize() > game.getMaxDiskSize()) { // game.getCurrentDiskSize()は再帰計算を行う
            std::cout << "DISK FULL. FILESYSTEM CORRUPTED. SHUTTING DOWN..." << std::endl;
            exit(1);
        }
    }    
}
bool CommandProcessor::executeInternal(const std::vector<std::string>& args){
    std::string command = args.front();
    //最初のコマンド名自体にワイルドカードが含まれていたら実行しない
    if (command.find('*') != std::string::npos) {
        std::cout << "Command not found: " << command << std::endl;
        return false;
    }
    auto nodes = game.resolvePaths(command);

    if (nodes.empty()) {
        std::cout << "Command not found: " << command << std::endl;
        return false;
    }
    // TODO: ワイルドカード等で複数見つかった場合の処理。今は最初の一つだけを対象とする。
    FileSystemNode* command_node = nodes.front();
    //確認
    Executable* exec_file = dynamic_cast<Executable*>(command_node);
    if (!exec_file) {//実行ファイルかどうか確認
        std::cout << command << ": is not an executable file." << std::endl;
        return false;
    }
    if (!exec_file->checkPerm(game.isSuperUser(), PERM_EXECUTE)) {//権限チェック
        std::cout << "Permission denied: " << command << std::endl;
        return false;
    }    

    //実行
    if (commands.count(command)) {
        commands[command](args); // Mapから関数を呼び出す
        //ログテキストのサイズ増加
    } else {// ゲーム内にファイルはあるが、C++側に実装がない場合
        std::cout << "Execution failed: internal error for command '" << command << "'" << std::endl;
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
                    std::cout << "ls: cannot open directory '" << node->name << "': Permission denied" << std::endl;
                    continue;
                }
                target_dir->listContents(long_format, all_files);
            } else if (node) { // ディレクトリではないが、ファイルとして存在する場合
                if(!node->checkPerm(game.isSuperUser(), PERM_READ)){
                     std::cout << "ls: cannot access '" << node->name << "': Permission denied" << std::endl;
                     continue;
                }
                std::cout << node->name << std::endl;
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
        std::cout<<"cd : cant "<<std::endl;
    }
    FileSystemNode* target_node = nodes.front();
    
    if (!target_node) {
        std::cout << "cd: No such file or directory: " << path << std::endl;
        return;
    }
    
    Directory* target_dir = dynamic_cast<Directory*>(target_node);
    if(!target_dir){
        std::cout<<"cd : cant cd such file"<<std::endl;
        return;
    }
    if(!target_dir->checkPerm(game.isSuperUser(), PERM_EXECUTE)){//権限チェック
        return;
    }
    if (target_dir) {
        // 権限チェックはここに集約できる
        if (path == ".." && !(game.getCurrentDirectory()->permissions & PERM_EXECUTE)) {
            std::cout << "cd: ..: Permission denied." << std::endl;
            return;
        }
        game.setCurrentDirectory(target_dir);
    } else {
        std::cout << "cd: Not a directory: " << path << std::endl;
    }
}

void CommandProcessor::cmd_cat(const std::vector<std::string>& args) {
    if (args.size() < 2) { std::cout << "cat: missing operand" << std::endl; return; }
    const std::string& filename = args[1];
    auto nodes = game.resolvePaths(args[1]);

    for(FileSystemNode* node : nodes){
        File* file = dynamic_cast<File*>(node);
        if(!file){
            std::cout<<"cat : cant cd such file"<<std::endl;
            return;
        }
        if(!file->checkPerm(game.isSuperUser(), PERM_READ)){//権限チェック
            return;
        }
        if(file) {
            std::cout << file->content << std::endl;
        } else {
            std::cout << "cat: " << filename << " is not a regular file." << std::endl;
        }
    }    
}

void CommandProcessor::cmd_pwd(const std::vector<std::string>& args) {
    if(args.size()!=1) return;
    // とりあえず現在のディレクトリ名だけ表示
    //std::cout << "/" << game.getCurrentDirectory()->name << std::endl;
    FileSystemNode *node = game.getCurrentDirectory();
    std::string s="";
    s=node->name+s;
    node=node->parent;
    while (node!=nullptr){
        s+="/";
        s=node->name+s;
        node=node->parent;
    }
    std::cout <<s<< std::endl;
}

void CommandProcessor::cmd_chmod(const std::vector<std::string>& args) {
    if (args.size() != 3) { std::cout << "chmod: missing operand" << std::endl; return; }
    const std::string& mode = args[1];
    const std::string& filename = args[2];
    auto nodes = game.resolvePaths(args[2]);

    for(FileSystemNode* node :nodes){
        //if (!node) { std::cout << "chmod: No such file: " << filename << std::endl; return; }
        
        if(!node->checkPerm(game.isSuperUser(), PERM_WRITE)){//権限チェック
            return;
        }
        if (mode == "+x") {
            node->permissions |= PERM_EXECUTE;
            std::cout << "Set execute permission on " << filename << std::endl;
        } else if (mode == "+r") {
            node->permissions |= PERM_READ;
            std::cout << "Set read permission on " << filename << std::endl;
        } // 他の権限も同様に追加可能
        else {
            std::cout << "chmod: Invalid mode: " << mode << std::endl;
        }
    }
}

// CommandProcessorクラスにパスワードを保持するメンバ変数を追加
// std::string sudo_password;

void CommandProcessor::cmd_sudo(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cout << "sudo: missing command" << std::endl;
        return;
    }

    std::cout << "[sudo] password for player: ";
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
        std::cout << "sudo: incorrect password" << std::endl;
    }
}

void CommandProcessor::cmd_rm(const std::vector<std::string>& args){
    if (args.size() < 2) {
        std::cout << "rm: missing command" << std::endl;
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

void CommandProcessor::cmd_find(const std::vector<std::string>& args){
    if (args.size() < 2) {
        std::cout << "find: missing command" << std::endl;
        return;
    }
}

void CommandProcessor::cmd_mv(const std::vector<std::string>& args) {
    // 引数が3未満（mv, src, dest）の場合はエラー
    if (args.size() < 3) {
        std::cout << "mv: missing destination file operand" << std::endl;
        return;
    }

    // 最後の引数を移動先パスとして取得
    std::string dest_path = args.back();
    auto dest_nodes = game.resolvePaths(dest_path);

    // 移動先が存在しない、または複数見つかった場合はエラー
    if (dest_nodes.size() != 1) {
        std::cout << "mv: target '" << dest_path << "' is not a directory or does not exist." << std::endl;
        return;
    }
    // 移動先がディレクトリでない場合はエラー
    Directory* dest_dir = dynamic_cast<Directory*>(dest_nodes.front());
    if (!dest_dir) {
        std::cout << "mv: target '" << dest_path << "' is not a directory" << std::endl;
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

//==============================================================================
// ## main関数
//==============================================================================
int main() {
    Game game;
    game.run();
    return 0;
}
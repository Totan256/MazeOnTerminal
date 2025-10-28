#include "shellGame.hpp"
#include "commandProcessor.hpp"


Game::Game() {
    //エイリアスの設定
    aliasesList.emplace("cd", "cd_locked");
    aliasesList.emplace("sudo", "sudo_locked");
    
    // --- ① 起点：「ルートディレクトリ」の準備 ---
    this->root = std::make_unique<Directory>("/", nullptr, PERM_READ | PERM_WRITE | PERM_EXECUTE, FileSystemNode::Owner::ROOT);
    
    //初期プロセスの追加
    int mazeMasterID = 0;
    int traderID = 1;
    processList.emplace(mazeMasterID, std::make_unique<MazeMaster>(*this, mazeMasterID));
    processList.emplace(traderID, std::make_unique<Trader>(*this, traderID));


    // --- ② 初期設定：ファイルとディレクトリの配置 ---
    auto floor1 = std::make_unique<Directory>("floor1", this->root.get(), PERM_READ, FileSystemNode::Owner::PLAYER);
    
    {// trashの用意　メンバ変数にしてアクセスできるように
        auto trash = std::make_unique<Directory>(".trash", this->root.get(), PERM_READ | PERM_WRITE | PERM_EXECUTE, FileSystemNode::Owner::ROOT);
        this->trash_directory_ = trash.get();
        this->root->addChild(std::move(trash));
        // trashに妨害ノード追加
        auto trap = std::make_unique<TrapNode>(".ls_trap", trash.get(), 50, FileSystemNode::Owner::ROOT);
        this->trash_directory_->addChild(std::move(trap));
    }
    {//ログファイル
        auto log = std::make_unique<File>("log.txt", this->root.get(), "", 0, 4, FileSystemNode::Owner::ROOT); // 権限なし
        this->logText = log.get();
        this->root->addChild(std::move(log));
    }
    {//初期から所持しているコマンド
        auto ls = std::make_unique<Executable>("ls", this->root.get(), PERM_EXECUTE,4 , FileSystemNode::Owner::PLAYER);
        this->root->addChild(std::move(ls));
    }{
        auto cat = std::make_unique<Executable>("cat", this->root.get(), PERM_EXECUTE,4 , FileSystemNode::Owner::PLAYER);
        this->root->addChild(std::move(cat));
    }
    
    // ファイルを作成してfloor1に追加
    auto readme = std::make_unique<File>("readme.txt", this->root.get(),
        "Escape from this maze.\nUse 'ls -l' to see details.\nUse 'chmod +x' to run commands.",
        PERM_READ | PERM_WRITE, 8, FileSystemNode::Owner::PLAYER);
    
    auto secret = std::make_unique<File>("secret.txt", this->root.get(),
        "The password is '1234'.", 0, 4, FileSystemNode::Owner::PLAYER); // 権限なし

    auto run_cmd = std::make_unique<Executable>("run.sh", this->root.get(), PERM_READ, 16, FileSystemNode::Owner::PLAYER); // 実行権限なし

    this->root->addChild(std::move(readme));
    this->root->addChild(std::move(secret));
    this->root->addChild(std::move(run_cmd));

    this->root->addChild(std::move(floor1));
    
    // ゲーム開始時の現在地を設定
    current_directory = this->root.get();
}

void Game::run() {
    CommandProcessor processor(*this);
    std::string line;
    std::cout << "Welcome to Command Maze. Type 'exit' to quit." << std::endl;

    while (true) {
        std::cout << current_directory->name << "> ";
        if (!std::getline(std::cin, line)) break;
        if (line == "exit") break;

        processor.execute(line, "user");

        for(auto const& [pid, process] : processList){
            if (process) {
                process->update();
            }
        }
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
    return resolvePaths(path, current_directory);
}

std::vector<FileSystemNode*> Game::resolvePaths(const std::string& path, Directory* baseDir){
    Directory* start_dir = (path.front() == '/') ? root.get() : baseDir;
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

std::vector<FileSystemNode*> Game::resolvePathsWithSymbolic(const std::string& path){
    std::vector<FileSystemNode*> final_results;
    std::vector<SymbolicLink*> link_work_queue;

    const int SYMLOOP_MAX = 8; // ループ上限
    int loop_count = 0;
    std::vector<FileSystemNode*> first_nodes = resolvePaths(path, current_directory);

    for (FileSystemNode* node : first_nodes) {
        SymbolicLink* symlink = dynamic_cast<SymbolicLink*>(node);
        if (symlink) {
            link_work_queue.push_back(symlink);
        } else {
            final_results.push_back(node);
        }
    }

    while (!link_work_queue.empty() && loop_count < SYMLOOP_MAX) {
        loop_count++;
        SymbolicLink* link_to_process = link_work_queue.back();
        link_work_queue.pop_back();

        std::vector<FileSystemNode*> resolved_nodes = resolvePaths(link_to_process->target_path, link_to_process->parent);
        for (FileSystemNode* node : resolved_nodes) {
            SymbolicLink* symlink = dynamic_cast<SymbolicLink*>(node);
            if (symlink) {
                link_work_queue.push_back(symlink);
            } else {
                final_results.push_back(node);
            }
        }
    }

    if (!link_work_queue.empty()) {
        std::cout << "Too many levels of symbolic links: " << path << std::endl;
        final_results.clear();
    }

    return final_results;
}
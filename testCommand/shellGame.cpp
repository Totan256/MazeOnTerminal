#include "shellGame.hpp"
#include "commandProcessor.hpp"


ShellGame::ShellGame() {
    this->executionHistory.clear();
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
    auto floor1 = root->buildDirectory("floor1").withPermissions(7).build();
    floor1->buildFile("blue_wall.data").isLarge(true).build();
    floor1->buildFile("green_wall.data").isLarge(true).build();
    floor1->buildFile("yellow_wall.data").isLarge(true).build();
    floor1->buildTrapNode("floor_trap")
                .withOwner(FileSystemNode::Owner::ROOT)
                .withSize(50)
                .build();
    
    
    {// trashの用意　メンバ変数にしてアクセスできるように
        this->trash_directory_ = root->buildDirectory(".trash").
                withOwner(FileSystemNode::Owner::ROOT)
                .build();
        this->trash_directory_->buildTrapNode(".trash_trap")
                .withOwner(FileSystemNode::Owner::ROOT)
                .withSize(50)
                .build();
    }
    //ログファイル
    this->logText = root->buildFile("log.txt")
                        .withPermissions(PERM_NONE)
                        .withOwner(FileSystemNode::Owner::ROOT)
                        .build();
    
    {//初期から所持しているコマンド
        root->buildExecutable("ls", "ls").withPermissions(PERM_EXECUTE).build();
        root->buildExecutable("cat", "cat").withPermissions(PERM_EXECUTE).build();
        root->buildExecutable("rm", "rm").withPermissions(PERM_EXECUTE).build();
        root->buildExecutable("mv", "mv").withPermissions(PERM_EXECUTE).build();
        root->buildExecutable("find", "find").withPermissions(PERM_EXECUTE).build();
        root->buildExecutable("pwd", "pwd").withPermissions(PERM_EXECUTE).build();
        root->buildExecutable("chmod", "chmod").withPermissions(PERM_EXECUTE).build();
    }

    root->buildFile("readme.txt")
            .withContent(std::vector<std::string>{"Escape from this maze.", "Use 'ls -l' to see details.", "Use 'chmod +x' to run commands."})
            .withSize(8)
            .build();

    root->buildFile("secret.txt")
            .withContent("The password is '1234'.")
            .withPermissions(PERM_NONE)
            .build();

    // ゲーム開始時の現在地を設定
    current_directory = this->root.get();
}

std::vector<std::string>& ShellGame::update(const std::string& input) {

    if(this->currentState == ShellState::PROMPT){
        this->executionHistory.push_back("> "+input);
        CommandProcessor processor(*this);
        processor.execute(input, "user");
    }else 
    if(this->currentState == ShellState::WAITING_PASSWORD){
        if(input==this->sudo_password){
            CommandProcessor processor(*this);
            this->setSuperUser(true);
            processor.execute(this->sudoPendingCommand, "user");
            this->setSuperUser(false);
        }
        this->currentState = ShellState::PROMPT;
    }
    for(auto const& [pid, process] : processList){
        if (process) {
            process->update();
        }
    }
    for(auto s : outputString){
        this->executionHistory.push_back(s);
    }
    
    return this->executionHistory;
}

void ShellGame::resolvePathsRecursive(const std::vector<std::string>& parts, size_t index, std::vector<FileSystemNode*>& current_nodes, std::vector<FileSystemNode*>& results) {
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

std::vector<FileSystemNode*> ShellGame::resolvePaths(const std::string& path) {
    return resolvePaths(path, current_directory);
}

std::vector<FileSystemNode*> ShellGame::resolvePaths(const std::string& path, Directory* baseDir){
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

std::vector<FileSystemNode*> ShellGame::resolvePathsWithSymbolic(const std::string& path){
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

void ShellGame::setSudoCommand(std::vector<std::string>& command){
    sudoPendingCommand = "";
    for(auto s : command){
        sudoPendingCommand += s + " ";
    }
}

void ShellGame::executeSudoCommand(const std::string& userName){
    CommandProcessor p(*this);
    p.execute(sudoPendingCommand, userName);
}
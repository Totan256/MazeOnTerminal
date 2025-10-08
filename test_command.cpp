#include "test_command.hpp"

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
Directory::Directory(const std::string& name, Directory* parent)
    : FileSystemNode(name, parent, PERM_READ | PERM_WRITE | PERM_EXECUTE, 4) {} // ディレクトリは常にrwx

void Directory::addChild(std::unique_ptr<FileSystemNode> child) {
    children.push_back(std::move(child));
}

FileSystemNode* Directory::findChild(const std::string& childName) {
    for (const auto& child : children) {
        if (child->name == childName) {
            return child.get();
        }
    }
    return nullptr;
}

void Directory::listContents(bool show_details) const {
    for (const auto& child : children) {
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
    root = std::make_unique<Directory>("/", nullptr);
    current_directory = root.get();

    // --- ② 初期設定：ファイルとディレクトリの配置 ---
    auto floor1 = std::make_unique<Directory>("floor1", root.get());
    
    // ファイルを作成してfloor1に追加
    auto readme = std::make_unique<File>("readme.txt", floor1.get(),
        "Escape from this maze.\nUse 'ls -l' to see details.\nUse 'chmod +x' to run commands.",
        PERM_READ | PERM_WRITE, 8);
    
    auto secret = std::make_unique<File>("secret.txt", floor1.get(),
        "The password is '1234'.",
        0, 4); // 権限なし

    auto run_cmd = std::make_unique<Executable>("run.sh", floor1.get(),
        PERM_READ, 16); // 実行権限なし

    floor1->addChild(std::move(readme));
    floor1->addChild(std::move(secret));
    floor1->addChild(std::move(run_cmd));

    root->addChild(std::move(floor1));
    
    // ゲーム開始時の現在地を設定
    current_directory = static_cast<Directory*>(root->findChild("floor1"));
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
}

void CommandProcessor::execute(const std::string& input_line) {
    std::stringstream ss(input_line);
    std::string command;
    ss >> command;

    std::vector<std::string> args;
    args.push_back(command);
    std::string arg;
    while(ss >> arg) {
        args.push_back(arg);
    }
    
    if (commands.count(command)) {
        commands[command](args); // Mapから関数を呼び出す
    } else if(!command.empty()) {
        std::cout << "Command not found: " << command << std::endl;
    }
}

// --- 各コマンドの処理 ---
void CommandProcessor::cmd_ls(const std::vector<std::string>& args) {
    bool long_format = (args.size() > 1 && args[1] == "-l");
    game.getCurrentDirectory()->listContents(long_format);
}

void CommandProcessor::cmd_cd(const std::vector<std::string>& args) {
    if (args.size() < 2) return;
    const std::string& dirname = args[1];
    Directory* current = game.getCurrentDirectory();

    if (dirname == "..") {
        if (current->parent) {
            game.setCurrentDirectory(current->parent);
        }
    } else {
        FileSystemNode* target = current->findChild(dirname);
        if (target && dynamic_cast<Directory*>(target)) {
             game.setCurrentDirectory(static_cast<Directory*>(target));
        } else {
            std::cout << "cd: No such directory: " << dirname << std::endl;
        }
    }
}

void CommandProcessor::cmd_cat(const std::vector<std::string>& args) {
    if (args.size() < 2) { std::cout << "cat: missing operand" << std::endl; return; }
    const std::string& filename = args[1];
    FileSystemNode* node = game.getCurrentDirectory()->findChild(filename);

    if (!node) { std::cout << "cat: No such file: " << filename << std::endl; return; }
    if (!(node->permissions & PERM_READ)) { std::cout << "cat: Permission denied" << std::endl; return; }
    
    File* file = dynamic_cast<File*>(node);
    if(file) {
        std::cout << file->content << std::endl;
    } else {
        std::cout << "cat: " << filename << " is not a regular file." << std::endl;
    }
}

void CommandProcessor::cmd_pwd(const std::vector<std::string>& args) {
    // とりあえず現在のディレクトリ名だけ表示
    std::cout << "/" << game.getCurrentDirectory()->name << std::endl;
}

void CommandProcessor::cmd_chmod(const std::vector<std::string>& args) {
    if (args.size() < 3) { std::cout << "chmod: missing operand" << std::endl; return; }
    const std::string& mode = args[1];
    const std::string& filename = args[2];
    FileSystemNode* node = game.getCurrentDirectory()->findChild(filename);

    if (!node) { std::cout << "chmod: No such file: " << filename << std::endl; return; }

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


//==============================================================================
// ## main関数
//==============================================================================
int main() {
    Game game;
    game.run();
    return 0;
}
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// 各コマンドの処理を行う関数のプロトタイプ宣言
void do_ls(int argc, char** argv);
void do_exit(int* is_running);

int main() {
    char line_buffer[256];
    int is_running = 1;

    printf("Command Maze Prototype. Type 'exit' to quit.\n");

    while (is_running) {
        // --- ① 入力受付 ---
        printf("maze> ");
        if (fgets(line_buffer, sizeof(line_buffer), stdin) == NULL) {
            break; // エラーまたはEOF
        }
        line_buffer[strcspn(line_buffer, "\n")] = '\0';

        // --- ② 文字列の解析 ---
        char* args[16];
        int arg_count = 0;
        char* token = strtok(line_buffer, " \t");
        while (token != NULL && arg_count < 15) {
            args[arg_count++] = token;
            token = strtok(NULL, " \t");
        }
        args[arg_count] = NULL;

        // --- ③ コマンドの実行 ---
        if (arg_count > 0) {
            if (strcmp(args[0], "ls") == 0) {
                do_ls(arg_count, args);
            } else if (strcmp(args[0], "exit") == 0) {
                do_exit(&is_running);
            } else {
                printf("Command not found: %s\n", args[0]);
            }
        }
    }

    printf("Exiting program.\n");
    return 0;
}

// --- コマンドごとの処理関数の実装例 ---

void do_ls(int argc, char** argv) {
    // argv[1], argv[2]... を見て、オプションを処理する
    if (argc > 1 && strcmp(argv[1], "-l") == 0) {
        printf("Listing files in long format...\n");
        printf("-rwxr-xr-x 1 player 8192 Oct 08 16:40 readme.txt\n");
    } else {
        printf("Listing files...\n");
        printf("readme.txt\n");
    }
}

void do_exit(int* is_running) {
    printf("Shutting down...\n");
    *is_running = 0;
}
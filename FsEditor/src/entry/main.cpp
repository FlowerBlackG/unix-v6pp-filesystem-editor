/*
 * 文件系统读写器 - 进入点。
 * 2051565 龚天遥
 * 创建于 2022年7月29日。
 */

#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>
#include "../include/MacroDefines.h"
#include "../include/structures/Inode.h"
#include "../include/FileSystemAdapter.h"

using namespace std;
using namespace std::filesystem;

void usage(const char* hint = nullptr) {
    if (hint != nullptr) {
        cout << hint << endl;
        cout << endl;
    }
    
    cout << "Unix V6++ 文件系统读写器" << endl;
    cout << "    by 2051565 GTY" << endl;
    cout << endl;
    cout << "usage: fsedit.exe imgFile option [imgsize]" << endl;
    cout << "   之后，使用标准输入传递操作指令。" << endl;
    cout << endl;
    cout << "options:" << endl;
    cout << "  c: 创建一个磁盘映像文件。大小默认为0。" << endl;
    cout << "  m: 格式化img文件。" << endl;
    cout << "  e: 打开文件系统，并对其进行编辑操作。" << endl;
    cout << "     注意，使用损坏的img文件会造成未定义的行为。" << endl;
    cout << endl;
    cout << "operations:" << endl;
    cout << "> h 或其他未定义操作: 显示帮助" << endl;
    cout << "> f: 格式化磁盘。" << endl;
    cout << "> l: 相当于 ls -l" << endl;
    cout << "> c [target dir]: 相当于 cd [target dir]" << endl;
    cout << "> p [file path] [v6++ fs path]: 将文件写入v6++文件系统。" << endl;
    cout << "> g [v6++ fs path] [file path]: 从文件系统取出文件。" << endl;
    cout << "> r [path]: 相当于 rm -rf。" << endl;
    cout << "> m [dir name]: 相当于 mkdir。" << endl;
    cout << "> k [file path]: 写入内核文件。" << endl;
    cout << "> b [file path]: 写入 bootloader 文件。" << endl;
    cout << "> x: 退出（并存盘）。" << endl;
    cout << endl;
    cout << "all path should be surrounded by a pair of '|'" << endl;
    cout << "example: > b |C://Program Files/soft/soft.exe|" << endl;
}

static int prepareImgFile(
    const char* filePath,
    const char option, 
    const unsigned long long imgSize = 0
) {
    
    // 解析基础操作选项。
    if (option == 'c') { 
        // create

        // 创建镜像文件。
        if (!ofstream(filePath).is_open()) {
            cout << "error: failed to create img file." << endl;
            return -1;
        }

        // 拓展文件大小。
        if (imgSize > 0) {
            resize_file(filePath, imgSize);
        }
    }

    

    int result = 0;
    
    if (option == 'm' || option == 'c') { 
        // make 清空盘，重建系统
        // 用 adapter 打开文件。失败会抛异常。
        FileSystemAdapter fsa(filePath);
        fsa.format();
            
    } else if (option == 'e') { 
        // 读盘
        // 不做任何处理。
    } else {
        usage("未知命令。");
        result = -1;
    }

    return result;
}

/**
 * 读取一个字母。
 */
static char readLatinChar() {
    int res;
    while ((res = cin.get()) != EOF) {
        if ((res >= 'a' && res <= 'z') || (res >= 'A' && res <= 'Z')) {
            return res;
        }
    }

    cout << "[error] bad stream!" << endl;
    exit(-1);
}

/**
 * 交互式命令行界面。 
 */
static void runInteractiveCli(FileSystemAdapter& fsAdapter) {
    while (true) {
        int operation = readLatinChar();

        if (operation == 'h') {
            usage();
        } else if (operation == 'f') {
            // todo
        } else if (operation == 'l') {
            // todo
        } else if (operation == 'c') {
            // todo
        } else if (operation == 'p') {
            // todo
        } else if (operation == 'g') {
            // todo
        } else if (operation == 'r') {
            // todo
        } else if (operation == 'm') {
            // todo
        } else if (operation == 'k') {
            // todo
        } else if (operation == 'b') {
            // todo
        } else if (operation == 'x') {
            // todo
        } else {
            string msg = "未知选项：";
            msg += char(operation);
            msg += " (";
            msg += to_string(operation);
            msg += ")"; 
            usage(msg.c_str());
        }
        // 注：你知道为什么要用一堆 if else，而不是一个 switch 么...

    }
}

/**
 * 程序进入点。 
 */
int main(int argc, const char* argv[]) {
    int x = 2;
    if (argc < 3) {
        usage("too few arguments.");
        return -1;
    }

    const char* imgPath = argv[1];
    char option = argv[2][0];
    if (option >= 'A' && option <= 'Z') {
        option += 'a' - 'A';
    }

    unsigned long long imgSize = 0;
    if (argc >= 4) { // 读取用户希望的磁盘大小。
        try {
            imgSize = stoull(argv[3]);
        } catch (...) {
            cout << "warning: failed to convert imgSize from argument list." << endl;
        }
    }

    if (prepareImgFile(imgPath, option, imgSize) == 0) {
        FileSystemAdapter fsAdapter(imgPath);
        runInteractiveCli(fsAdapter);
        return 0;
    } else {
        return -1;
    }
}

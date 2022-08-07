/* source files' encoding: utf-8 */

/*
 * 文件系统读写器 - 进入点。
 * 2051565 龚天遥
 * 创建于 2022年7月29日。
 * 
 * 参考构建环境：
 *   Windows 11 x64
 *   gcc 10.3.0 (tdm64-1)
 *   语言版本：C++17
 * 
 * 参考运行环境：
 *   LC_ALL: en_US.GB18030
 *   code page: 936
 *   shell: powershell 7.2.5
 *   terminal: Windows Terminal
 * 
 * 感谢：
 *   邓蓉老师
 *   方钰老师
 *   沈坚老师
 * 
 * 2022年8月7日：基本完工，并完成在内核上的测试。
 */

#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <filesystem>
#include <cstring>
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
    cout << "  c: 创建一个磁盘映像文件。大小默认为默认文件大小。" << endl;
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
    cout << "路径使用 '|' 分隔。" << endl;
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
            cout << "[error 1] failed to create img file." << endl;
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

    cout << "[error 2] bad stream!" << endl;
    cout << "        main::readLatinChar" << endl;
    exit(-1);
}


/**
 * 读取一个路径参数。
 * 输入者需要将该路径以 || 包裹。
 * 如：|tongji/dr.exe|。
 * 实际返回时，不包含双竖线。
 * 同时，读取过程会过滤路径前后的空字符和引号。
 * 
 * @return string 
 */
static string readPath() {
    string res;
    int ch;

    /**
     * 读取状态。
     * 0: 未遇到第一个竖线。
     * 1: 遇到了竖线，还没遇到有意义字符。
     * 2: 正在读路径。
     */
    int readingStatus = 0;
    while ((ch = cin.get()) != EOF) {
        
        if (ch == '|') {
            if (readingStatus == 2) { // 读取结束。
                while (res.length() && (
                        strchr("\'\" ", res.back()) 
                        || res.back() > 126 
                        || res.back() < 33
                    )
                ) {
                    res.pop_back();
                }
                
                return res; 
            } else {
                readingStatus = 1; // 开始读取。
                continue;
            }
        }

        if (readingStatus == 1 && ch >= 33 && ch <= 126 && ch != '\'' && ch != '\"') {
            // ascii 可见范围为 33~126 (不含空格)。
            readingStatus = 2;
        }

        if (readingStatus == 2 && ch >= 32 && ch <= 126) {
            res += char(ch);
        }
    }

    cout << "[error 3] bad stream!" << endl;
    cout << "        main::readPath" << endl;
    exit(-1);
}

static void readPath(vector<string>& pathSegments) {
    string pathStr = readPath();

    int currBeginIdx = 0;
    if (strchr("\\/", pathStr[0])) {
        currBeginIdx++;
        pathSegments.clear();
    }

    for (int searchIdx = currBeginIdx; searchIdx < pathStr.length(); searchIdx++) {
        if (searchIdx == currBeginIdx && (pathStr[searchIdx] < 33 || pathStr[searchIdx] > 126)) {
            currBeginIdx++;
        } else if (strchr("\\/", pathStr[searchIdx])) { // 检测到分隔符。

            // 提取。
            string segStr = pathStr.substr(currBeginIdx, searchIdx - currBeginIdx);
            
            // 去除无意义字符。
            while (segStr.length() > 0 && (segStr.back() < 33 || segStr.back() > 126)) {
                segStr.pop_back();
            }

            if (segStr.length() > 0) {
                if (segStr == "..") {
                    if (pathSegments.size() > 0) {
                        pathSegments.pop_back();
                    }
                } else if (segStr == ".") {
                    // nothing to do..
                } else {
                    pathSegments.push_back(segStr);
                }
            }

            currBeginIdx = searchIdx + 1; // 更新位置。
        }
    }
}

/**
 * 交互式命令行界面。 
 */
static void runInteractiveCli(FileSystemAdapter& fsAdapter) {
    vector<string> pathSegments;

    while (true) {
        // 输出 path。
        cout << '[';
        for (int idx = 0; idx < pathSegments.size(); idx++) {
            if (idx > 0) {
                cout << "/";
            }

            cout << pathSegments[idx];
        }

        cout << "] > ";

        // 读取输入内容。
        int operation = readLatinChar();

        // 处理用户命令。

        if (operation == 'h') { // help

            usage();

        } else if (operation == 'f') { // format

            fsAdapter.format();

        } else if (operation == 'l') { // list

            fsAdapter.ls();

        } else if (operation == 'c') { // change dir

            string path = readPath();
            
            if (fsAdapter.cd(path)) {
                pathSegments.push_back(path);
                cout << "[info] 切换路径。" << endl;
            } else {
                // nothing to do..
                cout << "[info] 试图切换路径，但没有任何事发生。" << endl;
            }

        } else if (operation == 'p') { // put

            string path = readPath();
            fstream f(path, ios::in | ios::binary);
            if (!f.is_open()) {
                cout << "[error 4] 无法打开：" << path << endl;
            } else {
                string v6ppFileName = readPath();
                fsAdapter.uploadFile(v6ppFileName, f);
                cout << "[info 5] 上传成功：" << v6ppFileName << endl;
            }

        } else if (operation == 'g') { // get

            string v6ppPath = readPath();
            string localPath = readPath();
            fstream f(localPath, ios::out | ios::binary);
            if (!f.is_open()) {
                cout << "[error 6] 无法打开：" << localPath << endl;
            } else {
                fsAdapter.downloadFile(v6ppPath, f);
                cout << "[info 7] 下载成功：" << v6ppPath << " -> " << localPath << endl;
            }

        } else if (operation == 'r') { // remove

            string path = readPath();
            int count = fsAdapter.rm(path);
            cout << "[info 8] 删除文件（夹）数：" << count << endl;

        } else if (operation == 'm') { // make dir

            string path = readPath();
            fsAdapter.mkdir(path);
            cout << "[info 9] 创建文件夹：" << path << endl;

        } else if (operation == 'k') { // write kernel

            string path = readPath();
            fstream f(path, ios::in | ios::binary);
            if (!f.is_open()) {
                cout << "[error 10] 无法打开：" << path << endl;
            } else {
                fsAdapter.writeKernel(f);
                f.close();
                cout << "[info 11] 内核写入完毕。" << endl;
            }
        
        } else if (operation == 'b') { // write bootloader
        
            string path = readPath();
            fstream f(path, ios::in | ios::binary);
            if (!f.is_open()) {
                cout << "[error 12] 无法打开：" << path << endl;
            } else {
                fsAdapter.writeBootLoader(f);
                f.close();
                cout << "[info 13] 启动引导程序写入完毕。" << endl;
            }
        
        } else if (operation == 'x') { // exit
        
            fsAdapter.sync();
            cout << "bye!" << endl;
            break; // 结束。
        
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

    unsigned long long imgSize = MachineProps::diskSize();
    if (argc >= 4) { // 读取用户希望的磁盘大小。
        try {
            imgSize = stoull(argv[3]);
        } catch (...) {
            cout << "warning: failed to convert imgSize from argument list." << endl;
        }
    }

    if (prepareImgFile(imgPath, option, imgSize) == 0) {
        FileSystemAdapter fsAdapter(imgPath);
        fsAdapter.load(); // 从磁盘文件载入文件系统（的 superblock 和 inodes）。
        runInteractiveCli(fsAdapter); // 进入交互式命令行。
        return 0;
    } else {
        return -1;
    }
}

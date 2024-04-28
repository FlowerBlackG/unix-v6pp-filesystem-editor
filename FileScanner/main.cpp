/* source files' encoding: utf-8 */

/*
 * Unix V6++ 文件系统构建工具 - 控制器。
 * 负责扫描本地文件并控制格式化工具工作。
 * 
 * 2051565 龚天遥
 * 创建于 2022年8月6日。
 */

#include <iostream>
#include <filesystem>

using namespace std;
using namespace std::filesystem;

/**
 * 上传路径下的所有文件（递归处理文件夹）。
 */
void uploadFiles(const directory_entry& entry, bool mkdir) {
    if (entry.status().type() == file_type::directory) {
        if (mkdir) {
            // 创建文件夹。
            cout << "m |" << entry.path().filename() << "|" << endl;
            
            // 切换路径。
            cout << "c |" << entry.path().filename() << "|" << endl;
        }

        // 扫描文件夹。
        for (auto& it : directory_iterator(entry.path())) {
            uploadFiles(it, true);
        }

        if (mkdir) {
            // 回到上级路径。
            cout << "c |..|" << endl;
        }
        
    } else if (entry.status().type() == file_type::regular) {
        // 上传文件。
        cout << "p |" << absolute(entry.path()) << "| "; 
        cout << "|" << entry.path().filename() << "|" << endl;
    }
}

int main() {
    cout << "f" << endl; // 格式化。
    cout << "k |kernel.bin|" << endl; // 写入内核文件。
    cout << "b |boot.bin|" << endl; // 写入 bootloader。

    path root("programs");
    if (!exists(root)) {
        cout << "x" << endl;
        return -1; // 异常退出。
    }

    uploadFiles(directory_entry(root), false);

    cout << "x" << endl; // 退出。
    return 0;
}

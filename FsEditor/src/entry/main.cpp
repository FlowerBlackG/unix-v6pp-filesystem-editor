/*
 * 文件系统读写器 - 进入点。
 * 2051565 龚天遥
 * 创建于 2022年7月29日。
 */

#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>

using namespace std;
using namespace std::filesystem;

void usage(const char* hint = nullptr) {
    if (hint != nullptr) {
        cout << hint << endl;
        cout << endl;
    }
    
    cout << "Unix V6++ File System Editor" << endl;
    cout << "    by 2051565 GTY" << endl;
    cout << endl;
    cout << "usage: fsedit.exe imgFile option [imgsize]" << endl;
    cout << "   then, use standard input stream to operate." << endl;
    cout << endl;
    cout << "options:" << endl;
    cout << "  c: create img file. by default, imgsize is set to 0." << endl;
    cout << "  m: make filesystem on img file." << endl;
    cout << "  e: open filesystem on img file, and edit it." << endl;
    cout << "     notice, using damaged img file would cause undefined behaviour" << endl;
    cout << endl;
    cout << "operations:" << endl;
    cout << "> h (or other undefined operation): get help" << endl;
    cout << "> f: format entire file. previous data would be deleted." << endl;
    cout << "> l: list files in current directory." << endl;
    cout << "> c [target dir]: change directory to target." << endl;
    cout << "> p [file path]: put file." << endl;
    cout << "> r [path]: remove file or directory." << endl;
    cout << "> m [dir name]: make directory." << endl;
    cout << "> k [file path]: write kernel file." << endl;
    cout << "> b [file path]: write bootloader file." << endl;
}

int prepareImgFile(
    fstream& fileHandler, 
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

    // 打开文件。
    fileHandler.open(filePath, ios::in | ios::out);
    if (!fileHandler.is_open()) {
        cout << "error: failed to reopen file." << endl;
        return -1;
    }
    
    if (option == 'm' || option == 'c') { 
        // make 清空盘，重建系统

            
    } else if (option == 'e') { 
        // 读盘


    } else {
        usage("unknown option.");
        return -1;
    }
}

/**
 * 程序进入点。 
 */
int main(int argc, const char* argv[]) {
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

    // 对 img 文件的流控制对象。
    fstream imgFile;

    

    return 0;
}

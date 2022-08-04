/*
 * 目录 Inode 节点。
 * 2051565 龚天遥
 * 创建于 2022年8月1日。
 */

#include <stdexcept>
#include <cmath>
#include <iostream>
#include "../include/FileSystemAdapter.h"
#include "../include/MacroDefines.h"
#include "../include/MachineProps.h"
#include "../include/structures/Inode.h"
#include "../include/structures/InodeDirectory.h"

using namespace std;

InodeDirectory::InodeDirectory(const Inode& inode, FileSystemAdapter& adapter, bool ignoreFileTypeCheck) {
    if (!ignoreFileTypeCheck && inode.file_type != Inode::FileType::DIR) {
        throw runtime_error("尝试打开的路径类型不是路径。");
    }

    int dirFileSize = inode.d_size;
    length = dirFileSize / sizeof(DirectoryEntry);
    this->entries = new DirectoryEntry[
        (dirFileSize / 512 + !!(dirFileSize % 512)) * 512 / sizeof(DirectoryEntry)
    ];
    
    adapter.readFile((char*) this->entries, inode);

} // InodeDirectory::InodeDirectory

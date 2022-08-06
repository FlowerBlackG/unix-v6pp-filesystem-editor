/*
 * 目录 Inode 节点。
 * 2051565 龚天遥
 * 创建于 2022年8月1日。
 */

#pragma once

#include <cstdint>
#include "../MacroDefines.h"
#include "../FileSystemAdapter.h"
#include "./Inode.h"

/**
 * 目录项。对应的可能是文件，也可能是另一个目录（不过目录的本质不也是文件么...）。
 */
class DirectoryEntry {
public:
    static const int DIRSIZE = 28;

public:
    /** 对应文件的 inode 编号。 */
    uint32_t m_ino;

    /** 文件名。 */
    char m_name[DIRSIZE];
} __packed;

/**
 * 目录文件。
 */
class InodeDirectory {
public:
    ~InodeDirectory() {
        if (entries != nullptr) {
            delete[] entries;
        }
    }

    InodeDirectory(
        Inode& inode, 
        class FileSystemAdapter& f, 
        bool ignoreFileTypeCheck = false, 
        int extraEntriesToAlloc = 0
    );

    InodeDirectory(int nEntries);

public:
    int length = 0;
    DirectoryEntry* entries = nullptr;

} __packed;



#if 0
void __check_size() {
    sizeof(DirectoryEntry); // 32 字节。
}
#endif

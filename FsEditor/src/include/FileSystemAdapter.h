/*
 * 文件系统适配器 - 头文件。
 * 2051565 龚天遥
 * 创建于 2022年7月29日。
 */

#pragma once

#include <fstream>
#include <vector>
#include <string>
#include "./structures/SuperBlock.h"
#include "./structures/Inode.h"
#include "./structures/Block.h"
#include "./MacroDefines.h"
#include "./structures/InodeDirectory.h"

class FileSystemAdapter {
public:
    const int FS_FILE_SIZE_MAX = MachineProps::BLOCK_SIZE * (6 + 128 + 128 * 128);

public:
    /**
     * 构造函数。自动打开磁盘映像文件。
     * 
     * @param filePath 文件路径。需要保证该文件可以打开。
     * @exception runtime_error 文件打开失败。
     */
    FileSystemAdapter(const char* filePath);

    ~FileSystemAdapter();

    void format();

    /** 加载文件系统。 */
    void load();
    void flush();

    void writeKernel(std::fstream& kernelFile);
    void writeBootLoader(std::fstream& bootLoaderFile);

public:
    bool readBlock(Block& block, const int blockIdx);
    bool readBlocks(char* buffer, const int blockIdx, const int blockCount);
    bool writeBlock(const Block& block, const int blockIdx);
    bool writeBlocks(const char* buffer, const int blockIdx, const int blockCount);

    bool readFile(char* buffer, const Inode& inode);
    bool writeFile(char* buffer, Inode& inode, int filesize);

    /**
     * 获取一个空的盘块。该盘块会被从空盘块列表移除。
     * @return int 盘块号。-1表示获取失败。
     */
    int getFreeBlock();

    /**
     * 释放一个盘块。
     */
    void freeBlock(int idx);

    int getFreeInode();
    void freeInode(int idx, bool freeBlocks = false);

public:
    void ls(const InodeDirectory& dir);
    void ls(const Inode& inode);
    void ls(const std::vector<std::string>& pathSegments, bool fromRoot);

private:
    std::fstream fileStream;
    SuperBlock superBlock;
    Inode inodes[
        MachineProps::INODE_ZONE_BLOCKS * MachineProps::BLOCK_SIZE / sizeof(Inode)
    ];

    /** 用户当前位于的路径。默认为0，即跟节点。 */
    int userInodeIdx = 0;

    /** 文件系统是否已经加载。 */
    bool fileSystemLoaded = false;
};

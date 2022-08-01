/*
 * 文件系统适配器 - 头文件。
 * 2051565 龚天遥
 * 创建于 2022年7月29日。
 */

#pragma once

#include <fstream>
#include "./structures/SuperBlock.h"
#include "./structures/Inode.h"
#include "./structures/Block.h"
#include "./MacroDefines.h"

class FileSystemAdapter {
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

    void writeKernel(std::fstream& kernelFile);
    void writeBootLoader(std::fstream& bootLoaderFile);

public:
    bool readBlock(Block& block, const int blockIdx);
    bool readBlocks(char* buffer, const int blockIdx, const int blockCount);
    bool writeBlock(const Block& block, const int blockIdx);
    bool writeBlocks(const char* buffer, const int blockIdx, const int blockCount);

private:
    std::fstream fileStream;
    SuperBlock superBlock;
    DiskInode diskInodes[
        MachineProps::INODE_ZONE_BLOCKS * MachineProps::BLOCK_SIZE / sizeof(DiskInode)
    ];

    /** 文件系统是否已经加载。 */
    bool fileSystemLoaded = false;
};

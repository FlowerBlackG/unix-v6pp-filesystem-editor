/*
 * 文件系统适配器 - 头文件。
 * 2051565 龚天遥
 * 创建于 2022年7月29日。
 */

#pragma once

#include <fstream>
#include <vector>
#include <string>
#include <functional>
#include "./structures/SuperBlock.h"
#include "./structures/Inode.h"
#include "./structures/Block.h"
#include "./MacroDefines.h"
#include "./structures/InodeDirectory.h"

class FileSystemAdapter {
public:
    const int FS_FILE_SIZE_MAX = MachineProps::BLOCK_SIZE * (6 + 128 + 128 * 128);
    const int ROOT_INODE_IDX = 0;

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
    void sync();

    void writeKernel(std::fstream& kernelFile);
    void writeBootLoader(std::fstream& bootLoaderFile);

public:
    bool readBlock(Block& block, const int blockIdx);
    bool readBlocks(char* buffer, const int blockIdx, const int blockCount);
    bool writeBlock(const Block& block, const int blockIdx);
    bool writeBlocks(const char* buffer, const int blockIdx, const int blockCount);

    bool iterateOverInodeDataBlocks(
        Inode& inode,

        const std::function<void (
            int dataByteOffset,
            int blockIdx
        )> blockDiscoveryHandler,

        const std::function<int (
            int prevBlockIdx
        )> blockAllocator,

        const std::function<void (
            Inode& inode,
            int sizeRemaining,
            const char* msg
        )> iterationFailedHandler,

        const std::function<void (
            int dataBlockIdx
        )> dataBlockPostProcess,

        const std::function<void (
            const char* pBlock,
            int blockIndex
        )> indirectIndexBlockPostProcess
    );

    bool readFile(char* buffer, Inode& inode);
    bool writeFile(char* buffer, Inode& inode, int filesize);

    bool downloadFile(const std::string& fname, std::fstream& f);
    bool uploadFile(const std::string& fname, std::fstream& f);

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
    void freeInodeBlocks(Inode& inode);
    void freeInode(int idx, bool freeBlocks = false);
    int removeChildren(Inode& inode);

public:
    void ls(const InodeDirectory& dir);
    void ls(Inode& inode);
    void ls();
    void ls(const std::vector<std::string>& pathSegments, bool fromRoot);
    bool cd(const std::string& folderName);
    int mkdir(const std::string& dirName);
    int rm(const std::string& path);
    int touch(const std::string& fileName, Inode::FileType type);

public:
    std::fstream fileStream;
    SuperBlock superBlock;
    Inode inodes[
        MachineProps::INODE_ZONE_BLOCKS * MachineProps::BLOCK_SIZE / sizeof(Inode)
    ];

    /** 文件系统是否已经加载。 */
    bool fileSystemLoaded = false;

    /** 用户路径 inode 号栈。 */
    std::vector<int> inodeIdxStack;
};

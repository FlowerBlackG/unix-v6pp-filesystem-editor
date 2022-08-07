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
    const int ROOT_INODE_IDX = 1;

public:
    /**
     * 构造函数。自动打开磁盘映像文件。
     * 
     * @param filePath 文件路径。需要保证该文件可以打开。
     * @exception runtime_error 文件打开失败。
     */
    FileSystemAdapter(const char* filePath);

    ~FileSystemAdapter();

    /**
     * 格式化文件系统。会清空所有数据。
     */
    void format();

    /** 加载文件系统。 */
    void load();

    /**
     * 同步。将内存中的 superblock 和 inodes 同步到映像文件内。
     */
    void sync();

    /** 写入内核文件。 */
    void writeKernel(std::fstream& kernelFile);

    /** 
     * 写入启动引导文件。 
     * 
     * @param bootLoaderFile 指向启动引导文件的文件流。需要打开且可读。
     */
    void writeBootLoader(std::fstream& bootLoaderFile);

public:
    bool readBlock(Block& block, const int blockIdx);
    bool readBlocks(char* buffer, const int blockIdx, const int blockCount);
    bool writeBlock(const Block& block, const int blockIdx);
    bool writeBlocks(const char* buffer, const int blockIdx, const int blockCount);

    /**
     * 迭代处理一个 inode 对应的所有数据块。
     * 
     * @param inode 数据节点 inode。
     * @param blockDiscoveryHandler 对于每个直接存储数据的盘块，会调用此方法。
     * @param blockAllocator 当需要申请盘块的时候，会调用此方法。
     * @param iterationFailedHandler 当内部遇到问题，会调用此方法，然后结束迭代过程。
     * @param dataBlockPostProcess 处理完一个直接存储数据的盘块后，会调用此后处理函数。
     * @param indirectIndexBlockPostProcess 对于存储索引信息的盘块，索引内的数据块处理完毕后调用此方法。
     */
    bool iterateOverInodeDataBlocks(
        Inode& inode,

        /**
         * 盘块探知处理。
         * 
         * @param dataByteOffset 数据在文件中的字节位置。
         * @param blockIdx 盘块号。
         */
        const std::function<void (
            int dataByteOffset,
            int blockIdx
        )> blockDiscoveryHandler,

        /**
         * 盘块申请处理。
         * 
         * @param prevBlockIdx 该位置此前的盘块。若为写入等操作，返回值可能是错误的，毕竟之前没有申请过盘块。
         * @return 新盘块号。对于读取操作，直接返回原来的盘块号就可以。
         */
        const std::function<int (
            int prevBlockIdx
        )> blockAllocator,

        /**
         * 危重错误处理逻辑。
         * 
         * @param inode 涉事的 inode。
         * @param sizeRemaining 未处理的字节数。
         * @param msg 错误信息。
         */
        const std::function<void (
            Inode& inode,
            int sizeRemaining,
            const char* msg
        )> iterationFailedHandler,

        /**
         * 数据块后处理。会在调用 blockDiscoveryHandler 后被调用。
         */
        const std::function<void (
            int dataBlockIdx
        )> dataBlockPostProcess,

        /**
         * 索引块后处理。会在处理完索引块内的所有信息后被调用。
         */
        const std::function<void (
            const char* pBlock,
            int blockIndex
        )> indirectIndexBlockPostProcess
    );

    /**
     * 读取一个文件的内容。
     * 
     * @param buffer 存储目标。
     * @param inode 文件 inode。
     * @return 是否未出现错误。
     */
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

    /**
     * 获取一个空 inode。
     * 
     * @return inode 号。-1表示获取失败。 
     */
    int getFreeInode();

    /**
     * 释放一个 inode 的所有数据块（含索引块）。
     * 同时会将 inode 的 d_size 设为0。
     * 
     * @param inode 
     */
    void freeInodeBlocks(Inode& inode);
    void freeInode(int idx, bool freeBlocks = false);

    /**
     * 相当于对一个路径执行 rm -rf ./*
     * 
     * @param inode 
     * @return int 成功删除的文件数。对文件夹的统计可能不准确。
     */
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

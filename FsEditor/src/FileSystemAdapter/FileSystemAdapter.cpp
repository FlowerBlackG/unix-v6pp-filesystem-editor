/*
 * 文件系统适配器 - 实现。
 * 2051565 龚天遥
 * 创建于 2022年7月29日。
 */

#include <stdexcept>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <functional>
#include <cmath>
#include <cstring>
#include <cstdint>
#include <chrono>
#include "../include/FileSystemAdapter.h"
#include "../include/MachineProps.h"
#include "../include/structures/Inode.h"
#include "../include/structures/SuperBlock.h"
#include "../include/structures/Block.h"
#include "../include/structures/InodeDirectory.h"

using namespace std;
using namespace std::chrono;

/**
 * 获取当前系统时间戳。单位：秒。
 * 
 * @return int 时间戳。单位：秒。
 */
static int getCurrentTimeStamp() {
    auto now = system_clock::now();
    nanoseconds nanosec = now.time_since_epoch();
    milliseconds millisec = duration_cast<milliseconds>(nanosec);

    return millisec.count() / 1000;
}

FileSystemAdapter::FileSystemAdapter(const char* filePath) {
    fileStream.open(filePath, ios::in | ios::out | ios::binary);
    // 别忘了加 binary。这个 bug 找了一晚上...
    // sj: ”0x1a？那要打屁股了。“

    if (!fileStream.is_open()) {
        throw runtime_error("failed to open file!");
    }

    // 校验文件尺寸。暂不支持自定义尺寸。要求尺寸精确。
    fileStream.seekg(0, ios::end);
    unsigned long long fileSize = fileStream.tellg();
        
    if (fileSize != MachineProps::diskSize()) {
        fileStream.close();
        throw runtime_error("bad filesize.");
    }
}

FileSystemAdapter::~FileSystemAdapter() {
    if (fileStream.is_open()) {
        sync();
        fileStream.close();
    }
}

bool FileSystemAdapter::readBlock(Block& block, const int blockIdx) {
    return this->readBlocks(block.asCharArray(), blockIdx, 1);
}


bool FileSystemAdapter::readBlocks(char* buffer, const int blockIdx, const int blockCount) {
    if (blockIdx + blockCount > MachineProps::diskBlocks()) {
        cout << "[critical 1] FileSystemAdapter::readBlocks c*ii" << endl;
        cout << "             pBuf: " << (int*) buffer << ", blockIdx: " 
            << blockIdx << ", count: " << blockCount << endl;
        exit(-1);
    }

    fileStream.clear();
    fileStream.seekg(blockIdx * sizeof(Block), ios::beg);
    fileStream.read(buffer, blockCount * sizeof(Block));
    return fileStream.gcount() == blockCount * sizeof(Block);
}

bool FileSystemAdapter::writeBlock(const Block& block, const int blockIdx) {
    return this->writeBlocks(block.asConstCharArray(), blockIdx, 1);
}


bool FileSystemAdapter::writeBlocks(const char* buffer, const int blockIdx, const int blockCount) {
    if (blockIdx + blockCount > MachineProps::diskBlocks()) {
        cout << "[critical 1] FileSystemAdapter::writeBlocks c*ii" << endl;
        cout << "             pBuf: " << (int*) buffer << ", blockIdx: " 
            << blockIdx << ", count: " << blockCount << endl;
        exit(-1);
    }

    fileStream.clear();
    fileStream.seekp(blockIdx * sizeof(Block), ios::beg);
    fileStream.write(buffer, blockCount * sizeof(Block));
    return true;
}

bool FileSystemAdapter::iterateOverInodeDataBlocks(

    Inode& inode,

    const function<void (
        int dataByteOffset,
        int blockIdx
    )> blockDiscoveryHandler,

    const function<int (
        int prevBlockIdx
    )> blockAllocator,

    const function<void (
        Inode& inode,
        int sizeRemaining,
        const char* msg
    )> iterationFailedHandler,

    const function<void (
        int dataBlockIdx
    )> dataBlockPostProcess,

    const function<void (
        const char* pBlock,
        int blockIndex
    )> indirectIndexBlockPostProcess

) {
    int sizeRemaining = inode.d_size; // 剩下的字节数。
    const char* errmsg = "";

    // 每个索引块的块条目数。
    const int entriesPerIdxBlock = sizeof(Block) / sizeof(uint32_t);
    uint32_t firstIdxBlockBuffer[entriesPerIdxBlock]; // 一级索引块缓存。
    uint32_t secondIdxBlockBuffer[entriesPerIdxBlock]; // 二级索引块缓存。
    // 上面这两个东西总共占用 1KB 栈空间，问题不大。

    // 直接索引读入。
    for (int idx = 0; sizeRemaining > 0 && idx < 6; idx++) {

        uint32_t nextBlkIdx = blockAllocator(inode.direct_index[idx]);
        if (nextBlkIdx < 0) {
            errmsg = "direct index, block allocation failed.";
            goto IT_INODE_DATA_BLOCKS_FAILED;
        }

        inode.direct_index[idx] = nextBlkIdx;
        blockDiscoveryHandler(sizeof(Block) * idx, inode.direct_index[idx]);
        dataBlockPostProcess(inode.direct_index[idx]);

        sizeRemaining -= sizeof(Block);
    }

    // 一级索引读入。
    for (int firIdxBlockIdx = 0; firIdxBlockIdx < 2 && sizeRemaining > 0; firIdxBlockIdx++) {
        uint32_t nextBlkIdx = blockAllocator(inode.indirect_index[firIdxBlockIdx]);
        if (nextBlkIdx < 0) {
            errmsg = "fir indirect index, block allocation failed.";
            goto IT_INODE_DATA_BLOCKS_FAILED;
        }

        inode.indirect_index[firIdxBlockIdx] = nextBlkIdx;
        
        this->readBlocks(
            (char*) firstIdxBlockBuffer,
            inode.indirect_index[firIdxBlockIdx], 1
        );

        // 内部：直接索引。
        for (int idx = 0; sizeRemaining > 0 && idx < entriesPerIdxBlock; idx++) {
            int entriesTargetByteOffset = MachineProps::BLOCK_SIZE 
                * (6 + entriesPerIdxBlock * firIdxBlockIdx + idx);

            uint32_t nextBlkIdx = blockAllocator(firstIdxBlockBuffer[idx]);
            if (nextBlkIdx < 0) {
                errmsg = "fir indirect index, block allocation failed (direct).";
                goto IT_INODE_DATA_BLOCKS_FAILED;
            }

            firstIdxBlockBuffer[idx] = nextBlkIdx;

            blockDiscoveryHandler(entriesTargetByteOffset, firstIdxBlockBuffer[idx]);
            dataBlockPostProcess(firstIdxBlockBuffer[idx]);

            sizeRemaining -= MachineProps::BLOCK_SIZE;
            
        } // for (int idx = 0; sizeRemaining > 0 && idx < 6; idx++)

        indirectIndexBlockPostProcess((char*) firstIdxBlockBuffer, nextBlkIdx);
    } // for (int firIdxBlockIdx = 0; firIdxBlockIdx < 2 && sizeRemaining > 0; firIdxBlockIdx++)

    // 二级索引读入。
    for (int secIdxBlockIdx = 0; secIdxBlockIdx < 2 && sizeRemaining > 0; secIdxBlockIdx++) {
        uint32_t nextBlkIdx = blockAllocator(inode.secondary_indirect_index[secIdxBlockIdx]);
        if (nextBlkIdx < 0) {
            errmsg = "sec indirect index, block allocation failed.";
            goto IT_INODE_DATA_BLOCKS_FAILED;
        }
        inode.secondary_indirect_index[secIdxBlockIdx] = nextBlkIdx;

        this->readBlocks(
            (char*) secondIdxBlockBuffer, 
            inode.secondary_indirect_index[secIdxBlockIdx], 1
        );

        // 内部：一级索引。
        for (
            int firIdxBlockIdx = 0; 
            firIdxBlockIdx < entriesPerIdxBlock && sizeRemaining > 0; 
            firIdxBlockIdx++
        ) {
            uint32_t nextBlkIdx = blockAllocator(secondIdxBlockBuffer[firIdxBlockIdx]);
            if (nextBlkIdx < 0) {
                errmsg = "sec indirect index, block allocation failed (fir).";
                goto IT_INODE_DATA_BLOCKS_FAILED;
            }
            secondIdxBlockBuffer[firIdxBlockIdx] = nextBlkIdx;

            this->readBlocks(
                (char*) firstIdxBlockBuffer,
                secondIdxBlockBuffer[firIdxBlockIdx], 1
            );

            // 内部：直接索引。
            for (int idx = 0; sizeRemaining > 0 && idx < entriesPerIdxBlock; idx++) {

                uint32_t nextBlkIdx = blockAllocator(firstIdxBlockBuffer[idx]);
                if (nextBlkIdx < 0) {
                    errmsg = "sec indirect index, block allocation failed (fir).";
                    goto IT_INODE_DATA_BLOCKS_FAILED;
                }
                firstIdxBlockBuffer[idx] = nextBlkIdx;

                int entriesTargetByteOffset = sizeof(Block) * (
                    6 
                    + 2 * entriesPerIdxBlock 
                    + entriesPerIdxBlock * entriesPerIdxBlock * secIdxBlockIdx 
                    + entriesPerIdxBlock * firIdxBlockIdx 
                    + idx
                );

                blockDiscoveryHandler(entriesTargetByteOffset, firstIdxBlockBuffer[idx]);
                dataBlockPostProcess(firstIdxBlockBuffer[idx]);

                sizeRemaining -= sizeof(Block);
                
            } // for (int idx = 0; sizeRemaining > 0 && idx < 6; idx++)

            indirectIndexBlockPostProcess((char*) firstIdxBlockBuffer, nextBlkIdx);
        } // 内部：一级索引。
        
        indirectIndexBlockPostProcess((char*) secondIdxBlockBuffer, nextBlkIdx);
    }

    return true;

IT_INODE_DATA_BLOCKS_FAILED:
    iterationFailedHandler(inode, sizeRemaining, errmsg);
    return false;

}

bool FileSystemAdapter::readFile(char* buffer, Inode& inode) {

    return this->iterateOverInodeDataBlocks(
        inode,
        [&] (int dataByteOffset, int blockIdx) {
            readBlocks(buffer + dataByteOffset, blockIdx, 1);
        },

        [] (int prevBlockIdx) {
            return prevBlockIdx;
        },

        [] (Inode& inode, int sizeRemaining, const char* msg) {
            cout << "[error] lambda 异常。" << endl;
        },

        [] (...) {},

        [] (...) {} // 不需要做后处理。
    );
}


bool FileSystemAdapter::writeFile(char* buffer, Inode& inode, int filesize) {
    this->freeInodeBlocks(inode);

    int filesizeRemaining = min(filesize, FileSystemAdapter::FS_FILE_SIZE_MAX);
    inode.d_size = filesizeRemaining;
    inode.ilarg = !!(filesizeRemaining > sizeof(Block) * 6);
    // 开放所有权限。
    inode.permission_group = inode.permission_others = inode.permission_owner = 7;

    return this->iterateOverInodeDataBlocks(
        inode, 
        
        [&] (int dataByteOffset, int blockIdx) {
            this->writeBlocks(
                buffer + dataByteOffset, blockIdx, 1
            );
        },

        [&] (int prevBlockIdx) {
            return this->getFreeBlock();
        },

        [&] (Inode& inode, int sizeRemaining, const char* msg) {
            inode.d_size -= sizeRemaining;
            cout << "[error] lambda 异常（可能的原因：盘满）。" << endl;
        },

        [] (...) {},

        [&] (const char* pBlock, int blockIndex) {
            this->writeBlocks(
                pBlock, blockIndex, 1
            );
        }
    );
}

bool FileSystemAdapter::downloadFile(const std::string& fname, std::fstream& f) {
    InodeDirectory dir(this->inodes[inodeIdxStack.back()], *this, false, 1);
    
    int targetIdx = -1;

    // 寻找目标。
    for (int entryIdx = 0; entryIdx < dir.length; entryIdx++) {
        if (dir.entries[entryIdx].m_name == fname) {
            targetIdx = dir.entries[entryIdx].m_ino;
            break;
        }
    }

    if (targetIdx < 0) {
        cout << "[error] 找不到：" << fname << endl;
        return false;
    }

    f.clear();
    f.seekp(0, ios::beg);

    return this->iterateOverInodeDataBlocks(
        this->inodes[targetIdx],

        [&] (int, int blockIdx) {
            Block b;
            this->readBlock(b, blockIdx);
            f.write(
                b.asCharArray(), 
                min(
                    sizeof(b), // 完整写入。
                    (size_t) this->inodes[targetIdx].d_size - f.tellp() // 最后一个 block。
                )
            );
        },

        [] (int prevIdx) { return prevIdx; },
        [] (...) {},
        [] (...) {},
        [] (...) {}
    );
}

bool FileSystemAdapter::uploadFile(const std::string& fname, std::fstream& f) {
    Inode& inode = this->inodes[this->touch(fname, Inode::FileType::NORMAL)];
    this->freeInodeBlocks(inode);

    f.clear();
    f.seekg(0, ios::end);
    int filesize = f.tellg();

    int filesizeRemaining = min(filesize, FileSystemAdapter::FS_FILE_SIZE_MAX);
    inode.d_size = filesizeRemaining;
    inode.ilarg = !!(filesizeRemaining > sizeof(Block) * 6);
    // 开放所有权限。
    inode.permission_group = inode.permission_others = inode.permission_owner = 7;

    f.seekg(0, ios::beg);

    return this->iterateOverInodeDataBlocks(
        inode, 
        
        [&] (int dataByteOffset, int blockIdx) {
            Block b;
            f.read(b.asCharArray(), sizeof(b));
            this->writeBlock(b, blockIdx);
        },

        [&] (int prevBlockIdx) {
            return this->getFreeBlock();
        },

        [&] (Inode& inode, int sizeRemaining, const char* msg) {
            inode.d_size -= sizeRemaining;
            cout << "[error] lambda 异常（可能的原因：盘满）。" << endl;
        },

        [] (...) {},

        [&] (const char* pBlock, int blockIndex) {
            this->writeBlocks(
                pBlock, blockIndex, 1
            );
        }
    );
}

void FileSystemAdapter::load() {
    this->superBlock.loadFromImg(this->fileStream);

    this->fileStream.clear();

    this->fileStream.seekg(
        this->superBlock.inode_zone_begin * MachineProps::BLOCK_SIZE,
        ios::beg
    );
    
    int inodeZoneSize = this->superBlock.inode_zone_blocks * MachineProps::BLOCK_SIZE;

    this->fileStream.read(
        (char*) this->inodes, 
        inodeZoneSize
    );

    if (fileStream.gcount() != inodeZoneSize) {
        cout << "[error] exception on loading inodes." << endl;
        cout << "        bytes read:   " << fileStream.gcount() << endl;
        cout << "        bytes wanted: " << inodeZoneSize << endl;
        cout << "        fp: " << fileStream.tellg() << endl;
        cout << "        fstream goodbit: " << !!fileStream.good() << endl;
        cout << "        fstream failbit: " << !!fileStream.fail() << endl;
        throw runtime_error("文件系统异常。");
    }

    fileSystemLoaded = true;
    inodeIdxStack.clear();
    inodeIdxStack.push_back(ROOT_INODE_IDX);
}

void FileSystemAdapter::sync() {
    this->superBlock.writeToImg(this->fileStream);

    this->fileStream.clear();
    this->fileStream.seekp(
        this->superBlock.inode_zone_begin * sizeof(Block),
        ios::beg
    );

    fileStream.write((char*) this->inodes, this->superBlock.inode_zone_blocks * sizeof(Block));
}

/**
 * 格式化。
 */
void FileSystemAdapter::format() {
    this->superBlock.loadDefaultProfile(); // 重置 superblock。

    // 释放 inode。

#if 0 // 两种 inode 登记顺序。
    for (
        int idx = ROOT_INODE_IDX + 1; 
        idx < MachineProps::INODE_ZONE_BLOCKS * sizeof(Block) / sizeof(Inode); 
        idx++
    ) {
#else
    for (
        int idx = MachineProps::INODE_ZONE_BLOCKS * sizeof(Block) / sizeof(Inode) - 1;
        idx > ROOT_INODE_IDX; 
        idx--
    ) {
#endif

        this->freeInode(idx);
    }

    // 链接所有 block。
#if 0 // 两种盘块登记顺序。
    for (
        int idx = superBlock.data_zone_begin;
        idx < superBlock.data_zone_begin + superBlock.data_zone_blocks;
        idx++
    ) {
#else
    for (
        int idx = superBlock.data_zone_begin + superBlock.data_zone_blocks - 1;
        idx >= superBlock.data_zone_begin;
        idx--
    ) {
#endif
        this->freeBlock(idx);
    }

    // 创建 root 目录，并写入 dev/tty1。
    
    inodeIdxStack.clear();
    inodeIdxStack.push_back(ROOT_INODE_IDX);
    Inode& rootInode = this->inodes[ROOT_INODE_IDX];

    rootInode.permission_group = 7;
    rootInode.permission_owner = 7;
    rootInode.permission_others = 7;
    rootInode.d_nlink = 1;

    rootInode.file_type = Inode::FileType::DIR;
    rootInode.d_size = 0;
    rootInode.d_mtime = getCurrentTimeStamp();
    rootInode.d_atime = getCurrentTimeStamp();
    rootInode.isgid = rootInode.isuid = 0;
    rootInode.d_gid = rootInode.d_uid = 0;
    rootInode.ialloc = 1;
    
    this->mkdir("dev");
    this->cd("dev");
    this->touch("tty1", Inode::FileType::CHAR_DEV);
    inodeIdxStack.pop_back(); // 回到 root。
    
    // 小彩蛋。当时debug用的，不想删了。
    this->touch("tongji-yyds", Inode::FileType::NORMAL); 
}

void FileSystemAdapter::writeKernel(fstream& kernelFile) {
    int kernelSize = MachineProps::BLOCK_SIZE * MachineProps::KERNEL_BIN_BLOCKS;
    // 申请缓冲区。不做失败检查，让其自然抛异常。
    char* buffer = new char[kernelSize];
    kernelFile.clear();
    kernelFile.seekg(0, ios::beg);
    kernelFile.read(buffer, kernelSize);

    this->writeBlocks(buffer, 1, MachineProps::KERNEL_BIN_BLOCKS);
    delete[] buffer;
}

void FileSystemAdapter::writeBootLoader(fstream& bootLoaderFile) {
    
    int bootloaderSize = MachineProps::BLOCK_SIZE * MachineProps::BOOT_LOADER_BLOCKS;
    // 申请缓冲区。不做失败检查，让其自然抛异常。另外，如果 512个字节都拿不到，也没什么可玩的了...
    char* buffer = new char[bootloaderSize];
    bootLoaderFile.clear();
    bootLoaderFile.seekg(0, ios::beg);
    bootLoaderFile.read(buffer, bootloaderSize);

    this->writeBlocks(buffer, 0, MachineProps::BOOT_LOADER_BLOCKS);
    delete[] buffer;
}

/* ------------ 盘块和 inode 获取与释放。 ------------ */
int FileSystemAdapter::getFreeBlock() {
    int ret;

    if (superBlock.s_nfree == 0) {

        ret = -1; // 无空盘块。
    } else if (superBlock.s_nfree >= 2) {

        ret = superBlock.s_free[--superBlock.s_nfree];
    } else {
        int result = superBlock.s_free[0];
        Block b;
        readBlock(b, result);
        memcpy(&superBlock.s_nfree, &b, 101 * sizeof(uint32_t));

        ret = result;
    }

    if (ret >= MachineProps::diskBlocks()) {
        cout << "[critical 2] FSA::getFreeBlock" << endl;
        cout << "             ret: " << ret << endl;
        cout << "s_nfree: " << superBlock.s_nfree << endl;
        for (int i = 0; i < 100; i++) {
            cout << "[" << i << "] " << superBlock.s_free[i] << endl;
        }
        exit(-1);
    }

    return ret;
}


void FileSystemAdapter::freeBlock(int idx) {

    if (idx >= MachineProps::diskBlocks()) {
        cout << "[critical 3] FSA::freeBlock" << endl;
        cout << "             idx: " << idx << endl;
        cout << "s_nfree: " << superBlock.s_nfree << endl;
        for (int i = 0; i < 100; i++) {
            cout << "[" << i << "] " << superBlock.s_free[i] << endl;
        }
        exit(-1);
    }

    if (superBlock.s_nfree == 0) {
        superBlock.s_free[0] = 0;
        superBlock.s_nfree = 1;
    }
    
    if (superBlock.s_nfree < 100) {
        superBlock.s_free[superBlock.s_nfree++] = idx;
    } else {

        Block b;
        memcpy(&b, &superBlock.s_nfree, 101 * sizeof(uint32_t));
        writeBlock(b, idx);

        superBlock.s_nfree = 1;
        superBlock.s_free[0] = idx;

    }
}

int FileSystemAdapter::getFreeInode() {
    auto searchForFreeInodes = [&] () {
        for (
            int idx = ROOT_INODE_IDX + 1; 
            idx < sizeof(this->inodes) / sizeof(Inode) && superBlock.s_ninode < 100;
            idx++
        ) {
            if (this->inodes[idx].ialloc == 0) {
                superBlock.s_inode[superBlock.s_ninode++] = idx;
            }
        }
    };

    if (superBlock.s_ninode == 0) { // 寻找空盘 inode。
        searchForFreeInodes();
    }
    
    if (superBlock.s_ninode > 0) {
        int result = superBlock.s_inode[--superBlock.s_ninode];

        this->inodes[result].ialloc = 1; // 表示已经被分配。
        this->inodes[result].permission_group = 7;
        this->inodes[result].permission_owner = 7;
        this->inodes[result].permission_others = 7;
        this->inodes[result].d_size = 0;
        this->inodes[result].d_nlink = 1;
        this->inodes[result].isgid = 0;
        this->inodes[result].isuid = 0;
        this->inodes[result].d_uid = 0;
        this->inodes[result].d_gid = 0;
        this->inodes[result].d_mtime = getCurrentTimeStamp();
        this->inodes[result].d_atime = getCurrentTimeStamp();
        
        if (superBlock.s_ninode == 0) { // 寻找空盘 inode。
            searchForFreeInodes();
        }

        return result;
    } else {
        return -1; // 获取失败。
    }
}

void FileSystemAdapter::freeInodeBlocks(Inode& inode) {
    this->iterateOverInodeDataBlocks(
        inode,

        [] (...) {}, // nothing..

        [] (int prevBlockIdx) {
            return prevBlockIdx;
        },

        [] (...) {}, // nothing...

        [&] (int dataBlockIdx) {
            this->freeBlock(dataBlockIdx);
        },

        [&] (const char* pBlock, int blockIndex) {
            this->freeBlock(blockIndex);
        }
    );

    inode.d_size = 0;
}

void FileSystemAdapter::freeInode(int idx, bool freeBlocks) {
    Inode& inode = this->inodes[idx];
    
    if (freeBlocks) {
        this->freeInodeBlocks(inode);
    }

    inode.loadEmptyProfile();

    if (superBlock.s_ninode < 100) {
        superBlock.s_inode[superBlock.s_ninode++] = idx;
    }
}


/* ------------ 文件系统用户界面操作。 ------------ */

void FileSystemAdapter::ls(const InodeDirectory& dir) {
    for (int idx = 0; idx < dir.length; idx++) {
        const DirectoryEntry& entry = dir.entries[idx];
        const Inode& entryInode = this->inodes[entry.m_ino];

        // 文件类型。
        switch (entryInode.file_type) {
            case Inode::FileType::BLOCK_DEV:
                cout << 'b';
                break;

            case Inode::FileType::CHAR_DEV:
                cout << 'c';
                break;

            case Inode::FileType::DIR:
                cout << 'd';
                break;

            case Inode::FileType::NORMAL:
                cout << '-';
                break;
            
            default:
                cout << '?';
                break;
        }

        // 权限。

        int permissionVars[] = {
            entryInode.permission_owner,
            entryInode.permission_group,
            entryInode.permission_others
        };

        for (int i = 0; i < 3; i++) {
            int& permission = permissionVars[i];
            cout << ((permission & 4) ? 'r' : '-');
            cout << ((permission & 2) ? 'w' : '-');
            cout << ((permission & 1) ? 'x' : '-');
        }

        // 软链接数。
        cout << setiosflags(ios::right);
        cout << setw(4) << entryInode.d_nlink;

        // 文件主。
        cout << setw(3) << entryInode.d_gid << " :";
        cout << setw(3) << entryInode.d_uid;

        // 文件大小。
        cout << setw(8) << entryInode.d_size;

        // 修改时间。
        cout << setw(12) << entryInode.d_mtime << ".m";
        
        // 访问时间。
        cout << setw(12) << entryInode.d_atime << ".a";

        // 文件名。
        cout << " " << entry.m_name << endl;
        
        cout << resetiosflags(ios::right);
    }
}

void FileSystemAdapter::ls(Inode& inode) {
    try {
        this->ls(InodeDirectory(inode, *this, true));
    } catch (const runtime_error& e) {
        cout << "[error] FSA::ls Inode& exception: " << e.what() << endl;
    }
}

void FileSystemAdapter::ls() {
    this->ls(this->inodes[inodeIdxStack.back()]);
}

void FileSystemAdapter::ls(const vector<string>& pathSegments, bool fromRoot) {
    int currInodeIdx = fromRoot ? FileSystemAdapter::ROOT_INODE_IDX : inodeIdxStack.back();

    cout << "[info] 正在打开：" << (fromRoot ? "根目录" : "当前目录") << endl;

    for (const auto& seg : pathSegments) {
        if (seg == ".") {
            continue;
        }

        // 确保当前路径是个目录。
        if (currInodeIdx != 0 && this->inodes[currInodeIdx].file_type != Inode::FileType::DIR) {
            cout << "[error] 该路径不是文件夹。拒绝执行指令。" << endl;
            cout << "        路径类型为：" << inodes[currInodeIdx].file_type << endl;
            return;
        }

        // 寻找下一个目录。
        bool nextDirFound = false;
        InodeDirectory dir(this->inodes[currInodeIdx], *this, true);
        for (int entryIdx = 0; entryIdx < dir.length; entryIdx++) {
            if (dir.entries[entryIdx].m_name == seg) {
                cout << "[info] 进入：" << seg << endl;
                currInodeIdx = dir.entries[entryIdx].m_ino;
                nextDirFound = true;
                break;
            }
        }

        if (!nextDirFound) {
            cout << "[error] 找不到：" << seg << endl;
            return;
        }
    }

    ls(inodes[currInodeIdx]);

}


bool FileSystemAdapter::cd(const string& folderName) {
    if (folderName == "\\" || folderName == "/") {
        inodeIdxStack.clear();
        inodeIdxStack.push_back(ROOT_INODE_IDX);
        return true;
    } else if (folderName == ".") {
        // nothing
        return true;
    } else if (folderName == "..") {
        if (inodeIdxStack.size() > 1) {
            inodeIdxStack.pop_back();
            return true;
        } else {
            return false;
        }
    }

    InodeDirectory dir(this->inodes[inodeIdxStack.back()], *this, true);
    
    for (int entryIdx = 0; entryIdx < dir.length; entryIdx++) {
        
        if (dir.entries[entryIdx].m_name == folderName) {
            Inode& inode = this->inodes[dir.entries[entryIdx].m_ino];
            if (inode.file_type != Inode::FileType::DIR) {
                cout << "[error] 名字：" << folderName << " 不是文件夹。" << endl;
                return false;
            } else {
                inodeIdxStack.push_back(dir.entries[entryIdx].m_ino);
                return true;
            }
        }
    }

    cout << "[error] 找不到：" << folderName << endl;
    return false;
}

int FileSystemAdapter::mkdir(const string& dirName) {
    int inodeIdx = touch(dirName, Inode::FileType::DIR);
    if (inodeIdx < 0) {
        cout << "[error] 无法创建文件夹。" << endl;
        return -1;
    } else {
        Inode& inode = this->inodes[inodeIdx];
        inode.d_size = 0;
        return inodeIdx;
    }
}

int FileSystemAdapter::removeChildren(Inode& inode) {
    if (inode.d_size == 0) {
        return 0;
    }

    if (inode.file_type != Inode::FileType::DIR) {
        this->freeInodeBlocks(inode);
        return 1;
    } else {
        int result = 0;
        
        InodeDirectory dir(inode, *this, true, 1);
        this->freeInodeBlocks(inode);
        for (int entryIdx = 0; entryIdx < dir.length; entryIdx++) {
            cout << "[info] 删除：" << dir.entries[entryIdx].m_name << endl;
            result += removeChildren(inodes[dir.entries[entryIdx].m_ino]);
        }
        
        return result;
    }
}

/**
 * rm -rf 
 */
int FileSystemAdapter::rm(const std::string& path) {
    InodeDirectory dir(this->inodes[inodeIdxStack.back()], *this, true, 1);
    
    int targetIdx = -1;
    int entryIdx;

    // 寻找删除目标。
    for (entryIdx = 0; entryIdx < dir.length; entryIdx++) {
        if (dir.entries[entryIdx].m_name == path) {
            targetIdx = dir.entries[entryIdx].m_ino;
            break;
        }
    }

    if (targetIdx < 0) {
        cout << "[error] 找不到：" << path << endl;
    }

    Inode& targetInode = this->inodes[targetIdx];
    int result = removeChildren(targetInode);

    // 目录项移除。
    for (int idx = entryIdx; idx + 1 < dir.length; idx++) {
        memcpy(dir.entries + idx, dir.entries + idx + 1, sizeof(DirectoryEntry));
    }

    dir.length--;

    this->writeFile(
        (char*) dir.entries, 
        this->inodes[inodeIdxStack.back()], 
        dir.length * sizeof(DirectoryEntry)
    );

    return result;
}


int FileSystemAdapter::touch(const std::string& fileName, Inode::FileType type) {

    InodeDirectory dir(this->inodes[inodeIdxStack.back()], *this, false, 1);
    
    // 同名校验。
    for (int entryIdx = 0; entryIdx < dir.length; entryIdx++) {
        if (dir.entries[entryIdx].m_name == fileName) {
            cout << "[info] 该文件已存在。" << endl;
            return dir.entries[entryIdx].m_ino;
        }
    }

    // 新建。
    int inodeIdx = this->getFreeInode();


    if (inodeIdx < 0) {
        cout << "[error] 无法获取空 inode。" << endl;
        return -1;
    } else {
        Inode& inode = this->inodes[inodeIdx];
        inode.file_type = type;

        dir.entries[dir.length].m_ino = inodeIdx;
        memset(dir.entries[dir.length].m_name, 0, sizeof(DirectoryEntry::m_name));
        memcpy(
            dir.entries[dir.length].m_name, 
            fileName.c_str(), 
            min(fileName.length() + 1, sizeof(DirectoryEntry::m_name))
        );
        dir.length++;

        this->writeFile(
            (char*) dir.entries, 
            this->inodes[inodeIdxStack.back()], 
            dir.length * sizeof(DirectoryEntry)
        );

        return inodeIdx;
    }
}

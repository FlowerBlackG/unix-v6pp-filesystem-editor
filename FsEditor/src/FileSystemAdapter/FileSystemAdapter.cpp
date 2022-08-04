/*
 * 文件系统适配器。
 * 2051565 龚天遥
 * 创建于 2022年7月29日。
 */

#include <stdexcept>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <cmath>
#include "../include/FileSystemAdapter.h"
#include "../include/MachineProps.h"
#include "../include/structures/Inode.h"
#include "../include/structures/SuperBlock.h"
#include "../include/structures/Block.h"
#include "../include/structures/InodeDirectory.h"

using namespace std;

FileSystemAdapter::FileSystemAdapter(const char* filePath) {
    fileStream.open(filePath, ios::in | ios::out | ios::binary);
    // 别忘了加 binary。这个 bug 找了一晚上...

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
        flush();
        fileStream.close();
    }
}

bool FileSystemAdapter::readBlock(Block& block, const int blockIdx) {
    return this->readBlocks(block.asCharArray(), blockIdx, 1);
}


bool FileSystemAdapter::readBlocks(char* buffer, const int blockIdx, const int blockCount) {
    fileStream.clear();
    fileStream.seekg(blockIdx * sizeof(Block), ios::beg);
    fileStream.read(buffer, blockCount * sizeof(Block));
    return fileStream.gcount() == blockCount * sizeof(Block);
}

bool FileSystemAdapter::writeBlock(const Block& block, const int blockIdx) {
    return this->writeBlocks(block.asConstCharArray(), blockIdx, 1);
}


bool FileSystemAdapter::writeBlocks(const char* buffer, const int blockIdx, const int blockCount) {
    fileStream.clear();
    fileStream.seekp(blockIdx * sizeof(Block), ios::beg);
    fileStream.write(buffer, blockCount * sizeof(Block));
    return true;
}

bool FileSystemAdapter::readFile(char* buffer, const Inode& inode) {
    int sizeRemaining = inode.d_size; // 剩下待读入的字节数。

    // 直接索引读入。
    for (int idx = 0; sizeRemaining > 0 && idx < 6; idx++) {
        this->readBlocks(
            buffer + sizeof(Block) * idx,
            inode.direct_index[idx], 1
        );

        sizeRemaining -= sizeof(Block);
    }

    // 每个索引块的块条目数。
    const int entriesPerIdxBlock = sizeof(Block) / sizeof(uint32_t);
    uint32_t firstIdxBlockBuffer[entriesPerIdxBlock]; // 一级索引块缓存。
    uint32_t secondIdxBlockBuffer[entriesPerIdxBlock]; // 二级索引块缓存。
    // 上面这两个东西总共占用 1KB 栈空间，问题不大。

    // 一级索引读入。
    for (int firIdxBlockIdx = 0; firIdxBlockIdx < 2 && sizeRemaining > 0; firIdxBlockIdx++) {

        this->readBlocks(
            (char*) firstIdxBlockBuffer,
            inode.indirect_index[firIdxBlockIdx], 1
        );

        // 内部：直接索引。
        for (int idx = 0; sizeRemaining > 0 && idx < entriesPerIdxBlock; idx++) {
            int entriesTargetByteOffset = MachineProps::BLOCK_SIZE 
                * (6 + entriesPerIdxBlock * firIdxBlockIdx + idx);

            this->readBlocks(
                buffer + entriesTargetByteOffset,
                inode.direct_index[idx], 1
            );

            sizeRemaining -= MachineProps::BLOCK_SIZE;
        } // for (int idx = 0; sizeRemaining > 0 && idx < 6; idx++)
    } // for (int firIdxBlockIdx = 0; firIdxBlockIdx < 2 && sizeRemaining > 0; firIdxBlockIdx++)

    // 二级索引读入。
    for (int secIdxBlockIdx = 0; secIdxBlockIdx < 2 && sizeRemaining > 0; secIdxBlockIdx++) {
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

            this->readBlocks(
                (char*) firstIdxBlockBuffer,
                inode.indirect_index[firIdxBlockIdx], 1
            );

            // 内部：直接索引。
            for (int idx = 0; sizeRemaining > 0 && idx < entriesPerIdxBlock; idx++) {
                int entriesTargetByteOffset = MachineProps::BLOCK_SIZE 
                    * (
                        6 
                        + 2 * entriesPerIdxBlock 
                        + entriesPerIdxBlock * entriesPerIdxBlock * secIdxBlockIdx 
                        + entriesPerIdxBlock * firIdxBlockIdx 
                        + idx
                    );

                this->readBlocks(
                    buffer + entriesTargetByteOffset,
                    inode.direct_index[idx], 1
                );

                sizeRemaining -= MachineProps::BLOCK_SIZE;
            } // for (int idx = 0; sizeRemaining > 0 && idx < 6; idx++)
        } // 内部：一级索引。
    }

    return true;
}


bool FileSystemAdapter::writeFile(char* buffer, Inode& inode, int filesize) {
    int filesizeRemaining = min(filesize, FileSystemAdapter::FS_FILE_SIZE_MAX);
    inode.d_size = filesizeRemaining;
    inode.ilarg = !!(filesizeRemaining > sizeof(Block) * 6);
    // 开放所有权限。
    inode.permission_group = inode.permission_others = inode.permission_owner = 7;

    for (int idx = 0; idx < 6 && filesizeRemaining > 0; idx++) {
        int nextBlk = this->getFreeBlock();
        if (nextBlk < 0) {
            cout << "写入失败。盘满。" << endl;
            return false;
        }

        this->writeBlocks(buffer + idx * sizeof(Block), nextBlk, 1);
        inode.direct_index[idx] = nextBlk;
        filesizeRemaining -= sizeof(Block);
    }

    // 每个索引块的块条目数。
    const int entriesPerIdxBlock = sizeof(Block) / sizeof(uint32_t);
    uint32_t firstIdxBlockBuffer[entriesPerIdxBlock]; // 一级索引块缓存。
    uint32_t secondIdxBlockBuffer[entriesPerIdxBlock]; // 二级索引块缓存。
    // 上面这两个东西总共占用 1KB 栈空间，问题不大。

    // 一级索引。
    for (int firIdxBlockIdx = 0; firIdxBlockIdx < 2 && filesizeRemaining > 0; firIdxBlockIdx++) {

        // 内部：直接索引。
        for (int idx = 0; filesizeRemaining > 0 && idx < entriesPerIdxBlock; idx++) {
            int entriesTargetByteOffset = sizeof(Block) * (6 + entriesPerIdxBlock * firIdxBlockIdx + idx);

            int blkIdx = getFreeBlock();
            if (blkIdx < 0) {
                cout << "盘满。存盘失败。" << endl;
                return false;
            }

            this->writeBlocks(buffer + entriesTargetByteOffset, blkIdx, 1);
            firstIdxBlockBuffer[idx] = blkIdx;
            
            filesizeRemaining -= sizeof(Block);
        } // for (int idx = 0; filesizeRemaining > 0 && idx < 6; idx++)

        int blkIdx = getFreeBlock();
        if (blkIdx < 0) {
            cout << "盘满。存盘失败。" << endl;
            return false;
        }

        this->writeBlocks((char*) firstIdxBlockBuffer, blkIdx, 1);
        inode.indirect_index[firIdxBlockIdx] = blkIdx;
    } // for (int firIdxBlockIdx = 0; firIdxBlockIdx < 2 && filesizeRemaining > 0; firIdxBlockIdx++)

    // 二级索引。
    for (int secIdxBlockIdx = 0; secIdxBlockIdx < 2 && filesizeRemaining > 0; secIdxBlockIdx++) {
        // 内部：一级索引。
        for (
            int firIdxBlockIdx = 0; 
            firIdxBlockIdx < entriesPerIdxBlock && filesizeRemaining > 0; 
            firIdxBlockIdx++
        ) {
            // 内部：直接索引。
            for (int idx = 0; filesizeRemaining > 0 && idx < entriesPerIdxBlock; idx++) {
                int entriesTargetByteOffset = sizeof(Block) 
                    * (
                        6 
                        + 2 * entriesPerIdxBlock 
                        + entriesPerIdxBlock * entriesPerIdxBlock * secIdxBlockIdx 
                        + entriesPerIdxBlock * firIdxBlockIdx 
                        + idx
                    );

                int blkIdx = getFreeBlock();
                if (blkIdx < 0) {
                    cout << "盘满。存盘失败。" << endl;
                    return false;
                }

                this->writeBlocks(buffer + entriesTargetByteOffset, blkIdx, 1);
                firstIdxBlockBuffer[idx] = blkIdx;

                filesizeRemaining -= sizeof(Block);
            } // for (int idx = 0; filesizeRemaining > 0 && idx < 6; idx++)

            int blkIdx = getFreeBlock();
            if (blkIdx < 0) {
                cout << "盘满。存盘失败。" << endl;
                return false;
            }
            this->writeBlocks((char*) firstIdxBlockBuffer, blkIdx, 1);
            secondIdxBlockBuffer[firIdxBlockIdx] = blkIdx;
        } // 内部：一级索引。

        int blkIdx = getFreeBlock();
        if (blkIdx < 0) {
            cout << "盘满。存盘失败。" << endl;
            return false;
        }
        this->writeBlocks((char*) secondIdxBlockBuffer, blkIdx, 1);
        inode.secondary_indirect_index[secIdxBlockIdx] = blkIdx;
    }

    return true;
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
}

void FileSystemAdapter::flush() {
    this->superBlock.writeToImg(this->fileStream);

    this->fileStream.clear();
    this->fileStream.seekp(
        this->superBlock.inode_zone_begin * sizeof(Block),
        ios::beg
    );

    fileStream.write((char*) this->inodes, this->superBlock.inode_zone_blocks * sizeof(Block));
}

void FileSystemAdapter::format() {
    // todo
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
    if (superBlock.s_nfree == 0) {
        return -1; // 无空盘块。
    } else if (superBlock.s_nfree >= 2) {
        return superBlock.s_free[--superBlock.s_nfree];
    } else {
        int result = superBlock.s_free[0];
        Block b;
        readBlock(b, result);
        memcpy(&superBlock.s_nfree, &b, 101 * sizeof(uint32_t));
        return result;
    }
}

void FileSystemAdapter::freeBlock(int idx) {
    if (superBlock.s_nfree < 100) {
        superBlock.s_free[superBlock.s_nfree++] = idx;
    } else {
        Block b;
        memcpy(&b, &superBlock.s_nfree, 101 * sizeof(uint32_t));
        writeBlock(b, superBlock.s_free[0]);
        superBlock.s_nfree = 1;
        superBlock.s_free[0] = idx;
    }
}

int FileSystemAdapter::getFreeInode() {
    if (superBlock.s_ninode == 0) {
        return -1; // 无空 inode。
    } else if (superBlock.s_ninode >= 2) {
        return superBlock.s_inode[--superBlock.s_nfree];
    } else {
        int result = superBlock.s_inode[0];
        Block b;
        readBlock(b, result);
        memcpy(&superBlock.s_ninode, &b, 101 * sizeof(uint32_t));
        return result;
    }
}

void FileSystemAdapter::freeInode(int idx, bool freeBlocks) {
    if (freeBlocks) {
        Inode& inode = this->inodes[idx]; // todo
    }

    if (superBlock.s_ninode < 100) {
        superBlock.s_inode[superBlock.s_ninode++] = idx;
    } else {
        Block b;
        memcpy(&b, &superBlock.s_ninode, 101 * sizeof(uint32_t));
        writeBlock(b, superBlock.s_inode[0]);
        superBlock.s_ninode = 1;
        superBlock.s_inode[0] = idx;
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

        // 文件大小。
        cout << setw(8) << entryInode.d_size;

        // 文件名。
        cout << " " << entry.m_name << endl;
        
        cout << resetiosflags(ios::right);
    }
}

void FileSystemAdapter::ls(const Inode& inode) {
    this->ls(InodeDirectory(inode, *this));
}

void FileSystemAdapter::ls(const vector<string>& pathSegments, bool fromRoot) {
    int currInodeIdx = fromRoot ? 0 : this->userInodeIdx;

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

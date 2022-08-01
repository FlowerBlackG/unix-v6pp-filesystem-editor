/*
 * 目录 Inode 节点。
 * 2051565 龚天遥
 * 创建于 2022年8月1日。
 */

#include <stdexcept>
#include <cmath>
#include "../include/FileSystemAdapter.h"
#include "../include/MacroDefines.h"
#include "../include/MachineProps.h"
#include "../include/structures/Inode.h"
#include "../include/structures/InodeDirectory.h"

using namespace std;

InodeDirectory::InodeDirectory(const DiskInode& inode, FileSystemAdapter& adapter) {
    if (inode.file_type != DiskInode::FileType::DIR) {
    //    throw runtime_error("trying to treat a non-dir inode as dir.");
    }

    int dirFileSize = inode.d_size;
    this->entries = new DirectoryEntry[dirFileSize / sizeof(DirectoryEntry)];
    
    int sizeRemaining = dirFileSize; // 剩下待读入的字节数。
    int sizeRead = 0;

    // 直接索引读入。
    for (int idx = 0; sizeRemaining > 0 && idx < 6; idx++) {
        adapter.readBlocks(
            ((char*) entries) + MachineProps::BLOCK_SIZE * idx,
            inode.direct_index[idx], 1
        );

        sizeRemaining -= MachineProps::BLOCK_SIZE;
    }

    // 每个索引块的块条目数。
    const int entriesPerIdxBlock = MachineProps::BLOCK_SIZE / sizeof(uint32_t);
    uint32_t firstIdxBlockBuffer[entriesPerIdxBlock]; // 一级索引块缓存。
    uint32_t secondIdxBlockBuffer[entriesPerIdxBlock]; // 二级索引块缓存。
    // 上面这两个东西总共占用 1KB 栈空间，问题不大。

    // 一级索引读入。
    for (int firIdxBlockIdx = 0; firIdxBlockIdx < 2 && sizeRemaining > 0; firIdxBlockIdx++) {

        adapter.readBlocks(
            (char*) firstIdxBlockBuffer,
            inode.indirect_index[firIdxBlockIdx], 1
        );

        // 内部：直接索引。
        for (int idx = 0; sizeRemaining > 0 && idx < entriesPerIdxBlock; idx++) {
            int entriesTargetByteOffset = MachineProps::BLOCK_SIZE 
                * (6 + entriesPerIdxBlock * firIdxBlockIdx + idx);

            adapter.readBlocks(
                ((char*) entries) + entriesTargetByteOffset,
                inode.direct_index[idx], 1
            );

            sizeRemaining -= MachineProps::BLOCK_SIZE;
        } // for (int idx = 0; sizeRemaining > 0 && idx < 6; idx++)
    } // for (int firIdxBlockIdx = 0; firIdxBlockIdx < 2 && sizeRemaining > 0; firIdxBlockIdx++)

    // 二级索引读入。
    for (int secIdxBlockIdx = 0; secIdxBlockIdx < 2 && sizeRemaining > 0; secIdxBlockIdx++) {
        adapter.readBlocks(
            (char*) secondIdxBlockBuffer, 
            inode.secondary_indirect_index[secIdxBlockIdx], 1
        );

        // 内部：一级索引。
        for (
            int firIdxBlockIdx = 0; 
            firIdxBlockIdx < entriesPerIdxBlock && sizeRemaining > 0; 
            firIdxBlockIdx++
        ) {

            adapter.readBlocks(
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

                adapter.readBlocks(
                    ((char*) entries) + entriesTargetByteOffset,
                    inode.direct_index[idx], 1
                );

                sizeRemaining -= MachineProps::BLOCK_SIZE;
            } // for (int idx = 0; sizeRemaining > 0 && idx < 6; idx++)
        } // 内部：一级索引。
    }

} // InodeDirectory::InodeDirectory

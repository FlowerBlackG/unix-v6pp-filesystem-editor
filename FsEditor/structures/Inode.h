/*
 * Inode。
 * 2051565 龚天遥
 * 创建于 2022年8月1日。
 */

#pragma once

#include <cstdint>
#include <fstream>
#include "../MacroDefines.h"


/**
 * 硬盘 Inode 结构。
 */
class Inode {
public:
    enum FileType {
        NORMAL = 0,
        CHAR_DEV = 1,
        DIR = 2,
        BLOCK_DEV = 3
    };

public:
    /* ------------ uint32_t d_mode : 32 ------------ */
    uint16_t permission_others : 3;
    uint16_t permission_group : 3;
    uint16_t permission_owner : 3;
    
    uint16_t isvtx : 1;
    uint16_t isgid : 1;
    uint16_t isuid : 1;

    /** 1: 大型或巨型文件。0: 小文件。 */
    uint16_t ilarg : 1;

    /**
     * 文件类型编码。
     * 00 普通数据文件。01 字符设备文件。
     * 10 目录文件。11 块设备文件。
     */
    uint16_t file_type : 2;

    /**
     * 是否已被分配。
     */
    uint16_t ialloc : 1;

    uint16_t d_mode_paddings = 0;

    uint32_t d_nlink;
    uint16_t d_uid;
    uint16_t d_gid;

    /** 文件大小。单位：字节。 */
    uint32_t d_size;

    /* ------------ uint32_t d_addr[10] ------------ */
    uint32_t direct_index[6]; // d_addr[0..5] 直接索引。
    uint32_t indirect_index[2]; // d_addr[6..7] 一级索引。
    uint32_t secondary_indirect_index[2]; // 二级索引。

    uint32_t d_atime;
    uint32_t d_mtime;

public:
    Inode() {
        // nothing to do.
    }

    /**
     * 从硬盘读取一个 Inode 信息。
     * 读取失败不会有特别反应。即：一定要保证输入信息是正确的。
     * 
     * @param f 文件流。必须可读且空间足够。
     * @param blockOffset 盘块偏移。
     */
    Inode(std::fstream& f, const int blockOffset) {
        this->loadFromImg(f, blockOffset);
    }

    void loadEmptyProfile();

public:
    /**
     * 将当前对象当作 char 数组看待。
     * @return char* 指向当前对象的指针。
     */
    inline char* asCharArray() {
        return (char*) this;
    }

    /**
     * 从磁盘映像文件读取一个 DiskInode 的信息。
     * 
     * @param f 文件流。需要保证是打开的，可读的，且空间足够。
     * @param blockOffset 盘块偏移。从第几个盘块开始操作。
     * @return true 读取成功。
     * @return false 读取失败。也可能是没完全读入。
     *         注意，读入失败可能会带来未定义的行为，不一定返回false。
     */
    bool loadFromImg(std::fstream& f, const int blockOffset);
    
    /**
     * 将当前对象写到磁盘映像文件中。
     * 
     * @param f 文件流。需要保证是打开的，可写的，且空间足够。
     * @param blockOffset 盘块偏移。从第几个盘块开始操作。
     * @return true 写入成功。
     * @return false 写入失败。也可能是没完全写入。
     *         注意，写入失败可能会带来未定义的行为，不一定返回false。
     */
    bool writeToImg(std::fstream& f, const int blockOffset);
} __packed;

#if 0
static void __check_size() {
    sizeof(Inode); // 64 字节。
}
#endif

/*
 * SuperBlock 结构。
 * 2051565 龚天遥
 * 创建于 2022年8月1日。
 */

#pragma once

#include <cstdint>
#include <fstream>
#include "../MacroDefines.h"
#include "../MachineProps.h"

/**
 * SuperBlock 结构。
 */
class SuperBlock {
public:
    /** 外存 Inode 区占用的盘块数。 */
    uint32_t s_isize = MachineProps::INODE_ZONE_BLOCKS;

    /** 盘块总数。 */
    uint32_t s_fsize = MachineProps::diskBlocks();

    /** 直接管理的空闲盘块数量。 */
    uint32_t s_nfree;

    /** 直接管理的空闲盘块索引表。 */
    uint32_t s_free[100];

    /** 直接管理的空闲外存 Inode 数量。 */
    uint32_t s_ninode;
    
    /** 直接管理的空闲外存 Inode 索引表。 */
    uint32_t s_inode[100];

    
    /** 封锁空闲盘块索引表标志。 */
    uint32_t s_flock;

    /** 封锁空间 Inode 表标志。 */
    uint32_t s_ilock;

    
    /** 内存中 SuperBlock 副本被修改标志。 */
    uint32_t s_fmod = 0;
    
    /** 本文件系统只能读出。 */
    uint32_t s_ronly = 0;
    
    /** 最近一次更新时间。 */
    uint32_t s_time;
    
    /* ------------ padding 区。 ------------ */

    /** padding 区是否被修改过。 */
    uint32_t padding_changed = 1;

    /** 磁盘总扇区数。 */
    uint32_t disk_sector_count = MachineProps::CYLINDERS
        * MachineProps::HEADS * MachineProps::SECTORS_PER_TRACK;

    /** Inode 区起始位置（单位：扇区）。 */
    uint32_t inode_zone_begin = MachineProps::SUPER_BLOCK_ZONE_BLOCKS 
        + MachineProps::KERNEL_AND_BOOT_BLOCKS;

    // 备注：此处包括后续部分，扇区和盘块单位有些混乱。然而，Unix V6++内，
    //      盘块与扇区的大小一致，所以可以先忽略这个问题（摆烂）。

    /** Inode 区域大小（单位：盘块）。 */
    uint32_t inode_zone_blocks = MachineProps::INODE_ZONE_BLOCKS;

    /** data 区起始位置（单位：盘块）。 */
    uint32_t data_zone_begin = inode_zone_begin + inode_zone_blocks;

    /** data 区大小（单位：盘块）。 */
    uint32_t data_zone_blocks = disk_sector_count 
        - data_zone_begin - MachineProps::SWAP_ZONE_BLOCKS;

    /** 交换区起始位置。 */
    uint32_t swap_zone_begin = data_zone_begin + data_zone_blocks;

    /** 交换区大小。 */
    uint32_t swap_zone_blocks = MachineProps::SWAP_ZONE_BLOCKS;

    /** 空白填充。 */
    uint32_t blank_paddings[39] = {0};

public:
    /* ------------ 方法区。 ------------ */

    /**
     * 将当前对象当作 char 数组看待。
     * @return char* 指向当前对象的指针。
     */
    inline char* asCharArray() {
        return (char*) this;
    }

    /**
     * 从磁盘映像文件读取一个 SuperBlock 结构。
     * 
     * @param f 文件流。需要保证是打开的，可读的，且空间足够。
     * @param blockOffset 块偏移。
     * @return true 读入成功。
     * @return false 读入失败。也可能是没完全读入。
     *         注意，读入失败可能会带来未定义的行为，不一定返回false。
     */
    bool loadFromImg(
        std::fstream& f, 
        const int blockOffset = MachineProps::KERNEL_AND_BOOT_BLOCKS
    );

    /**
     * 将当前对象写到磁盘映像文件中。
     * 
     * @param f 文件流。需要保证是打开的，可写的，且空间足够。
     * @param blockOffset 块偏移。
     * @return true 写入成功。
     * @return false 写入失败。也可能是没完全写入。
     *         注意，写入失败可能会带来未定义的行为，不一定返回false。
     */
    bool writeToImg(
        std::fstream& f, 
        const int blockOffset = MachineProps::KERNEL_AND_BOOT_BLOCKS
    );

    void loadDefaultProfile();
} __packed;


#if 0
static void __check_size() {
    sizeof(SuperBlock); // 应为 1024。
}
#endif

/*
 * 宏定义：磁盘硬件信息。
 * 2051565 龚天遥
 * 创建于 2022年7月29日。
 */

#pragma once

/**
 * 硬盘硬件信息。
 */
class MachineProps {
public:
    /** 盘块尺寸（字节）。 */
    static const int BLOCK_SIZE = 512;

    /** 扇区尺寸（字节）。 */
    static const int SECTOR_SIZE = 512;
    
    /** 启动引导程序 bootloader 占用块数。 */
    static const int BOOT_LOADER_BLOCKS = 1;

    /** 盘柱面数。 */
    static const int CYLINDERS = 20;

    /** 盘磁头数。 */
    static const int HEADS = 16;

    /** 盘上每磁道扇区数。 */
    static const int SECTORS_PER_TRACK = 63;

    /** Super Block 区占用块数。 */
    static const int SUPER_BLOCK_ZONE_BLOCKS = 2;

    /** Inode 区占用块数。 */
    static const int INODE_ZONE_BLOCKS = 822;

    /** 交换区占用块数。 */
    static const int SWAP_ZONE_BLOCKS = 2160;

    /** 内核映像文件区占用块数。不含 bootloader。 */
    static const int KERNEL_BIN_BLOCKS = 199;

    /** 内核映像文件与启动引导区占用总块数。 */
    static const int KERNEL_AND_BOOT_BLOCKS = BOOT_LOADER_BLOCKS + KERNEL_BIN_BLOCKS;

    /** 推荐的硬盘大小。 */
    static inline unsigned long long diskSize() {
        return diskBlocks() * BLOCK_SIZE;
    }

    static inline unsigned long long diskBlocks() {
        return 1ULL * CYLINDERS * SECTORS_PER_TRACK * HEADS;
    }

private:

    // 禁止构造对象。

    MachineProps() {}
    MachineProps(const MachineProps&) {}
};

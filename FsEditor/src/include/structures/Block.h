/*
 * 一个盘块。
 * 2051565 龚天遥
 * 创建于 2022年8月1日。
 */

#pragma once

#include <cstdint>
#include "../MachineProps.h"

class Block {
public:
    uint8_t bytes[MachineProps::BLOCK_SIZE] = {0};

public:
    inline char* asCharArray() {
        return (char*) this;
    }

    inline const char* asConstCharArray() const {
        return (const char*) this;
    }
} __packed;

#if 0
static void __check_size() {
    sizeof(Block); // 512 字节。
}
#endif

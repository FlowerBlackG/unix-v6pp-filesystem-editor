/*
 * SuperBlock。
 * 2051565 龚天遥
 * 创建于 2022年8月1日。
 */

#include <fstream>
#include "../include/structures/SuperBlock.h"

using namespace std;

bool SuperBlock::writeToImg(fstream& f, const int blockOffset) {
    f.clear();
    f.seekp(
        blockOffset * MachineProps::BLOCK_SIZE, 
        ios::beg
    );
    f.write(this->asCharArray(), sizeof(SuperBlock));
    return true;
}

bool SuperBlock::loadFromImg(fstream& f, const int blockOffset) {
    f.clear();
    f.seekg(
        blockOffset * MachineProps::BLOCK_SIZE, 
        ios::beg
    );
    f.read(this->asCharArray(), sizeof(SuperBlock));
    return f.gcount() == sizeof(SuperBlock);
}

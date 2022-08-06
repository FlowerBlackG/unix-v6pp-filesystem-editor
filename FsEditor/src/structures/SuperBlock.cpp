/*
 * SuperBlock。
 * 2051565 龚天遥
 * 创建于 2022年8月1日。
 */

#include <fstream>
#include <cstring>
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

void SuperBlock::loadDefaultProfile() {
    SuperBlock sb;
    sb.s_ninode = 0;
    sb.s_nfree = 0;
    sb.s_ronly = 0;
    sb.s_ilock = sb.s_flock = 0;
    memcpy(this, &sb, sizeof(sb));
}

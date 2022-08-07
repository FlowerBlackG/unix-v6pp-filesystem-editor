/*
 * Inode。
 * 2051565 龚天遥
 * 创建于 2022年8月1日。
 */

#include "../include/MachineProps.h"
#include "../include/structures/Inode.h"
#include <fstream>
using namespace std;

bool Inode::loadFromImg(fstream& f, const int blockOffset) {
    f.clear();
    f.seekg(
        MachineProps::BLOCK_SIZE * blockOffset, 
        ios::beg
    );
    f.read(this->asCharArray(), sizeof(Inode));
    return f.gcount() == sizeof(Inode);
}

bool Inode::writeToImg(fstream& f, const int blockOffset) {
    f.clear();
    f.seekp(
        MachineProps::BLOCK_SIZE * blockOffset, 
        ios::beg
    );
    f.write(this->asCharArray(), sizeof(Inode));
    return true;
}

void Inode::loadEmptyProfile() {
    this->ialloc = 0;
    this->ilarg = 0;
    this->direct_index[0] = 0;
    this->d_size = 0;
}

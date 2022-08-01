/*
 * Inode。
 * 2051565 龚天遥
 * 创建于 2022年8月1日。
 */

#include "../include/MachineProps.h"
#include "../include/structures/Inode.h"
#include <fstream>
using namespace std;

bool DiskInode::loadFromImg(fstream& f, const int blockOffset) {
    f.clear();
    f.seekg(
        MachineProps::BLOCK_SIZE * blockOffset, 
        ios::beg
    );
    f.read(this->asCharArray(), sizeof(DiskInode));
    return f.tellg() == sizeof(DiskInode);
}

bool DiskInode::writeToImg(fstream& f, const int blockOffset) {
    f.clear();
    f.seekp(
        MachineProps::BLOCK_SIZE * blockOffset, 
        ios::beg
    );
    f.write(this->asCharArray(), sizeof(DiskInode));
    return f.tellp() == sizeof(DiskInode);
}


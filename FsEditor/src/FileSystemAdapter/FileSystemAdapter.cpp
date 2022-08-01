/*
 * 文件系统适配器。
 * 2051565 龚天遥
 * 创建于 2022年7月29日。
 */

#include <stdexcept>
#include <fstream>
#include <iostream>
#include "../include/FileSystemAdapter.h"
#include "../include/MachineProps.h"
#include "../include/structures/Inode.h"
#include "../include/structures/SuperBlock.h"
#include "../include/structures/Block.h"
#include "../include/structures/InodeDirectory.h"

using namespace std;

FileSystemAdapter::FileSystemAdapter(const char* filePath) {
    fileStream.open(filePath, ios::in | ios::out);
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
        fileStream.close();
    }
}

bool FileSystemAdapter::readBlock(Block& block, const int blockIdx) {
    return this->readBlocks(block.asCharArray(), blockIdx, 1);
}


bool FileSystemAdapter::readBlocks(char* buffer, const int blockIdx, const int blockCount) {
    fileStream.clear();
    fileStream.seekg(blockIdx * MachineProps::BLOCK_SIZE, ios::beg);
    fileStream.read(buffer, blockCount * MachineProps::BLOCK_SIZE);
    return fileStream.tellg() == blockCount * MachineProps::BLOCK_SIZE;
}

bool FileSystemAdapter::writeBlock(const Block& block, const int blockIdx) {
    return this->writeBlocks(block.asConstCharArray(), blockIdx, 1);
}


bool FileSystemAdapter::writeBlocks(const char* buffer, const int blockIdx, const int blockCount) {
    fileStream.clear();
    fileStream.seekp(blockIdx * MachineProps::BLOCK_SIZE, ios::beg);
    fileStream.write(buffer, blockCount * MachineProps::BLOCK_SIZE);
    return fileStream.tellp() == blockCount * MachineProps::BLOCK_SIZE;
}

void FileSystemAdapter::load() {
    this->superBlock.loadFromImg(this->fileStream);

    this->fileStream.clear();

    this->fileStream.seekg(
        this->superBlock.inode_zone_begin * MachineProps::BLOCK_SIZE,
        ios::beg
    );
    
    this->fileStream.read(
        (char*) this->diskInodes, 
        this->superBlock.inode_zone_blocks * MachineProps::BLOCK_SIZE
    );

    fileSystemLoaded = true;

    InodeDirectory id(diskInodes[0], *this);
    cout << sizeof(DiskInode) << endl;
    
    int x;
}

void FileSystemAdapter::format() {
    
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

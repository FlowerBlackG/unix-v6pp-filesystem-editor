/*
 * 模拟文件系统。
 * 2051565 龚天遥
 * 创建于 2022年7月29日。
 */

#include <stdexcept>
#include <fstream>
#include "../include/MockFileSystem.h"
#include "../include/MachineProps.h"

using namespace std;

MockFileSystem::MockFileSystem(const char* filePath) {
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

MockFileSystem::~MockFileSystem() {
    fileStream.close();
}

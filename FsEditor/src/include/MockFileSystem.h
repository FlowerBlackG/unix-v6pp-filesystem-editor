/*
 * 模拟文件系统 - 头文件。
 * 2051565 龚天遥
 * 创建于 2022年7月29日。
 */

#pragma once

#include <fstream>

class MockFileSystem {
public:
    /**
     * 构造函数。自动打开磁盘映像文件。
     * 
     * @param filePath 文件路径。需要保证该文件可以打开。
     * @exception runtime_error 文件打开失败。
     */
    MockFileSystem(const char* filePath);

    ~MockFileSystem();

    void format();

    /** 加载文件系统。 */
    void load();

    void writeKernel();
    void writeBootLoader();
private:
    std::fstream fileStream;

    /** 文件系统是否已经加载。 */
    bool fileSystemLoaded = false;
};

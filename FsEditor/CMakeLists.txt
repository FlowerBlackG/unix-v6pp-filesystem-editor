
cmake_minimum_required(VERSION 3.29.2)


string(TIMESTAMP build_time_iso8601 UTC)  # example: 2024-04-19T13:39Z
string(TIMESTAMP build_time_human_readable "%B %d, %Y %H:%M:%S %Z")
set(V6PP_BUILD_TIME_ISO8601 ${build_time_iso8601})
set(V6PP_BUILD_TIME_HUMAN_READABLE ${build_time_human_readable})



# 指定使用 C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)


# 头文件路径。
include_directories("${PROJECT_SOURCE_DIR}")


# GCC(g++) 编译选项。
# 无


#[[ 
    项目构造产物
]]
file(GLOB_RECURSE CPP_SOURCE_FILES *.cpp)

add_executable(fsedit ${CPP_SOURCE_FILES})

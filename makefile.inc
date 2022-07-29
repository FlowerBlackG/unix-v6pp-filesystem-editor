# 构建文件包含文件。
# 2051565 龚天遥
# 创建于 2022年7月29日

CC = g++ # 编译器。
MAKE = mingw32-make # make 软件。

CFLAGS := -Wall # 显示所有报错信息。
CFLAGS += -std=c++17 # 启用C++17，使用 filesystem 等库。

CFLAGS := $(strip $(CFLAGS)) # 删除多余空格。

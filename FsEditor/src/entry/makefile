# 进入点构建文件。
# 2051565 龚天遥
# 创建于 2022年7月29日

.DEFAULT_GOAL := all
include ../../../makefile.inc

TARGET = ..\..\objs\entry

SOURCE = $(wildcard *.cpp) # 定位所有cpp文件。
OBJS = $(patsubst %.cpp, $(TARGET)/%.o, $(SOURCE)) # 获取目标文件集。

$(TARGET)/%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: all
all: $(OBJS)

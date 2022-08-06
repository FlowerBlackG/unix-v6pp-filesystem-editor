# 文件扫描器构建文件。
# 2051565 龚天遥
# 创建于 2022年7月29日

.DEFAULT_GOAL := all
include ../../makefile.inc

OUTPUT_PATH = ..\..\output
OBJ_PATH = ..\objs

OBJS = $(OBJ_PATH)\main.o

$(OBJ_PATH)\main.o: main.cpp
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: all
all: $(OBJS)
	$(CC) $(CFLAGS) -o $(OUTPUT_PATH)/filescanner.exe $(OBJS)

.PHONY: clean
clean: $(OBJS)
	del $^

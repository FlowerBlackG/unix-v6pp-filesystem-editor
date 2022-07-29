# 文件系统读写器构建文件。
# 2051565 龚天遥
# 创建于 2022年7月29日

.DEFAULT_GOAL := all
include ../../makefile.inc

OUTPUT_PATH = ..\..\output
OBJ_PATH = ..\objs

OBJS = $(OBJ_PATH)/entry/main.o \
		$(OBJ_PATH)/MockFileSystem/MockFileSystem.o

DIRS = entry MockFileSystem
.PHONY: $(DIRS)
$(DIRS):
	$(MAKE) --directory=$@

.PHONY: all
all: $(DIRS)
	$(CC) -o $(OUTPUT_PATH)/fsedit.exe $(OBJS)	

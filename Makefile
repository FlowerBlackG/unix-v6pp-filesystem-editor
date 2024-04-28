
.DEFAULT_GOAL := all


.PHONY: prepare
prepare:
	mkdir -p target
	mkdir -p build/FsEditor
	mkdir -p build/FileScanner
	cd build/FsEditor && cmake -G"Unix Makefiles" ../../FsEditor
	cd build/FileScanner && cmake -G"Unix Makefiles" ../../FileScanner


.PHONY: build
build: prepare
	cd build/FsEditor && cmake --build . -- -j 4 && cp ./fsedit* ../../target/
	cd build/FileScanner && cmake --build . -- -j 1 && cp ./filescanner* ../../target/


.PHONY: clean
clean:
	rm -rf build
	rm -rf target


.PHONY: all
all: build
	@echo -e "\033[32mbuild success (fsedit & filescanner).\033[0m"

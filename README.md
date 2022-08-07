# unix-v6pp-filesystem-editor

Unix V6++ 文件系统编辑工具。

## 用法

前往 `output` 文件夹，将需要上传到文件系统内的文件放置到 programs 文件夹内，并在 output 同路径下放置 kernel.bin 和 boot.bin 文件。

之后，通过命令行 `./filescanner | ./fsedit c.img c` 完成系统盘的构建。

独立使用 fsedit 程序可以交互式地完成对磁盘映像文件的读写。

# 编译

## 平台

WSL(linux)下的vscode

## 注意

注意本实验以linux为平台

* 你可以首先删除源码中build文件夹中的内容，然后使用cmake命令重新编译
`/usr/bin/cmake --build 你存储源码的路径/Ubuntu_18.04.3_64_Desktop/build --config Debug --target all -j 18 --`
该指令会在Stop_Wait/bin/目录下生成stop_wait可执行文件，执行该文件即可
* 你也可以直接将可执行文件夹下的stop_wait放到源码文件下的Stop_Wait/bin/目录中，执行

第三方组件中的check_linux脚本和可执行文件放到同一目录，使其能找到stop_wait，它会生成output.txt和result.txt

你可以更改Stop_Wait/bin中的setting.txt，可执行文件会读取setting.txt的内容分别执行GBN、SR、TCP协议

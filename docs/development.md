# 开发注意事项

## 环境配置

运行`zsh ./script/setup.sh zsh`或`bash ./script/setup.sh`

## 代码风格

源文件命名采用lower_case_with_underscores

开发完成后一定要在使用`./script/format.sh`来格式化代码（注意不要用clang format格式化`.h, .S`的代码），本项目coding style基于llvm clang-format做了一些调整。

## 编译

使用`./script/build.sh`来编译

## 运行QEMU

使用`./script/qemu_run.sh`来运行

## QEMU Debug

使用`./script/qemu_debug.sh`来debug
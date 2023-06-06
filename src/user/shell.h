#pragma once
extern unsigned char _elf_shell[];
extern int _length_shell;
extern int get_shell_file(const char *file_name, unsigned char **binary, int *length);

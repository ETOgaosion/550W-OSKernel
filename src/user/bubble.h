#pragma once
extern unsigned char _elf_bubble[];
extern int _length_bubble;
typedef struct ElfFile {
    char *file_name;
    unsigned char *file_content;
    int *file_length;
} ElfFile;

#define ELF_FILE_NUM 1
extern ElfFile elf_files[1];
extern int get_bubble_file(const char *file_name, unsigned char **binary, int *length);

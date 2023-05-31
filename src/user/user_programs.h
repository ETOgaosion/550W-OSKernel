
#pragma once
extern unsigned char _elf_shell[];
extern int _length_shell;
#pragma once
extern unsigned char _elf_test_virtio[];
extern int _length_test_virtio;
#pragma once
extern unsigned char _elf_bubble[];
extern int _length_bubble;
#pragma once
typedef struct ElfFile {
    char *file_name;
    unsigned char *file_content;
    int *file_length;
} ElfFile;

#define ELF_FILE_NUM 3
extern ElfFile elf_files[3];
extern int get_elf_file(const char *file_name, unsigned char **binary, int *length);
extern int match_elf(char *file_name);

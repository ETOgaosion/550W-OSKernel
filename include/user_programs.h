
extern unsigned char _elf___test_test_shell_elf[];
int _length___test_test_shell_elf;
extern unsigned char _elf___test_test_fs_elf[];
int _length___test_test_fs_elf;
extern unsigned char _elf___test_test_bigfile_elf[];
int _length___test_test_bigfile_elf;
extern unsigned char _elf___test_emo_elf[];
int _length___test_emo_elf;
extern unsigned char _elf___test_test_multi_elf[];
int _length___test_test_multi_elf;
extern unsigned char _elf___test_check_elf[];
int _length___test_check_elf;
extern unsigned char _elf___test_fly_elf[];
int _length___test_fly_elf;
extern unsigned char _elf___test_test_all_elf[];
int _length___test_test_all_elf;
typedef struct ElfFile {
  char *file_name;
  unsigned char* file_content;
  int* file_length;
} ElfFile;

#define ELF_FILE_NUM 8
extern ElfFile elf_files[8];
extern int get_elf_file(const char *file_name, unsigned char **binary, int *length);


#pragma once
extern unsigned char _elf_shell[];
extern int _length_shell;
#pragma once
extern unsigned char _elf_openat[];
extern int _length_openat;
#pragma once
extern unsigned char _elf_execve[];
extern int _length_execve;
#pragma once
extern unsigned char _elf_dup2[];
extern int _length_dup2;
#pragma once
extern unsigned char _elf_mkdir_[];
extern int _length_mkdir_;
#pragma once
extern unsigned char _elf_write[];
extern int _length_write;
#pragma once
extern unsigned char _elf_getppid[];
extern int _length_getppid;
#pragma once
extern unsigned char _elf_unlink[];
extern int _length_unlink;
#pragma once
extern unsigned char _elf_waitpid[];
extern int _length_waitpid;
#pragma once
extern unsigned char _elf_clone[];
extern int _length_clone;
#pragma once
extern unsigned char _elf_test_echo[];
extern int _length_test_echo;
#pragma once
extern unsigned char _elf_getdents[];
extern int _length_getdents;
#pragma once
extern unsigned char _elf_sleep[];
extern int _length_sleep;
#pragma once
extern unsigned char _elf_yield[];
extern int _length_yield;
#pragma once
extern unsigned char _elf_dup[];
extern int _length_dup;
#pragma once
extern unsigned char _elf_mount[];
extern int _length_mount;
#pragma once
extern unsigned char _elf_umount[];
extern int _length_umount;
#pragma once
extern unsigned char _elf_gettimeofday[];
extern int _length_gettimeofday;
#pragma once
extern unsigned char _elf_times[];
extern int _length_times;
#pragma once
extern unsigned char _elf_getpid[];
extern int _length_getpid;
#pragma once
extern unsigned char _elf_chdir[];
extern int _length_chdir;
#pragma once
extern unsigned char _elf_wait[];
extern int _length_wait;
#pragma once
extern unsigned char _elf_fork[];
extern int _length_fork;
#pragma once
extern unsigned char _elf_pipe[];
extern int _length_pipe;
#pragma once
extern unsigned char _elf_exit[];
extern int _length_exit;
#pragma once
extern unsigned char _elf_fstat[];
extern int _length_fstat;
#pragma once
extern unsigned char _elf_uname[];
extern int _length_uname;
#pragma once
extern unsigned char _elf_getcwd[];
extern int _length_getcwd;
#pragma once
extern unsigned char _elf_close[];
extern int _length_close;
#pragma once
extern unsigned char _elf_mmap[];
extern int _length_mmap;
#pragma once
extern unsigned char _elf_read[];
extern int _length_read;
#pragma once
extern unsigned char _elf_open[];
extern int _length_open;
#pragma once
extern unsigned char _elf_munmap[];
extern int _length_munmap;
#pragma once
extern unsigned char _elf_brk[];
extern int _length_brk;
#pragma once
typedef struct ElfFile {
  char *file_name;
  unsigned char* file_content;
  int* file_length;
} ElfFile;

#define ELF_FILE_NUM 34
extern ElfFile elf_files[34];
extern int get_elf_file(const char *file_name, unsigned char **binary, int *length);
extern int match_elf(char *file_name);

#include <user/user_programs.h>
#include <user/bubble.h>
#include <user/shell.h>
#include <lib/string.h>
#include <fs/fs.h>
#include <os/mm.h>

#define ELF_BUF_SIZE 10

unsigned char *elf_files_binary_buf[ELF_BUF_SIZE] = {0};
int elf_files_len_buf[ELF_BUF_SIZE] = {0};
char file_names_in_buf[ELF_BUF_SIZE][50] = {0};
int elf_files_buf_hot[ELF_BUF_SIZE] = {0};

int get_elf_file(const char *file_name, unsigned char **binary, int *length) {
    if (k_strcmp(file_name, "bubble") == 0) {
        return get_bubble_file(file_name, binary, length);
    }
    if (k_strcmp(file_name, "shell") == 0) {
        return get_shell_file(file_name, binary, length);
    }
    for (int i = 0; i < ELF_BUF_SIZE; i++) {
        if (strcmp(file_names_in_buf[i], file_name) == 0) {
            *binary = elf_files_binary_buf[i];
            *length = elf_files_len_buf[i];
            if (elf_files_buf_hot[i] < INT_MAX) {
                elf_files_buf_hot[i]++;
            }
            return 0;
        }
    }
    int ret = fs_load_file(file_name, (uint8_t **)binary, length);
    int cur = 0, hot = elf_files_buf_hot[0];
    for (int i = 1; i < ELF_BUF_SIZE; i++) {
        if (hot > elf_files_buf_hot[i]) {
            cur = i;
            hot = elf_files_buf_hot[i];
        }
    }
    k_strcpy(file_names_in_buf[cur], file_name);
    elf_files_len_buf[cur] = *length;
    elf_files_binary_buf[cur] = *binary;
    elf_files_buf_hot[cur] = 1;
    cur = (cur + 1) % ELF_BUF_SIZE;
    return ret;
}

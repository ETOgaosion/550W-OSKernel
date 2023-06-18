#include <user/user_programs.h>
#include <user/bubble.h>
#include <user/shell.h>
#include <lib/string.h>
#include <fs/fs.h>

int get_elf_file(const char *file_name, unsigned char **binary, int *length) {
    if (k_strcmp(file_name, "bubble") == 0) {
        return get_bubble_file(file_name, binary, length);
    }
    if (k_strcmp(file_name, "shell") == 0) {
        return get_shell_file(file_name, binary, length);
    }
    return fs_load_file(file_name, (uint8_t **)binary, length);
}
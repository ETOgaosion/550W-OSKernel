#include <user/user_programs.h>
#include <user/bubble.h>
#include <lib/string.h>
#include <fs/fs.h>

int get_elf_file(const char *file_name, unsigned char **binary, int *length) {
    if (k_strcmp(file_name, "bubble") == 0) {
        return get_bubble_file(file_name, binary, length);
    }
    return k_load_file(file_name, (uint8_t **)binary, length);
}
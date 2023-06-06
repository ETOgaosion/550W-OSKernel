#include <user/user_programs.h>
#include <fs/fs.h>

int get_elf_file(const char *file_name, unsigned char **binary, int *length) {
    return k_load_file(file_name, (uint8_t **)binary, length);
}
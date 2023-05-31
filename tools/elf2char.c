#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// replace all illegal character with `_`
char *file_name_to_variable_name(char *name) {
    int l = strlen(name);
    for (int i = 0; i < l; ++i) {
        if (!isalpha(name[i]) && !isdigit(name[i])) {
            name[i] = '_';
        }
    }
    return name;
}

static inline char to_hex(int num) {
    if (0 <= num && num <= 9) {
        return '0' + num;
    } else {
        return 'a' + num - 10;
    }
}

char *escaped_file_content(FILE *f, int *file_size) {
    static char *buff = NULL;
    static int size = 1;
    int length = 0;
    int ch;

    *file_size = 0;
    while ((ch = fgetc(f)) != EOF) {
        if (length + 6 >= size) {
            size *= 2;
            buff = realloc(buff, size);
        }

        assert(buff != NULL);

        *file_size = (*file_size) + 1;
        buff[length++] = '0';
        buff[length++] = 'x';
        buff[length++] = to_hex((ch & 0xf0) >> 4);
        buff[length++] = to_hex(ch & 0xf);
        buff[length++] = ',';

        if ((*file_size) % 16 == 0) {
            buff[length++] = '\n';
        }
    }
    return buff;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: elf2char [-o] [Exec Name] [--header-only] <File Name> <File "
               "Name> ...");
        return -1;
    }

    int is_header_only = 0;
    int defined_kernel_call_name = 0;
    char kernel_call_name[20];

    for (int i = 0; i < argc; ++i) {
        if (strcmp(argv[i], "--header-only") == 0) {
            is_header_only = 1;
            break;
        }
    }

    for (int i = 0; i < argc - 1; ++i) {
        if (strcmp(argv[i], "-o") == 0) {
            strcpy(kernel_call_name, argv[i + 1]);
            defined_kernel_call_name = 1;
            break;
        }
    }

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--header-only") == 0) {
            continue;
        }

        if (strcmp(argv[i], "-o") == 0) {
            i += 1;
            continue;
        }

        FILE *f = fopen(argv[i], "rb");
        if (f == NULL) {
            printf("ERROR: Unable to open %s\n", argv[i]);
            return -1;
        }

        if (is_header_only) {
            printf("#pragma once\n");
            if (defined_kernel_call_name) {
                printf("extern unsigned char _elf_%s[];\n", file_name_to_variable_name(kernel_call_name));
                // argv[i] already changed by
                // file_name_to_variable_name
                printf("extern int _length_%s;\n", kernel_call_name);
            } else {
                printf("extern unsigned char _elf_%s[];\n", file_name_to_variable_name(argv[i]));
                // argv[i] already changed by
                // file_name_to_variable_name
                printf("extern int _length_%s;\n", argv[i]);
            }
        } else {
            int file_size = 0;
            printf("#include <user/user_programs.h>\n");
            if (defined_kernel_call_name) {
                printf("unsigned char _elf_%s[] = {\n%s\n};\n", file_name_to_variable_name(kernel_call_name), escaped_file_content(f, &file_size));
                // argv[i] already changed by
                // file_name_to_variable_name
                printf("int _length_%s = %d;\n", kernel_call_name, file_size);
            } else {
                printf("unsigned char _elf_%s[] = {\n%s\n};\n", file_name_to_variable_name(kernel_call_name), escaped_file_content(f, &file_size));
                // argv[i] already changed by
                // file_name_to_variable_name
                printf("int _length_%s = %d;\n", argv[i], file_size);
            }
        }
        fclose(f);
    }

    return 0;
}

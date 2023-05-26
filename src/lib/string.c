#include <lib/string.h>

int k_strlen(const char *src) {
    int i;
    for (i = 0; src[i] != '\0'; i++) {}
    return i;
}

void k_memcpy(uint8_t *dest, const uint8_t *src, uint32_t len) {
    for (; len != 0; len--) {
        *dest++ = *src++;
    }
}

void k_memset(void *dest, uint8_t val, uint32_t len) {
    uint8_t *dst = (uint8_t *)dest;

    for (; len != 0; len--) {
        *dst++ = val;
    }
}

void k_bzero(void *dest, uint32_t len) {
    k_memset(dest, 0, len);
}

int k_strcmp(const char *str1, const char *str2) {
    while (*str1 && *str2) {
        if (*str1 != *str2) {
            return (*str1) - (*str2);
        }
        ++str1;
        ++str2;
    }
    return (*str1) - (*str2);
}

char *k_strcpy(char *dest, const char *src) {
    char *tmp = dest;

    while (*src) {
        *dest++ = *src++;
    }

    *dest = '\0';

    return tmp;
}

char *k_strcat(char *dest, const char *src) {
    char *tmp = dest;

    while (*dest != '\0') {
        dest++;
    }
    while (*src) {
        *dest++ = *src++;
    }

    *dest = '\0';

    return tmp;
}

int k_strlistlen(char *src[]) {
    int num = 0;
    if (src) {
        while (src[num]) {
            num++;
        }
    }
    return num;
}
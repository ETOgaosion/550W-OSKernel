#pragma once

#include <common/type.h>

void memcpy(uint8_t *dest, const uint8_t *src, uint32_t len);
void kmemset(void *dest, uint8_t val, uint32_t len);
void kbzero(void *dest, uint32_t len);
int strcmp(const char *str1, const char *str2);
char *kstrcpy(char *dest, const char *src);
char *kstrcat(char *dest, const char *src);
int kstrlen(const char *src);

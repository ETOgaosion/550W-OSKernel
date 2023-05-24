#pragma once

#include <common/types.h>

void k_memcpy(uint8_t *dest, const uint8_t *src, uint32_t len);
void k_memset(void *dest, uint8_t val, uint32_t len);
void k_bzero(void *dest, uint32_t len);
int k_strcmp(const char *str1, const char *str2);
char *k_strcpy(char *dest, const char *src);
char *k_strcat(char *dest, const char *src);
int k_strlen(const char *src);
int k_strlistlen(char *src[]);
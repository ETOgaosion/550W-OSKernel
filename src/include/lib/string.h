#pragma once

#include <common/types.h>

#define NUMBER_OF_DIGITS 64

int k_isspace(int c);
int k_isdigit(int c);
int k_isalpha(int x);
int k_isupper(int x);

long k_atol(const char *str);
int k_atoi(const char *s);

void k_memcpy(uint8_t *dest, const uint8_t *src, uint32_t len);
void k_memset(void *dest, uint8_t val, uint32_t len);
void k_bzero(void *dest, uint32_t len);

int k_strcmp(const char *str1, const char *str2);
int k_strncmp(const char *_l, const char *_r, size_t n);
char *k_strcpy(char *dest, const char *src);
char *k_strncpy(char *restrict d, const char *restrict s, size_t n);
char *k_strcat(char *dest, const char *src);
int k_strlen(const char *src);
size_t k_strnlen(const char *s, size_t n);
int k_strlistlen(char *src[]);
char *k_strtok(char *substr, char *str, const char delim, int length);
long k_strtol(const char *nptr, char **endptr, register int base);
void k_ultoa(unsigned long value, char *string, int radix);
void k_ltoa(long value, char *string, int radix);
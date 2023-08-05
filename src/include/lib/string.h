#pragma once

#include <common/types.h>

#define NUMBER_OF_DIGITS 64

#define k_isspace(c) isspace((int)c)
#define k_isdigit(c) isdigit((int)c)
#define k_isalpha(x) isalpha((int)x)
#define k_isupper(x) isupper((int)x)

int isspace(int c);
int isdigit(int c);
int isalpha(int x);
int isupper(int x);

#define k_atol(str) atol((const char *)str)
#define k_atoi(s) atoi((const char *)s)

long atol(const char *str);
int atoi(const char *s);

#define k_strtol(nptr, endptr, base) strtol((const char *)nptr, (char **)endptr, (register int)base)
#define k_ultoa(value, string, radix) ultoa((unsigned long)value, (char *)string, (int)radix)
#define k_ltoa(value, string, radix) ltoa((long)value, (char *)string, (int)radix)

long strtol(const char *nptr, char **endptr, register int base);
void ultoa(unsigned long value, char *string, int radix);
void ltoa(long value, char *string, int radix);

#define k_memcpy(dest, src, len) memcpy((uint8_t *)dest, (const uint8_t *)src, (uint32_t)len)
#define k_memset(dest, val, len) memset((void *)dest, (uint8_t)val, (uint32_t)len)
#define k_memcmp(ptr1, ptr2, num) memcmp((const void *)ptr1, (const void *)ptr2, (size_t)num)
#define k_bzero(dest, len) bzero((void *)dest, (uint32_t)len)

void memcpy(uint8_t *dest, const uint8_t *src, uint32_t len);
void memset(void *dest, uint8_t val, uint32_t len);
int memcmp(const void *ptr1, const void *ptr2, size_t num);
void bzero(void *dest, uint32_t len);

#define k_strcmp(str1, str2) strcmp((const char *)str1, (const char *)str2)
#define k_strncmp(_l, _r, n) strncmp((const char *)_l, (const char *)_r, (size_t)n)
#define k_strcpy(dest, src) strcpy((char *)dest, (const char *)src);
#define k_strncpy(d, s, n) strncpy((char *restrict)d, (const char *restrict)s, (size_t)n)
#define k_strcat(dest, src) strcat((char *)dest, (const char *)src)
#define k_strlen(src) strlen((const char *)src)
#define k_strnlen(s, n) strnlen((const char *)s, (size_t)n)
#define k_strlistlen(src) strlistlen(src)
#define k_strtok(substr, str, delim, length) strtok((char *)substr, (char *)str, (const char)delim, (int)length)

int strcmp(const char *str1, const char *str2);
int strncmp(const char *_l, const char *_r, size_t n);
char *strcpy(char *dest, const char *src);
char *strncpy(char *restrict d, const char *restrict s, size_t n);
char *strcat(char *dest, const char *src);
int strlen(const char *src);
size_t strnlen(const char *s, size_t n);
int strlistlen(char *src[]);
char *strtok(char *substr, char *str, const char delim, int length);
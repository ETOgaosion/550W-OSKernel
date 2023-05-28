#pragma once

#include <stddef.h>

int isspace(int c);
int isdigit(int c);
int isalpha(int x);
int isupper(int x);
long atol(const char *str);
int atoi(const char *s);
void *memset(void *dest, int c, size_t n);
void bzero(void *dest, uint32_t len);
void *memcpy(uint8_t *dest, const uint8_t *src, uint32_t len);
int memcmp(const void *ptr1, const void *ptr2, uint32_t num);

int strcmp(const char *l, const char *r);
int strncmp(const char *_l, const char *_r, size_t n);
size_t strlen(const char *);
size_t strnlen(const char *s, size_t n);
char *strcat(char *dest, const char *src);
char *strcpy(char *restrict d, const char *s);
char *strncpy(char *restrict d, const char *restrict s, size_t n);
char *strtok(char *substr, char *str, const char delim, int length);
long strtol(const char *nptr, char **endptr, register int base);

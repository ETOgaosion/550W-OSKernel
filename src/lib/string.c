#include <lib/string.h>

#define SS (sizeof(size_t))
#define ALIGN (sizeof(size_t) - 1)
#define ONES ((size_t)-1 / UCHAR_MAX)
#define HIGHS (ONES * (UCHAR_MAX / 2 + 1))
#define HASZERO(x) (((x)-ONES) & ~(x)&HIGHS)

int isspace(int c) {
    return c == ' ' || (unsigned)c - '\t' < 5;
}

int isdigit(int c) {
    return (unsigned)c - '0' < 10;
}

int isalpha(int x) {
    if ((x <= 'f' && x >= 'a') || (x <= 'F' && x >= 'A')) {
        return 1;
    } else {
        return 0;
    }
}

int isupper(int x) {
    if (x >= 'A' && x <= 'Z') {
        return 1;
    }
    return 0;
}

long atol(const char *str) {
    int base = 10;
    if ((str[0] == '0' && str[1] == 'x') || (str[0] == '0' && str[1] == 'X')) {
        base = 16;
        str += 2;
    }
    long ret = 0;
    while (*str != '\0') {
        if ('0' <= *str && *str <= '9') {
            ret = ret * base + (*str - '0');
        } else if (base == 16) {
            if ('a' <= *str && *str <= 'f') {
                ret = ret * base + (*str - 'a' + 10);
            } else if ('A' <= *str && *str <= 'F') {
                ret = ret * base + (*str - 'A' + 10);
            } else {
                return 0;
            }
        } else {
            return 0;
        }
        ++str;
    }
    return ret;
}

int atoi(const char *s) {
    int n = 0, neg = 0;
    while (isspace(*s)) {
        s++;
    }
    switch (*s) {
    case '-':
        neg = 1;
    case '+':
        s++;
    }
    /* Compute n as a negative number to avoid overflow on INT_MIN */
    while (isdigit(*s)) {
        n = 10 * n - (*s++ - '0');
    }
    return neg ? n : -n;
}

void memcpy(uint8_t *dest, const uint8_t *src, uint32_t len) {
    for (; len != 0; len--) {
        *dest++ = *src++;
    }
}

void memset(void *dest, uint8_t val, uint32_t len) {
    uint8_t *dst = (uint8_t *)dest;

    for (; len != 0; len--) {
        *dst++ = val;
    }
}

int memcmp(const void *ptr1, const void *ptr2, size_t num) {
    for (int i = 0; i < num; ++i) {
        if (((char *)ptr1)[i] != ((char *)ptr2)[i]) {
            return ((char *)ptr1)[i] - ((char *)ptr2)[i];
        }
    }
    return 0;
}

void bzero(void *dest, uint32_t len) {
    memset(dest, 0, len);
}

int strcmp(const char *str1, const char *str2) {
    while (*str1 && *str2) {
        if (*str1 != *str2) {
            return (*str1) - (*str2);
        }
        ++str1;
        ++str2;
    }
    return (*str1) - (*str2);
}

int strncmp(const char *_l, const char *_r, size_t n) {
    const unsigned char *l = (void *)_l, *r = (void *)_r;
    if (!n--) {
        return 0;
    }
    for (; *l && *r && n && *l == *r; l++, r++, n--)
        ;
    return *l - *r;
}

char *strcpy(char *dest, const char *src) {
    char *tmp = dest;

    while (*src) {
        *dest++ = *src++;
    }

    *dest = '\0';

    return tmp;
}

char *strncpy(char *restrict d, const char *s, size_t n) {
    typedef size_t __attribute__((__may_alias__)) word;
    word *wd;
    const word *ws;
    if (((uintptr_t)s & ALIGN) == ((uintptr_t)d & ALIGN)) {
        for (; ((uintptr_t)s & ALIGN) && n && (*d = *s); n--, s++, d++)
            ;
        if (!n || !*s) {
            goto tail;
        }
        wd = (void *)d;
        ws = (const void *)s;
        for (; n >= sizeof(size_t) && !HASZERO(*ws); n -= sizeof(size_t), ws++, wd++) {
            *wd = *ws;
        }
        d = (void *)wd;
        s = (const void *)ws;
    }
    for (; n && (*d = *s); n--, s++, d++)
        ;
tail:
    bzero(d, n);
    return d;
}

char *strcat(char *dest, const char *src) {
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

int strlen(const char *src) {
    int i;
    for (i = 0; src[i] != '\0'; i++) {}
    return i;
}

void *memchr(const void *src, int c, size_t n) {
    const unsigned char *s = src;
    c = (unsigned char)c;
    for (; ((uintptr_t)s & ALIGN) && n && *s != c; s++, n--)
        ;
    if (n && *s != c) {
        typedef size_t __attribute__((__may_alias__)) word;
        const word *w;
        size_t k = ONES * c;
        for (w = (const void *)s; n >= SS && !HASZERO(*w ^ k); w++, n -= SS)
            ;
        s = (const void *)w;
    }
    for (; n && *s != c; s++, n--)
        ;
    return n ? (void *)s : 0;
}

size_t strnlen(const char *s, size_t n) {
    const char *p = memchr(s, 0, n);
    return p ? p - s : n;
}

int strlistlen(char *src[]) {
    int num = 0;
    if (src) {
        while (src[num]) {
            num++;
        }
    }
    return num;
}

char *strtok(char *substr, char *str, const char delim, int length) {
    int len = strlen(str);
    int i;
    if (len == 0) {
        return NULL;
    }
    for (i = 0; i < len; i++) {
        if (str[i] != delim) {
            if (i < length) {
                substr[i] = str[i];
            }
        } else {
            substr[i] = 0;
            return &str[i + 1];
        }
    }
    return &str[i + 1];
}

#ifndef ULONG_MAX
#define ULONG_MAX ((unsigned long)(~0L)) /* 0xFFFFFFFF */
#endif
#ifndef LONG_MAX
#define LONG_MAX ((long)(ULONG_MAX >> 1)) /* 0x7FFFFFFF */
#endif
#ifndef LONG_MIN
#define LONG_MIN ((long)(~LONG_MAX)) /* 0x80000000 */
#endif

long strtol(const char *nptr, char **endptr, register int base) {
    register const char *s = nptr;
    register unsigned long acc;
    register int c;
    register unsigned long cutoff;
    register int neg = 0, any, cutlim;
    do {
        c = *s++;
    } while (isspace(c));
    if (c == '-') {
        neg = 1;
        c = *s++;
    } else if (c == '+') {
        c = *s++;
    }
    if ((base == 0 || base == 16) && c == '0' && (*s == 'x' || *s == 'X')) {
        c = s[1];
        s += 2;
        base = 16;
    }
    if (base == 0) {
        base = c == '0' ? 8 : 10;
    }
    cutoff = neg ? -(unsigned long)LONG_MIN : LONG_MAX;
    cutlim = cutoff % (unsigned long)base;
    cutoff /= (unsigned long)base;
    for (acc = 0, any = 0;; c = *s++) {
        if (isdigit(c)) {
            c -= '0';
        } else if (isalpha(c)) {
            c -= isupper(c) ? 'A' - 10 : 'a' - 10;
        } else {
            break;
        }
        if (c >= base) {
            break;
        }
        if (any < 0 || acc > cutoff || (acc == cutoff && c > cutlim)) {
            any = -1;
        } else {
            any = 1;
            acc *= base;
            acc += c;
        }
    }
    if (any < 0) {
        acc = neg ? LONG_MIN : LONG_MAX;
    } else if (neg) {
        acc = -acc;
    }
    if (endptr != 0) {
        *endptr = (char *)(any ? s - 1 : nptr);
    }
    return (acc);
}

void ultoa(unsigned long value, char *string, int radix) {
    unsigned char index;
    char buffer[NUMBER_OF_DIGITS + 1];

    index = NUMBER_OF_DIGITS;

    do {
        buffer[--index] = '0' + (value % radix);
        if (buffer[index] > '9') {
            buffer[index] += 'A' - ':';
        }
        value /= radix;
    } while (value != 0);

    do {
        *string++ = buffer[index++];
    } while (index < NUMBER_OF_DIGITS);

    *string = 0;
}

void ltoa(long value, char *string, int radix) {
    if (value < 0 && radix == 10) {
        *string++ = '-';
        ultoa(-value, string, radix);
    } else {
        ultoa(value, string, radix);
    }
}
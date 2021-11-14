#pragma once
#include <stdarg.h>

#define ALIGN_TO_PREVIOUS(n, to) (n & -to)
#define ALIGN_TO_NEXT(n, to) (((n - 1) & -to) + to)
#define IS_ALIGNED(n, to) ((n & (to - 1)) == 0)
#ifndef DEBUG
#define MUST_BE(condition, message)             \
    if (!(condition))                           \
    {                                           \
        console_print("[util] " message " \n"); \
        while (1)                               \
        {                                       \
        }                                       \
    }
#endif

unsigned long string_length(const char *str);
int string_compare(const char *a, const char *b);
void string_reverse(char *buffer, int size);
unsigned int string_from_i32(char *destination, int num, int base);
unsigned int string_from_i64(char *destination, long num, int base);
unsigned int string_from_u32(char *destination, unsigned int num, int base);
unsigned int string_from_u64(char *destination, unsigned long num, int base);
void string_format_args(char *destination, const char *format, va_list l);
void string_format(char *destination, const char *format, ...);
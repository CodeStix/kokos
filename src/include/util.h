#pragma once

#define ALIGN_TO_PREVIOUS(n, to) (n & -to)
#define ALIGN_TO_NEXT(n, to) (((n - 1) & -to) + to)
#define IS_ALIGNED(n, to) ((n & (to - 1)) == 0)

int string_length(const char *str);
int string_compare(const char *a, const char *b);
void reverse_buffer(char *buffer, int size);
void convert_i32_string(char *dest, int num, int base);
void convert_i64_string(char *dest, long num, int base);
void convert_u32_string(char *dest, unsigned int num, int base);
void convert_u64_string(char *dest, unsigned long num, int base);
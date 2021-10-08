#include "../include/util.h"

int string_length(const char *str)
{
    int i = 0;
    while (*str)
    {
        i++;
        str++;
    }
    return i;
}

int string_compare(const char *a, const char *b)
{
    while (1)
    {
        int d = *b - *a;
        if (d)
        {
            return d;
        }

        if (!*a || !*b)
        {
            return *a == *b ? 0 : 1;
        }

        a++;
        b++;
    }
}

void reverse_buffer(char *buffer, int size)
{
    for (int j = 0; j < size / 2; j++)
    {
        char t = buffer[j];
        buffer[j] = buffer[size - j - 1];
        buffer[size - j - 1] = t;
    }
}

// Converts num to a string with base in dest.
// Make sure there is at least 11 bytes available (max length of 32 bit integer with sign)
void convert_i32_string(char *dest, int num, int base)
{
    int i = 0;

    int isSigned = 0;
    if (num < 0)
    {
        num = -num;
        isSigned = 1;
    }

    // Convert number to ascii representation
    do
    {
        int digit = num % base;
        num /= base;
        dest[i++] = (digit > 9 ? 'A' + digit - 10 : '0' + digit);
    } while (num != 0);

    // Add sign character
    if (isSigned)
    {
        dest[i++] = '-';
    }

    reverse_buffer(dest, i);

    // Add null character
    dest[i] = '\0';
}

void convert_i64_string(char *dest, long num, int base)
{
    int i = 0;

    int isSigned = 0;
    if (num < 0)
    {
        num = -num;
        isSigned = 1;
    }

    // Convert number to ascii representation
    do
    {
        int digit = num % base;
        num /= base;
        dest[i++] = (digit > 9 ? 'A' + digit - 10 : '0' + digit);
    } while (num != 0);

    // Add sign character
    if (isSigned)
    {
        dest[i++] = '-';
    }

    reverse_buffer(dest, i);

    // Add null character
    dest[i] = '\0';
}

void convert_u32_string(char *dest, unsigned int num, int base)
{
    int i = 0;

    // Convert number to ascii representation
    do
    {
        int digit = num % base;
        num /= base;
        dest[i++] = (digit > 9 ? 'A' + digit - 10 : '0' + digit);
    } while (num != 0);

    reverse_buffer(dest, i);

    // Add null character
    dest[i] = '\0';
}

void convert_u64_string(char *dest, unsigned long num, int base)
{
    int i = 0;

    // Convert number to ascii representation
    do
    {
        int digit = num % base;
        num /= base;
        dest[i++] = (digit > 9 ? 'A' + digit - 10 : '0' + digit);
    } while (num != 0);

    reverse_buffer(dest, i);

    // Add null character
    dest[i] = '\0';
}

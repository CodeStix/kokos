#pragma once

#define ALIGN_TO_PREVIOUS(n, to) (n & -to)
#define ALIGN_TO_NEXT(n, to) (((n - 1) & -to) + to)
#define IS_ALIGNED(n, to) ((n & (to - 1)) == 0)

int string_length(char *str)
{
    int i = 0;
    while (*str)
    {
        i++;
        str++;
    }
    return i;
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

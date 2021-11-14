#include "kokos/util.h"
#include <stdarg.h>

unsigned long string_length(const char *str)
{
    unsigned long i = 0;
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

void string_reverse(char *buffer, int size)
{
    for (int j = 0; j < size / 2; j++)
    {
        char t = buffer[j];
        buffer[j] = buffer[size - j - 1];
        buffer[size - j - 1] = t;
    }
}

// Converts num to a string with base in destination.
// Make sure there is at least 11 bytes available (max length of 32 bit integer with sign)
unsigned int string_from_i32(char *destination, int num, int base)
{
    unsigned int i = 0;

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
        destination[i++] = (digit > 9 ? 'A' + digit - 10 : '0' + digit);
    } while (num != 0);

    // Add sign character
    if (isSigned)
    {
        destination[i++] = '-';
    }

    string_reverse(destination, i);

    // Add null character
    destination[i] = '\0';

    return i;
}

unsigned int string_from_i64(char *destination, long num, int base)
{
    unsigned int i = 0;

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
        destination[i++] = (digit > 9 ? 'A' + digit - 10 : '0' + digit);
    } while (num != 0);

    // Add sign character
    if (isSigned)
    {
        destination[i++] = '-';
    }

    string_reverse(destination, i);

    // Add null character
    destination[i] = '\0';

    return i;
}

unsigned int string_from_u32(char *destination, unsigned int num, int base)
{
    int i = 0;

    // Convert number to ascii representation
    do
    {
        int digit = num % base;
        num /= base;
        destination[i++] = (digit > 9 ? 'A' + digit - 10 : '0' + digit);
    } while (num != 0);

    string_reverse(destination, i);

    // Add null character
    destination[i] = '\0';

    return i;
}

unsigned int string_from_u64(char *destination, unsigned long num, int base)
{
    int i = 0;

    // Convert number to ascii representation
    do
    {
        int digit = num % base;
        num /= base;
        destination[i++] = (digit > 9 ? 'A' + digit - 10 : '0' + digit);
    } while (num != 0);

    string_reverse(destination, i);

    // Add null character
    destination[i] = '\0';

    return i;
}

void string_format_args(char *destination, const char *format, va_list l)
{
    for (char *c = format; *c; c++)
    {
        if (*c == '%')
        {
            c++;

            int unsign = 0;
            if (*c == 'u')
            {
                unsign = 1;
                c++;
            }

            int base = 10;
            if (*c == 'b')
            {
                base = 2;
                c++;
            }
            else if (*c == 'h')
            {
                base = 16;
                c++;
            }
            else if (*c == 'o')
            {
                base = 8;
                c++;
            }

            switch (*c)
            {
            case 'i':
            {
                if (unsign)
                {
                    unsigned int i = va_arg(l, unsigned int);
                    destination += string_from_u32(destination, i, base);
                }
                else
                {
                    int i = va_arg(l, int);
                    destination += string_from_i32(destination, i, base);
                }
                break;
            }

            case 'l':
            {
                if (unsign)
                {
                    unsigned long i = va_arg(l, unsigned long);
                    destination += string_from_u64(destination, i, base);
                }
                else
                {
                    long i = va_arg(l, long);
                    destination += string_from_i64(destination, i, base);
                }
                break;
            }

            case 'p':
            {
                void *i = va_arg(l, void *);
                destination += string_from_u64(destination, (unsigned long)i, 16);
                break;
            }

            case '%':
            {
                *destination = '%';
                destination++;
                break;
            }

            default:
                break;
            }
        }
        else
        {
            *destination = *c;
            destination++;
        }
    }
}

void string_format(char *destination, const char *format, ...)
{
    va_list l;
    va_start(l, format);
    string_format_args(destination, format, l);
    va_end(l);
}
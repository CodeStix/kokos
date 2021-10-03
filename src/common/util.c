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
    while (num != 0)
    {
        int digit = (num % base);
        num /= base;
        dest[i++] = (digit > 9 ? 'A' : '0') + digit;
    }

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
    while (num != 0)
    {
        int digit = (num % base);
        num /= base;
        dest[i++] = (digit > 9 ? 'A' : '0') + digit;
    }

    reverse_buffer(dest, i);

    // Add null character
    dest[i] = '\0';
}

#define RSDP_SIGNATURE 0x2052545020445352

// The Root System Description Pointer is a structure found in memory, between address 0x000E0000 and 0x000FFFFF or between 0x00080000 and 0x0009FFFF
// that points to a Root System Description Table which points to a lot of other tables which contain a lot of information about the systems hardware. https://wiki.osdev.org/RSDP and https://wiki.osdev.org/ACPI

struct RSDP
{
    char signature[8];
    unsigned char checksum;
    char oemid[6];
    unsigned char version;
    unsigned int address;
} __attribute__((packed)); // Make sure the compiler does not put unused memory between fields

struct RSDP2
{
    struct RSDP base;
    unsigned int length;
    unsigned long long address;
    unsigned char checksum;
    unsigned char reserved[3];
} __attribute__((packed));

// The following function iterates ram on a specific location looking for the RSDP structure. The RSDP starts with the string "RSD PTR " which is 0x2052545020445352 in reversed ascii (little endian)
// When RSDP.version >= 2, you can cast it to RSDP2.
struct RSDP *rsdp_find()
{
    for (unsigned char *a = (unsigned char *)0x00080000; a < (unsigned char *)0x000FFFFF; a += 16)
    {
        if (*(long *)a == 0x2052545020445352)
        {
            return (struct RSDP *)a;
        }
    }
    return 0;
}

void rsdp_validate()
{
}

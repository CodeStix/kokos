
#define RSDP_SIGNATURE 0x2052545020445352

// ACPI stands for Advanced Configuration and Power interface, more information here https://wiki.osdev.org/ACPI

// The Root System Description Pointer is a structure found in memory, between address 0x000E0000 and 0x000FFFFF or between 0x00080000 and 0x0009FFFF
// that points to a Root System Description Table which points to a lot of other tables which contain a lot of information about the systems hardware. https://wiki.osdev.org/RSDP

typedef struct
{
    char signature[8];
    unsigned char checksum;
    char oemId[6];
    unsigned char revision;
    unsigned int address;              // Points to a RSDT structure using a 32 bit address
} __attribute__((packed)) RSDTPointer; // Make sure the compiler does not put unused memory between fields

typedef struct
{
    RSDTPointer base;
    unsigned int length;
    unsigned long long address; // Points to a XSDT structure using a 64 bit address
    unsigned char checksum;
    unsigned char reserved[3];
} __attribute__((packed)) XSDTPointer;

// The following function iterates ram on a specific location looking for the RSDP structure. The RSDP starts with the string "RSD PTR " which is 0x2052545020445352 in reversed ascii (little endian)
// When RSDP.revision >= 2, you can cast it to XSDTPointer.
RSDTPointer *acpi_find_rsdt_pointer()
{
    for (unsigned char *a = (unsigned char *)0x00080000; a < (unsigned char *)0x000FFFFF; a += 16)
    {
        if (*(long *)a == 0x2052545020445352)
        {
            return (RSDTPointer *)a;
        }
    }
    return 0;
}

// Returns 0 if the RSDTPointer structure is valid
int acpi_validate_rsdt_pointer(const RSDTPointer *rsdp)
{
    unsigned int sum = 0;
    for (int i = 0; i < sizeof(RSDTPointer); i++)
    {
        sum += *(((unsigned char *)rsdp) + i);
    }
    return sum & 0xff;
}

// Returns 0 if the XSDTPointer structure is valid
int acpi_validate_xsdt_pointer(const XSDTPointer *rsdp)
{
    unsigned int sum = 0;
    for (int i = 0; i < sizeof(XSDTPointer); i++)
    {
        sum += *(((unsigned char *)rsdp) + i);
    }
    return sum & 0xff;
}

typedef struct
{
    char signature[4];
    unsigned int length;
    unsigned char revision;
    unsigned char checksum;
    char oemId[6];
    char oemTableId[8];
    unsigned int oemRevision;
    unsigned int creatorId;
    unsigned int creatorRevision;
} __attribute__((packed)) RSDTHeader;

typedef struct
{
    RSDTHeader header;
    unsigned int tableAddresses[];
} __attribute__((packed)) RSDT;

typedef struct
{
    RSDTHeader header;
    unsigned long long tableAddresses[];
} __attribute__((packed)) XSDT;

// Returns 0 if the RSDT or XSDT structure is valid
int acpi_validate_rsdt(const RSDTHeader *rsdt)
{
    unsigned char sum = 0;
    for (int i = 0; i < rsdt->length; i++)
    {
        sum += *(((unsigned char *)rsdt) + i);
    }
    return sum;
}
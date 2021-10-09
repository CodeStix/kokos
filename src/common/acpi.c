#include "../include/acpi.h"

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

int acpi_validate_rsdt_pointer(const RSDTPointer *rsdp)
{
    unsigned int sum = 0;
    for (int i = 0; i < sizeof(RSDTPointer); i++)
    {
        sum += *(((unsigned char *)rsdp) + i);
    }
    return sum & 0xff;
}

int acpi_validate_xsdt_pointer(const XSDTPointer *rsdp)
{
    unsigned int sum = 0;
    for (int i = 0; i < sizeof(XSDTPointer); i++)
    {
        sum += *(((unsigned char *)rsdp) + i);
    }
    return sum & 0xff;
}

int acpi_validate_rsdt(const RSDTHeader *rsdt)
{
    unsigned char sum = 0;
    for (int i = 0; i < rsdt->length; i++)
    {
        sum += *(((unsigned char *)rsdt) + i);
    }
    return sum;
}

int acpi_rsdt_entry_count(RSDT *rsdt)
{
    return (rsdt->header.length - sizeof(rsdt->header)) / 4;
}

int acpi_xsdt_entry_count(XSDT *xsdt)
{
    return (xsdt->header.length - sizeof(xsdt->header)) / 8;
}
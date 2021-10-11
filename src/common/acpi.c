#include "../include/acpi.h"
#include "../include/console.h"

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

int acpi_rsdt_entry_count(const RSDT *rsdt)
{
    return (rsdt->header.length - sizeof(rsdt->header)) / 4;
}

int acpi_xsdt_entry_count(const XSDT *xsdt)
{
    return (xsdt->header.length - sizeof(xsdt->header)) / 8;
}

void acpi_print_madt(const MADT *madt)
{
    console_print("madt.local_apic_address = 0x");
    console_print_u32(madt->local_apic_address, 16);
    console_print("\n");

    for (int i = sizeof(MADT); i < madt->header.length;)
    {
        MADTEntryHeader *madt_entry = (MADTEntryHeader *)(((unsigned char *)madt) + i);

        switch (madt_entry->type)
        {
        case 0:
        {
            MADTEntry0LocalAPIC *madt_entry_0 = (MADTEntry0LocalAPIC *)madt_entry;
            console_print(" - found local processor with id ");
            console_print_u32(madt_entry_0->apic_processor_id, 10);
            console_print(" and apic id ");
            console_print_u32(madt_entry_0->apic_id, 10);
            console_new_line();
            break;
        }
        case 1:
        {
            MADTEntry1IOAPIC *madt_entry_1 = (MADTEntry1IOAPIC *)madt_entry;
            console_print(" - found io apic with address 0x");
            console_print_u32(madt_entry_1->io_apic_address, 16);
            console_print(" and id ");
            console_print_u32(madt_entry_1->io_apic_id, 10);
            console_print(" and base ");
            console_print_u32(madt_entry_1->global_system_interrupt_base, 10);
            console_new_line();
            break;
        }
        case 2:
        {
            MADTEntry2IOAPICSource *madt_entry_2 = (MADTEntry2IOAPICSource *)madt_entry;
            console_print(" - found io apic source with bus ");
            console_print_u32(madt_entry_2->bus_source, 10);
            console_print(" and irq ");
            console_print_u32(madt_entry_2->irq_source, 10);
            console_print(" and global system interrupt ");
            console_print_u32(madt_entry_2->global_system_interrupt, 10);
            console_new_line();
            break;
        }
        case 3:
        {
            MADTEntry3IOAPICNonMaskable *madt_entry_3 = (MADTEntry3IOAPICNonMaskable *)madt_entry;
            console_print(" - found io apic non maskable with source ");
            console_print_u32(madt_entry_3->non_maskable_interrupt_source, 10);
            console_print(" and global system interrupt ");
            console_print_u32(madt_entry_3->global_system_interrupt, 10);
            console_new_line();
            break;
        }
        case 4:
        {
            MADTEntry4LocalAPICNonMaskable *madt_entry_4 = (MADTEntry4LocalAPICNonMaskable *)madt_entry;
            console_print(" - found local apic non maskable with processor target ");
            console_print_u32(madt_entry_4->acpi_processor_id_target, 10);
            console_print(" and local interrupt ");
            console_print_u32(madt_entry_4->local_interrupt_target, 10);
            console_new_line();
            break;
        }
        case 5:
        {
            MADTEntry5LocalAPICAddressOverride *madt_entry_5 = (MADTEntry5LocalAPICAddressOverride *)madt_entry;
            console_print(" - local apic 64 bit address 0x");
            console_print_u32(madt_entry_5->local_apic_address, 16);
            console_new_line();
            break;
        }
        case 9:
        {
            MADTEntry9LocalX2APIC *madt_entry_9 = (MADTEntry9LocalX2APIC *)madt_entry;
            console_print(" - found local processor with id ");
            console_print_u32(madt_entry_9->apic_processor_id, 10);
            console_print(" and x2apic id ");
            console_print_u32(madt_entry_9->apic_id, 10);
            console_new_line();
            break;
        }

        default:
        {
            console_print(" - madt entry of unknown type ");
            console_print_u32(madt_entry->type, 10);
            console_new_line();
            break;
        }
        }

        i += madt_entry->length;
    }

    console_new_line();
}
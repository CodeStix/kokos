#include "kokos/acpi.h"
#include "kokos/console.h"
#include "kokos/util.h"

struct acpi_rsdtp *acpi_find_rsdt_pointer()
{
    for (unsigned char *a = (unsigned char *)0x00080000; a < (unsigned char *)0x000FFFFF; a += 16)
    {
        if (*(long *)a == ACPI_RSDTP_SIGNATURE)
        {
            return (struct acpi_rsdtp *)a;
        }
    }
    return 0;
}

int acpi_validate_rsdt_pointer(const struct acpi_rsdtp *rsdp)
{
    unsigned int sum = 0;
    for (int i = 0; i < sizeof(struct acpi_rsdtp); i++)
    {
        sum += *(((unsigned char *)rsdp) + i);
    }
    return sum & 0xff;
}

int acpi_validate_xsdt_pointer(const struct acpi_xsdtp *rsdp)
{
    unsigned int sum = 0;
    for (int i = 0; i < sizeof(struct acpi_xsdtp); i++)
    {
        sum += *(((unsigned char *)rsdp) + i);
    }
    return sum & 0xff;
}

int acpi_validate_sdt(const struct acpi_sdt *sdt)
{
    unsigned char sum = 0;
    for (int i = 0; i < sdt->length; i++)
    {
        sum += *(((unsigned char *)sdt) + i);
    }
    return sum;
}

int acpi_rsdt_entry_count(const struct acpi_rsdt *rsdt)
{
    return (rsdt->base.length - sizeof(rsdt->base)) / 4;
}

int acpi_xsdt_entry_count(const struct acpi_xsdt *xsdt)
{
    return (xsdt->base.length - sizeof(xsdt->base)) / 8;
}

void acpi_print_madt(const struct acpi_madt *madt)
{
    console_print("madt.local_apic_address = 0x");
    console_print_u32(madt->local_apic_address, 16);
    console_print("\n");

    for (int i = sizeof(struct acpi_madt); i < madt->base.length;)
    {
        struct acpi_madt_entry *madt_entry = (struct acpi_madt_entry *)(((unsigned char *)madt) + i);

        switch (madt_entry->type)
        {
        case 0:
        {
            struct acpi_madt_entry_local_apic *madt_entry_0 = (struct acpi_madt_entry_local_apic *)madt_entry;
            console_print(" - found local processor with id ");
            console_print_u32(madt_entry_0->processor_id, 10);
            console_print(" and apic id ");
            console_print_u32(madt_entry_0->apic_id, 10);
            console_print(" and flags ");
            console_print_u32(madt_entry_0->flags, 2);
            console_new_line();
            break;
        }
        case 1:
        {
            struct acpi_madt_entry_io_apic *madt_entry_1 = (struct acpi_madt_entry_io_apic *)madt_entry;
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
            struct acpi_madt_entry_io_apic_source *madt_entry_2 = (struct acpi_madt_entry_io_apic_source *)madt_entry;
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
            struct acpi_madt_entry_io_apic_non_maskable *madt_entry_3 = (struct acpi_madt_entry_io_apic_non_maskable *)madt_entry;
            console_print(" - found io apic non maskable with source ");
            console_print_u32(madt_entry_3->non_maskable_interrupt_source, 10);
            console_print(" and global system interrupt ");
            console_print_u32(madt_entry_3->global_system_interrupt, 10);
            console_new_line();
            break;
        }
        case 4:
        {
            struct acpi_madt_entry_local_apic_non_maskable *madt_entry_4 = (struct acpi_madt_entry_local_apic_non_maskable *)madt_entry;
            console_print(" - found local apic non maskable with processor target ");
            console_print_u32(madt_entry_4->acpi_processor_id_target, 10);
            console_print(" and local interrupt ");
            console_print_u32(madt_entry_4->local_interrupt_target, 10);
            console_new_line();
            break;
        }
        case 5:
        {
            struct acpi_madt_entry_local_apic_address *madt_entry_5 = (struct acpi_madt_entry_local_apic_address *)madt_entry;
            console_print(" - local apic 64 bit address 0x");
            console_print_u32(madt_entry_5->local_apic_address, 16);
            console_new_line();
            break;
        }
        case 9:
        {
            struct acpi_madt_entry_local_x2apic *madt_entry_9 = (struct acpi_madt_entry_local_x2apic *)madt_entry;
            console_print(" - found local processor with id ");
            console_print_u32(madt_entry_9->processor_id, 10);
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
}

struct acpi_madt_entry *acpi_madt_iterate(const struct acpi_madt *madt, struct acpi_madt_entry *previous)
{
    if (previous == 0)
    {
        // Start of iteration
        return (struct acpi_madt_entry *)((unsigned long)madt + sizeof(struct acpi_madt));
    }
    else
    {
        struct acpi_madt_entry *next = (struct acpi_madt_entry *)((unsigned long)previous + previous->length);
        if ((unsigned long)next >= (unsigned long)madt + madt->base.length)
        {
            // Reached end
            return 0;
        }
        else
        {
            // Return next entry
            return next;
        }
    }
}

struct acpi_madt_entry *acpi_madt_iterate_type(const struct acpi_madt *madt, struct acpi_madt_entry *previous, unsigned int entry_type)
{
    while ((previous = acpi_madt_iterate(madt, previous)) && previous->type != entry_type)
    {
    }

    return previous;
}

struct acpi_sdt *acpi_rsdt_get_table(const struct acpi_rsdt *root_table, unsigned int table_signature)
{
    for (int i = 0; i < acpi_rsdt_entry_count(root_table); i++)
    {
        struct acpi_sdt *table = (struct acpi_sdt *)root_table->table_addresses[i];
        if (table->signature == table_signature)
            return table;
    }
    return 0;
}

struct acpi_sdt *acpi_xsdt_get_table(const struct acpi_xsdt *root_table, unsigned int table_signature)
{
    for (int i = 0; i < acpi_xsdt_entry_count(root_table); i++)
    {
        struct acpi_sdt *table = (struct acpi_sdt *)root_table->table_addresses[i];
        if (table->signature == table_signature)
            return table;
    }
    return 0;
}

void acpi_print_sdt(const struct acpi_sdt *sdt)
{
    if (acpi_validate_sdt(sdt))
    {
        console_print("!!INVALID!! ");
    }

    console_print("acpi table ");
    console_print_length(sdt->signature_string, 4);
    console_print(" 0x");
    console_print_u32(sdt->signature, 16);

    if (sdt->signature == ACPI_MADT_SIGNATURE)
    {
        acpi_print_madt((struct acpi_madt *)sdt);
    }

    console_print(" at 0x");
    console_print_u64((unsigned long)sdt, 16);
    console_new_line("\n");
}

void acpi_print_rsdt(const struct acpi_rsdt *rsdt)
{
    for (int i = 0; i < acpi_rsdt_entry_count(rsdt); i++)
    {
        struct acpi_sdt *header = (struct acpi_sdt *)rsdt->table_addresses[i];
        acpi_print_sdt(header);
    }
}

void acpi_print_xsdt(const struct acpi_xsdt *xsdt)
{
    for (int i = 0; i < acpi_xsdt_entry_count(xsdt); i++)
    {
        struct acpi_sdt *header = (struct acpi_sdt *)xsdt->table_addresses[i];
        acpi_print_sdt(header);
    }
}

void acpi_print_rsdtp(const struct acpi_rsdtp *rsdtp)
{
    console_print("acpi rsdtp address: 0x");
    console_print_u64((unsigned long)rsdtp, 16);
    console_new_line();

    console_print("acpi rsdtp revision: ");
    console_print_u32(rsdtp->revision, 10);
    console_new_line();

    console_print("acpi rsdt address: 0x");
    console_print_u32(rsdtp->address, 16);
    console_new_line();

    console_print("acpi rsdtp oem: ");
    console_print_length(rsdtp->oem_id, sizeof(rsdtp->oem_id));
    console_new_line();
}

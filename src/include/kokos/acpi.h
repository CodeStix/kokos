#pragma once
#include "kokos/core.h"

// The signature that identifies the RSDP structure, ascii "RSD PTR " in a packed long
#define ACPI_RSDTP_SIGNATURE 0x2052545020445352

// The signature that identifies the MADT structure, ascii "ACPI" in a packed int. To be used with the acpi_get_table function
#define ACPI_MADT_SIGNATURE 0x43495041

// The signature that identifies the FADT structure, ascii "FACP" in a packed int. To be used with the acpi_get_table function
#define ACPI_FADT_SIGNATURE 0x50434146

// ACPI stands for Advanced Configuration and Power interface, more information here https://wiki.osdev.org/ACPI

// The Root System Description Pointer is a structure found in memory, between address 0x000E0000 and 0x000FFFFF or between 0x00080000 and 0x0009FFFF
// that points to a Root System Description Table which points to a lot of other tables which contain a lot of information about the systems hardware. https://wiki.osdev.org/RSDP

struct acpi_rsdtp
{
    union
    {
        char signature_string[8];
        unsigned long signature;
    };
    unsigned char checksum;
    char oem_id[6];
    unsigned char revision;
    unsigned int address; // Points to a struct acpi_rsdt structure using a 32 bit address
} ATTRIBUTE_PACKED;       // Make sure the compiler does not put unused memory between fields

struct acpi_xsdtp
{
    struct acpi_rsdtp base;
    unsigned int length;
    unsigned long address; // Points to a struct acpi_xsdt structure using a 64 bit address
    unsigned char checksum;
    unsigned char reserved[3];
} ATTRIBUTE_PACKED;

struct acpi_sdt
{
    // The signature of this table, which will consist of 4 ascii codes. This will determine what kind of table it is
    union
    {
        char signature_string[4];
        unsigned int signature;
    };
    // Contains the length (in bytes) of the entire table, including this header
    unsigned int length;
    // The version of this table
    unsigned char revision;
    // The checksum of this table, this table is valid if the sum of the entire table's lower byte is zero (see acpi_validate_sdt)
    unsigned char checksum;
    // The vendor that created this table
    char oem_id[6];
    // The vendor specific id of this table
    char oem_table_id[8];
    // The vendor specific version of this table
    unsigned int oem_revision;
    unsigned int creator_id;
    unsigned int creator_revision;
} ATTRIBUTE_PACKED;

struct acpi_rsdt
{
    struct acpi_sdt base;
    // The length of this array can be calculated using acpi_rsdt_entry_count
    unsigned int table_addresses[];
} ATTRIBUTE_PACKED;

struct acpi_xsdt
{
    struct acpi_sdt base;
    // The length of this array can be calculated using acpi_xsdt_entry_count
    unsigned long long table_addresses[];
} ATTRIBUTE_PACKED;

// An address structure used in the struct acpi_fadt table below
struct acpi_fadt_address
{
    unsigned char address_space;
    unsigned char bit_width;
    unsigned char bit_offset;
    unsigned char access_size;
    unsigned long address;
} ATTRIBUTE_PACKED;

// The struct acpi_fadt is a ACPI table with signature 'FACP'
// https://wiki.osdev.org/struct acpi_fadt
// https://kokos.run/#WzAsIkFDUEkucGRmIiwxNzgsWzE3OCwxMCwxNzgsMTFdXQ==
// The extended fields are only used when the addresses do not fit in their non-extended variants
struct acpi_fadt
{
    struct acpi_sdt base;

    unsigned int firmware_control;
    unsigned int dsdt;

    // field used in ACPI 1.0; no longer in use, for compatibility only
    unsigned char unused;

    unsigned char preferred_power_management_profile;
    unsigned short sci_interrupt;
    unsigned int smi_commandport;
    unsigned char acpi_enable;
    unsigned char acpi_disable;
    unsigned char s4_bios_req;
    unsigned char pstate_control;
    unsigned int pm1a_event_block;
    unsigned int pm1b_event_block;
    unsigned int pm1a_control_block;
    unsigned int pm1b_control_block;
    unsigned int pm2_control_block;
    unsigned int pm_timer_block;
    unsigned int gpe0_block;
    unsigned int gpe1_block;
    unsigned char pm1_event_length;
    unsigned char pm1_control_length;
    unsigned char pm2_control_length;
    unsigned char pm_timer_length;
    unsigned char gpe0_length;
    unsigned char gpe1_length;
    unsigned char gpe1_base;
    unsigned char cstate_control;
    unsigned short worst_c2_latency;
    unsigned short worst_c3_latency;
    unsigned short flush_size;
    unsigned short flush_stride;
    unsigned char duty_offset;
    unsigned char duty_width;
    unsigned char day_alarm;
    unsigned char month_alarm;
    unsigned char century;
    // Only available on ACPI 2.0+
    unsigned short extended_boot_architecture_flags;
    unsigned char unused2;
    unsigned int flags;
    struct acpi_fadt_address reset_register;
    unsigned char reset_value;
    unsigned char unused3[3];
    // Only available on ACPI 2.0+
    unsigned long extended_firmware_control;
    // Only available on ACPI 2.0+
    unsigned long extended_dsdt;
    // Only available on ACPI 2.0+
    struct acpi_fadt_address extended_pm1a_event_block;
    // Only available on ACPI 2.0+
    struct acpi_fadt_address extended_pm1b_event_block;
    // Only available on ACPI 2.0+
    struct acpi_fadt_address extended_pm1a_control_block;
    // Only available on ACPI 2.0+
    struct acpi_fadt_address extended_pm1b_control_block;
    // Only available on ACPI 2.0+
    struct acpi_fadt_address extended_pm2_control_block;
    // Only available on ACPI 2.0+
    struct acpi_fadt_address extended_pm_timer_block;
    // Only available on ACPI 2.0+
    struct acpi_fadt_address extended_gpe0_block;
    // Only available on ACPI 2.0+
    struct acpi_fadt_address extended_gpe1_block;
} ATTRIBUTE_PACKED;

struct acpi_dsdt
{
    struct acpi_sdt base;
    unsigned char aml_code[];
};

#define ACPI_MADT_TYPE_LOCAL_APIC 0
#define ACPI_MADT_TYPE_IO_APIC 1
#define ACPI_MADT_TYPE_IO_APIC_SOURCE_OVERRIDE 2
#define ACPI_MADT_TYPE_IO_APIC_NON_MASKABLE 3
#define ACPI_MADT_TYPE_LOCAL_APIC_NON_MASKABLE 4
#define ACPI_MADT_TYPE_LOCAL_APIC_ADDRESS_OVERRIDE 5
#define ACPI_MADT_TYPE_LOCAL_X2APIC 9

// Every struct acpi_madt entry starts with this header
struct acpi_madt_entry
{
    unsigned char type;
    unsigned char length;
} ATTRIBUTE_PACKED;

// The MADT ACPI table has signature 'APIC', it contains information about the different programmable interrupt controllers (PIC, APIC, IOAPIC) in the system
// After this struct, a variable list of MADT entries are stored.
// https://wiki.osdev.org/struct acpi_madt
// https://kokos.run/#WzAsIkFDUEkucGRmIiwyMDMsWzIwMywzMiwyMDMsMzNdXQ==
struct acpi_madt
{
    struct acpi_sdt base;
    unsigned int local_apic_address;
    unsigned int flags;
} ATTRIBUTE_PACKED;

// struct acpi_madt entry of type 0
// This type represents a single physical processor and its local interrupt controller.
struct acpi_madt_entry_local_apic
{
    struct acpi_madt_entry base;
    unsigned char processor_id;
    unsigned char apic_id;
    unsigned int flags;
} ATTRIBUTE_PACKED;

// struct acpi_madt entry of type 1
// Represents an IO APIC
struct acpi_madt_entry_io_apic
{
    struct acpi_madt_entry base;
    unsigned char io_apic_id;
    unsigned char unused;
    unsigned int io_apic_address;
    unsigned int global_system_interrupt_base;
} ATTRIBUTE_PACKED;

// struct acpi_madt entry of type 2
// Represents a IO APIC source
struct acpi_madt_entry_io_apic_source
{
    struct acpi_madt_entry base;
    unsigned char bus_source;
    unsigned char irq_source;
    unsigned int global_system_interrupt;
    unsigned short flags;
} ATTRIBUTE_PACKED;

// struct acpi_madt entry of type 3
// Specifies which IO APIC interrupt inputs should be enabled as non-maskable
struct acpi_madt_entry_io_apic_non_maskable
{
    struct acpi_madt_entry base;
    unsigned char non_maskable_interrupt_source;
    unsigned char unused;
    unsigned short flags;
    unsigned int global_system_interrupt;
} ATTRIBUTE_PACKED;

// struct acpi_madt entry of type 4
struct acpi_madt_entry_local_apic_non_maskable
{
    struct acpi_madt_entry base;
    unsigned char acpi_processor_id_target;
    unsigned short flags;
    // Should be 0 to trigger LINT0 and 1 to trigger LINT1 on the processor's local APIC
    unsigned char local_interrupt_target;
} ATTRIBUTE_PACKED;

// struct acpi_madt entry of type 5
// Provides the 64 bit address of the local APIC, this one should be used instead of the struct acpi_madt's local_apic_address
struct acpi_madt_entry_local_apic_address
{
    struct acpi_madt_entry base;
    unsigned short unused;
    unsigned long local_apic_address;
} ATTRIBUTE_PACKED;

// struct acpi_madt entry of type 9
// Same as type 0 but bigger
struct acpi_madt_entry_local_x2apic
{
    struct acpi_madt_entry base;
    unsigned short unused;
    unsigned int processor_id;
    unsigned int flags;
    unsigned int apic_id;
} ATTRIBUTE_PACKED;

// The following function iterates ram on a specific location looking for the RSDP structure. The RSDP starts with the string "RSD PTR " which is 0x2052545020445352 in reversed ascii (little endian)
// When acpi_rsdtp.revision >= 2, you can cast it to struct acpi_xsdtp.
struct acpi_rsdtp *acpi_find_rsdt_pointer();

// Returns 0 if the acpi_rsdtp structure is valid
int acpi_validate_rsdt_pointer(const struct acpi_rsdtp *rsdp);

// Returns 0 if the struct acpi_xsdtp structure is valid
int acpi_validate_xsdt_pointer(const struct acpi_xsdtp *rsdp);

// Returns 0 if the struct acpi_rsdt or struct acpi_xsdt structure is valid
int acpi_validate_sdt(const struct acpi_sdt *rsdt);

// Returns the length of the struct acpi_rsdt.table_addresses array
int acpi_rsdt_entry_count(const struct acpi_rsdt *rsdt);

// Returns the length of the struct acpi_xsdt.table_addresses array
int acpi_xsdt_entry_count(const struct acpi_xsdt *xsdt);

// Returns a Root System Description table with the passed signature
struct acpi_sdt *acpi_rsdt_get_table(const struct acpi_rsdt *root_table, unsigned int table_signature);

// Returns a Root System Description table with the passed signature
struct acpi_sdt *acpi_xsdt_get_table(const struct acpi_xsdt *root_table, unsigned int table_signature);

void acpi_print_madt(const struct acpi_madt *madt);

void acpi_print_rsdt(const struct acpi_rsdt *rsdt);

void acpi_print_xsdt(const struct acpi_xsdt *xsdt);

void acpi_print_sdt(const struct acpi_sdt *sdt);

void acpi_print_rsdtp(const struct acpi_rsdtp *rsdtp);

// Enumerate the MADT table
struct acpi_madt_entry *acpi_madt_iterate(const struct acpi_madt *madt, struct acpi_madt_entry *previous);

// Enumerate the MADT table filtering by entry_type
struct acpi_madt_entry *acpi_madt_iterate_type(const struct acpi_madt *madt, struct acpi_madt_entry *previous, unsigned int entry_type);

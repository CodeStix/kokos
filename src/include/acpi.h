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

typedef struct
{
    // The signature of this table, which will consist of 4 ascii codes. This will determine what kind of table it is
    char signature[4];
    // Contains the length (in bytes) of the entire table, including this header
    unsigned int length;
    // The version of this table
    unsigned char revision;
    // The checksum of this table, this table is valid if the sum of the entire table's lower byte is zero (see acpi_validate_rsdt)
    unsigned char checksum;
    // The vendor that created this table
    char oemId[6];
    // The vendor specific id of this table
    char oemTableId[8];
    // The vendor specific version of this table
    unsigned int oemRevision;
    unsigned int creatorId;
    unsigned int creatorRevision;
} __attribute__((packed)) RSDTHeader;

typedef struct
{
    RSDTHeader header;
    // The length of this array can be calculated using apci_rsdt
    unsigned int tableAddresses[];
} __attribute__((packed)) RSDT;

typedef struct
{
    RSDTHeader header;
    unsigned long long tableAddresses[];
} __attribute__((packed)) XSDT;

// The following function iterates ram on a specific location looking for the RSDP structure. The RSDP starts with the string "RSD PTR " which is 0x2052545020445352 in reversed ascii (little endian)
// When RSDP.revision >= 2, you can cast it to XSDTPointer.
RSDTPointer *acpi_find_rsdt_pointer();

// Returns 0 if the RSDTPointer structure is valid
int acpi_validate_rsdt_pointer(const RSDTPointer *rsdp);

// Returns 0 if the XSDTPointer structure is valid
int acpi_validate_xsdt_pointer(const XSDTPointer *rsdp);

// Returns 0 if the RSDT or XSDT structure is valid
int acpi_validate_rsdt(const RSDTHeader *rsdt);

// Returns the length of the RSDT.tableAddresses array
int acpi_rsdt_entry_count(const RSDT *rsdt);

// Returns the length of the XSDT.tableAddresses array
int acpi_xsdt_entry_count(const XSDT *xsdt);

// An address structure used in the FADT table below
typedef struct
{
    unsigned char address_space;
    unsigned char bit_width;
    unsigned char bit_offset;
    unsigned char access_size;
    unsigned long address;
} __attribute__((packed)) FADTAddress;

// The FADT is a ACPI table with signature 'FACP'
// https://wiki.osdev.org/FADT
// The extended fields are only used when the addresses do not fit in their non-extended variants
typedef struct
{
    RSDTHeader header;

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
    FADTAddress reset_register;
    unsigned char reset_value;
    unsigned char unused3[3];
    // Only available on ACPI 2.0+
    unsigned long extended_firmware_control;
    // Only available on ACPI 2.0+
    unsigned long extended_dsdt;
    // Only available on ACPI 2.0+
    FADTAddress extended_pm1a_event_block;
    // Only available on ACPI 2.0+
    FADTAddress extended_pm1b_event_block;
    // Only available on ACPI 2.0+
    FADTAddress extended_pm1a_control_block;
    // Only available on ACPI 2.0+
    FADTAddress extended_pm1b_control_block;
    // Only available on ACPI 2.0+
    FADTAddress extended_pm2_control_block;
    // Only available on ACPI 2.0+
    FADTAddress extended_pm_timer_block;
    // Only available on ACPI 2.0+
    FADTAddress extended_gpe0_block;
    // Only available on ACPI 2.0+
    FADTAddress extended_gpe1_block;
} __attribute__((packed)) FADT;

// Every MADT entry starts with this header
typedef struct
{
    unsigned char type;
    unsigned char length;
} __attribute__((packed)) MADTEntryHeader;

// The MADT ACPI table has signature 'APIC', it contains information about the different programmable interrupt controllers (PIC, APIC, IOAPIC) in the system
// After this struct, a variable list of MADT entries are stored.
// https://wiki.osdev.org/MADT
typedef struct
{
    RSDTHeader header;
    unsigned int local_apic_address;
    unsigned int flags;
} __attribute__((packed)) MADT;

// MADT entry of type 0
// This type represents a single physical processor and its local interrupt controller.
typedef struct
{
    MADTEntryHeader header;
    unsigned char apic_processor_id;
    unsigned char apic_id;
    unsigned int flags;
} __attribute__((packed)) MADTEntry0LocalAPIC;

// MADT entry of type 1
// Represents an IO APIC
typedef struct
{
    MADTEntryHeader header;
    unsigned char io_apic_id;
    unsigned char unused;
    unsigned int io_apic_address;
    unsigned int global_system_interrupt_base;
} __attribute__((packed)) MADTEntry1IOAPIC;

// MADT entry of type 2
// Represents a IO APIC source
typedef struct
{
    MADTEntryHeader header;
    unsigned char bus_source;
    unsigned char irq_source;
    unsigned int global_system_interrupt;
    unsigned short flags;
} __attribute__((packed)) MADTEntry2IOAPICSource;

// MADT entry of type 3
// Specifies which IO APIC interrupt inputs should be enabled as non-maskable
typedef struct
{
    MADTEntryHeader header;
    unsigned char non_maskable_interrupt_source;
    unsigned char unused;
    unsigned short flags;
    unsigned int global_system_interrupt;
} __attribute__((packed)) MADTEntry3IOAPICNonMaskable;

// MADT entry of type 4
typedef struct
{
    MADTEntryHeader header;
    unsigned char acpi_processor_id_target;
    unsigned short flags;
    // Should be 0 to trigger LINT0 and 1 to trigger LINT1 on the processor's local APIC
    unsigned char local_interrupt_target;
} __attribute__((packed)) MADTEntry4LocalAPICNonMaskable;

// MADT entry of type 5
// Provides the 64 bit address of the local APIC, this one should be used instead of the MADT's local_apic_address
typedef struct
{
    MADTEntryHeader header;
    unsigned short unused;
    unsigned long local_apic_address;
} __attribute__((packed)) MADTEntry5LocalAPICAddressOverride;

// MADT entry of type 9
// Same as type 0 but bigger
typedef struct
{
    MADTEntryHeader header;
    unsigned short unused;
    unsigned int apic_processor_id;
    unsigned int flags;
    unsigned int apic_id;
} __attribute__((packed)) MADTEntry9LocalX2APIC;

void acpi_print_madt(const MADT *madt);
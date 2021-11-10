// The signature that identifies the RSDP structure, ascii "RSD PTR " in a packed long
#define ACPI_RSDTP_SIGNATURE 0x2052545020445352

// The signature that identifies the MADT structure, ascii "ACPI" in a packed int. To be used with the acpi_get_table function
#define ACPI_MADT_SIGNATURE 0x43495041

// The signature that identifies the FADT structure, ascii "FACP" in a packed int. To be used with the acpi_get_table function
#define ACPI_FADT_SIGNATURE 0x50434146

// ACPI stands for Advanced Configuration and Power interface, more information here https://wiki.osdev.org/ACPI

// The Root System Description Pointer is a structure found in memory, between address 0x000E0000 and 0x000FFFFF or between 0x00080000 and 0x0009FFFF
// that points to a Root System Description Table which points to a lot of other tables which contain a lot of information about the systems hardware. https://wiki.osdev.org/RSDP

typedef struct
{
    union
    {
        char signature_string[8];
        unsigned long signature;
    };
    unsigned char checksum;
    char oem_id[6];
    unsigned char revision;
    unsigned int address;            // Points to a AcpiRsdt structure using a 32 bit address
} __attribute__((packed)) AcpiRsdtp; // Make sure the compiler does not put unused memory between fields

typedef struct
{
    AcpiRsdtp base;
    unsigned int length;
    unsigned long long address; // Points to a AcpiXsdt structure using a 64 bit address
    unsigned char checksum;
    unsigned char reserved[3];
} __attribute__((packed)) AcpiXsdtp;

typedef struct
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
} __attribute__((packed)) AcpiSdt;

typedef struct
{
    AcpiSdt base;
    // The length of this array can be calculated using acpi_rsdt_entry_count
    unsigned int table_addresses[];
} __attribute__((packed)) AcpiRsdt;

typedef struct
{
    AcpiSdt base;
    // The length of this array can be calculated using acpi_xsdt_entry_count
    unsigned long long table_addresses[];
} __attribute__((packed)) AcpiXsdt;

// An address structure used in the AcpiFadt table below
typedef struct
{
    unsigned char address_space;
    unsigned char bit_width;
    unsigned char bit_offset;
    unsigned char access_size;
    unsigned long address;
} __attribute__((packed)) AcpiFadtAddress;

// The AcpiFadt is a ACPI table with signature 'FACP'
// https://wiki.osdev.org/AcpiFadt
// https://kokos.run/#WzAsIkFDUEkucGRmIiwxNzgsWzE3OCwxMCwxNzgsMTFdXQ==
// The extended fields are only used when the addresses do not fit in their non-extended variants
typedef struct
{
    AcpiSdt base;

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
    AcpiFadtAddress reset_register;
    unsigned char reset_value;
    unsigned char unused3[3];
    // Only available on ACPI 2.0+
    unsigned long extended_firmware_control;
    // Only available on ACPI 2.0+
    unsigned long extended_dsdt;
    // Only available on ACPI 2.0+
    AcpiFadtAddress extended_pm1a_event_block;
    // Only available on ACPI 2.0+
    AcpiFadtAddress extended_pm1b_event_block;
    // Only available on ACPI 2.0+
    AcpiFadtAddress extended_pm1a_control_block;
    // Only available on ACPI 2.0+
    AcpiFadtAddress extended_pm1b_control_block;
    // Only available on ACPI 2.0+
    AcpiFadtAddress extended_pm2_control_block;
    // Only available on ACPI 2.0+
    AcpiFadtAddress extended_pm_timer_block;
    // Only available on ACPI 2.0+
    AcpiFadtAddress extended_gpe0_block;
    // Only available on ACPI 2.0+
    AcpiFadtAddress extended_gpe1_block;
} __attribute__((packed)) AcpiFadt;

typedef struct
{
    AcpiSdt base;
    unsigned char aml_code[];
} AcpiDsdt;

#define ACPI_MADT_TYPE_LOCAL_APIC 0
#define ACPI_MADT_TYPE_IO_APIC 1
#define ACPI_MADT_TYPE_IO_APIC_SOURCE_OVERRIDE 2
#define ACPI_MADT_TYPE_IO_APIC_NON_MASKABLE 3
#define ACPI_MADT_TYPE_LOCAL_APIC_NON_MASKABLE 4
#define ACPI_MADT_TYPE_LOCAL_APIC_ADDRESS_OVERRIDE 5
#define ACPI_MADT_TYPE_LOCAL_X2APIC 9

// Every AcpiMadt entry starts with this header
typedef struct
{
    unsigned char type;
    unsigned char length;
} __attribute__((packed)) AcpiMadtEntry;

// The MADT ACPI table has signature 'APIC', it contains information about the different programmable interrupt controllers (PIC, APIC, IOAPIC) in the system
// After this struct, a variable list of MADT entries are stored.
// https://wiki.osdev.org/AcpiMadt
// https://kokos.run/#WzAsIkFDUEkucGRmIiwyMDMsWzIwMywzMiwyMDMsMzNdXQ==
typedef struct
{
    AcpiSdt base;
    unsigned int local_apic_address;
    unsigned int flags;
} __attribute__((packed)) AcpiMadt;

// AcpiMadt entry of type 0
// This type represents a single physical processor and its local interrupt controller.
typedef struct
{
    AcpiMadtEntry base;
    unsigned char processor_id;
    unsigned char apic_id;
    unsigned int flags;
} __attribute__((packed)) AcpiMadtEntry0LocalAPIC;

// AcpiMadt entry of type 1
// Represents an IO APIC
typedef struct
{
    AcpiMadtEntry base;
    unsigned char io_apic_id;
    unsigned char unused;
    unsigned int io_apic_address;
    unsigned int global_system_interrupt_base;
} __attribute__((packed)) AcpiMadtEntry1IOAPIC;

// AcpiMadt entry of type 2
// Represents a IO APIC source
typedef struct
{
    AcpiMadtEntry base;
    unsigned char bus_source;
    unsigned char irq_source;
    unsigned int global_system_interrupt;
    unsigned short flags;
} __attribute__((packed)) AcpiMadtEntry2IOAPICSource;

// AcpiMadt entry of type 3
// Specifies which IO APIC interrupt inputs should be enabled as non-maskable
typedef struct
{
    AcpiMadtEntry base;
    unsigned char non_maskable_interrupt_source;
    unsigned char unused;
    unsigned short flags;
    unsigned int global_system_interrupt;
} __attribute__((packed)) AcpiMadtEntry3IOAPICNonMaskable;

// AcpiMadt entry of type 4
typedef struct
{
    AcpiMadtEntry base;
    unsigned char acpi_processor_id_target;
    unsigned short flags;
    // Should be 0 to trigger LINT0 and 1 to trigger LINT1 on the processor's local APIC
    unsigned char local_interrupt_target;
} __attribute__((packed)) AcpiMadtEntry4LocalAPICNonMaskable;

// AcpiMadt entry of type 5
// Provides the 64 bit address of the local APIC, this one should be used instead of the AcpiMadt's local_apic_address
typedef struct
{
    AcpiMadtEntry base;
    unsigned short unused;
    unsigned long local_apic_address;
} __attribute__((packed)) AcpiMadtEntry5LocalAPICAddressOverride;

// AcpiMadt entry of type 9
// Same as type 0 but bigger
typedef struct
{
    AcpiMadtEntry base;
    unsigned short unused;
    unsigned int processor_id;
    unsigned int flags;
    unsigned int apic_id;
} __attribute__((packed)) AcpiMadtEntry9LocalX2APIC;

// The following function iterates ram on a specific location looking for the RSDP structure. The RSDP starts with the string "RSD PTR " which is 0x2052545020445352 in reversed ascii (little endian)
// When AcpiRsdtp.revision >= 2, you can cast it to AcpiXsdtp.
AcpiRsdtp *acpi_find_rsdt_pointer();

// Returns 0 if the AcpiRsdtp structure is valid
int acpi_validate_rsdt_pointer(const AcpiRsdtp *rsdp);

// Returns 0 if the AcpiXsdtp structure is valid
int acpi_validate_xsdt_pointer(const AcpiXsdtp *rsdp);

// Returns 0 if the AcpiRsdt or AcpiXsdt structure is valid
int acpi_validate_sdt(const AcpiSdt *rsdt);

// Returns the length of the AcpiRsdt.table_addresses array
int acpi_rsdt_entry_count(const AcpiRsdt *rsdt);

// Returns the length of the AcpiXsdt.table_addresses array
int acpi_xsdt_entry_count(const AcpiXsdt *xsdt);

// Returns a Root System Description table with the passed signature
AcpiSdt *acpi_rsdt_get_table(const AcpiRsdt *root_table, unsigned int table_signature);

// Returns a Root System Description table with the passed signature
AcpiSdt *acpi_xsdt_get_table(const AcpiXsdt *root_table, unsigned int table_signature);

void acpi_print_madt(const AcpiMadt *madt);

void acpi_print_rsdt(const AcpiRsdt *rsdt);

void acpi_print_xsdt(const AcpiXsdt *xsdt);

void acpi_print_sdt(const AcpiSdt *sdt);

void acpi_print_rsdtp(const AcpiRsdtp *rsdtp);

AcpiMadtEntry *acpi_madt_iterate2(const AcpiMadt *madt, AcpiMadtEntry *previous);

// Enumerate the MADT table
AcpiMadtEntry *acpi_madt_iterate(const AcpiMadt *madt, AcpiMadtEntry *previous);

// Enumerate the MADT table filtering by entry_type
AcpiMadtEntry *acpi_madt_iterate_type(const AcpiMadt *madt, AcpiMadtEntry *previous, unsigned int entry_type);

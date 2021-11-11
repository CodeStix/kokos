#pragma once
#include "kokos/core.h"

// This structure defines all the APIC (advanced programmable interrupt controller) registers
typedef struct
{
    unsigned int unused ATTRIBUTE_ALIGN(16);
    unsigned int unused2 ATTRIBUTE_ALIGN(16);

    unsigned int id ATTRIBUTE_ALIGN(16);      // at 0x20
    unsigned int version ATTRIBUTE_ALIGN(16); // at 0x30

    unsigned int unused3 ATTRIBUTE_ALIGN(16);
    unsigned int unused4 ATTRIBUTE_ALIGN(16);
    unsigned int unused5 ATTRIBUTE_ALIGN(16);
    unsigned int unused6 ATTRIBUTE_ALIGN(16);

    unsigned int task_priority ATTRIBUTE_ALIGN(16);             // at 0x80
    unsigned int arbitration_priority ATTRIBUTE_ALIGN(16);      // at 0x90
    unsigned int processor_priority ATTRIBUTE_ALIGN(16);        // at 0xA0
    unsigned int end_of_interrupt ATTRIBUTE_ALIGN(16);          // at 0xB0
    unsigned int remote_read ATTRIBUTE_ALIGN(16);               // at 0xC0
    unsigned int logical_destination ATTRIBUTE_ALIGN(16);       // at 0xD0
    unsigned int destination_format ATTRIBUTE_ALIGN(16);        // at 0xE0
    unsigned int spurious_interrupt_vector ATTRIBUTE_ALIGN(16); // at 0xF0

    unsigned int in_service0 ATTRIBUTE_ALIGN(16); // at 0x100
    unsigned int in_service1 ATTRIBUTE_ALIGN(16);
    unsigned int in_service2 ATTRIBUTE_ALIGN(16);
    unsigned int in_service3 ATTRIBUTE_ALIGN(16);
    unsigned int in_service4 ATTRIBUTE_ALIGN(16);
    unsigned int in_service5 ATTRIBUTE_ALIGN(16);
    unsigned int in_service6 ATTRIBUTE_ALIGN(16);
    unsigned int in_service7 ATTRIBUTE_ALIGN(16);

    unsigned int trigger_mode0 ATTRIBUTE_ALIGN(16); // at 0x180
    unsigned int trigger_mode1 ATTRIBUTE_ALIGN(16);
    unsigned int trigger_mode2 ATTRIBUTE_ALIGN(16);
    unsigned int trigger_mode3 ATTRIBUTE_ALIGN(16);
    unsigned int trigger_mode4 ATTRIBUTE_ALIGN(16);
    unsigned int trigger_mode5 ATTRIBUTE_ALIGN(16);
    unsigned int trigger_mode6 ATTRIBUTE_ALIGN(16);
    unsigned int trigger_mode7 ATTRIBUTE_ALIGN(16);

    unsigned int interrupt_request0 ATTRIBUTE_ALIGN(16); // at 0x200
    unsigned int interrupt_request1 ATTRIBUTE_ALIGN(16);
    unsigned int interrupt_request2 ATTRIBUTE_ALIGN(16);
    unsigned int interrupt_request3 ATTRIBUTE_ALIGN(16);
    unsigned int interrupt_request4 ATTRIBUTE_ALIGN(16);
    unsigned int interrupt_request5 ATTRIBUTE_ALIGN(16);
    unsigned int interrupt_request6 ATTRIBUTE_ALIGN(16);
    unsigned int interrupt_request7 ATTRIBUTE_ALIGN(16);

    unsigned int error_status ATTRIBUTE_ALIGN(16); // at 0x280

    unsigned int unused7 ATTRIBUTE_ALIGN(16);
    unsigned int unused8 ATTRIBUTE_ALIGN(16);
    unsigned int unused9 ATTRIBUTE_ALIGN(16);
    unsigned int unused10 ATTRIBUTE_ALIGN(16);
    unsigned int unused11 ATTRIBUTE_ALIGN(16);
    unsigned int unused12 ATTRIBUTE_ALIGN(16);
    unsigned int unused13 ATTRIBUTE_ALIGN(16);

    unsigned int interrupt_command0 ATTRIBUTE_ALIGN(16);         // at 0x300
    unsigned int interrupt_command1 ATTRIBUTE_ALIGN(16);         // at 0x310
    unsigned int timer_vector ATTRIBUTE_ALIGN(16);               // at 0x320
    unsigned int thermal_vector ATTRIBUTE_ALIGN(16);             // at 0x330
    unsigned int performance_counter_vector ATTRIBUTE_ALIGN(16); // at 0x340
    unsigned int interrupt0_vector ATTRIBUTE_ALIGN(16);          // at 0x350
    unsigned int interrupt1_vector ATTRIBUTE_ALIGN(16);          // at 0x360
    unsigned int error_vector ATTRIBUTE_ALIGN(16);               // at 0x370
    unsigned int timer_initial_count ATTRIBUTE_ALIGN(16);        // at 0x380
    unsigned int timer_current_count ATTRIBUTE_ALIGN(16);        // at 0x390

    unsigned int unused14 ATTRIBUTE_ALIGN(16);
    unsigned int unused15 ATTRIBUTE_ALIGN(16);
    unsigned int unused16 ATTRIBUTE_ALIGN(16);
    unsigned int unused17 ATTRIBUTE_ALIGN(16);

    unsigned int timer_divide_config ATTRIBUTE_ALIGN(16); // at 0x3e0

    unsigned int unused18 ATTRIBUTE_ALIGN(16);

    unsigned int extended_apic_features ATTRIBUTE_ALIGN(16);    // at 0x400
    unsigned int extended_apic_control ATTRIBUTE_ALIGN(16);     // at 0x410
    unsigned int specific_end_of_interrupt ATTRIBUTE_ALIGN(16); // at 0x420

    unsigned int unused19 ATTRIBUTE_ALIGN(16);
    unsigned int unused20 ATTRIBUTE_ALIGN(16);
    unsigned int unused21 ATTRIBUTE_ALIGN(16);
    unsigned int unused22 ATTRIBUTE_ALIGN(16);
    unsigned int unused23 ATTRIBUTE_ALIGN(16);

    unsigned int interrupt_enable0 ATTRIBUTE_ALIGN(16); // at 0x480
    unsigned int interrupt_enable1 ATTRIBUTE_ALIGN(16);
    unsigned int interrupt_enable2 ATTRIBUTE_ALIGN(16);
    unsigned int interrupt_enable3 ATTRIBUTE_ALIGN(16);
    unsigned int interrupt_enable4 ATTRIBUTE_ALIGN(16);
    unsigned int interrupt_enable5 ATTRIBUTE_ALIGN(16);
    unsigned int interrupt_enable6 ATTRIBUTE_ALIGN(16);
    unsigned int interrupt_enable7 ATTRIBUTE_ALIGN(16);

    unsigned int extended_interrupt_vector0 ATTRIBUTE_ALIGN(16); // at 0x500
    unsigned int extended_interrupt_vector1 ATTRIBUTE_ALIGN(16);
    unsigned int extended_interrupt_vector2 ATTRIBUTE_ALIGN(16);
    unsigned int extended_interrupt_vector3 ATTRIBUTE_ALIGN(16);
} ATTRIBUTE_PACKED Apic;

#define APIC_IO_REGISTER_ID 0
#define APIC_IO_REGISTER_VERSION 1
#define APIC_IO_REGISTER_ARB 2
#define APIC_IO_REGISTER_ENTRY(n) 0x10 + n * 2

// https://wiki.osdev.org/IOAPIC
typedef struct
{
    unsigned int register_select ATTRIBUTE_ALIGN(16); // at 0x0
    unsigned int register_data ATTRIBUTE_ALIGN(16);   // at 0x10 (16)
} ATTRIBUTE_PACKED IOApic;

typedef enum
{
    FIXED = 0b000,
    LOW_PRIORITY = 0b001,
    SMI = 0b010,
    NMI = 0b100,
    INIT = 0b101,
    EXTENDED_INIT = 0b111,
} IOApicEntryDeliveryMode;

typedef enum
{
    PHYSICAL = 0b0,
    LOGICAL = 0b1
} IOApicEntryDestinationMode;

typedef enum
{
    IDLE = 0b0,
    BUSY = 0b1
} IOApicEntryDeliveryStatus;

typedef enum
{
    ACTIVE_HIGH = 0b0,
    ACTIVE_LOW = 0b1,
} IOApicEntryPinPolarity;

typedef enum
{
    EDGE = 0b0,
    LEVEL = 0b1,
} IOApicEntryTriggerMode;

typedef struct
{
    unsigned char vector : 8;
    IOApicEntryDeliveryMode delivery_mode : 3;
    IOApicEntryDestinationMode destination_mode : 1;
    IOApicEntryDeliveryStatus delivery_status : 1;
    IOApicEntryPinPolarity pin_polarity : 1;
    unsigned char remote_irr : 1;
    IOApicEntryTriggerMode trigger_mode : 1;
    unsigned char mask : 1;
    unsigned long unused : 39;
    unsigned char destination : 8;
} ATTRIBUTE_PACKED IOApicEntry;

unsigned char apic_io_get_id(IOApic *apic);
unsigned char apic_io_get_version(IOApic *apic);
unsigned char apic_io_get_max_entries(IOApic *apic);
unsigned long apic_io_get_entry(IOApic *apic, unsigned char index);
void apic_io_set_entry(IOApic *apic, unsigned char index, unsigned long entry);
void apic_io_register(IOApic *apic, unsigned char irq, IOApicEntry entry);

// Disables the normal PIC
void apic_disable_pic();

// Returns non-zero if a local APIC is supported
int apic_check_supported();
#pragma once
#include "kokos/core.h"

#define APIC_IO_REGISTER_ID 0
#define APIC_IO_REGISTER_VERSION 1
#define APIC_IO_REGISTER_ARB 2
#define APIC_IO_REGISTER_ENTRY(n) 0x10 + n * 2

// This structure defines all the APIC (advanced programmable interrupt controller) registers
struct apic
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
} ATTRIBUTE_PACKED;

// https://wiki.osdev.org/IOAPIC
struct ioapic
{
    unsigned int register_select ATTRIBUTE_ALIGN(16); // at 0x0
    unsigned int register_data ATTRIBUTE_ALIGN(16);   // at 0x10 (16)
} ATTRIBUTE_PACKED;

enum ioapic_delivery_mode
{
    IOAPIC_DELIVERY_MODE_FIXED = 0b000,
    IOAPIC_DELIVERY_MODE_LOW_PRIORITY = 0b001,
    IOAPIC_DELIVERY_MODE_SMI = 0b010,
    IOAPIC_DELIVERY_MODE_NMI = 0b100,
    IOAPIC_DELIVERY_MODE_INIT = 0b101,
    IOAPIC_DELIVERY_MODE_EXTENDED_INIT = 0b111,
};

enum ioapic_destination_mode
{
    IOAPIC_DESTINATION_MODE_PHYSICAL = 0b0,
    IOAPIC_DESTINATION_MODE_LOGICAL = 0b1
};

enum ioapic_delivery_status
{
    IOAPIC_DELIVERY_STATUS_IDLE = 0b0,
    IOAPIC_DELIVERY_STATUS_BUSY = 0b1
};

enum ioapic_pin_polarity
{
    IOAPIC_PIN_POLARITY_ACTIVE_HIGH = 0b0,
    IOAPIC_PIN_POLARITY_ACTIVE_LOW = 0b1,
};

enum ioapic_trigger_mode
{
    IOAPIC_TRIGGER_MODE_EDGE = 0b0,
    IOAPIC_TRIGGER_MODE_LEVEL = 0b1,
};

struct ioapic_entry
{
    unsigned char vector : 8;
    enum ioapic_delivery_mode delivery_mode : 3;
    enum ioapic_destination_mode destination_mode : 1;
    enum ioapic_delivery_status delivery_status : 1;
    enum ioapic_pin_polarity pin_polarity : 1;
    unsigned char remote_irr : 1;
    enum ioapic_trigger_mode trigger_mode : 1;
    unsigned char mask : 1;
    unsigned long unused : 39;
    unsigned char destination : 8;
} ATTRIBUTE_PACKED;

unsigned char apic_io_get_id(struct ioapic *apic);
unsigned char apic_io_get_version(struct ioapic *apic);
unsigned char apic_io_get_max_entries(struct ioapic *apic);
unsigned long apic_io_get_entry(struct ioapic *apic, unsigned char index);
void apic_io_set_entry(struct ioapic *apic, unsigned char index, unsigned long entry);
void apic_io_register(struct ioapic *apic, unsigned char irq, struct ioapic_entry entry);

// Disables the normal PIC
void apic_disable_pic();

// Returns non-zero if a local APIC is supported
int apic_check_supported();
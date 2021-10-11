#define ALIGN16 __attribute__((aligned(16)))

// This structure defines all the APIC (advanced programmable interrupt controller)  egisters
typedef struct
{
    unsigned int unused ALIGN16;
    unsigned int unused2 ALIGN16;

    unsigned int id ALIGN16;      // at 0x20
    unsigned int version ALIGN16; // at 0x30

    unsigned int unused3 ALIGN16;
    unsigned int unused4 ALIGN16;
    unsigned int unused5 ALIGN16;
    unsigned int unused6 ALIGN16;

    unsigned int task_priority ALIGN16;             // at 0x80
    unsigned int arbitration_priority ALIGN16;      // at 0x90
    unsigned int processor_priority ALIGN16;        // at 0xA0
    unsigned int end_of_interrupt ALIGN16;          // at 0xB0
    unsigned int remote_read ALIGN16;               // at 0xC0
    unsigned int logical_destination ALIGN16;       // at 0xD0
    unsigned int destination_format ALIGN16;        // at 0xE0
    unsigned int spurious_interrupt_vector ALIGN16; // at 0xF0

    unsigned int in_service0 ALIGN16; // at 0x100
    unsigned int in_service1 ALIGN16;
    unsigned int in_service2 ALIGN16;
    unsigned int in_service3 ALIGN16;
    unsigned int in_service4 ALIGN16;
    unsigned int in_service5 ALIGN16;
    unsigned int in_service6 ALIGN16;
    unsigned int in_service7 ALIGN16;

    unsigned int trigger_mode0 ALIGN16; // at 0x180
    unsigned int trigger_mode1 ALIGN16;
    unsigned int trigger_mode2 ALIGN16;
    unsigned int trigger_mode3 ALIGN16;
    unsigned int trigger_mode4 ALIGN16;
    unsigned int trigger_mode5 ALIGN16;
    unsigned int trigger_mode6 ALIGN16;
    unsigned int trigger_mode7 ALIGN16;

    unsigned int interrupt_request0 ALIGN16; // at 0x200
    unsigned int interrupt_request1 ALIGN16;
    unsigned int interrupt_request2 ALIGN16;
    unsigned int interrupt_request3 ALIGN16;
    unsigned int interrupt_request4 ALIGN16;
    unsigned int interrupt_request5 ALIGN16;
    unsigned int interrupt_request6 ALIGN16;
    unsigned int interrupt_request7 ALIGN16;

    unsigned int error_status ALIGN16; // at 0x280

    unsigned int unused7 ALIGN16;
    unsigned int unused8 ALIGN16;
    unsigned int unused9 ALIGN16;
    unsigned int unused10 ALIGN16;
    unsigned int unused11 ALIGN16;
    unsigned int unused12 ALIGN16;
    unsigned int unused13 ALIGN16;

    unsigned int interrupt_command0 ALIGN16;         // at 0x300
    unsigned int interrupt_command1 ALIGN16;         // at 0x310
    unsigned int timer_vector ALIGN16;               // at 0x320
    unsigned int thermal_vector ALIGN16;             // at 0x330
    unsigned int performance_counter_vector ALIGN16; // at 0x340
    unsigned int interrupt0_vector ALIGN16;          // at 0x350
    unsigned int interrupt1_vector ALIGN16;          // at 0x360
    unsigned int error_vector ALIGN16;               // at 0x370
    unsigned int timer_initial_count ALIGN16;        // at 0x380
    unsigned int timer_current_count ALIGN16;        // at 0x390

    unsigned int unused14 ALIGN16;
    unsigned int unused15 ALIGN16;
    unsigned int unused16 ALIGN16;
    unsigned int unused17 ALIGN16;

    unsigned int timer_divide_config ALIGN16; // at 0x3e0

    unsigned int unused18 ALIGN16;

    unsigned int extended_apic_features ALIGN16;    // at 0x400
    unsigned int extended_apic_control ALIGN16;     // at 0x410
    unsigned int specific_end_of_interrupt ALIGN16; // at 0x420

    unsigned int unused19 ALIGN16;
    unsigned int unused20 ALIGN16;
    unsigned int unused21 ALIGN16;
    unsigned int unused22 ALIGN16;
    unsigned int unused23 ALIGN16;

    unsigned int interrupt_enable0 ALIGN16; // at 0x480
    unsigned int interrupt_enable1 ALIGN16;
    unsigned int interrupt_enable2 ALIGN16;
    unsigned int interrupt_enable3 ALIGN16;
    unsigned int interrupt_enable4 ALIGN16;
    unsigned int interrupt_enable5 ALIGN16;
    unsigned int interrupt_enable6 ALIGN16;
    unsigned int interrupt_enable7 ALIGN16;

    unsigned int extended_interrupt_vector0 ALIGN16; // at 0x500
    unsigned int extended_interrupt_vector1 ALIGN16;
    unsigned int extended_interrupt_vector2 ALIGN16;
    unsigned int extended_interrupt_vector3 ALIGN16;
} __attribute__((packed)) Apic;

// Disables the normal pic
extern void apic_disable_pic();

// Returns 1 if a local apic is supported
extern int apic_check_supported();
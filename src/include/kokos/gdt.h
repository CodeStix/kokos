#pragma once
#include "kokos/core.h"

// The GDT is defined in main.asm
#define GDT_KERNEL_CODE_SEGMENT (unsigned short)0x8
#define GDT_KERNEL_DATA_SEGMENT (unsigned short)0x10
#define GDT_KERNEL_TASK_SEGMENT (unsigned short)0x18
#define GDT_USER_CODE_SEGMENT (unsigned short)0x28
#define GDT_USER_DATA_SEGMENT (unsigned short)0x30

// https://wiki.osdev.org/GDT
// https://kokos.run/#WzAsIkFNRDY0Vm9sdW1lMi5wZGYiLDE1NCxbMTU0LDI3LDE1NCwyN11d
typedef struct GdtEntry
{

    unsigned short limit1 : 16;
    unsigned short base1 : 16;
    unsigned char base2 : 8;

    unsigned char accessed : 1;
    unsigned char read_write : 1;
    unsigned char direction_conforming : 1;
    unsigned char executable : 1;
    unsigned char non_system : 1;
    unsigned char privilege : 2;
    unsigned char present : 1;

    unsigned char limit2 : 4;
    unsigned char unused1 : 1;
    unsigned char long_mode : 1;
    unsigned char size : 1;
    unsigned char granularity : 1;

    unsigned char base3 : 8;

} ATTRIBUTE_PACKED GdtEntry;

// https://kokos.run/#WzAsIkFNRDY0Vm9sdW1lMi5wZGYiLDE1NyxbMTU3LDM5LDE1NywzOV1d
typedef struct GdtSystemEntry
{
    GdtEntry base;
    // Only used when this is a segment is a system segment
    unsigned int base4;
    // Only used when this is a segment is a system segment
    unsigned int unused;
} ATTRIBUTE_PACKED GdtSystemEntry;

typedef struct
{
    unsigned short limit;
    GdtEntry *address;
} ATTRIBUTE_PACKED GdtPointer;

// Format described at
// https://kokos.run/#WzAsIkFNRDY0Vm9sdW1lMi5wZGYiLDQxOCxbNDE4LDI5LDQxOCwyOV1d
// to load it, it must be specified using the task register (TR) which can be loaded using the ltr instruction, pointing to an entry in the global descriptor table (GDT)
// https://kokos.run/#WzAsIkFNRDY0Vm9sdW1lMi5wZGYiLDQxMyxbNDEzLDgsNDEzLDhdXQ==
// https://kokos.run/#WzAsIkFNRDY0Vm9sdW1lMi5wZGYiLDE1NCxbMTU0LDI3LDE1NCwyN11d
typedef struct GdtTaskState
{
    unsigned int unused0;
    // Stack pointer used when switching to ring 0
    unsigned long rsp0;
    // Stack pointer used when switching to ring 1
    unsigned long rsp1;
    // Stack pointer used when switching to ring 2
    unsigned long rsp2;
    unsigned int unused1;
    unsigned int unused2;
    unsigned long ist1;
    unsigned long ist2;
    unsigned long ist3;
    unsigned long ist4;
    unsigned long ist5;
    unsigned long ist6;
    unsigned long ist7;
    unsigned int unused3;
    unsigned int unused4;
    unsigned int iopb_offset;
} ATTRIBUTE_PACKED GdtTaskState;

void gdt_debug();

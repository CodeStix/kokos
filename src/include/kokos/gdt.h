#pragma once
#include "kokos/core.h"

// https://wiki.osdev.org/GDT
typedef struct GdtEntry
{
    union
    {
        struct
        {
            unsigned short limit1 : 16;
            unsigned short base1 : 16;
            unsigned char base2 : 8;

            unsigned char accessed : 1;
            unsigned char read_write : 1;
            unsigned char direction_conforming : 1;
            unsigned char executable : 1;
            unsigned char type : 1;
            unsigned char privilege : 2;
            unsigned char present : 1;

            unsigned char limit2 : 4;
            unsigned char unused1 : 1;
            unsigned char long_mode : 1;
            unsigned char size : 1;
            unsigned char granularity : 1;

            unsigned char base3 : 8;
        };
        unsigned long as_long;
    };
} ATTRIBUTE_PACKED GdtEntry;

typedef struct GdtTaskState
{
    unsigned int unused0;
    unsigned long rsp[3];
    unsigned int unused1;
    unsigned int unused2;
    unsigned long ist[7];
    unsigned int unused3;
    unsigned int unused4;
    unsigned int iopb_offset;
} ATTRIBUTE_PACKED GdtTaskState;

void gdt_debug();

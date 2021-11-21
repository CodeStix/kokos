#pragma once
#include "kokos/core.h"

// http://nongnu.askapache.com/grub/phcoder/multiboot.pdf
// https://www.gnu.org/software/grub/manual/multiboot2/multiboot.html

#define MULTIBOOT2_TYPE unsigned int
#define MULTIBOOT2_TYPE_END 0
#define MULTIBOOT2_TYPE_BOOT_COMMAND_LINE 1
#define MULTIBOOT2_TYPE_BOOT_LOADER_NAME 2
#define MULTIBOOT2_TYPE_MODULES 3
#define MULTIBOOT2_TYPE_BASIC_MEMORY 4
#define MULTIBOOT2_TYPE_BOOT_DEVICE 5
#define MULTIBOOT2_TYPE_MEMORY_MAP 6
#define MULTIBOOT2_TYPE_VBE_INFO 7
#define MULTIBOOT2_TYPE_ELF_SYMBOLS 9
#define MULTIBOOT2_TYPE_FRAMEBUFFER_INFO 8
#define MULTIBOOT2_TYPE_APM_TABLE 10

#define MULTIBOOT2_MEMORY_TYPE_FREE 1
#define MULTIBOOT2_MEMORY_TYPE_HARDWARE_RESERVED 2
#define MULTIBOOT2_MEMORY_TYPE_ACPI 3
#define MULTIBOOT2_MEMORY_TYPE_HIBERNATE 4

typedef struct
{
    unsigned int total_size;
    unsigned int unused;
} ATTRIBUTE_PACKED Multiboot2Info;

typedef struct
{
    MULTIBOOT2_TYPE type;
    unsigned int size;
} ATTRIBUTE_PACKED Multiboot2InfoTag;

typedef struct
{
    Multiboot2InfoTag base;
    unsigned long framebuffer_address;
    unsigned int framebuffer_pitch;
    unsigned int framebuffer_width;
    unsigned int framebuffer_height;
    unsigned char framebuffer_bits_per_pixel;
    unsigned char framebuffer_type;
    unsigned char unused;
} ATTRIBUTE_PACKED Multiboot2InfoTagFrameBuffer;

typedef struct
{
    unsigned long address;
    unsigned long length;
    // Type = 1 means usable ram, 2 means hardware device, 3 means ACPI, 5 means defective ram, other values reserved
    unsigned int type;
    unsigned int unused;
} ATTRIBUTE_PACKED Multiboot2InfoTagMemoryMapEntry;

typedef struct
{
    Multiboot2InfoTag base;
    unsigned int entry_size;
    unsigned int entry_version;
    Multiboot2InfoTagMemoryMapEntry entries[];
} ATTRIBUTE_PACKED Multiboot2InfoTagMemoryMap;

// TODO create all multiboot2 structures

// This variable is set in main.asm, because multiboot loads an operating system and then sets the ECX register to point to a Multiboot2 info structure.
// https://kokos.run/#WzAsIk11bHRpYm9vdC5wZGYiLDEwLFsxMCw1NywxMCw1N11d
extern Multiboot2Info *multiboot2_info;

// Returns 1 if multiboot2 boot information is supported
int multiboot2_info_available();

// Returns a pointer to a boot information tag of given type
Multiboot2InfoTag *multiboot2_info_get(MULTIBOOT2_TYPE type);
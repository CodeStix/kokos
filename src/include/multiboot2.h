#pragma once

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

typedef struct
{
    unsigned int total_size;
    unsigned int unused;
} __attribute__((packed)) Multiboot2Info;

typedef struct
{
    MULTIBOOT2_TYPE type;
    unsigned int size;
} __attribute__((packed)) Multiboot2InfoTag;

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
} __attribute__((packed)) Multiboot2InfoTagFrameBuffer;

typedef struct
{
    unsigned long address;
    unsigned long length;
    // Type = 1 means usable ram, 3 means ACPI, 5 means defective ram, other values reserved
    unsigned int type;
    unsigned int unused;
} __attribute__((packed)) Multiboot2InfoTagMemoryMapEntry;

typedef struct
{
    Multiboot2InfoTag base;
    unsigned int entry_size;
    unsigned int entry_version;
    Multiboot2InfoTagMemoryMapEntry entries[];
} __attribute__((packed)) Multiboot2InfoTagMemoryMap;

// TODO create all multiboot2 structures

extern Multiboot2Info *multiboot2_info;

// Returns 1 if multiboot2 boot information is supported
int multiboot2_info_available();

// Returns a pointer to a boot information tag of given type
Multiboot2InfoTag *multiboot2_info_get(MULTIBOOT2_TYPE type);
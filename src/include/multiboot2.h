#pragma once

typedef struct
{
    unsigned int total_size;
    unsigned int unused;
} __attribute__((packed)) Multiboot2Info;

typedef struct
{
    unsigned int type;
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

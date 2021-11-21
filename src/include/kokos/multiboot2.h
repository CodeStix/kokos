#pragma once
#include "kokos/core.h"

// http://nongnu.askapache.com/grub/phcoder/multiboot.pdf
// https://www.gnu.org/software/grub/manual/multiboot2/multiboot.html

enum multiboot_tag_type
{
    MULTIBOOT_TAG_TYPE_END = 0,
    MULTIBOOT_TAG_TYPE_BOOT_COMMAND_LINE = 1,
    MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME = 2,
    MULTIBOOT_TAG_TYPE_MODULES = 3,
    MULTIBOOT_TAG_TYPE_BASIC_MEMORY = 4,
    MULTIBOOT_TAG_TYPE_BOOT_DEVICE = 5,
    MULTIBOOT_TAG_TYPE_MEMORY_MAP = 6,
    MULTIBOOT_TAG_TYPE_VBE_INFO = 7,
    MULTIBOOT_TAG_TYPE_ELF_SYMBOLS = 9,
    MULTIBOOT_TAG_TYPE_FRAMEBUFFER_INFO = 8,
    MULTIBOOT_TAG_TYPE_APM_TABLE = 10,
};

enum multiboot_memory_type
{
    MULTIBOOT_MEMORY_TYPE_FREE = 1,
    MULTIBOOT_MEMORY_TYPE_HARDWARE_RESERVED = 2,
    MULTIBOOT_MEMORY_TYPE_ACPI = 3,
    MULTIBOOT_MEMORY_TYPE_HIBERNATE = 4,
};

struct multiboot_info
{
    unsigned int total_size;
    unsigned int unused;
} ATTRIBUTE_PACKED;

struct multiboot_tag
{
    enum multiboot_tag_type type;
    unsigned int size;
} ATTRIBUTE_PACKED;

struct multiboot_tag_framebuffer
{
    struct multiboot_tag base;
    unsigned long framebuffer_address;
    unsigned int framebuffer_pitch;
    unsigned int framebuffer_width;
    unsigned int framebuffer_height;
    unsigned char framebuffer_bits_per_pixel;
    unsigned char framebuffer_type;
    unsigned char unused;
} ATTRIBUTE_PACKED;

struct multiboot_tag_memory_map_entry
{
    unsigned long address;
    unsigned long length;
    // Type = 1 means usable ram, 2 means hardware device, 3 means ACPI, 5 means defective ram, other values reserved
    unsigned int type;
    unsigned int unused;
} ATTRIBUTE_PACKED;

struct multiboot_tag_memory_map
{
    struct multiboot_tag base;
    unsigned int entry_size;
    unsigned int entry_version;
    struct multiboot_tag_memory_map_entry entries[];
} ATTRIBUTE_PACKED;

// TODO create all multiboot2 structures

// This variable is set in main.asm, because multiboot loads an operating system and then sets the ECX register to point to a Multiboot2 info structure.
// https://kokos.run/#WzAsIk11bHRpYm9vdC5wZGYiLDEwLFsxMCw1NywxMCw1N11d
extern struct multiboot_info *multiboot2_info;

// Returns 1 if multiboot2 boot information is supported
int multiboot2_info_available();

// Returns a pointer to a boot information tag of given type
struct multiboot_tag *multiboot2_info_get(enum multiboot_tag_type type);
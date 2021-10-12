#include "../include/paging.h"
#include "../include/console.h"

// Every page entry types start with the following bits:
// 0: present bit (0 for absent, 1 for present)
// 1: read/write bit (0 for read only, 1 for read and write)
// 2: user/supervisor bit (0 for only privilege level 1 2 and 3 can access, 1 for everyone can access)
// 3: page level writethrough bit (?)
// 4: page level cache disable bit (set to 1 to disable caching for the next level)
// 5: access bit, this is set by the processor if the page was accessed, you can set it to 0
// 6: dirty bit (only lowest level)
// 7: page size bit
// 8: global bit (only lowest level)
// 9-11: available for software, you can use this to store custom state

// Virtual address format (amd64 vol2 p134)
// Bits 47:39 index into the 512-entry page-map level-4 table.
// Bits 38:30 index into the 512-entry page-directory pointer table.
// Bits 29:21 index into the 512-entry page-directory table.
// Bits 20:12 index into the 512-entry page table.
// Bits 11:0 provide the byte offset into the physical page.

void *paging_get_physical_address(void *virtual_address)
{
    unsigned long a = (unsigned long)virtual_address;

    unsigned int level4_offset = (a >> 39) & 0b111111111;
    unsigned long level4_entry = page_table_level4[level4_offset];

    // Check if page is present
    if (!(level4_entry & 0b00000001))
    {
        return 0;
    }

    unsigned long *page_table_level3 = (unsigned long *)(level4_entry & 0xFFFFFFFFFF000); // 1111 1111 1111 1111 1111 1111 1111 1111 1111 1111 0000 0000 0000 (the table is always aligned to 4096 bytes, or, first 12 bits set to 0)
    unsigned int level3_offset = (a >> 30) & 0b111111111;
    unsigned long level3_entry = page_table_level3[level3_offset];

    // Check if page is present
    if (!(level3_entry & 0b00000001))
    {
        return 0;
    }

    // Check if this is 1gb page
    if (level3_entry & 0b01000000)
    {
        return (void *)((level3_entry & 0xFFFFFFFFFF000) + (a & 0x3FFFFFFF));
    }

    unsigned long *page_table_level2 = (unsigned long *)(level3_entry & 0xFFFFFFFFFF000);
    unsigned int level2_offset = (a >> 21) & 0b111111111;
    unsigned long level2_entry = page_table_level2[level2_offset];

    // Check if page is present
    if (!(level2_entry & 0b00000001))
    {
        return 0;
    }

    // Check if this is 2mb page
    if (level2_entry & 0b01000000)
    {
        return (void *)((level2_entry & 0xFFFFFFFFFF000) + (a & 0x1FFFFF));
    }

    unsigned long *page_table_level1 = (unsigned long *)(level2_entry & 0xFFFFFFFFFF000);
    unsigned int level1_offset = (a >> 12) & 0b111111111; // The other offset fields are 9 bit indices into their 512 entry tables
    unsigned long level1_entry = page_table_level1[level1_offset];

    // Check if page is present
    if (!(level1_entry & 0b00000001))
    {
        return 0;
    }

    unsigned int offset = a & 0b111111111111; // Offset is 12 bits because each page is 4096 bytes in size
    return (void *)((level1_entry & 0xFFFFFFFFFF000) + offset);
}
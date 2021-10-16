#include "../include/paging.h"
#include "../include/console.h"
#include "../include/util.h"
#include "../include/memory_physical.h"

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

static unsigned long min_virtual_address = 0x10000000ull;
static unsigned long max_virtual_address = 0xF00000000000ull; // Just under the 48 bit limit
static unsigned long available_pages;

static unsigned long *current_level4_table = 0;
static unsigned long current_level4_index = 1;
static unsigned long *current_level3_table = 0;
static unsigned long current_level3_index = 0;
static unsigned long *current_level2_table = 0;
static unsigned long current_level2_index = 0;
static unsigned long *current_level1_table = 0;
static unsigned long current_level1_index = 0;

#define PAGE_FLAG_PRESENT 0b000000001
#define PAGE_FLAG_WRITABLE 0b000000010
#define PAGE_FLAG_EVERYONE_ACCESS 0b000000100
#define PAGE_FLAG_WRITETHROUGH 0b000001000
#define PAGE_FLAG_CACHE_DISABLED 0b000010000
#define PAGE_FLAG_ACCESSED 0b000100000
// This flag is only present in the lowest table
#define PAGE_FLAG_DIRTY 0b001000000
// This flag is only present in the level 2 and level 3 tables
#define PAGE_FLAG_SIZE 0b010000000
// This flag is only present in the lowest table
#define PAGE_FLAG_GLOBAL 0b100000000
#define PAGE_FLAG_NO_EXECUTE 0x8000000000000000

// Bits 11-9 in each page table entry are user definable (AVL bits, available for software) and can be used for anything, in this case for the following:
// This flag indicates that the underlaying page table does not contain at least 1 empty entry
#define PAGE_FLAG_FULL 0b1000000000

// Physical memory allocation must be initialized first!
void paging_initialize()
{
    available_pages = (max_virtual_address - min_virtual_address) >> 12;
    current_level4_table = page_table_level4;
    current_level4_index = 1;
    current_level3_table = 0;
    current_level3_index = 0;
    current_level2_table = 0;
    current_level2_index = 0;
    current_level1_table = 0;
    current_level1_index = 0;
}

static unsigned long *paging_next_level3_table()
{
    if (available_pages <= 0)
    {
        console_print("warning: paging_next_level3_table no pages available\n");
    }

    if (++current_level4_index >= 512)
    {
        // Reached end of table, wrap around
        current_level4_index = 0;
    }

    while (1)
    {
        unsigned long entry = current_level4_table[current_level4_index];
        if (entry & PAGE_FLAG_PRESENT)
        {
            if (!(entry & PAGE_FLAG_FULL))
            {
                // This page table is not full and available, select it
                // 0x7FFFFFFFFFFFF000 = all ones without the 63 and first 12 bits set
                return (unsigned long *)(entry & 0x7FFFFFFFFFFFF000ull);
            }
            else
            {
                // This entry is skipped because it is full
                if (++current_level4_index >= 512)
                {
                    // Reached end of table, wrap around
                    current_level4_index = 0;
                }
            }
        }
        else
        {
            // This page table is not present, create and enter it
            unsigned long *new_table = memory_physical_allocate();

            // Clear table
            for (int i = 0; i < 512; i++)
            {
                new_table[i] = 0;
            }

            // Insert new table
            current_level4_table[current_level4_index] = ((unsigned long)new_table) | PAGE_FLAG_PRESENT | PAGE_FLAG_WRITABLE;

            return (unsigned long *)((current_level1_index << 12) | (current_level2_index << 21) | (current_level3_index << 30) | (current_level4_index << 39));
        }
    }
}

static unsigned long *paging_next_level2_table()
{
    if (++current_level3_index >= 512)
    {
        current_level3_table = paging_next_level3_table();
        current_level3_index = 0;
    }

    while (1)
    {
        unsigned long entry = current_level3_table[current_level3_index];
        if (entry & PAGE_FLAG_PRESENT)
        {
            if (!(entry & (PAGE_FLAG_FULL | PAGE_FLAG_SIZE)))
            {
                // This page table is not full and not huge and available, select it
                // 0x7FFFFFFFFFFFF000 = all ones without the 63 and first 12 bits set
                return (unsigned long *)(entry & 0x7FFFFFFFFFFFF000ull);
            }
            else
            {
                // This entry is skipped because it is full
                if (++current_level3_index >= 512)
                {
                    current_level3_table = paging_next_level3_table();
                    current_level3_index = 0;
                }
            }
        }
        else
        {
            // This page table is not present, create and enter it
            unsigned long *new_table = memory_physical_allocate();

            // Clear table
            for (int i = 0; i < 512; i++)
            {
                new_table[i] = 0;
            }

            // Insert new table
            current_level3_table[current_level3_index] = ((unsigned long)new_table) | PAGE_FLAG_PRESENT | PAGE_FLAG_WRITABLE;

            return (unsigned long *)((current_level1_index << 12) | (current_level2_index << 21) | (current_level3_index << 30) | (current_level4_index << 39));
        }
    }
}

static unsigned long *paging_next_level1_table()
{
    if (++current_level2_index >= 512)
    {
        current_level3_table[current_level3_index] |= PAGE_FLAG_FULL;
        current_level2_table = paging_next_level3_table();
        current_level2_index = 0;
    }

    while (1)
    {
        unsigned long entry = current_level2_table[current_level2_index];
        if (entry & PAGE_FLAG_PRESENT)
        {
            if (!(entry & (PAGE_FLAG_FULL | PAGE_FLAG_SIZE)))
            {
                // This page table is not full and not huge and present, select it
                // 0x7FFFFFFFFFFFF000 = all ones without the 63 and first 12 bits set, which only masks the address
                return (unsigned long *)(entry & 0x7FFFFFFFFFFFF000ull);
            }
            else
            {
                // This entry is skipped because it is full
                if (++current_level2_index >= 512)
                {
                    current_level3_table[current_level3_index] |= PAGE_FLAG_FULL;
                    current_level2_table = paging_next_level3_table();
                    current_level2_index = 0;
                }
            }
        }
        else
        {
            // This page table is not present, create and enter it
            unsigned long *new_table = memory_physical_allocate();

            // Clear table
            for (int i = 0; i < 512; i++)
            {
                new_table[i] = 0;
            }

            // Insert new table
            current_level2_table[current_level2_index] = ((unsigned long)new_table) | PAGE_FLAG_PRESENT | PAGE_FLAG_WRITABLE;

            return (unsigned long *)((current_level1_index << 12) | (current_level2_index << 21) | (current_level3_index << 30) | (current_level4_index << 39));
        }
    }
}

void *paging_map(void *physical_address, unsigned short flags)
{
    if (++current_level1_index >= 512)
    {
        current_level2_table[current_level2_index] |= PAGE_FLAG_FULL;
        current_level1_table = paging_next_level1_table();
        current_level1_index = 0;
    }

    while (1)
    {
        unsigned long entry = current_level1_table[current_level1_index];
        if (entry & PAGE_FLAG_PRESENT)
        {
            // Check next entry, this entry is already used
            if (++current_level1_index >= 512)
            {
                current_level2_table[current_level2_index] |= PAGE_FLAG_FULL;
                current_level1_table = paging_next_level1_table();
                current_level1_index = 0;
            }
        }
        else
        {
            // Insert new entry
            current_level1_table[current_level1_index] = ((unsigned long)physical_address) | PAGE_FLAG_PRESENT | PAGE_FLAG_WRITABLE;

            // Create virtual address
            return (unsigned long *)((current_level1_index << 12) | (current_level2_index << 21) | (current_level3_index << 30) | (current_level4_index << 39));
        }
    }
}

void *paging_allocate(unsigned short flags)
{
    unsigned long *new_page = memory_physical_allocate();
    return paging_map(new_page, flags);
}

void paging_free(void *virtual_address)
{
    // TODO is there an instruction for this slow mess?

    unsigned int level4_index = ((unsigned long)virtual_address >> 39) & 0b111111111;
    unsigned int level3_index = ((unsigned long)virtual_address >> 30) & 0b111111111;
    unsigned int level2_index = ((unsigned long)virtual_address >> 21) & 0b111111111;
    unsigned int level1_index = ((unsigned long)virtual_address >> 12) & 0b111111111;

    unsigned long level4_entry = current_level4_table[level4_index];
    if (!(level4_entry & PAGE_FLAG_PRESENT))
    {
        console_print("warning: paging_free tried to free invalid page (level4_entry)\n");
        return;
    }

    unsigned long level3_entry = ((unsigned long *)(level4_entry & 0x7FFFFFFFFFFFF000ull))[level3_index];
    if (!(level3_entry & PAGE_FLAG_PRESENT))
    {
        console_print("warning: paging_free tried to free invalid page (level3_entry)\n");
        return;
    }

    unsigned long level2_entry = ((unsigned long *)(level3_entry & 0x7FFFFFFFFFFFF000ull))[level2_index];
    if (!(level2_entry & PAGE_FLAG_PRESENT))
    {
        console_print("warning: paging_free tried to free invalid page (level2_entry)\n");
        return;
    }

    unsigned long *level1_table_address = (unsigned long *)(level2_entry & 0x7FFFFFFFFFFFF000ull);
    unsigned long level1_entry = level1_table_address[level1_index];
    if (!(level1_entry & PAGE_FLAG_PRESENT))
    {
        console_print("warning: paging_free tried to free invalid page (level1_entry)\n");
        return;
    }

    // Remove present flag
    level1_table_address[level1_index] = level1_entry & ~PAGE_FLAG_PRESENT;
}

void paging()
{
}

void paging_allocate_consecutive(unsigned long pages)
{
}
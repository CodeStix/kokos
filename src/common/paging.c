#include "../include/paging.h"
#include "../include/console.h"
#include "../include/util.h"

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

// Points to a table that contains bits that indicate which physical chunks are allocated
static unsigned long *allocation_table = 0;
// The current position in the allocation_table, this number increases until the end of the table is reached (end of physical RAM)
static unsigned long allocation_index = 0;
// The amount of unsigned long entries in the allocation table
static unsigned long allocation_table_length = 0;
static unsigned long pages_taken = 0;

void paging_physical_initialize(void *allocation_table_location, unsigned long total_memory)
{
    allocation_table = allocation_table_location;
    allocation_index = 0;
    // Can store 8 bytes in each allocation table entry
    allocation_table_length = (total_memory / 4096) / 64;

    // Clear the table, all memory pages (chunks of 4096 bytes) are free upon initialize
    for (unsigned long i = 0; i < allocation_table_length; i++)
    {
        allocation_table[i] = 0;
    }
}

void paging_physical_reserve(void *physical_address, unsigned long bytes)
{
    // >> 12 is the same as divide by 4096
    unsigned long start_index = ((unsigned long)physical_address) >> 12;
    unsigned long end_index = start_index + (bytes >> 12);

    for (unsigned long index = start_index; index <= end_index; index++)
    {
        if (index >= allocation_table_length || index < 0)
        {
            console_print("warning: paging_physical_reserve out of physical memory range (0x");
            console_print_u64((unsigned long)physical_address, 16);
            console_print(" ... 0x");
            console_print_u64((unsigned long)physical_address + bytes, 16);
            console_print(")\n");
            break;
        }

        // >> 6 is the same as divide by 64
        unsigned int byte = index >> 6;

        // & 0b111111 is the same as modulo 64
        unsigned char bit = index & 0b111111;

        // Set bit bit of allocation_table[byte] to one
        if (!(allocation_table[byte] & (1 << bit)))
        {
            allocation_table[byte] |= 1 << bit;
            pages_taken++;
        }
    }
}

// Frees a physical page
void paging_physical_free(void *physical_address)
{
    // >> 12 is the same as divide by 4096
    unsigned long index = ((unsigned long)physical_address) >> 12;

    // >> 6 is the same as divide by 64
    unsigned int byte = index >> 6;

    // & 0b111111 is the same as modulo 64
    unsigned char bit = index & 0b111111;

    if (byte >= allocation_table_length)
    {
        console_print("warning: invalid address passed to paging_physical_free\n");
        return;
    }

    // Set bit bit of allocation_table[byte] to zero
    if (allocation_table[byte] & (1 << bit))
    {
        allocation_table[byte] &= ~(1 << bit);
        pages_taken--;
    }
    else
    {
        console_print("warning: paging_physical_free called on already freed page 0x");
        console_print_u64((unsigned long)physical_address, 16);
        console_new_line();
    }
}

void *paging_physical_allocate()
{
    // TODO this is infinite loop when no memory is available
    // Find first empty spot where a single bit is 0 (0xFFFFFFFFFFFFFFFFull represents all bits set to 1)
    unsigned long spot = allocation_index;
    while (allocation_table[spot] == 0xFFFFFFFFFFFFFFFFull)
    {
        if (++spot >= allocation_table_length)
        {
            spot = 0;
        }
    }

    // TODO bsr instruction to find first 0 bit
    // Find the index of the first bit that is zero
    unsigned long bits = allocation_table[spot];
    unsigned long bit = 0;
    while (bits & 0b1)
    {
        bit++;
        bits >>= 1;
    }

    if (bit >= 64)
    {
        // This cannot happen
        console_print("error: paging_physical_allocate bit scan did not find bit in\n");
        console_print_u64(allocation_table[spot], 2);
        console_new_line();
        return 0;
    }

    // Set bit bit of allocation_table[spot] to zero
    allocation_table[spot] |= 1 << bit;
    allocation_index = spot;
    pages_taken++;

    return (void *)((((spot << 6) + bit) << 12));
}

void *paging_physical_allocate_consecutive(unsigned int chunk_count)
{
    // TODO this is infinite loop when no memory is available
    unsigned long spot = allocation_index;
    unsigned int consecutive = 0;
    while (1)
    {
        if (allocation_table[spot] == 0)
        {
            if (++consecutive >= chunk_count)
            {
                spot -= consecutive - 1;
                break;
            }
        }
        else
        {
            consecutive = 0;
        }

        if (++spot >= allocation_table_length)
        {
            spot = 0;
            consecutive = 0;
        }
    }

    // Set all 'taken' flags
    for (unsigned int i = 0; i < chunk_count; i++)
    {
        allocation_table[spot + i] = 0xFFFFFFFFFFFFFFFFull;
    }

    allocation_index = spot + chunk_count;
    return (void *)(((spot << 6) << 12));
}

int paging_physical_allocated(void *phyisical_address)
{
    unsigned long index = ((unsigned long)phyisical_address) >> 12;

    if (index >= allocation_table_length)
    {
        console_print("warning: invalid address passed to paging_physical_allocated, returning 1: 0x");
        console_print_u64((unsigned long)phyisical_address, 16);
        console_new_line();
        return 1;
    }

    unsigned int byte = index >> 6;
    unsigned char bit = index & 0b111111;
    return (allocation_table[index] >> bit) & 0b1;
}

unsigned long paging_physical_allocated_count()
{
    return pages_taken;
}

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
// This flag is set by the os to indicate that the underlaying page entries are all allocated
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
                return entry & 0x7FFFFFFFFFFFF000;
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
            unsigned long *new_table = paging_physical_allocate();

            // Clear table
            for (int i = 0; i < 512; i++)
            {
                new_table[i] = 0;
            }

            // Insert new table
            current_level4_table[current_level4_index] = ((unsigned long)new_table) | PAGE_FLAG_PRESENT | PAGE_FLAG_WRITABLE;

            return (void *)((current_level1_index << 12) | (current_level2_index << 21) | (current_level3_index << 30) | (current_level4_index << 39));
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
                return entry & 0x7FFFFFFFFFFFF000;
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
            unsigned long *new_table = paging_physical_allocate();

            // Clear table
            for (int i = 0; i < 512; i++)
            {
                new_table[i] = 0;
            }

            // Insert new table
            current_level3_table[current_level3_index] = ((unsigned long)new_table) | PAGE_FLAG_PRESENT | PAGE_FLAG_WRITABLE;

            if (++current_level3_index >= 512)
            {
                current_level3_table = paging_next_level3_table();
                current_level3_index = 0;
            }

            return (void *)((current_level1_index << 12) | (current_level2_index << 21) | (current_level3_index << 30) | (current_level4_index << 39));
        }
    }
}

static unsigned long *paging_next_level1_table()
{
    if (++current_level2_index >= 512)
    {
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
                return entry & 0x7FFFFFFFFFFFF000;
            }
            else
            {
                // This entry is skipped because it is full
                if (++current_level2_index >= 512)
                {
                    current_level2_table = paging_next_level3_table();
                    current_level2_index = 0;
                }
            }
        }
        else
        {
            // This page table is not present, create and enter it
            unsigned long *new_table = paging_physical_allocate();

            // Clear table
            for (int i = 0; i < 512; i++)
            {
                new_table[i] = 0;
            }

            // Insert new table
            current_level2_table[current_level2_index] = ((unsigned long)new_table) | PAGE_FLAG_PRESENT | PAGE_FLAG_WRITABLE;

            return (void *)((current_level1_index << 12) | (current_level2_index << 21) | (current_level3_index << 30) | (current_level4_index << 39));
        }
    }
}

// Allocates a single page and returns the virtual address to it
void *paging_allocate()
{
    while (1)
    {
        unsigned long entry = current_level1_table[current_level1_index];
        if (entry & PAGE_FLAG_PRESENT)
        {
            // Check next entry
            if (++current_level1_index >= 512)
            {
                current_level1_table = paging_next_level1_table();
                current_level1_index = 0;
            }
        }
        else
        {
            // This page table is not present, create and enter it
            unsigned long *new_page = paging_physical_allocate();

            // Insert new table
            current_level1_table[current_level1_index] = ((unsigned long)new_page) | PAGE_FLAG_PRESENT | PAGE_FLAG_WRITABLE;

            if (++current_level1_index >= 512)
            {
                current_level1_table = paging_next_level1_table();
                current_level1_index = 0;
            }

            // Create virtual address
            return (void *)((current_level1_index << 12) | (current_level2_index << 21) | (current_level3_index << 30) | (current_level4_index << 39));
        }
    }
}

// Frees a single page allocated using paging_allocate
void paging_free(void *virtual_address)
{
}

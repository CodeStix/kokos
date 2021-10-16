#include "../include/paging.h"
#include "../include/console.h"
#include "../include/util.h"
#include "../include/memory_physical.h"

static unsigned long used_virtual_pages = 0;

static unsigned long *current_level4_table = 0;
static unsigned long current_level4_index = 1;
static unsigned long *current_level3_table = 0;
static unsigned long current_level3_index = 0;
static unsigned long *current_level2_table = 0;
static unsigned long current_level2_index = 0;
static unsigned long *current_level1_table = 0;
static unsigned long current_level1_index = 0;

static inline void paging_clear_table(unsigned long *table)
{
    for (int i = 0; i < 512; i++)
    {
        table[i] = 0;
    }
}

void paging_initialize(unsigned long *level4_table)
{
    // Each table has 512 other tables, there are 4 types of nested tables
    used_virtual_pages = 0;

    current_level1_table = 0;
    current_level2_table = 0;
    current_level3_table = 0;
    current_level4_table = level4_table;

    // 512 is outside of a valid table range, so its creates a new tables on first allocation
    current_level4_index = 512;
    current_level3_index = 512;
    current_level2_index = 512;
    current_level1_index = 512;

    // current_level4_index = 1;
    // current_level3_index = 0;
    // current_level2_index = 0;
    // current_level1_index = 0;
    // current_level3_table = memory_physical_allocate();
    // current_level4_table[current_level4_index] = ((unsigned long)current_level3_table & 0x7FFFFFFFFFFFF000ull) | PAGE_FLAG_PRESENT;
    // current_level2_table = memory_physical_allocate();
    // current_level3_table[current_level3_index] = ((unsigned long)current_level2_table & 0x7FFFFFFFFFFFF000ull) | PAGE_FLAG_PRESENT;
    // current_level1_table = memory_physical_allocate();
    // current_level2_table[current_level3_index] = ((unsigned long)current_level1_table & 0x7FFFFFFFFFFFF000ull) | PAGE_FLAG_PRESENT;
    // paging_clear_table(current_level3_table);
    // paging_clear_table(current_level2_table);
    // paging_clear_table(current_level1_table);
}

static unsigned long *paging_next_level3_table()
{
    console_print("paging_next_level3_table\n");

    if (used_virtual_pages >= PAGING_MAX_VIRTUAL_PAGES)
    {
        console_print("warning: paging_next_level3_table no pages available\n");
    }

    if (++current_level4_index >= 512)
    {
        // Reached end of table, wrap around
        current_level4_index = 1;
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
                    current_level4_index = 1;
                }
            }
        }
        else
        {
            // This page table is not present, create and enter it
            unsigned long *new_table = memory_physical_allocate();

            console_print("create new level 3 table: 0x");
            console_print_u64((unsigned long)new_table, 16);
            console_new_line();

            // Clear table
            paging_clear_table(new_table);

            console_print("cleared level 3\n");

            // Insert new table
            current_level4_table[current_level4_index] = ((unsigned long)new_table) | PAGE_FLAG_PRESENT | PAGE_FLAG_WRITABLE;

            unsigned long *virtual_address = (unsigned long *)((current_level1_index << 12) | (current_level2_index << 21) | (current_level3_index << 30) | (current_level4_index << 39));

            console_print("indices 4-1: ");
            console_print_u64(current_level4_index, 10);
            console_print(" ");
            console_print_u64(current_level3_index, 10);
            console_print(" ");
            console_print_u64(current_level2_index, 10);
            console_print(" ");
            console_print_u64(current_level1_index, 10);
            console_new_line();

            console_print("virtual: 0x");
            console_print_u64((unsigned long)virtual_address, 16);
            console_new_line();

            return new_table;
        }
    }
}

static unsigned long *paging_next_level2_table()
{
    console_print("paging_next_level2_table\n");

    if (++current_level3_index >= 512)
    {
        current_level3_index = 0;
        current_level3_table = paging_next_level3_table();
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
                    current_level3_index = 0;
                    current_level3_table = paging_next_level3_table();
                }
            }
        }
        else
        {
            // This page table is not present, create and enter it
            unsigned long *new_table = memory_physical_allocate();

            // Clear table
            paging_clear_table(new_table);

            // Insert new table
            current_level3_table[current_level3_index] = ((unsigned long)new_table) | PAGE_FLAG_PRESENT | PAGE_FLAG_WRITABLE;

            // unsigned long *virtual_address = (unsigned long *)((current_level1_index << 12) | (current_level2_index << 21) | (current_level3_index << 30) | (current_level4_index << 39));
            return new_table;
        }
    }
}

// Returns the physical
static unsigned long *paging_next_level1_table()
{
    console_print("paging_next_level1_table\n");

    if (++current_level2_index >= 512)
    {
        // current_level3_table[current_level3_index] |= PAGE_FLAG_FULL;
        current_level2_index = 0;
        current_level2_table = paging_next_level2_table();
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
                    // current_level3_table[current_level3_index] |= PAGE_FLAG_FULL;
                    current_level2_index = 0;
                    current_level2_table = paging_next_level2_table();
                }
            }
        }
        else
        {
            // This page table is not present, create and enter it
            unsigned long *new_table = memory_physical_allocate();

            // Clear table
            paging_clear_table(new_table);

            // Insert new table
            current_level2_table[current_level2_index] = ((unsigned long)new_table) | PAGE_FLAG_PRESENT | PAGE_FLAG_WRITABLE;

            // unsigned long *virtual_address = (unsigned long *)((current_level1_index << 12) | (current_level2_index << 21) | (current_level3_index << 30) | (current_level4_index << 39));
            return new_table;
        }
    }
}

void *paging_map(void *physical_address, unsigned short flags)
{
    if (++current_level1_index >= 512)
    {
        // current_level2_table[current_level2_index] |= PAGE_FLAG_FULL;
        current_level1_index = 0;
        current_level1_table = paging_next_level1_table();
    }

    while (1)
    {
        unsigned long entry = current_level1_table[current_level1_index];
        if (entry & PAGE_FLAG_PRESENT)
        {
            // Check next entry, this entry is already used
            if (++current_level1_index >= 512)
            {
                // current_level2_table[current_level2_index] |= PAGE_FLAG_FULL;
                current_level1_index = 0;
                current_level1_table = paging_next_level1_table();
            }
        }
        else
        {
            console_print("insert\n");

            // Insert new entry
            current_level1_table[current_level1_index] = ((unsigned long)physical_address) | PAGE_FLAG_PRESENT | PAGE_FLAG_WRITABLE;
            used_virtual_pages++;

            // Return virtual address
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
    used_virtual_pages--;

    // TODO unallocate empty table
}

void *paging_get_physical_address(void *virtual_address)
{
    unsigned long a = (unsigned long)virtual_address;

    unsigned int level4_offset = (a >> 39) & 0b111111111;
    unsigned long level4_entry = current_level4_table[level4_offset];

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

unsigned long paging_used_pages()
{
    return used_virtual_pages;
}
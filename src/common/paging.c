#include "../include/paging.h"
#include "../include/console.h"
#include "../include/cpu.h"
#include "../include/util.h"
#include "../include/memory_physical.h"

static unsigned long used_virtual_pages = 0;

static unsigned long *current_level4_table = 0;
static unsigned long current_level4_index = 0;
static unsigned long *current_level3_table = 0;
static unsigned long current_level3_index = 0;
static unsigned long *current_level2_table = 0;
static unsigned long current_level2_index = 0;
static unsigned long *current_level1_table = 0;
static unsigned long current_level1_index = 0;

static int hugepages_supported = 0;

static inline void paging_clear_table(unsigned long *table)
{
    for (int i = 0; i < 512; i++)
    {
        table[i] = 0;
    }
}

int paging_get_hugepages_supported()
{
    return hugepages_supported != 0;
}

void paging_initialize(unsigned long *level4_table)
{
    // Each table has 512 other tables, there are 4 types of nested tables
    used_virtual_pages = 0;

    current_level1_table = 0;
    current_level2_table = 0;
    current_level3_table = 0;
    current_level4_table = level4_table;

    // 512 is outside of a valid table range, so its creates new tables on first allocation
    current_level4_index = 512;
    current_level3_index = 512;
    current_level2_index = 512;
    current_level1_index = 512;

    // Check if hugepages is supported using cpuid
    CpuIdResult result = cpu_id(0x80000001);
    hugepages_supported = result.edx & CPU_ID_1GB_PAGES_EDX;
}

// This function can return a partially full table
static unsigned long *paging_next_empty_level3_table()
{
    if (used_virtual_pages >= PAGING_MAX_VIRTUAL_PAGES)
    {
        console_print("warning: paging_next_empty_level3_table no pages available\n");
    }

    if (++current_level4_index >= 512)
    {
        // Reached end of table, wrap around
        current_level4_index = 0;
    }

    while (1)
    {
        unsigned long entry = current_level4_table[current_level4_index];
        if (entry != 0)
        {
            // This entry is skipped because it is full
            if (++current_level4_index >= 512)
            {
                // Reached end of table, wrap around
                current_level4_index = 0;
            }
        }
        else
        {
            // This page table is not present, create and enter it
            unsigned long *new_table = memory_physical_allocate();

            // Clear table
            paging_clear_table(new_table);

            // Insert new table
            current_level4_table[current_level4_index] = ((unsigned long)new_table) | PAGING_ENTRY_FLAG_PRESENT | PAGING_ENTRY_FLAG_WRITABLE;

            // unsigned long *virtual_address = (unsigned long *)((current_level1_index << 12) | (current_level2_index << 21) | (current_level3_index << 30) | (current_level4_index << 39));
            return new_table;
        }
    }
}

// This function can return a partially full table
static unsigned long *paging_next_empty_level2_table()
{
    if (++current_level3_index >= 512)
    {
        current_level3_index = 0;
        current_level3_table = paging_next_empty_level3_table();
    }

    while (1)
    {
        unsigned long entry = current_level3_table[current_level3_index];
        if (entry != 0)
        {
            // This entry is skipped because it is full
            if (++current_level3_index >= 512)
            {
                current_level3_index = 0;
                current_level3_table = paging_next_empty_level3_table();
            }
        }
        else
        {
            // This page table is not present, create and enter it
            unsigned long *new_table = memory_physical_allocate();

            // Clear table
            paging_clear_table(new_table);

            // Insert new table
            current_level3_table[current_level3_index] = ((unsigned long)new_table) | PAGING_ENTRY_FLAG_PRESENT | PAGING_ENTRY_FLAG_WRITABLE;

            // unsigned long *virtual_address = (unsigned long *)((current_level1_index << 12) | (current_level2_index << 21) | (current_level3_index << 30) | (current_level4_index << 39));
            return new_table;
        }
    }
}

// This function always returns a completely empty table
static unsigned long *paging_next_empty_level1_table()
{
    if (++current_level2_index >= 512)
    {
        // current_level3_table[current_level3_index] |= PAGING_ENTRY_FLAG_FULL;
        current_level2_index = 0;
        current_level2_table = paging_next_empty_level2_table();
    }

    while (1)
    {
        unsigned long entry = current_level2_table[current_level2_index];
        if (entry != 0)
        {
            // This entry is skipped because it is full
            if (++current_level2_index >= 512)
            {
                // current_level3_table[current_level3_index] |= PAGING_ENTRY_FLAG_FULL;
                current_level2_index = 0;
                current_level2_table = paging_next_empty_level2_table();
            }
        }
        else
        {
            // This page table is not present, create and enter it
            unsigned long *new_table = memory_physical_allocate();

            // Clear table
            paging_clear_table(new_table);

            // Insert new table
            current_level2_table[current_level2_index] = ((unsigned long)new_table) | PAGING_ENTRY_FLAG_PRESENT | PAGING_ENTRY_FLAG_WRITABLE;

            // unsigned long *virtual_address = (unsigned long *)((current_level1_index << 12) | (current_level2_index << 21) | (current_level3_index << 30) | (current_level4_index << 39));
            return new_table;
        }
    }
}

void *paging_map_physical(void *virtual_address, void *physical_address, unsigned long flags)
{
    unsigned int level4_index = ((unsigned long)virtual_address >> 39) & 0b111111111;
    unsigned int level3_index = ((unsigned long)virtual_address >> 30) & 0b111111111;
    unsigned int level2_index = ((unsigned long)virtual_address >> 21) & 0b111111111;
    unsigned int level1_index = ((unsigned long)virtual_address >> 12) & 0b111111111;

    unsigned long page_entry_flags = PAGING_ENTRY_FLAG_PRESENT;
    if (flags & PAGING_FLAG_WRITE)
        page_entry_flags |= PAGING_ENTRY_FLAG_WRITABLE | PAGING_ENTRY_FLAG_PRESENT;
    if (flags & PAGING_FLAG_USER)
        page_entry_flags |= PAGING_ENTRY_FLAG_EVERYONE_ACCESS;
    // if (flags & PAGING_FLAG_READ)
    //     page_entry_flags |= PAGING_ENTRY_FLAG_PRESENT;
    // TODO noexecute support
    // if (!(flags & PAGING_FLAG_EXECUTE))
    //     page_entry_flags |= PAGING_ENTRY_FLAG_NO_EXECUTE;

    unsigned long *level4_table = current_level4_table;
    unsigned long level4_entry = level4_table[level4_index];
    unsigned long *level3_table;
    if (level4_entry != 0)
    {
        level3_table = (unsigned long *)(level4_entry & PAGING_ADDRESS_MASK);
    }
    else
    {
        level3_table = memory_physical_allocate();
        paging_clear_table(level3_table);

        level4_table[level4_index] = ((unsigned long)level3_table & PAGING_ADDRESS_MASK) | page_entry_flags;
    }

    unsigned long level3_entry = level3_table[level3_index];
    unsigned long *level2_table;
    if ((level3_entry & PAGING_ENTRY_FLAG_SIZE) || (flags & PAGING_FLAG_1GB))
    {
        if (level3_entry != 0 && !(flags & PAGING_FLAG_REPLACE))
        {
            console_print("error: paging_map_physical tried to map at already mapped address (1gb)\n");
            return 0;
        }
        else
        {
            level3_table[level3_index] = ((unsigned long)physical_address & PAGING_ADDRESS_MASK) | page_entry_flags | PAGING_ENTRY_FLAG_SIZE;
            used_virtual_pages += 512 * 512;
            return virtual_address;
        }
    }
    else
    {
        if (level3_entry != 0)
        {
            level2_table = (unsigned long *)(level3_entry & PAGING_ADDRESS_MASK);
        }
        else
        {
            level2_table = memory_physical_allocate();
            paging_clear_table(level2_table);

            level3_table[level3_index] = ((unsigned long)level2_table & PAGING_ADDRESS_MASK) | page_entry_flags;
        }
    }

    // Check if mapping huge pages (2mb)
    unsigned long level2_entry = level2_table[level2_index];
    unsigned long *level1_table;
    if ((level2_entry & PAGING_ENTRY_FLAG_SIZE) || (flags & PAGING_FLAG_2MB))
    {
        if (level2_entry != 0 && !(flags & PAGING_FLAG_REPLACE))
        {
            console_print("error: paging_map_physical tried to map at already mapped address (2mb)\n");
            return 0;
        }
        else
        {
            level2_table[level2_index] = ((unsigned long)physical_address & PAGING_ADDRESS_MASK) | page_entry_flags | PAGING_ENTRY_FLAG_SIZE;
            used_virtual_pages += 512;
            return virtual_address;
        }
    }
    else
    {
        if (level2_entry != 0)
        {
            level1_table = (unsigned long *)(level2_entry & PAGING_ADDRESS_MASK);
        }
        else
        {
            level1_table = memory_physical_allocate();
            paging_clear_table(level1_table);

            level2_table[level3_index] = ((unsigned long)level1_table & PAGING_ADDRESS_MASK) | page_entry_flags;
        }
    }

    if (level1_table[level1_index] != 0)
    {
        console_print("error: paging_map_physical tried to map at already mapped address\n");
        return 0;
    }
    else
    {
        level1_table[level1_index] = ((unsigned long)physical_address & PAGING_ADDRESS_MASK) | page_entry_flags;
        used_virtual_pages++;
        return virtual_address;
    }
}

// Allocates 'pages' pages of 4096 bytes. Free these pages using paging_free_consecutive
void *paging_map_consecutive(void *physical_address, unsigned long pages, unsigned short flags)
{
    if (pages == 0)
    {
        console_print("[paging] warning: paging_map_consecutive tried to map 0 pages");
        return (void *)0;
    }

    if (pages >= 512)
    {
        if (hugepages_supported && pages >= 512 * 512)
        {
            // Allocate 'hugepages' consecutive 1GB pages
            unsigned long hugepages = ((pages - 1) >> 9 >> 9) + 1;

            // Check if this allocation can fit in the current level3 table
            if (++current_level3_index + hugepages > 512)
            {
                // Does not fit anymore in this table, request next empty table
                current_level1_index = 0;
                current_level2_index = 0;
                current_level3_index = 0;
                current_level3_table = paging_next_empty_level3_table();
            }

            // Fill page table with consecutive entries
            if (physical_address)
            {
                for (unsigned long i = 0; i < hugepages; i++)
                {
                    current_level3_table[current_level3_index + i] = ((unsigned long)physical_address + i * 512ul * 512ul * 4096ull) | PAGING_ENTRY_FLAG_PRESENT | PAGING_ENTRY_FLAG_WRITABLE;
                }
            }
            else
            {
                for (unsigned long i = 0; i < hugepages; i++)
                {
                    current_level3_table[current_level3_index + i] = (unsigned long)memory_physical_allocate_consecutive(512 * 512) | PAGING_ENTRY_FLAG_PRESENT | PAGING_ENTRY_FLAG_WRITABLE;
                }
            }

            used_virtual_pages += hugepages * 512 * 512;
            unsigned long level3_index = current_level3_index;
            current_level3_index += hugepages;
            return (unsigned long *)((current_level1_index << 12) | (current_level2_index << 21) | (level3_index << 30) | (current_level4_index << 39));
        }
        else
        {
            // Allocate 'hugepages' consecutive 2MB pages
            unsigned long hugepages = ((pages - 1) >> 9) + 1;

            // Check if this allocation can fit in the current level2 table
            if (++current_level2_index + hugepages > 512)
            {
                // Does not fit anymore in this table, request next empty table
                current_level1_index = 0;
                current_level2_index = 0;
                current_level2_table = paging_next_empty_level2_table();
            }

            // Fill page table with consecutive entries
            if (physical_address)
            {
                for (unsigned long i = 0; i < hugepages; i++)
                {
                    current_level2_table[current_level2_index + i] = ((unsigned long)physical_address + i * 512ul * 4096ull) | PAGING_ENTRY_FLAG_PRESENT | PAGING_ENTRY_FLAG_WRITABLE;
                }
            }
            else
            {
                for (unsigned long i = 0; i < hugepages; i++)
                {
                    current_level2_table[current_level2_index + i] = (unsigned long)memory_physical_allocate_consecutive(512) | PAGING_ENTRY_FLAG_PRESENT | PAGING_ENTRY_FLAG_WRITABLE;
                }
            }

            used_virtual_pages += hugepages * 512;
            unsigned long level2_index = current_level2_index;
            current_level2_index += hugepages;
            return (unsigned long *)((current_level1_index << 12) | (level2_index << 21) | (current_level3_index << 30) | (current_level4_index << 39));
        }
    }
    else
    {
        // Allocate 'hugepages' consecutive 4096 bytes pages
        // Check if this allocation can fit in the current level1 table
        if (++current_level1_index + pages > 512)
        {
            // Does not fit anymore in this table, request next empty table
            current_level1_index = 0;
            current_level1_table = paging_next_empty_level1_table();
        }

        // Fill page table with consecutive entries
        if (physical_address)
        {
            for (unsigned long i = 0; i < pages; i++)
            {
                current_level1_table[current_level1_index + i] = ((unsigned long)physical_address + i * 4096ull) | PAGING_ENTRY_FLAG_PRESENT | PAGING_ENTRY_FLAG_WRITABLE;
            }
        }
        else
        {
            for (unsigned long i = 0; i < pages; i++)
            {
                current_level1_table[current_level1_index + i] = (unsigned long)memory_physical_allocate() | PAGING_ENTRY_FLAG_PRESENT | PAGING_ENTRY_FLAG_WRITABLE;
            }
        }

        used_virtual_pages += pages;
        unsigned long level1_index = current_level1_index;
        current_level1_index += pages;
        return (unsigned long *)((level1_index << 12) | (current_level2_index << 21) | (current_level3_index << 30) | (current_level4_index << 39));
    }
}

// Allocates a single page of 4096 bytes. Free this page using paging_free
void *paging_map(void *physical_address, unsigned short flags)
{
    if (++current_level1_index >= 512)
    {
        // current_level2_table[current_level2_index] |= PAGING_ENTRY_FLAG_FULL;
        current_level1_index = 0;
        current_level1_table = paging_next_empty_level1_table();
    }

    current_level1_table[current_level1_index] = ((unsigned long)physical_address) | PAGING_ENTRY_FLAG_PRESENT | PAGING_ENTRY_FLAG_WRITABLE;
    used_virtual_pages++;

    // Return virtual address
    return (unsigned long *)((current_level1_index << 12) | (current_level2_index << 21) | (current_level3_index << 30) | (current_level4_index << 39));
}

void *paging_allocate(unsigned short flags)
{
    unsigned long *new_page = memory_physical_allocate();
    return paging_map(new_page, flags);
}

void paging_free(void *virtual_address)
{
    // TODO is there an instruction for this mess?

    unsigned int level4_index = ((unsigned long)virtual_address >> 39) & 0b111111111;
    unsigned long level4_entry = current_level4_table[level4_index];
    if (level4_entry == 0)
    {
        console_print("warning: paging_free tried to free unallocated page (level4_entry)\n");
        return;
    }

    unsigned long *level3_table = (unsigned long *)(level4_entry & PAGING_ADDRESS_MASK);
    unsigned int level3_index = ((unsigned long)virtual_address >> 30) & 0b111111111;
    unsigned long level3_entry = level3_table[level3_index];
    if (level3_entry == 0)
    {
        console_print("warning: paging_free tried to free unallocated page (level3_entry)\n");
        return;
    }
    if (level3_entry & PAGING_ENTRY_FLAG_SIZE)
    {
        // Free 1GB huge page
        level3_table[level3_index] = 0;
        used_virtual_pages -= 512 * 512;
        return;
    }

    unsigned long *level2_table = (unsigned long *)(level3_entry & PAGING_ADDRESS_MASK);
    unsigned int level2_index = ((unsigned long)virtual_address >> 21) & 0b111111111;
    unsigned long level2_entry = level2_table[level2_index];
    if (level2_entry == 0)
    {
        console_print("warning: paging_free tried to free unallocated page (level2_entry)\n");
        return;
    }
    if (level2_entry & PAGING_ENTRY_FLAG_SIZE)
    {
        // Free 2MB huge page
        level2_table[level2_index] = 0;
        used_virtual_pages -= 512;
        return;
    }

    unsigned long *level1_table = (unsigned long *)(level2_entry & PAGING_ADDRESS_MASK);
    unsigned int level1_index = ((unsigned long)virtual_address >> 12) & 0b111111111;
    unsigned long level1_entry = level1_table[level1_index];
    if (level1_entry == 0)
    {
        console_print("warning: paging_free tried to free unallocated page (level1_entry)\n");
        return;
    }

    // Null entire entry, including present flag
    level1_table[level1_index] = 0;
    used_virtual_pages--;

    // TODO unallocate empty table
}

void *paging_get_physical_address(void *virtual_address)
{
    unsigned long address = (unsigned long)virtual_address;

    unsigned int level4_offset = (address >> 39) & 0b111111111;
    unsigned long level4_entry = current_level4_table[level4_offset];

    // Check if page is present
    if (level4_entry == 0)
    {
        return 0;
    }

    unsigned long *level3_table = (unsigned long *)(level4_entry & PAGING_ADDRESS_MASK);
    unsigned int level3_offset = (address >> 30) & 0b111111111;
    unsigned long level3_entry = level3_table[level3_offset];

    // Check if page is present
    if (level3_entry == 0)
    {
        return 0;
    }

    // Check if this is 1gb page
    if (level3_entry & PAGING_ENTRY_FLAG_SIZE)
    {
        return (void *)((level3_entry & 0xFFFFFFFFFF000) + (address & 0x3FFFFFFF));
    }

    unsigned long *level2_table = (unsigned long *)(level3_entry & PAGING_ADDRESS_MASK);
    unsigned int level2_offset = (address >> 21) & 0b111111111;
    unsigned long level2_entry = level2_table[level2_offset];

    // Check if page is present
    if (level2_entry == 0)
    {
        return 0;
    }

    // Check if this is 2mb page
    if (level2_entry & PAGING_ENTRY_FLAG_SIZE)
    {
        return (void *)((level2_entry & 0xFFFFFFFFFF000) + (address & 0x1FFFFF));
    }

    unsigned long *level1_table = (unsigned long *)(level2_entry & PAGING_ADDRESS_MASK);
    unsigned int level1_offset = (address >> 12) & 0b111111111; // The other offset fields are 9 bit indices into their 512 entry tables
    unsigned long level1_entry = level1_table[level1_offset];

    // Check if page is present
    if (level1_entry == 0)
    {
        return 0;
    }

    unsigned int offset = address & 0b111111111111; // Offset is 12 bits because each page is 4096 bytes in size
    return (void *)((level1_entry & 0xFFFFFFFFFF000) + offset);
}

unsigned long paging_used_pages()
{
    return used_virtual_pages;
}
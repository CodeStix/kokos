#include "../include/paging.h"
#include "../include/console.h"
#include "../include/cpu.h"
#include "../include/util.h"
#include "../include/memory.h"
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
static unsigned long *paging_next_level3_table()
{
    if (used_virtual_pages >= PAGING_MAX_VIRTUAL_PAGES)
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
        if (entry != 0)
        {
            return entry & PAGING_ADDRESS_MASK;
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
static unsigned long *paging_next_level2_table()
{
    if (++current_level3_index >= 512)
    {
        current_level3_index = 0;
        current_level3_table = paging_next_level3_table();
    }

    while (1)
    {
        unsigned long entry = current_level3_table[current_level3_index];
        if (entry != 0)
        {
            if (entry & PAGING_ENTRY_FLAG_SIZE)
            {
                // When the size flag is set, it does not point to a table, but a huge page, skip it
                if (++current_level3_index >= 512)
                {
                    current_level3_index = 0;
                    current_level3_table = paging_next_level3_table();
                }
            }
            else
            {
                return entry & PAGING_ADDRESS_MASK;
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
static unsigned long *paging_next_level1_table()
{
    if (++current_level2_index >= 512)
    {
        current_level2_index = 0;
        current_level2_table = paging_next_level2_table();
    }

    while (1)
    {
        unsigned long entry = current_level2_table[current_level2_index];
        if (entry != 0)
        {
            if (entry & PAGING_ENTRY_FLAG_SIZE)
            {
                // When the size flag is set, it does not point to a table, but a huge page, skip it
                if (++current_level2_index >= 512)
                {
                    current_level2_index = 0;
                    current_level2_table = paging_next_level2_table();
                }
            }
            else
            {
                return entry & PAGING_ADDRESS_MASK;
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

void *paging_map_physical(void *physical_address_or_null, void *virtual_address_or_null, unsigned long bytes, unsigned long flags)
{
    if (bytes <= 0)
    {
        console_print("[paging_map_physical] cannot map 0 bytes\n");
        return 0;
    }

    // Convert function flags to page table entry compatible flags
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

    unsigned int level4_index = ((unsigned long)virtual_address_or_null >> 39) & 0b111111111;
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
        memory_zero(level3_table, 4096);
        level4_table[level4_index] = (unsigned long)level3_table | PAGING_ENTRY_FLAG_PRESENT | PAGING_ENTRY_FLAG_WRITABLE;
    }

    // Check if mapping huge pages (1GiB)
    unsigned int level3_index = ((unsigned long)virtual_address_or_null >> 30) & 0b111111111;
    unsigned long level3_entry = level3_table[level3_index];
    unsigned long *level2_table;
    if ((level3_entry & PAGING_ENTRY_FLAG_SIZE) || (flags & PAGING_FLAG_1GB))
    {
        if (level3_entry != 0 && !(flags & PAGING_FLAG_REPLACE))
        {
            console_print("[paging_map_physical] tried to map at already mapped address (1GiB)\n");
            return 0;
        }
        else
        {
            if ((unsigned long)physical_address_or_null & 0x3FFFFFFF)
            {
                console_print("[paging_map_physical] physical_address_or_null must be aligned to 0x3FFFFFFF bytes (1GiB, 1073741824 bytes) when mapping 1GB page!\n");
                return 0;
            }

            if (virtual_address_or_null)
            {
                if ((unsigned long)virtual_address_or_null & 0x3FFFFFFF)
                {
                    console_print("[paging_map_physical] virtual_address must be aligned to 0x3FFFFFFF bytes (1GiB, 1073741824 bytes) when mapping 1GB page!\n");
                    return 0;
                }
            }
            else
            {
                // Find fitting spot
            }

            unsigned long pages = ((bytes - 1) >> 30) + 1; // Divide by 1GiB
            for (unsigned long i = 0; i < pages; i++)
            {
                if (physical_address_or_null)
                {
                    level3_table[level3_index] = (((unsigned long)physical_address_or_null + 4096 * 512 * 512 * i) & PAGING_ADDRESS_MASK) | page_entry_flags | PAGING_ENTRY_FLAG_SIZE;
                }
                else
                {
                    // TODO not implemented
                    // level3_table[level3_index] = ((unsigned long)memory_physical_allocate_1gb() & PAGING_ADDRESS_MASK) | page_entry_flags | PAGING_ENTRY_FLAG_SIZE;
                    console_print("[paging_map_physical] 1GiB allocation not implemented!\n");
                    return 0;
                }

                if (++level3_index >= 512)
                {
                    if (++level4_index >= 512)
                    {
                        console_print("[paging_map_physical] failed to allocate 1GiB pages, reached end of virtual address space.\n");
                        return 0;
                    }
                    level3_table = current_level4_table[level4_index];
                    if (!level3_table)
                    {
                        level3_table = memory_physical_allocate();
                        memory_zero(level3_table, 4096);
                        current_level4_table[level4_index] = (unsigned long)level3_table | PAGING_ENTRY_FLAG_PRESENT | PAGING_ENTRY_FLAG_WRITABLE;
                    }
                    level3_index = 0;
                }
            }
            used_virtual_pages += pages * 512 * 512;
            return virtual_address_or_null;
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
            memory_zero(level2_table, 4096);
            level3_table[level3_index] = (unsigned long)level2_table | PAGING_ENTRY_FLAG_PRESENT | PAGING_ENTRY_FLAG_WRITABLE;
        }
    }

    // Check if mapping huge pages (2MiB)
    unsigned int level2_index = ((unsigned long)virtual_address_or_null >> 21) & 0b111111111;
    unsigned long level2_entry = level2_table[level2_index];
    unsigned long *level1_table;
    if ((level2_entry & PAGING_ENTRY_FLAG_SIZE) || (flags & PAGING_FLAG_2MB))
    {
        if (level2_entry != 0 && !(flags & PAGING_FLAG_REPLACE))
        {
            console_print("[paging_map_physical] tried to map at already mapped address (2MiB)\n");
            return 0;
        }
        else
        {
            if ((unsigned long)physical_address_or_null & 0x1FFFFF)
            {
                console_print("[paging_map_physical] physical_address_or_null must be aligned to 0x1FFFFF bytes (2MiB, 2097152 bytes) when mapping 2MiB page!\n");
                return 0;
            }

            if (virtual_address_or_null)
            {
                if ((unsigned long)virtual_address_or_null & 0x1FFFFF)
                {
                    console_print("[paging_map_physical] virtual_address must be aligned to 0x1FFFFF bytes (2MiB, 2097152 bytes) when mapping 2MiB page!\n");
                    return 0;
                }
            }
            else
            {
                // Find fitting spot
            }

            unsigned long pages = ((bytes - 1) >> 21) + 1; // Divide by 2MiB
            for (unsigned long i = 0; i < pages; i++)
            {
                if (physical_address_or_null)
                {
                    level2_table[level2_index] = (((unsigned long)physical_address_or_null + 4096 * 512 * i) & PAGING_ADDRESS_MASK) | page_entry_flags | PAGING_ENTRY_FLAG_SIZE;
                }
                else
                {
                    // TODO not implemented
                    // level2_table[level2_index] = ((unsigned long)memory_physical_allocate_2mb() & PAGING_ADDRESS_MASK) | page_entry_flags | PAGING_ENTRY_FLAG_SIZE;
                    console_print("[paging_map_physical] 2MiB allocation not implemented!\n");
                    return 0;
                }

                if (++level2_index >= 512)
                {
                    if (++level3_index >= 512)
                    {
                        if (++level4_index >= 512)
                        {
                            console_print("[paging_map_physical] failed to allocate 2MiB pages, reached end of virtual address space.\n");
                            return 0;
                        }
                        level3_table = (unsigned long *)(current_level4_table[level4_index] & PAGING_ADDRESS_MASK);
                        if (!level3_table)
                        {
                            level3_table = memory_physical_allocate();
                            memory_zero(level3_table, 4096);
                            current_level4_table[level4_index] = (unsigned long)level3_table | PAGING_ENTRY_FLAG_PRESENT | PAGING_ENTRY_FLAG_WRITABLE;
                        }
                        level3_index = 0;
                    }
                    level2_table = (unsigned long *)(level3_table[level3_index] & PAGING_ADDRESS_MASK);
                    if (!level2_table)
                    {
                        level2_table = memory_physical_allocate();
                        memory_zero(level2_table, 4096);
                        level3_table[level3_index] = (unsigned long)level2_table | PAGING_ENTRY_FLAG_PRESENT | PAGING_ENTRY_FLAG_WRITABLE;
                    }
                    level2_index = 0;
                }
            }

            used_virtual_pages += pages * 512;
            return virtual_address_or_null;
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
            memory_zero(level1_table, 4096);
            level2_table[level3_index] = (unsigned long)level1_table | PAGING_ENTRY_FLAG_PRESENT | PAGING_ENTRY_FLAG_WRITABLE;
        }
    }

    unsigned int level1_index = ((unsigned long)virtual_address_or_null >> 12) & 0b111111111;
    if (level1_table[level1_index] != 0)
    {
        console_print("[paging_map_physical] tried to map at already mapped address (4KiB)\n");
        return 0;
    }
    else
    {
        if ((unsigned long)physical_address_or_null & 0xFFF)
        {
            console_print("[paging_map_physical] physical_address_or_null must be aligned to 0x1000 bytes (4KiB, 4096 bytes)!\n");
            return 0;
        }

        if (virtual_address_or_null)
        {
            if ((unsigned long)virtual_address_or_null & 0xFFF)
            {
                console_print("[paging_map_physical] virtual_address_or_null must be aligned to 0x1000 bytes (4KiB, 4096 bytes)!\n");
                return 0;
            }
        }
        else
        {
            // Find fitting spot
        }

        unsigned long pages = ((bytes - 1) >> 12) + 1;
        for (unsigned long i = 0; i < pages; i++)
        {
            if (physical_address_or_null)
            {
                level1_table[level1_index] = ((unsigned long)(physical_address_or_null + 4096 * i) & PAGING_ADDRESS_MASK) | page_entry_flags;
            }
            else
            {
                level1_table[level1_index] = ((unsigned long)memory_physical_allocate() & PAGING_ADDRESS_MASK) | page_entry_flags;
            }

            if (++level1_index >= 512)
            {
                if (++level2_index >= 512)
                {
                    if (++level3_index >= 512)
                    {
                        if (++level4_index >= 512)
                        {
                            console_print("[paging_map_physical] failed to allocate 4KiB pages, reached end of virtual address space.\n");
                            return 0;
                        }
                        level3_table = (unsigned long *)(current_level4_table[level4_index] & PAGING_ADDRESS_MASK);
                        if (!level3_table)
                        {
                            level3_table = memory_physical_allocate();
                            memory_zero(level3_table, 4096);
                            current_level4_table[level4_index] = (unsigned long)level3_table | PAGING_ENTRY_FLAG_PRESENT | PAGING_ENTRY_FLAG_WRITABLE;
                        }
                        level3_index = 0;
                    }
                    level2_table = (unsigned long *)(level3_table[level3_index] & PAGING_ADDRESS_MASK);
                    if (!level2_table)
                    {
                        level2_table = memory_physical_allocate();
                        memory_zero(level2_table, 4096);
                        level3_table[level3_index] = (unsigned long)level2_table | PAGING_ENTRY_FLAG_PRESENT | PAGING_ENTRY_FLAG_WRITABLE;
                    }
                    level2_index = 0;
                }
                level1_table = (unsigned long *)(level2_table[level2_index] & PAGING_ADDRESS_MASK);
                if (!level1_table)
                {
                    level1_table = memory_physical_allocate();
                    memory_zero(level1_table, 4096);
                    level2_table[level2_index] = (unsigned long)level1_table | PAGING_ENTRY_FLAG_PRESENT | PAGING_ENTRY_FLAG_WRITABLE;
                }
                level1_index = 0;
            }
        }

        used_virtual_pages += pages;
        return virtual_address_or_null;
    }
}

void paging_free(void *virtual_address, unsigned long bytes)
{
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
        unsigned long pages = ((bytes - 1) >> 30) + 1; // Divide by 1GB
        if ((unsigned long)virtual_address & 0x3FFFFFFF)
        {
            console_print("[paging_free] virtual_address must be aligned to 0x3FFFFFFF bytes (1GiB, 1073741824 bytes) when freeing 1GB page!\n");
            return 0;
        }

        // Free 1GB huge pages
        for (unsigned long i = 0; i < pages; i++)
        {
            level3_table[level3_index] = 0;
            if (++level3_index >= 512)
            {
                if (++level4_index >= 512)
                {
                    console_print("[paging_free] invalid 1GiB free, reached end of level4 table\n");
                    return 0;
                }
                level3_table = (unsigned long *)(current_level4_table[level4_index] & PAGING_ADDRESS_MASK);
                if (!level3_table)
                {
                    console_print("[paging_free] invalid 1GiB free, entering null level 3 table\n");
                    return 0;
                }
                level3_index = 0;
            }
        }
        used_virtual_pages -= pages * 512 * 512;
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
        unsigned long pages = ((bytes - 1) >> 21) + 1; // Divide by 2MB
        if ((unsigned long)virtual_address & 0x1FFFFF)
        {
            console_print("[paging_free] virtual_address must be aligned to 0x1FFFFF bytes (2MiB, 2097152 bytes) when freeing 2MB page!\n");
            return 0;
        }

        // Free 2MB huge pages
        for (unsigned long i = 0; i < pages; i++)
        {
            level2_table[level2_index] = 0;
            if (++level2_index >= 512)
            {
                if (++level3_index >= 512)
                {
                    if (++level4_index >= 512)
                    {
                        console_print("[paging_free] invalid 2MiB free, reached end of level4 table\n");
                        return 0;
                    }
                    level3_table = (unsigned long *)(current_level4_table[level4_index] & PAGING_ADDRESS_MASK);
                    if (!level3_table)
                    {
                        console_print("[paging_free] invalid 2MiB free, entering null level 3 table\n");
                        return 0;
                    }
                    level3_index = 0;
                }
                level2_table = (unsigned long *)(level3_table[level3_index] & PAGING_ADDRESS_MASK);
                if (!level2_table)
                {
                    console_print("[paging_free] invalid 2MiB free, entering null level 2 table\n");
                    return 0;
                }
                level2_index = 0;
            }
        }
        used_virtual_pages -= pages * 512;
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

    unsigned long pages = ((bytes - 1) >> 12) + 1; // Divide by 4KB
    if ((unsigned long)virtual_address & 0xFFF)
    {
        console_print("[paging_free] virtual_address must be aligned to 0xFFF bytes (4KiB, 4096 bytes) when freeing page!\n");
        return 0;
    }

    for (unsigned long i = 0; i < pages; i++)
    {
        // Null entire entry, including present flag
        level1_table[level1_index] = 0;
        if (++level1_index >= 512)
        {
            if (++level2_index >= 512)
            {
                if (++level3_index >= 512)
                {
                    if (++level4_index >= 512)
                    {
                        console_print("[paging_free] invalid 4KiB free, reached end of level4 table\n");
                        return 0;
                    }
                    level3_table = (unsigned long *)(current_level4_table[level4_index] & PAGING_ADDRESS_MASK);
                    if (!level3_table)
                    {
                        console_print("[paging_free] invalid 2MiB free, entering null level 3 table\n");
                        return 0;
                    }
                    level3_index = 0;
                }
                level2_table = (unsigned long *)(level3_table[level3_index] & PAGING_ADDRESS_MASK);
                if (!level2_table)
                {
                    console_print("[paging_free] invalid 2MiB free, entering null level 2 table\n");
                    return 0;
                }
                level2_index = 0;
            }
            level1_table = (unsigned long *)(level2_table[level2_index] & PAGING_ADDRESS_MASK);
            if (!level1_table)
            {
                console_print("[paging_free] invalid 2MiB free, entering null level 1 table\n");
                return 0;
            }
            level1_index = 0;
        }
    }
    used_virtual_pages -= pages;

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
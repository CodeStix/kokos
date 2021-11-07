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

// Represents a location in the page structure
typedef struct PagingIndex
{
    unsigned long *level1_table; // The current level1 table selected by level2_index
    unsigned long *level2_table; // The current level2 table selected by level3_index
    unsigned long *level3_table; // The current level3 table selected by level4_index
    unsigned long *level4_table;
    unsigned short level1_index;
    unsigned short level2_index;
    unsigned short level3_index;
    unsigned short level4_index;
} PagingIndex;

void *paging_map_physical_any(void *physical_address_or_null, unsigned long bytes, unsigned long flags)
{
    if (bytes <= 0)
    {
        console_print("[paging_map_physical] cannot map 0 bytes\n");
        return 0;
    }

    PagingIndex *index;
    unsigned long page_flags;

    // If less than 2MiB is being allocated, it can fit in a single level1 table
    if (bytes <= 512 * 4096)
    {
        // Check if it still fits in the current level1 table
        unsigned long pages = ((bytes - 1) >> 12) + 1;
        if (index->level1_index + pages > 512)
        {
            // Does not fit anymore in current level1 table, find and allocate next empty level1 table
            while (index->level2_table[index->level2_index])
            {
                if (++index->level2_index >= 512)
                {
                    if (++index->level3_index >= 512)
                    {
                        if (++index->level4_index >= 512)
                        {
                            index->level4_index = 0;
                        }

                        index->level3_index = 0;
                        index->level3_table = (unsigned long *)(index->level4_table[index->level4_index] & PAGING_ADDRESS_MASK);
                        if (!index->level3_table)
                        {
                            index->level3_table = memory_physical_allocate();
                            memory_zero(index->level3_table, 4096);
                            index->level4_table[index->level4_index] = (unsigned long)index->level3_table | PAGING_ENTRY_FLAG_WRITABLE | PAGING_ENTRY_FLAG_PRESENT;
                        }
                    }

                    index->level2_index = 0;
                    index->level2_table = (unsigned long *)(index->level3_table[index->level3_index] & PAGING_ADDRESS_MASK);
                    if (!index->level2_table)
                    {
                        index->level2_table = memory_physical_allocate();
                        memory_zero(index->level2_table, 4096);
                        index->level3_table[index->level3_index] = (unsigned long)index->level2_table | PAGING_ENTRY_FLAG_WRITABLE | PAGING_ENTRY_FLAG_PRESENT;
                    }
                }
            }

            index->level1_index = 0;
            index->level1_table = memory_physical_allocate();
            memory_zero(index->level1_table, 4096);
            index->level2_table[index->level2_index] = (unsigned long)index->level1_table | PAGING_ENTRY_FLAG_WRITABLE | PAGING_ENTRY_FLAG_PRESENT;
        }

        paging_map_index(physical_address_or_null, index, bytes, flags);
    }
    else if (bytes <= 512 * 512 * 4096)
    {
        // Check 2th level table
        unsigned long hugepages = ((bytes - 1) >> 21) + 1;
        if (index->level2_index + hugepages > 512)
        {
            // Does not fit anymore in current level2 table, find and allocate next empty level2 table
            while (index->level3_table[index->level3_index])
            {
                if (++index->level3_index >= 512)
                {
                    if (++index->level4_index >= 512)
                    {
                        index->level4_index = 0;
                    }

                    index->level3_index = 0;
                    index->level3_table = (unsigned long *)(index->level4_table[index->level4_index] & PAGING_ADDRESS_MASK);
                    if (!index->level3_table)
                    {
                        index->level3_table = memory_physical_allocate();
                        memory_zero(index->level3_table, 4096);
                        index->level4_table[index->level4_index] = (unsigned long)index->level3_table | PAGING_ENTRY_FLAG_WRITABLE | PAGING_ENTRY_FLAG_PRESENT;
                    }
                }
            }

            index->level2_index = 0;
            index->level2_table = memory_physical_allocate();
            memory_zero(index->level2_table, 4096);
            index->level3_table[index->level3_index] = (unsigned long)index->level2_table | PAGING_ENTRY_FLAG_WRITABLE | PAGING_ENTRY_FLAG_PRESENT;
        }

        paging_map_index(physical_address_or_null, index, bytes, flags);
    }
    else if (bytes <= 512 * 512 * 512 * 4096)
    {
        // Check 3th level table
        unsigned long hugepages = ((bytes - 1) >> 30) + 1;
        if (index->level3_index + hugepages > 512)
        {
            // Does not fit anymore in current level3 table, find and allocate next empty level3 table
            while (index->level4_table[index->level4_index])
            {
                if (++index->level4_index >= 512)
                {
                    index->level4_index = 0;
                }
            }

            index->level3_index = 0;
            index->level3_table = memory_physical_allocate();
            memory_zero(index->level3_table, 4096);
            index->level4_table[index->level4_index] = (unsigned long)index->level3_table | PAGING_ENTRY_FLAG_WRITABLE | PAGING_ENTRY_FLAG_PRESENT;
        }

        paging_map_index(physical_address_or_null, index, bytes, flags);
    }
    else
    {
        // Check 4th level table
        unsigned long hugepages = ((bytes - 1) >> 39) + 1;
        if (index->level4_index + hugepages > 512)
        {
            // Does not fit anymore in current level4 table
            console_print("[paging_map_physical] exceeded available virtual memory\n");
            return 0;
        }

        paging_map_index(physical_address_or_null, index, bytes, flags);
    }
}

// Converts mapping function flags to actual page entry flags
static unsigned long paging_convert_flags(unsigned short flags)
{
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

    return page_entry_flags;
}

void *paging_map_physical(void *physical_address_or_null, void *virtual_address_or_null, unsigned long bytes, unsigned long flags)
{
    PagingIndex index;

    index.level4_table = current_level4_table;
    index.level4_index = ((unsigned long)virtual_address_or_null >> 39) & 0b111111111;

    index.level3_table = (unsigned long *)(index.level4_table[index.level4_index] & PAGING_ADDRESS_MASK);
    if (!index.level3_table)
    {
        index.level3_table = memory_physical_allocate();
        memory_zero(index.level3_table, 4096);
        index.level4_table[index.level4_index] = (unsigned long)index.level3_table | PAGING_ENTRY_FLAG_PRESENT | PAGING_ENTRY_FLAG_WRITABLE;
    }
    index.level3_index = ((unsigned long)virtual_address_or_null >> 30) & 0b111111111;

    index.level2_table = (unsigned long *)(index.level3_table[index.level3_index] & PAGING_ADDRESS_MASK);
    if (!index.level2_table)
    {
        index.level2_table = memory_physical_allocate();
        memory_zero(index.level2_table, 4096);
        index.level3_table[index.level3_index] = (unsigned long)index.level2_table | PAGING_ENTRY_FLAG_PRESENT | PAGING_ENTRY_FLAG_WRITABLE;
    }
    index.level2_index = ((unsigned long)virtual_address_or_null >> 21) & 0b111111111;

    index.level1_table = (unsigned long *)(index.level2_table[index.level2_index] & PAGING_ADDRESS_MASK);
    if (!index.level1_table)
    {
        index.level1_table = memory_physical_allocate();
        memory_zero(index.level1_table, 4096);
        index.level2_table[index.level3_index] = (unsigned long)index.level1_table | PAGING_ENTRY_FLAG_PRESENT | PAGING_ENTRY_FLAG_WRITABLE;
    }
    index.level1_index = ((unsigned long)virtual_address_or_null >> 12) & 0b111111111;

    paging_map_index(physical_address_or_null, &index, bytes, flags);
}

static void paging_map_index(void *physical_address_or_null, PagingIndex *index, unsigned long bytes, unsigned short flags)
{
    if (bytes <= 0)
    {
        console_print("[paging_map_physical] cannot map 0 bytes\n");
        return 0;
    }

    // Check if mapping huge pages (1GiB)
    if ((index->level3_table[index->level3_index] & PAGING_ENTRY_FLAG_SIZE) || (flags & PAGING_FLAG_1GB))
    {
        if (!hugepages_supported)
        {
            console_print("[paging_map_physical] huge pages not supported (1GiB)\n");
            return 0;
        }

        if (index->level3_table[index->level3_index] != 0 && !(flags & PAGING_FLAG_REPLACE))
        {
            console_print("[paging_map_physical] tried to map at already mapped address (1GiB)\n");
            return 0;
        }

        if ((unsigned long)physical_address_or_null & 0x3FFFFFFF)
        {
            console_print("[paging_map_physical] physical_address_or_null must be aligned to 0x3FFFFFFF bytes (1GiB, 1073741824 bytes) when mapping 1GB page!\n");
            return 0;
        }

        if (index->level1_index != 0 || index->level2_index != 0)
        {
            console_print("[paging_map_physical] virtual_address must be aligned to 0x3FFFFFFF bytes (1GiB, 1073741824 bytes) when mapping 1GB page!\n");
            return 0;
        }

        unsigned long pages = ((bytes - 1) >> 30) + 1; // Divide by 1GiB
        paging_map_1gb(physical_address_or_null, &index, pages, flags);
        used_virtual_pages += pages * 512 * 512;
    }

    // Check if mapping huge pages (2MiB)
    if ((index->level2_table[index->level2_index] & PAGING_ENTRY_FLAG_SIZE) || (flags & PAGING_FLAG_2MB))
    {
        if (index->level2_table[index->level2_index] != 0 && !(flags & PAGING_FLAG_REPLACE))
        {
            console_print("[paging_map_physical] tried to map at already mapped address (2MiB)\n");
            return 0;
        }

        if ((unsigned long)physical_address_or_null & 0x1FFFFF)
        {
            console_print("[paging_map_physical] physical_address_or_null must be aligned to 0x1FFFFF bytes (2MiB, 2097152 bytes) when mapping 2MiB page!\n");
            return 0;
        }

        if (index->level1_index != 0)
        {
            console_print("[paging_map_physical] virtual_address must be aligned to 0x1FFFFF bytes (2MiB, 2097152 bytes) when mapping 2MiB page!\n");
            return 0;
        }

        unsigned long pages = ((bytes - 1) >> 21) + 1; // Divide by 2MiB
        paging_map_2mb(physical_address_or_null, &index, pages, flags);

        used_virtual_pages += pages * 512;
    }

    // Map using 4KiB pages
    if (index->level1_table[index->level1_index] != 0)
    {
        console_print("[paging_map_physical] tried to map at already mapped address (4KiB)\n");
        return 0;
    }

    if ((unsigned long)physical_address_or_null & 0xFFF)
    {
        console_print("[paging_map_physical] physical_address_or_null must be aligned to 0x1000 bytes (4KiB, 4096 bytes)!\n");
        return 0;
    }

    unsigned long pages = ((bytes - 1) >> 12) + 1; // Divide by 4KiB
    paging_map_4kb(physical_address_or_null, &index, pages, flags);

    used_virtual_pages += pages;
}

// Maps a consecutive amount of 4KiB pages
static void paging_map_4kb(void *physical_address_or_null, PagingIndex *index, unsigned long pages, unsigned short flags)
{
    unsigned long page_entry_flags = paging_convert_flags(flags);
    for (unsigned long i = 0; i < pages; i++)
    {
        if (physical_address_or_null)
        {
            index->level1_table[index->level1_index] = ((unsigned long)(physical_address_or_null + 4096 * i) & PAGING_ADDRESS_MASK) | page_entry_flags;
        }
        else
        {
            index->level1_table[index->level1_index] = ((unsigned long)memory_physical_allocate() & PAGING_ADDRESS_MASK) | page_entry_flags;
        }

        if (++index->level1_index >= 512)
        {
            if (++index->level2_index >= 512)
            {
                if (++index->level3_index >= 512)
                {
                    if (++index->level4_index >= 512)
                    {
                        console_print("[paging_map_physical] failed to allocate 4KiB pages, reached end of virtual address space.\n");
                        return 0;
                    }

                    index->level3_index = 0;
                    index->level3_table = (unsigned long *)(index->level4_table[index->level4_index] & PAGING_ADDRESS_MASK);
                    if (!index->level3_table)
                    {
                        index->level3_table = memory_physical_allocate();
                        memory_zero(index->level3_table, 4096);
                        index->level4_table[index->level4_index] = (unsigned long)index->level3_table | PAGING_ENTRY_FLAG_PRESENT | PAGING_ENTRY_FLAG_WRITABLE;
                    }
                }

                index->level2_index = 0;
                index->level2_table = (unsigned long *)(index->level3_table[index->level3_index] & PAGING_ADDRESS_MASK);
                if (!index->level2_table)
                {
                    index->level2_table = memory_physical_allocate();
                    memory_zero(index->level2_table, 4096);
                    index->level3_table[index->level3_index] = (unsigned long)index->level2_table | PAGING_ENTRY_FLAG_PRESENT | PAGING_ENTRY_FLAG_WRITABLE;
                }
            }

            index->level1_index = 0;
            index->level1_table = (unsigned long *)(index->level2_table[index->level2_index] & PAGING_ADDRESS_MASK);
            if (!index->level1_table)
            {
                index->level1_table = memory_physical_allocate();
                memory_zero(index->level1_table, 4096);
                index->level2_table[index->level2_index] = (unsigned long)index->level1_table | PAGING_ENTRY_FLAG_PRESENT | PAGING_ENTRY_FLAG_WRITABLE;
            }
        }
    }
}

// Maps a consecutive amount of 2MiB pages
static void paging_map_2mb(void *physical_address_or_null, PagingIndex *index, unsigned long pages, unsigned short flags)
{
    unsigned long page_entry_flags = paging_convert_flags(flags);
    for (unsigned long i = 0; i < pages; i++)
    {
        if (physical_address_or_null)
        {
            index->level2_table[index->level2_index] = (((unsigned long)physical_address_or_null + 4096 * 512 * i) & PAGING_ADDRESS_MASK) | page_entry_flags | PAGING_ENTRY_FLAG_SIZE;
        }
        else
        {
            // TODO not implemented
            // index->level2_table[index->level2_index] = ((unsigned long)memory_physical_allocate_2mb() & PAGING_ADDRESS_MASK) | page_entry_flags | PAGING_ENTRY_FLAG_SIZE;
            console_print("[paging_map_physical] 2MiB physical allocation not implemented yet!\n");
            return 0;
        }

        if (++index->level2_index >= 512)
        {
            if (++index->level3_index >= 512)
            {
                if (++index->level4_index >= 512)
                {
                    console_print("[paging_map_physical] failed to allocate 2MiB pages, reached end of virtual address space.\n");
                    return 0;
                }

                index->level3_index = 0;
                index->level3_table = (unsigned long *)(index->level4_table[index->level4_index] & PAGING_ADDRESS_MASK);
                if (!index->level3_table)
                {
                    index->level3_table = memory_physical_allocate();
                    memory_zero(index->level3_table, 4096);
                    index->level4_table[index->level4_index] = (unsigned long)index->level3_table | PAGING_ENTRY_FLAG_PRESENT | PAGING_ENTRY_FLAG_WRITABLE;
                }
            }

            index->level2_index = 0;
            index->level2_table = (unsigned long *)(index->level3_table[index->level3_index] & PAGING_ADDRESS_MASK);
            if (!index->level2_table)
            {
                index->level2_table = memory_physical_allocate();
                memory_zero(index->level2_table, 4096);
                index->level3_table[index->level3_index] = (unsigned long)index->level2_table | PAGING_ENTRY_FLAG_PRESENT | PAGING_ENTRY_FLAG_WRITABLE;
            }
        }
    }
}

// Maps a consecutive amount of 1GiB pages
static void paging_map_1gb(void *physical_address_or_null, PagingIndex *index, unsigned long pages, unsigned short flags)
{
    unsigned long page_entry_flags = paging_convert_flags(flags);
    for (unsigned long i = 0; i < pages; i++)
    {
        if (physical_address_or_null)
        {
            index->level3_table[index->level3_index] = (((unsigned long)physical_address_or_null + 4096 * 512 * 512 * i) & PAGING_ADDRESS_MASK) | page_entry_flags | PAGING_ENTRY_FLAG_SIZE;
        }
        else
        {
            // TODO not implemented
            // index->level3_table[index->level3_index] = ((unsigned long)memory_physical_allocate_1gb() & PAGING_ADDRESS_MASK) | page_entry_flags | PAGING_ENTRY_FLAG_SIZE;
            console_print("[paging_map_physical] 1GiB physical allocation not implemented yet!\n");
            return 0;
        }

        if (++index->level3_index >= 512)
        {
            if (++index->level4_index >= 512)
            {
                console_print("[paging_map_physical] failed to allocate 1GiB pages, reached end of virtual address space.\n");
                return 0;
            }

            index->level3_index = 0;
            index->level3_table = index->level4_table[index->level4_index];
            if (!index->level3_table)
            {
                index->level3_table = memory_physical_allocate();
                memory_zero(index->level3_table, 4096);
                index->level4_table[index->level4_index] = (unsigned long)index->level3_table | PAGING_ENTRY_FLAG_PRESENT | PAGING_ENTRY_FLAG_WRITABLE;
            }
        }
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

                level3_index = 0;
                level3_table = (unsigned long *)(current_level4_table[level4_index] & PAGING_ADDRESS_MASK);
                if (!level3_table)
                {
                    console_print("[paging_free] invalid 1GiB free, entering null level 3 table\n");
                    return 0;
                }
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

                    level3_index = 0;
                    level3_table = (unsigned long *)(current_level4_table[level4_index] & PAGING_ADDRESS_MASK);
                    if (!level3_table)
                    {
                        console_print("[paging_free] invalid 2MiB free, entering null level 3 table\n");
                        return 0;
                    }
                }

                level2_index = 0;
                level2_table = (unsigned long *)(level3_table[level3_index] & PAGING_ADDRESS_MASK);
                if (!level2_table)
                {
                    console_print("[paging_free] invalid 2MiB free, entering null level 2 table\n");
                    return 0;
                }
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

                    level3_index = 0;
                    level3_table = (unsigned long *)(current_level4_table[level4_index] & PAGING_ADDRESS_MASK);
                    if (!level3_table)
                    {
                        console_print("[paging_free] invalid 2MiB free, entering null level 3 table\n");
                        return 0;
                    }
                }

                level2_index = 0;
                level2_table = (unsigned long *)(level3_table[level3_index] & PAGING_ADDRESS_MASK);
                if (!level2_table)
                {
                    console_print("[paging_free] invalid 2MiB free, entering null level 2 table\n");
                    return 0;
                }
            }

            level1_index = 0;
            level1_table = (unsigned long *)(level2_table[level2_index] & PAGING_ADDRESS_MASK);
            if (!level1_table)
            {
                console_print("[paging_free] invalid 2MiB free, entering null level 1 table\n");
                return 0;
            }
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
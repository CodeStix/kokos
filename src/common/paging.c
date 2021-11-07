#include "../include/paging.h"
#include "../include/console.h"
#include "../include/cpu.h"
#include "../include/util.h"
#include "../include/memory.h"
#include "../include/memory_physical.h"

static unsigned long used_virtual_pages = 0;
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

void paging_initialize()
{
    // Each table has 512 other tables, there are 4 types of nested tables
    used_virtual_pages = 0;

    // Check if hugepages is supported using cpuid
    CpuIdResult result = cpu_id(0x80000001);
    hugepages_supported = result.edx & CPU_ID_1GB_PAGES_EDX;
}

// Updates the paging index so that it points to a spot where x sequential bytes of virtual memory can be allocated
static int paging_index_find_spot(PagingIndex *index, unsigned long bytes)
{
    if (bytes <= 0)
    {
        return 1;
    }

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
    }
    else
    {
        // Check 4th level table
        unsigned long hugepages = ((bytes - 1) >> 39) + 1;
        if (index->level4_index + hugepages > 512)
        {
            // Does not fit anymore in current level4 table
            console_print("[paging_index_find_spot] exceeded available virtual memory\n");
            return 0;
        }
    }
    return 1;
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

// Maps a consecutive amount of 4KiB pages
static int paging_map_4kb(void *physical_address_or_null, PagingIndex *index, unsigned long pages, unsigned short flags)
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
                        console_print("[paging_map_4kb] failed to allocate 4KiB pages, reached end of virtual address space.\n");
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
    return 1;
}

// Maps a consecutive amount of 2MiB pages
static int paging_map_2mb(void *physical_address_or_null, PagingIndex *index, unsigned long pages, unsigned short flags)
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
            console_print("[paging_map_2mb] 2MiB physical allocation not implemented yet!\n");
            return 0;
        }

        if (++index->level2_index >= 512)
        {
            if (++index->level3_index >= 512)
            {
                if (++index->level4_index >= 512)
                {
                    console_print("[paging_map_2mb] failed to allocate 2MiB pages, reached end of virtual address space.\n");
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
    return 1;
}

// Maps a consecutive amount of 1GiB pages
static int paging_map_1gb(void *physical_address_or_null, PagingIndex *index, unsigned long pages, unsigned short flags)
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
            console_print("[paging_map_1gb] 1GiB physical allocation not implemented yet!\n");
            return 0;
        }

        if (++index->level3_index >= 512)
        {
            if (++index->level4_index >= 512)
            {
                console_print("[paging_map_1gb] failed to allocate 1GiB pages, reached end of virtual address space.\n");
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
    return 1;
}

static int paging_map_index(void *physical_address_or_null, PagingIndex *index, unsigned long bytes, unsigned short flags)
{
    if (bytes <= 0)
    {
        console_print("[paging_map_index] cannot map 0 bytes\n");
        return 0;
    }

    // Check if mapping huge pages (1GiB)
    if ((index->level3_table[index->level3_index] & PAGING_ENTRY_FLAG_SIZE) || (flags & PAGING_FLAG_1GB))
    {
        if (!hugepages_supported)
        {
            console_print("[paging_map_index] huge pages not supported (1GiB)\n");
            return 0;
        }

        if (index->level3_table[index->level3_index] != 0 && !(flags & PAGING_FLAG_REPLACE))
        {
            console_print("[paging_map_index] tried to map at already mapped address (1GiB)\n");
            return 0;
        }

        if ((unsigned long)physical_address_or_null & 0x3FFFFFFF)
        {
            console_print("[paging_map_index] physical_address_or_null must be aligned to 0x3FFFFFFF bytes (1GiB, 1073741824 bytes) when mapping 1GB page!\n");
            return 0;
        }

        if (index->level1_index != 0 || index->level2_index != 0)
        {
            console_print("[paging_map_index] virtual_address must be aligned to 0x3FFFFFFF bytes (1GiB, 1073741824 bytes) when mapping 1GB page!\n");
            return 0;
        }

        unsigned long pages = ((bytes - 1) >> 30) + 1; // Divide by 1GiB
        used_virtual_pages += pages * 512 * 512;
        return paging_map_1gb(physical_address_or_null, index, pages, flags);
    }

    // Check if mapping huge pages (2MiB)
    if ((index->level2_table[index->level2_index] & PAGING_ENTRY_FLAG_SIZE) || (flags & PAGING_FLAG_2MB))
    {
        if (index->level2_table[index->level2_index] != 0 && !(flags & PAGING_FLAG_REPLACE))
        {
            console_print("[paging_map_index] tried to map at already mapped address (2MiB)\n");
            return 0;
        }

        if ((unsigned long)physical_address_or_null & 0x1FFFFF)
        {
            console_print("[paging_map_index] physical_address_or_null must be aligned to 0x1FFFFF bytes (2MiB, 2097152 bytes) when mapping 2MiB page!\n");
            return 0;
        }

        if (index->level1_index != 0)
        {
            console_print("[paging_map_index] virtual_address must be aligned to 0x1FFFFF bytes (2MiB, 2097152 bytes) when mapping 2MiB page!\n");
            return 0;
        }

        unsigned long pages = ((bytes - 1) >> 21) + 1; // Divide by 2MiB
        used_virtual_pages += pages * 512;
        return paging_map_2mb(physical_address_or_null, index, pages, flags);
    }

    // Map using 4KiB pages
    if (index->level1_table[index->level1_index] != 0)
    {
        console_print("[paging_map_index] tried to map at already mapped address (4KiB)\n");
        return 0;
    }

    if ((unsigned long)physical_address_or_null & 0xFFF)
    {
        console_print("[paging_map_index] physical_address_or_null must be aligned to 0x1000 bytes (4KiB, 4096 bytes)!\n");
        return 0;
    }

    unsigned long pages = ((bytes - 1) >> 12) + 1; // Divide by 4KiB
    used_virtual_pages += pages;
    return paging_map_4kb(physical_address_or_null, index, pages, flags);
}

void *paging_map(void *physical_address_or_null, void *virtual_address_or_null, unsigned long bytes, unsigned long flags)
{
    Cpu *cpu = cpu_get_current();
    if (virtual_address_or_null)
    {
        if ((unsigned long)virtual_address_or_null & 0xFFF)
        {
            console_print("[paging_map] virtual_address must be aligned to 0xFFF bytes (4KiB, 4096 bytes) when mapping page!\n");
            return 0;
        }

        // A virtual address was given, only get the uppermost (level 4) page table from the current process
        PagingIndex index;
        index.level4_table = cpu->current_process->paging_index.level4_table;
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

        if (paging_map_index(physical_address_or_null, &index, bytes, flags))
        {
            return virtual_address_or_null;
        }
        else
        {
            return 0;
        }
    }
    else
    {
        // Find a spot to allocate in the current process' virtual memory
        PagingIndex *index = &cpu->current_process->paging_index;
        if (paging_index_find_spot(index, bytes))
        {
            void *virtual_address = (void *)(((unsigned long)index->level1_index << 12) | ((unsigned long)index->level2_index << 21) | ((unsigned long)index->level3_index << 30) | ((unsigned long)index->level4_index << 39));
            if (paging_map_index(physical_address_or_null, index, bytes, flags))
            {
                return virtual_address;
            }
            else
            {
                // Mapping failed for some reason
                return 0;
            }
        }
        else
        {
            // Could not find a spot
            return 0;
        }
    }
}

int paging_unmap(void *virtual_address, unsigned long bytes)
{
    Cpu *cpu = cpu_get_current();

    unsigned long *level4_table = cpu->current_process->paging_index.level4_table;
    unsigned int level4_index = ((unsigned long)virtual_address >> 39) & 0b111111111;
    unsigned long level4_entry = level4_table[level4_index];
    if (level4_entry == 0)
    {
        console_print("[paging_unmap] tried to unmap unmapped page (level4_entry)\n");
        return 0;
    }

    unsigned long *level3_table = (unsigned long *)(level4_entry & PAGING_ADDRESS_MASK);
    unsigned int level3_index = ((unsigned long)virtual_address >> 30) & 0b111111111;
    unsigned long level3_entry = level3_table[level3_index];
    if (level3_entry == 0)
    {
        console_print("[paging_unmap] tried to unmap unmapped page (level3_entry)\n");
        return 0;
    }
    if (level3_entry & PAGING_ENTRY_FLAG_SIZE)
    {
        unsigned long pages = ((bytes - 1) >> 30) + 1; // Divide by 1GB
        if ((unsigned long)virtual_address & 0x3FFFFFFF)
        {
            console_print("[paging_unmap] virtual_address must be aligned to 0x3FFFFFFF bytes (1GiB, 1073741824 bytes) when unmapping 1GB page!\n");
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
                    console_print("[paging_unmap] invalid 1GiB unmap, reached end of level4 table\n");
                    return 0;
                }

                level3_index = 0;
                level3_table = (unsigned long *)(level4_table[level4_index] & PAGING_ADDRESS_MASK);
                if (!level3_table)
                {
                    console_print("[paging_unmap] invalid 1GiB unmap, entering null level 3 table\n");
                    return 0;
                }
            }
        }

        used_virtual_pages -= pages * 512 * 512;
        return 1;
    }

    unsigned long *level2_table = (unsigned long *)(level3_entry & PAGING_ADDRESS_MASK);
    unsigned int level2_index = ((unsigned long)virtual_address >> 21) & 0b111111111;
    unsigned long level2_entry = level2_table[level2_index];
    if (level2_entry == 0)
    {
        console_print("[paging_unmap] tried to unmap unallocated page (level2_entry)\n");
        return 0;
    }
    if (level2_entry & PAGING_ENTRY_FLAG_SIZE)
    {
        unsigned long pages = ((bytes - 1) >> 21) + 1; // Divide by 2MB
        if ((unsigned long)virtual_address & 0x1FFFFF)
        {
            console_print("[paging_unmap] virtual_address must be aligned to 0x1FFFFF bytes (2MiB, 2097152 bytes) when unmapping 2MB page!\n");
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
                        console_print("[paging_unmap] invalid 2MiB unmap, reached end of level4 table\n");
                        return 0;
                    }

                    level3_index = 0;
                    level3_table = (unsigned long *)(level4_table[level4_index] & PAGING_ADDRESS_MASK);
                    if (!level3_table)
                    {
                        console_print("[paging_unmap] invalid 2MiB unmap, entering null level 3 table\n");
                        return 0;
                    }
                }

                level2_index = 0;
                level2_table = (unsigned long *)(level3_table[level3_index] & PAGING_ADDRESS_MASK);
                if (!level2_table)
                {
                    console_print("[paging_unmap] invalid 2MiB unmap, entering null level 2 table\n");
                    return 0;
                }
            }
        }

        used_virtual_pages -= pages * 512;
        return 1;
    }

    unsigned long *level1_table = (unsigned long *)(level2_entry & PAGING_ADDRESS_MASK);
    unsigned int level1_index = ((unsigned long)virtual_address >> 12) & 0b111111111;
    unsigned long level1_entry = level1_table[level1_index];
    if (level1_entry == 0)
    {
        console_print("[paging_unmap] tried to unmap unmapped page (level1_entry)\n");
        return 0;
    }

    unsigned long pages = ((bytes - 1) >> 12) + 1; // Divide by 4KB
    if ((unsigned long)virtual_address & 0xFFF)
    {
        console_print("[paging_unmap] virtual_address must be aligned to 0xFFF bytes (4KiB, 4096 bytes) when unmapping page!\n");
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
                        console_print("[paging_unmap] invalid 4KiB unmap, reached end of level4 table\n");
                        return 0;
                    }

                    level3_index = 0;
                    level3_table = (unsigned long *)(level4_table[level4_index] & PAGING_ADDRESS_MASK);
                    if (!level3_table)
                    {
                        console_print("[paging_unmap] invalid 2MiB unmap, entering null level 3 table\n");
                        return 0;
                    }
                }

                level2_index = 0;
                level2_table = (unsigned long *)(level3_table[level3_index] & PAGING_ADDRESS_MASK);
                if (!level2_table)
                {
                    console_print("[paging_unmap] invalid 2MiB unmap, entering null level 2 table\n");
                    return 0;
                }
            }

            level1_index = 0;
            level1_table = (unsigned long *)(level2_table[level2_index] & PAGING_ADDRESS_MASK);
            if (!level1_table)
            {
                console_print("[paging_unmap] invalid 2MiB unmap, entering null level 1 table\n");
                return 0;
            }
        }
    }

    // TODO unallocate empty table
    used_virtual_pages -= pages;
    return 1;
}

void *paging_get_physical_address(void *virtual_address)
{
    unsigned long address = (unsigned long)virtual_address;

    Cpu *cpu = cpu_get_current();

    unsigned long *level4_table = cpu->current_process->paging_index.level4_table;
    unsigned int level4_offset = (address >> 39) & 0b111111111;
    unsigned long level4_entry = level4_table[level4_offset];

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

void paging_debug()
{

    Cpu *cpu = cpu_get_current();

    PagingIndex *index = &cpu->current_process->paging_index;

    console_print("level4 0x");
    console_print_u64(index->level4_table, 16);
    console_print(" [0x000000000000 .. 0xffffffffffff]\n");

    for (int level4_index = 0; level4_index < 512; level4_index++)
    {
        unsigned long level4_entry = index->level4_table[level4_index];
        if (!level4_entry)
            continue;

        unsigned long *level3_table = (unsigned long *)(level4_entry & PAGING_ADDRESS_MASK);
        unsigned long level3_starting_address = (unsigned long)level4_index * 512 * 512 * 512 * 4096;
        unsigned long level3_ending_address = ((unsigned long)level4_index + 1) * 512 * 512 * 512 * 4096 - 1;

        console_print("|- level3 0x");
        console_print_u64(level3_table, 16);
        console_print(" [0x");
        console_print_u64(level3_starting_address, 16);
        console_print(" .. 0x");
        console_print_u64(level3_ending_address, 16);
        console_print("]\n");

        for (int level3_index = 0; level3_index < 512; level3_index++)
        {
            unsigned long level3_entry = level3_table[level3_index];
            if (!level3_entry)
                continue;

            unsigned long *level2_table = (unsigned long *)(level3_entry & PAGING_ADDRESS_MASK);
            unsigned long level2_starting_address = level3_starting_address + (unsigned long)level3_index * 512 * 512 * 4096;
            unsigned long level2_ending_address = level3_starting_address + (((unsigned long)level3_index + 1) * 512 * 512 * 4096 - 1);

            if (level3_entry & PAGING_ENTRY_FLAG_SIZE)
                console_print("|- hugepage1gb 0x");
            else
                console_print("   |- level2 0x");
            console_print_u64(level2_table, 16);
            console_print(" [0x");
            console_print_u64(level2_starting_address, 16);
            console_print(" .. 0x");
            console_print_u64(level2_ending_address, 16);
            console_print("]\n");

            for (int level2_index = 0; level2_index < 512; level2_index++)
            {
                unsigned long level2_entry = level2_table[level2_index];
                if (!level2_entry)
                    continue;

                unsigned long *level1_table = (unsigned long *)(level2_entry & PAGING_ADDRESS_MASK);
                unsigned long level1_starting_address = level2_starting_address + (unsigned long)level2_index * 512 * 4096;
                unsigned long level1_ending_address = level2_starting_address + ((unsigned long)level2_index + 1) * 512 * 4096 - 1;

                if (level2_entry & PAGING_ENTRY_FLAG_SIZE)
                    console_print("      |- hugepage2mb 0x");
                else
                    console_print("      |- level1 0x");
                console_print_u64(level1_table, 16);
                console_print(" [0x");
                console_print_u64(level1_starting_address, 16);
                console_print(" .. 0x");
                console_print_u64(level1_ending_address, 16);
                console_print("]\n");

                for (int level1_index = 0; level1_index < 512; level1_index++)
                {
                    unsigned long level1_entry = level1_table[level1_index];
                    if (!level1_entry)
                        continue;

                    unsigned long *page = (unsigned long *)(level1_entry & PAGING_ADDRESS_MASK);
                    unsigned long page_starting_address = level1_starting_address + (unsigned long)level1_index * 4096;
                    unsigned long page_ending_address = level1_starting_address + ((unsigned long)level1_index + 1) * 4096 - 1;

                    console_print("         |- page 0x");
                    console_print_u64(page, 16);
                    console_print(" [0x");
                    console_print_u64(page_starting_address, 16);
                    console_print(" .. 0x");
                    console_print_u64(page_ending_address, 16);
                    console_print("]\n");
                }
            }
        }
    }
}
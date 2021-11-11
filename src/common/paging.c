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
    // https://kokos.run/#WzAsIkFNRDY0Vm9sdW1lMy5wZGYiLDY1NCxbNjU0LDg0LDY1NCw4NF1d
    CpuIdResult result = cpu_id(0x80000001);
    hugepages_supported = result.edx & CPU_ID_1GB_PAGES_EDX;
}

// Updates the paging index so that it points to a spot where x sequential bytes of virtual memory can be allocated
static int paging_index_find_spot(PagingContext *index, unsigned long bytes, unsigned long flags)
{
    if (bytes <= 0)
    {
        return 0;
    }

    // If less than 2MiB is being allocated, it can fit in a single level1 table
    if (bytes <= 512ul * 4096ul && index->level2_table != 0 && !(flags & PAGING_FLAG_1GB))
    {
        // Check if it still fits in the current level1 table
        unsigned long pages = ((bytes - 1) >> 12) + 1;
        if (index->level1_index + pages > 512ul || index->level1_table == 0)
        {
            // Does not fit anymore in current level1 table, find and allocate next empty level1 table
            while (index->level2_table[index->level2_index])
            {
                if (++index->level2_index >= 512ul)
                {
                    if (++index->level3_index >= 512ul)
                    {
                        if (++index->level4_index >= 512ul)
                        {
                            index->level4_index = 0;
                        }

                        index->level3_index = 0;
                        index->level3_table = (unsigned long *)(index->level4_table[index->level4_index] & PAGING_ADDRESS_MASK);
                        if (!index->level3_table)
                        {
                            index->level3_table = memory_physical_allocate();
                            memory_zero(index->level3_table, 4096ul);
                            index->level4_table[index->level4_index] = (unsigned long)index->level3_table | PAGING_ENTRY_FLAG_WRITABLE | PAGING_ENTRY_FLAG_PRESENT;
                        }
                    }

                    index->level2_index = 0;
                    index->level2_table = (unsigned long *)(index->level3_table[index->level3_index] & PAGING_ADDRESS_MASK);
                    if (!index->level2_table)
                    {
                        index->level2_table = memory_physical_allocate();
                        memory_zero(index->level2_table, 4096ul);
                        index->level3_table[index->level3_index] = (unsigned long)index->level2_table | PAGING_ENTRY_FLAG_WRITABLE | PAGING_ENTRY_FLAG_PRESENT;
                    }
                }
            }

            if (!(flags & PAGING_FLAG_2MB))
            {
                index->level1_index = 0;
                index->level1_table = memory_physical_allocate();
                memory_zero(index->level1_table, 4096ul);
                index->level2_table[index->level2_index] = (unsigned long)index->level1_table | PAGING_ENTRY_FLAG_WRITABLE | PAGING_ENTRY_FLAG_PRESENT;
            }
        }
    }
    else if (bytes <= 512ul * 512ul * 4096ul && index->level3_table != 0)
    {
        // Check 2th level table
        unsigned long hugepages = ((bytes - 1) >> 21) + 1;
        if (index->level2_index + hugepages > 512 || index->level2_table == 0)
        {
            // Does not fit anymore in current level2 table, find and allocate next empty level2 table
            while (index->level3_table[index->level3_index])
            {
                if (++index->level3_index >= 512ul)
                {
                    if (++index->level4_index >= 512ul)
                    {
                        index->level4_index = 0;
                    }

                    index->level3_index = 0;
                    index->level3_table = (unsigned long *)(index->level4_table[index->level4_index] & PAGING_ADDRESS_MASK);
                    if (!index->level3_table)
                    {
                        index->level3_table = memory_physical_allocate();
                        memory_zero(index->level3_table, 4096ul);
                        index->level4_table[index->level4_index] = (unsigned long)index->level3_table | PAGING_ENTRY_FLAG_WRITABLE | PAGING_ENTRY_FLAG_PRESENT;
                    }
                }
            }

            if (!(flags & PAGING_FLAG_1GB))
            {
                index->level2_index = 0;
                index->level2_table = memory_physical_allocate();
                memory_zero(index->level2_table, 4096ul);
                index->level3_table[index->level3_index] = (unsigned long)index->level2_table | PAGING_ENTRY_FLAG_WRITABLE | PAGING_ENTRY_FLAG_PRESENT;

                if (!(flags & PAGING_FLAG_2MB))
                {
                    index->level1_index = 0;
                    index->level1_table = memory_physical_allocate();
                    memory_zero(index->level1_table, 4096ul);
                    index->level2_table[index->level2_index] = (unsigned long)index->level1_table | PAGING_ENTRY_FLAG_WRITABLE | PAGING_ENTRY_FLAG_PRESENT;
                }
            }
        }
    }
    else if (bytes <= 512ul * 512ul * 512ul * 4096ul)
    {
        // Check 3th level table
        unsigned long hugepages = ((bytes - 1) >> 30) + 1;
        if (index->level3_index + hugepages > 512ul || index->level3_table == 0)
        {
            // Does not fit anymore in current level3 table, find and allocate next empty level3 table
            while (index->level4_table[index->level4_index])
            {
                if (++index->level4_index >= 512ul)
                {
                    index->level4_index = 0;
                }
            }

            index->level3_index = 0;
            index->level3_table = memory_physical_allocate();
            memory_zero(index->level3_table, 4096ul);
            index->level4_table[index->level4_index] = (unsigned long)index->level3_table | PAGING_ENTRY_FLAG_WRITABLE | PAGING_ENTRY_FLAG_PRESENT;

            if (!(flags & PAGING_FLAG_1GB))
            {
                index->level2_index = 0;
                index->level2_table = memory_physical_allocate();
                memory_zero(index->level2_table, 4096ul);
                index->level3_table[index->level3_index] = (unsigned long)index->level2_table | PAGING_ENTRY_FLAG_WRITABLE | PAGING_ENTRY_FLAG_PRESENT;

                if (!(flags & PAGING_FLAG_2MB))
                {
                    index->level1_index = 0;
                    index->level1_table = memory_physical_allocate();
                    memory_zero(index->level1_table, 4096ul);
                    index->level2_table[index->level2_index] = (unsigned long)index->level1_table | PAGING_ENTRY_FLAG_WRITABLE | PAGING_ENTRY_FLAG_PRESENT;
                }
            }
        }
    }
    else
    {
        // Check 4th level table
        unsigned long hugepages = ((bytes - 1) >> 39) + 1;
        if (index->level4_index + hugepages > 512ul)
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

static int paging_map_index_physical(void *physical_address, PagingContext *index, unsigned long bytes, unsigned short flags)
{
    if (bytes <= 0)
    {
        console_print("[paging_map_index_physical] cannot map 0 bytes\n");
        return 0;
    }

    unsigned long page_entry_flags = paging_convert_flags(flags);

    // Check if mapping huge pages (1GiB)
    if ((index->level3_table[index->level3_index] & PAGING_ENTRY_FLAG_SIZE) || (flags & PAGING_FLAG_1GB))
    {
        if (!hugepages_supported)
        {
            console_print("[paging_map_index_physical] huge pages not supported (1GiB)\n");
            return 0;
        }

        if ((unsigned long)physical_address & 0x3FFFFFFFul)
        {
            console_print("[paging_map_index_physical] physical_address must be aligned to 0x40000000 bytes (1GiB, 1073741824 bytes) when mapping 1GB page!\n");
            return 0;
        }

        if (index->level3_table[index->level3_index] != 0 && !(flags & PAGING_FLAG_REPLACE))
        {
            console_print("[paging_map_index_physical] tried to map at already mapped address (1GiB)\n");
            return 0;
        }

        unsigned long pages = ((bytes - 1) >> 30) + 1; // Divide by 1GiB
        for (unsigned long i = 0; i < pages; i++)
        {
            index->level3_table[index->level3_index] = (((unsigned long)physical_address + 4096ul * 512ul * 512ul * i) & PAGING_ADDRESS_MASK) | page_entry_flags | PAGING_ENTRY_FLAG_SIZE;

            if (++index->level3_index >= 512ul)
            {
                if (++index->level4_index >= 512ul)
                {
                    console_print("[paging_map_index_physical] failed to allocate 1GiB pages, reached end of virtual address space.\n");
                    return 0;
                }

                index->level3_index = 0;
                index->level3_table = index->level4_table[index->level4_index];
                if (!index->level3_table)
                {
                    index->level3_table = memory_physical_allocate();
                    memory_zero(index->level3_table, 4096ul);
                    index->level4_table[index->level4_index] = (unsigned long)index->level3_table | PAGING_ENTRY_FLAG_PRESENT | PAGING_ENTRY_FLAG_WRITABLE;
                }
            }
        }
        used_virtual_pages += pages * 512ul * 512ul;
        return 1;
    }

    // Check if mapping huge pages (2MiB)
    if ((index->level2_table[index->level2_index] & PAGING_ENTRY_FLAG_SIZE) || (flags & PAGING_FLAG_2MB))
    {
        if ((unsigned long)physical_address & 0x1FFFFFul)
        {
            console_print("[paging_map_index_physical] physical_address_or_null must be aligned to 0x200000 bytes (2MiB, 2097152 bytes) when mapping 2MiB page!\n");
            return 0;
        }

        if (index->level2_table[index->level2_index] != 0 && !(flags & PAGING_FLAG_REPLACE))
        {
            console_print("[paging_map_index_physical] tried to map at already mapped address (2MiB)\n");
            return 0;
        }

        unsigned long pages = ((bytes - 1) >> 21) + 1; // Divide by 2MiB
        for (unsigned long i = 0; i < pages; i++)
        {
            unsigned int x, y;
            console_get_cursor(&x, &y);
            // console_print_u64
            console_set_cursor(x, y);

            index->level2_table[index->level2_index] = (((unsigned long)physical_address + 4096ul * 512ul * i) & PAGING_ADDRESS_MASK) | page_entry_flags | PAGING_ENTRY_FLAG_SIZE;

            if (++index->level2_index >= 512ul)
            {
                if (++index->level3_index >= 512ul)
                {
                    if (++index->level4_index >= 512ul)
                    {
                        console_print("[paging_map_index_physical] failed to allocate 2MiB pages, reached end of virtual address space.\n");
                        return 0;
                    }

                    index->level3_index = 0;
                    index->level3_table = (unsigned long *)(index->level4_table[index->level4_index] & PAGING_ADDRESS_MASK);
                    if (!index->level3_table)
                    {
                        index->level3_table = memory_physical_allocate();
                        memory_zero(index->level3_table, 4096ul);
                        index->level4_table[index->level4_index] = (unsigned long)index->level3_table | PAGING_ENTRY_FLAG_PRESENT | PAGING_ENTRY_FLAG_WRITABLE;
                    }
                }

                index->level2_index = 0;
                index->level2_table = (unsigned long *)(index->level3_table[index->level3_index] & PAGING_ADDRESS_MASK);
                if (!index->level2_table)
                {
                    index->level2_table = memory_physical_allocate();
                    memory_zero(index->level2_table, 4096ul);
                    index->level3_table[index->level3_index] = (unsigned long)index->level2_table | PAGING_ENTRY_FLAG_PRESENT | PAGING_ENTRY_FLAG_WRITABLE;
                }
            }
        }
        used_virtual_pages += pages * 512ul;
        return 1;
    }

    // Map using 4KiB pages
    if (index->level1_table[index->level1_index] != 0)
    {
        console_print("[paging_map_index_physical] tried to map at already mapped address (4KiB)\n");
        return 0;
    }

    if ((unsigned long)physical_address & 0xFFFul)
    {
        console_print("[paging_map_index_physical] physical_address_or_null must be aligned to 0x1000 bytes (4KiB, 4096 bytes)!\n");
        return 0;
    }

    unsigned long pages = ((bytes - 1) >> 12) + 1; // Divide by 4KiB
    for (unsigned long i = 0; i < pages; i++)
    {
        index->level1_table[index->level1_index] = ((unsigned long)(physical_address + 4096ul * i) & PAGING_ADDRESS_MASK) | page_entry_flags;

        if (++index->level1_index >= 512ul)
        {
            if (++index->level2_index >= 512ul)
            {
                if (++index->level3_index >= 512ul)
                {
                    if (++index->level4_index >= 512ul)
                    {
                        console_print("[paging_map_index_physical] failed to allocate 4KiB pages, reached end of virtual address space.\n");
                        return 0;
                    }

                    index->level3_index = 0;
                    index->level3_table = (unsigned long *)(index->level4_table[index->level4_index] & PAGING_ADDRESS_MASK);
                    if (!index->level3_table)
                    {
                        index->level3_table = memory_physical_allocate();
                        memory_zero(index->level3_table, 4096ul);
                        index->level4_table[index->level4_index] = (unsigned long)index->level3_table | PAGING_ENTRY_FLAG_PRESENT | PAGING_ENTRY_FLAG_WRITABLE;
                    }
                }

                index->level2_index = 0;
                index->level2_table = (unsigned long *)(index->level3_table[index->level3_index] & PAGING_ADDRESS_MASK);
                if (!index->level2_table)
                {
                    index->level2_table = memory_physical_allocate();
                    memory_zero(index->level2_table, 4096ul);
                    index->level3_table[index->level3_index] = (unsigned long)index->level2_table | PAGING_ENTRY_FLAG_PRESENT | PAGING_ENTRY_FLAG_WRITABLE;
                }
            }

            index->level1_index = 0;
            index->level1_table = (unsigned long *)(index->level2_table[index->level2_index] & PAGING_ADDRESS_MASK);
            if (!index->level1_table)
            {
                index->level1_table = memory_physical_allocate();
                memory_zero(index->level1_table, 4096ul);
                index->level2_table[index->level2_index] = (unsigned long)index->level1_table | PAGING_ENTRY_FLAG_PRESENT | PAGING_ENTRY_FLAG_WRITABLE;
            }
        }
    }
    used_virtual_pages += pages;
    return 1;
}

// Begins to map x bytes at the current location where index points to, all the tables and indices should be filled in in index
static int paging_map_index_current(PagingContext *context, unsigned long bytes, unsigned short flags)
{
    if (bytes <= 0)
    {
        console_print("[paging_map_index_current] cannot map 0 bytes\n");
        return 0;
    }

    unsigned long page_entry_flags = paging_convert_flags(flags);

    // Check if mapping huge pages (1GiB)
    if ((context->level3_table[context->level3_index] & PAGING_ENTRY_FLAG_SIZE) || (flags & PAGING_FLAG_1GB))
    {
        if (!hugepages_supported)
        {
            console_print("[paging_map_index_current] huge pages not supported (1GiB)\n");
            return 0;
        }

        if (context->level3_table[context->level3_index] != 0 && !(flags & PAGING_FLAG_REPLACE))
        {
            console_print("[paging_map_index_current] tried to map at already mapped address (1GiB)\n");
            return 0;
        }

        unsigned long pages = ((bytes - 1) >> 30) + 1; // Divide by 1GiB
        for (unsigned long i = 0; i < pages; i++)
        {
            // TODO not implemented
            // index->level3_table[index->level3_index] = ((unsigned long)memory_physical_allocate_1gb() & PAGING_ADDRESS_MASK) | page_entry_flags | PAGING_ENTRY_FLAG_SIZE;
            console_print("[paging_map_index_current] 1GiB physical allocation not implemented yet!\n");
            return 0;

            if (++context->level3_index >= 512ul)
            {
                if (++context->level4_index >= 512ul)
                {
                    console_print("[paging_map_index_current] failed to allocate 1GiB pages, reached end of virtual address space.\n");
                    return 0;
                }

                context->level3_index = 0;
                context->level3_table = context->level4_table[context->level4_index];
                if (!context->level3_table)
                {
                    context->level3_table = memory_physical_allocate();
                    memory_zero(context->level3_table, 4096ul);
                    context->level4_table[context->level4_index] = (unsigned long)context->level3_table | PAGING_ENTRY_FLAG_PRESENT | PAGING_ENTRY_FLAG_WRITABLE;
                }
            }
        }
        used_virtual_pages += pages * 512ul * 512ul;
        return 1;
    }

    // Check if mapping huge pages (2MiB)
    if ((context->level2_table[context->level2_index] & PAGING_ENTRY_FLAG_SIZE) || (flags & PAGING_FLAG_2MB))
    {
        if (context->level2_table[context->level2_index] != 0 && !(flags & PAGING_FLAG_REPLACE))
        {
            console_print("[paging_map_index_current] tried to map at already mapped address (2MiB)\n");
            return 0;
        }

        unsigned long pages = ((bytes - 1) >> 21) + 1; // Divide by 2MiB
        for (unsigned long i = 0; i < pages; i++)
        {
            // TODO not implemented
            // index->level2_table[index->level2_index] = ((unsigned long)memory_physical_allocate_2mb() & PAGING_ADDRESS_MASK) | page_entry_flags | PAGING_ENTRY_FLAG_SIZE;
            console_print("[paging_map_index_current] 2MiB physical allocation not implemented yet!\n");
            return 0;

            if (++context->level2_index >= 512ul)
            {
                if (++context->level3_index >= 512ul)
                {
                    if (++context->level4_index >= 512ul)
                    {
                        console_print("[paging_map_index_current] failed to allocate 2MiB pages, reached end of virtual address space.\n");
                        return 0;
                    }

                    context->level3_index = 0;
                    context->level3_table = (unsigned long *)(context->level4_table[context->level4_index] & PAGING_ADDRESS_MASK);
                    if (!context->level3_table)
                    {
                        context->level3_table = memory_physical_allocate();
                        memory_zero(context->level3_table, 4096ul);
                        context->level4_table[context->level4_index] = (unsigned long)context->level3_table | PAGING_ENTRY_FLAG_PRESENT | PAGING_ENTRY_FLAG_WRITABLE;
                    }
                }

                context->level2_index = 0;
                context->level2_table = (unsigned long *)(context->level3_table[context->level3_index] & PAGING_ADDRESS_MASK);
                if (!context->level2_table)
                {
                    context->level2_table = memory_physical_allocate();
                    memory_zero(context->level2_table, 4096ul);
                    context->level3_table[context->level3_index] = (unsigned long)context->level2_table | PAGING_ENTRY_FLAG_PRESENT | PAGING_ENTRY_FLAG_WRITABLE;
                }
            }
        }
        used_virtual_pages += pages * 512ul;
        return 1;
    }

    // Map using 4KiB pages
    if (context->level1_table[context->level1_index] != 0)
    {
        console_print("[paging_map_index_current] tried to map at already mapped address (4KiB)\n");
        return 0;
    }

    unsigned long pages = ((bytes - 1) >> 12) + 1; // Divide by 4KiB
    for (unsigned long i = 0; i < pages; i++)
    {
        context->level1_table[context->level1_index] = ((unsigned long)memory_physical_allocate() & PAGING_ADDRESS_MASK) | page_entry_flags;

        if (++context->level1_index >= 512ul)
        {
            if (++context->level2_index >= 512ul)
            {
                if (++context->level3_index >= 512ul)
                {
                    if (++context->level4_index >= 512ul)
                    {
                        console_print("[paging_map_index_current] failed to allocate 4KiB pages, reached end of virtual address space.\n");
                        return 0;
                    }

                    context->level3_index = 0;
                    context->level3_table = (unsigned long *)(context->level4_table[context->level4_index] & PAGING_ADDRESS_MASK);
                    if (!context->level3_table)
                    {
                        context->level3_table = memory_physical_allocate();
                        memory_zero(context->level3_table, 4096ul);
                        context->level4_table[context->level4_index] = (unsigned long)context->level3_table | PAGING_ENTRY_FLAG_PRESENT | PAGING_ENTRY_FLAG_WRITABLE;
                    }
                }

                context->level2_index = 0;
                context->level2_table = (unsigned long *)(context->level3_table[context->level3_index] & PAGING_ADDRESS_MASK);
                if (!context->level2_table)
                {
                    context->level2_table = memory_physical_allocate();
                    memory_zero(context->level2_table, 4096ul);
                    context->level3_table[context->level3_index] = (unsigned long)context->level2_table | PAGING_ENTRY_FLAG_PRESENT | PAGING_ENTRY_FLAG_WRITABLE;
                }
            }

            context->level1_index = 0;
            context->level1_table = (unsigned long *)(context->level2_table[context->level2_index] & PAGING_ADDRESS_MASK);
            if (!context->level1_table)
            {
                context->level1_table = memory_physical_allocate();
                memory_zero(context->level1_table, 4096ul);
                context->level2_table[context->level2_index] = (unsigned long)context->level1_table | PAGING_ENTRY_FLAG_PRESENT | PAGING_ENTRY_FLAG_WRITABLE;
            }
        }
    }
    used_virtual_pages += pages;
    return 1;
}

static int paging_virtual_address_to_index(PagingContext *destination_context, void *virtual_address, unsigned long flags)
{
    destination_context->level4_index = ((unsigned long)virtual_address >> 39) & 0b111111111ul;

    destination_context->level3_table = (unsigned long *)(destination_context->level4_table[destination_context->level4_index] & PAGING_ADDRESS_MASK);
    if (!destination_context->level3_table)
    {
        destination_context->level3_table = memory_physical_allocate();
        memory_zero(destination_context->level3_table, 4096ul);
        destination_context->level4_table[destination_context->level4_index] = (unsigned long)destination_context->level3_table | PAGING_ENTRY_FLAG_PRESENT | PAGING_ENTRY_FLAG_WRITABLE;
    }

    destination_context->level3_index = ((unsigned long)virtual_address >> 30) & 0b111111111ul;

    if (flags & PAGING_FLAG_1GB)
    {
        if ((unsigned long)virtual_address & 0x3FFFFFFFul)
        {
            console_print("[paging_virtual_address_to_index] address must be aligned to 0x40000000 (1Gib, 1073741824 bytes) when mapping 1GiB page\n");
            return 0;
        }

        destination_context->level2_table = 0;
        destination_context->level2_index = 0;
        destination_context->level1_table = 0;
        destination_context->level1_index = 0;
        return 1;
    }

    destination_context->level2_table = (unsigned long *)(destination_context->level3_table[destination_context->level3_index] & PAGING_ADDRESS_MASK);
    if (!destination_context->level2_table)
    {
        destination_context->level2_table = memory_physical_allocate();
        memory_zero(destination_context->level2_table, 4096ul);
        destination_context->level3_table[destination_context->level3_index] = (unsigned long)destination_context->level2_table | PAGING_ENTRY_FLAG_PRESENT | PAGING_ENTRY_FLAG_WRITABLE;
    }

    destination_context->level2_index = ((unsigned long)virtual_address >> 21) & 0b111111111ul;

    if (flags & PAGING_FLAG_2MB)
    {
        if ((unsigned long)virtual_address & 0x1FFFFFul)
        {
            console_print("[paging_virtual_address_to_index] address must be aligned to 0x200000 (2MiB, 2097152 bytes) when mapping 2MiB page\n");
            return 0;
        }

        destination_context->level1_table = 0;
        destination_context->level1_index = 0;
        return 1;
    }

    destination_context->level1_table = (unsigned long *)(destination_context->level2_table[destination_context->level2_index] & PAGING_ADDRESS_MASK);
    if (!destination_context->level1_table)
    {
        destination_context->level1_table = memory_physical_allocate();
        memory_zero(destination_context->level1_table, 4096ul);
        destination_context->level2_table[destination_context->level3_index] = (unsigned long)destination_context->level1_table | PAGING_ENTRY_FLAG_PRESENT | PAGING_ENTRY_FLAG_WRITABLE;
    }
    destination_context->level1_index = ((unsigned long)virtual_address >> 12) & 0b111111111ul;

    if ((unsigned long)virtual_address & 0xFFFul)
    {
        // Address is not aligned
        console_print("[paging_virtual_address_to_index] address must be aligned to 0x1000 (4KiB, 4096 bytes) when mapping page\n");
        return 0;
    }

    return 1;
}

// void *paging_map_physical(void* physical_address, unsigned long bytes, unsigned long flags)
// {
//     Cpu *cpu = cpu_get_current();
//     return paging_map_index_physical(&cpu->current_process->paging_context, physical_address, bytes, flags);
// }

void *paging_map_physical(PagingContext *context, void *physical_address, unsigned long bytes, unsigned long flags)
{
    if (paging_index_find_spot(context, bytes, flags))
    {
        void *virtual_address = (void *)(((unsigned long)context->level1_index << 12) | ((unsigned long)context->level2_index << 21) | ((unsigned long)context->level3_index << 30) | ((unsigned long)context->level4_index << 39));
        if (paging_map_index_physical(physical_address, context, bytes, flags))
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
        return 0;
    }
}

// void *paging_map(unsigned long bytes, unsigned long flags)
// {
//     Cpu *cpu = cpu_get_current();
//     return paging_map_index(&cpu->current_process->paging_context, bytes, flags);
// }

void *paging_map(PagingContext *context, unsigned long bytes, unsigned long flags)
{
    if (paging_index_find_spot(context, bytes, flags))
    {
        void *virtual_address = (void *)(((unsigned long)context->level1_index << 12) | ((unsigned long)context->level2_index << 21) | ((unsigned long)context->level3_index << 30) | ((unsigned long)context->level4_index << 39));
        if (paging_map_index_current(context, bytes, flags))
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
        return 0;
    }
}

// void *paging_map_physical_at(void *physical_address, void *virtual_address, unsigned long bytes, unsigned long flags)
// {
//     Cpu *cpu = cpu_get_current();
//     return paging_map_index_physical_at(&cpu->current_process->paging_context, physical_address, virtual_address, bytes, flags);
// }

void *paging_map_physical_at(PagingContext *context, void *physical_address, void *virtual_address, unsigned long bytes, unsigned long flags)
{
    PagingContext new_index;
    // A virtual address was given, only get the uppermost (level 4) page table from the current process
    new_index.level4_table = context->level4_table;
    if (paging_virtual_address_to_index(&new_index, virtual_address, flags))
    {
        if (paging_map_index_physical(physical_address, &new_index, bytes, flags))
        {
            return virtual_address;
        }
        else
        {
            return 0;
        }
    }
    else
    {
        return 0;
    }
}

// void *paging_map_at(void *virtual_address, unsigned long bytes, unsigned long flags)
// {
//     Cpu *cpu = cpu_get_current();
//     return paging_map_index_at(&cpu->current_process->paging_context, virtual_address, bytes, flags);
// }

void *paging_map_at(PagingContext *context, void *virtual_address, unsigned long bytes, unsigned long flags)
{
    PagingContext new_index;
    // A virtual address was given, only get the uppermost (level 4) page table from the current process
    new_index.level4_table = context->level4_table;
    if (paging_virtual_address_to_index(&new_index, virtual_address, flags))
    {
        if (paging_map_index_current(&new_index, bytes, flags))
        {
            return virtual_address;
        }
        else
        {
            return 0;
        }
    }
    else
    {
        return 0;
    }
}

int paging_unmap(PagingContext *context, void *virtual_address, unsigned long bytes)
{
    unsigned long *level4_table = context->level4_table;
    unsigned int level4_index = ((unsigned long)virtual_address >> 39) & 0b111111111ul;
    unsigned long level4_entry = level4_table[level4_index];
    if (level4_entry == 0)
    {
        console_print("[paging_unmap] tried to unmap unmapped page (level4_entry)\n");
        return 0;
    }

    unsigned long *level3_table = (unsigned long *)(level4_entry & PAGING_ADDRESS_MASK);
    unsigned int level3_index = ((unsigned long)virtual_address >> 30) & 0b111111111ul;
    unsigned long level3_entry = level3_table[level3_index];
    if (level3_entry == 0)
    {
        console_print("[paging_unmap] tried to unmap unmapped page (level3_entry)\n");
        return 0;
    }
    if (level3_entry & PAGING_ENTRY_FLAG_SIZE)
    {
        unsigned long pages = ((bytes - 1) >> 30) + 1; // Divide by 1GB
        if ((unsigned long)virtual_address & 0x3FFFFFFFul)
        {
            console_print("[paging_unmap] virtual_address must be aligned to 0x40000000 bytes (1GiB, 1073741824 bytes) when unmapping 1GB page!\n");
            return 0;
        }

        // Free 1GB huge pages
        for (unsigned long i = 0; i < pages; i++)
        {
            level3_table[level3_index] = 0;
            if (++level3_index >= 512ul)
            {
                if (++level4_index >= 512ul)
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

        used_virtual_pages -= pages * 512ul * 512ul;
        return 1;
    }

    unsigned long *level2_table = (unsigned long *)(level3_entry & PAGING_ADDRESS_MASK);
    unsigned int level2_index = ((unsigned long)virtual_address >> 21) & 0b111111111ul;
    unsigned long level2_entry = level2_table[level2_index];
    if (level2_entry == 0)
    {
        console_print("[paging_unmap] tried to unmap unallocated page (level2_entry)\n");
        return 0;
    }
    if (level2_entry & PAGING_ENTRY_FLAG_SIZE)
    {
        unsigned long pages = ((bytes - 1) >> 21) + 1; // Divide by 2MB
        if ((unsigned long)virtual_address & 0x1FFFFFul)
        {
            console_print("[paging_unmap] virtual_address must be aligned to 0x200000 bytes (2MiB, 2097152 bytes) when unmapping 2MB page!\n");
            return 0;
        }

        // Free 2MB huge pages
        for (unsigned long i = 0; i < pages; i++)
        {
            level2_table[level2_index] = 0;
            if (++level2_index >= 512ul)
            {
                if (++level3_index >= 512ul)
                {
                    if (++level4_index >= 512ul)
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

        used_virtual_pages -= pages * 512ul;
        return 1;
    }

    unsigned long *level1_table = (unsigned long *)(level2_entry & PAGING_ADDRESS_MASK);
    unsigned int level1_index = ((unsigned long)virtual_address >> 12) & 0b111111111ul;
    unsigned long level1_entry = level1_table[level1_index];
    if (level1_entry == 0)
    {
        console_print("[paging_unmap] tried to unmap unmapped page (level1_entry)\n");
        return 0;
    }

    unsigned long pages = ((bytes - 1) >> 12) + 1; // Divide by 4KB
    if ((unsigned long)virtual_address & 0xFFFul)
    {
        console_print("[paging_unmap] virtual_address must be aligned to 0xFFF bytes (4KiB, 4096 bytes) when unmapping page!\n");
        return 0;
    }

    for (unsigned long i = 0; i < pages; i++)
    {
        // Null entire entry, including present flag
        level1_table[level1_index] = 0;
        if (++level1_index >= 512ul)
        {
            if (++level2_index >= 512ul)
            {
                if (++level3_index >= 512ul)
                {
                    if (++level4_index >= 512ul)
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

void *paging_get_physical_address(PagingContext *context, void *virtual_address)
{
    unsigned long address = (unsigned long)virtual_address;

    unsigned long *level4_table = context->level4_table;
    unsigned int level4_offset = (address >> 39) & 0b111111111ul;
    unsigned long level4_entry = level4_table[level4_offset];

    // Check if page is present
    if (level4_entry == 0)
    {
        return 0;
    }
    if (!(level4_entry & PAGING_ENTRY_FLAG_PRESENT))
    {
        console_print("[paging_get_physical_address] warning: level4 not present but has address\n");
    }

    unsigned long *level3_table = (unsigned long *)(level4_entry & PAGING_ADDRESS_MASK);
    unsigned int level3_offset = (address >> 30) & 0b111111111ul;
    unsigned long level3_entry = level3_table[level3_offset];

    // Check if page is present
    if (level3_entry == 0)
    {
        return 0;
    }
    if (!(level3_entry & PAGING_ENTRY_FLAG_PRESENT))
    {
        console_print("[paging_get_physical_address] warning: level3 not present but has address\n");
    }

    // Check if this is 1gb page
    if (level3_entry & PAGING_ENTRY_FLAG_SIZE)
    {
        return (void *)((level3_entry & 0xFFFFFFFFFF000ul) + (address & 0x3FFFFFFFul));
    }

    unsigned long *level2_table = (unsigned long *)(level3_entry & PAGING_ADDRESS_MASK);
    unsigned int level2_offset = (address >> 21) & 0b111111111ul;
    unsigned long level2_entry = level2_table[level2_offset];

    // Check if page is present
    if (level2_entry == 0)
    {
        return 0;
    }
    if (!(level2_entry & PAGING_ENTRY_FLAG_PRESENT))
    {
        console_print("[paging_get_physical_address] warning: level2 not present but has address\n");
    }

    // Check if this is 2mb page
    if (level2_entry & PAGING_ENTRY_FLAG_SIZE)
    {
        return (void *)((level2_entry & 0xFFFFFFFFFF000ul) + (address & 0x1FFFFFul));
    }

    unsigned long *level1_table = (unsigned long *)(level2_entry & PAGING_ADDRESS_MASK);
    unsigned int level1_offset = (address >> 12) & 0b111111111ul; // The other offset fields are 9 bit indices into their 512 entry tables
    unsigned long level1_entry = level1_table[level1_offset];

    // Check if page is present
    if (level1_entry == 0)
    {
        return 0;
    }
    if (!(level1_entry & PAGING_ENTRY_FLAG_PRESENT))
    {
        console_print("[paging_get_physical_address] warning: level1 not present but has address\n");
    }

    unsigned int offset = address & 0b111111111111ul; // Offset is 12 bits because each page is 4096 bytes in size
    return (void *)((level1_entry & 0xFFFFFFFFFF000ul) + offset);
}

unsigned long paging_used_pages()
{
    return used_virtual_pages;
}

static void paging_debug_attributes(unsigned long entry)
{
    // Format described at https://kokos.run/#WzAsIkFNRDY0Vm9sdW1lMi5wZGYiLDIwMV0=
    if (entry & PAGING_ENTRY_FLAG_PRESENT)
        console_print_char('P');
    if (entry & PAGING_ENTRY_FLAG_WRITABLE)
        console_print_char('W');
    if (entry & PAGING_ENTRY_FLAG_EVERYONE_ACCESS)
        console_print_char('U');
    if (entry & PAGING_ENTRY_FLAG_SIZE)
        console_print_char('S');
    if (entry & PAGING_ENTRY_FLAG_NO_EXECUTE)
        console_print_char('X');
    if (entry & PAGING_ENTRY_FLAG_CACHE_DISABLED)
        console_print_char('C');
    if (entry & PAGING_ENTRY_FLAG_WRITETHROUGH)
        console_print_char('T');
    if (entry & PAGING_ENTRY_FLAG_ACCESSED)
        console_print_char('a');
    if (entry & PAGING_ENTRY_FLAG_DIRTY)
        console_print_char('d');
}

void paging_debug(PagingContext *context)
{
    console_print("level4 0x");
    console_print_u64(context->level4_table, 16);
    console_print(" [0x000000000000 .. 0xffffffffffff]\n");

    for (int level4_index = 0; level4_index < 512ul; level4_index++)
    {
        unsigned long level4_entry = context->level4_table[level4_index];
        if (!level4_entry)
            continue;

        unsigned long *level3_table = (unsigned long *)(level4_entry & PAGING_ADDRESS_MASK);
        unsigned long level3_starting_address = (unsigned long)level4_index * 512ul * 512ul * 512ul * 4096ul;
        unsigned long level3_ending_address = ((unsigned long)level4_index + 1) * 512ul * 512ul * 512ul * 4096ul - 1;

        console_print("|- level3 0x");
        console_print_u64(level3_table, 16);
        console_print_char(' ');
        paging_debug_attributes(level4_entry);
        console_print(" [0x");
        console_print_u64(level3_starting_address, 16);
        console_print(" .. 0x");
        console_print_u64(level3_ending_address, 16);
        console_print("]\n");

        for (int level3_index = 0; level3_index < 512ul; level3_index++)
        {
            unsigned long level3_entry = level3_table[level3_index];
            if (!level3_entry)
                continue;

            unsigned long *level2_table = (unsigned long *)(level3_entry & PAGING_ADDRESS_MASK);
            unsigned long level2_starting_address = level3_starting_address + (unsigned long)level3_index * 512ul * 512ul * 4096ul;
            unsigned long level2_ending_address = level3_starting_address + (((unsigned long)level3_index + 1) * 512ul * 512ul * 4096ul - 1);

            if (level3_entry & PAGING_ENTRY_FLAG_SIZE)
                console_print("   |- hugepage1gb 0x");
            else
                console_print("   |- level2 0x");
            console_print_u64(level2_table, 16);
            console_print_char(' ');
            paging_debug_attributes(level3_entry);
            console_print(" [0x");
            console_print_u64(level2_starting_address, 16);
            console_print(" .. 0x");
            console_print_u64(level2_ending_address, 16);
            console_print("]\n");

            if (level3_entry & PAGING_ENTRY_FLAG_SIZE)
                continue;

            for (int level2_index = 0; level2_index < 512ul; level2_index++)
            {
                unsigned long level2_entry = level2_table[level2_index];
                if (!level2_entry)
                    continue;

                unsigned long *level1_table = (unsigned long *)(level2_entry & PAGING_ADDRESS_MASK);
                unsigned long level1_starting_address = level2_starting_address + (unsigned long)level2_index * 512ul * 4096ul;
                unsigned long level1_ending_address = level2_starting_address + ((unsigned long)level2_index + 1) * 512ul * 4096ul - 1;

                if (level2_entry & PAGING_ENTRY_FLAG_SIZE)
                    console_print("      |- hugepage2mb 0x");
                else
                    console_print("      |- level1 0x");
                console_print_u64(level1_table, 16);
                console_print_char(' ');
                paging_debug_attributes(level2_entry);
                console_print(" [0x");
                console_print_u64(level1_starting_address, 16);
                console_print(" .. 0x");
                console_print_u64(level1_ending_address, 16);
                console_print("]\n");

                if (level2_entry & PAGING_ENTRY_FLAG_SIZE)
                    continue;

                unsigned long level1_count = 0;
                for (int level1_index = 0; level1_index < 512ul; level1_index++)
                {
                    if (!level1_table[level1_index])
                        break;
                    else
                        level1_count++;
                }

                if (level1_count >= 20)
                {
                    console_print("         |- ... ");
                    console_print_u64(level1_count, 10);
                    console_print(" pages ...\n");
                    continue;
                }

                for (int level1_index = 0; level1_index < 512ul; level1_index++)
                {
                    unsigned long level1_entry = level1_table[level1_index];
                    if (!level1_entry)
                        continue;

                    unsigned long *page = (unsigned long *)(level1_entry & PAGING_ADDRESS_MASK);
                    unsigned long page_starting_address = level1_starting_address + (unsigned long)level1_index * 4096ul;
                    unsigned long page_ending_address = level1_starting_address + ((unsigned long)level1_index + 1) * 4096ul - 1;

                    console_print("         |- page 0x");
                    console_print_u64(page, 16);
                    console_print_char(' ');
                    paging_debug_attributes(level1_entry);
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
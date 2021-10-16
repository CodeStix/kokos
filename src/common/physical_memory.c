#include "../include/physical_memory.h"
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
    // This will scan 64 pages per iterations for a spot (262144 bytes) (~30000 iterations worst case, when all ram is used on a 16gb pc)
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
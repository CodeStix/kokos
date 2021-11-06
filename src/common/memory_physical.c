#include "memory_physical.h"
#include "console.h"
#include "lock.h"

// Points to a table that contains bits that indicate which physical chunks are allocated
static unsigned long *allocation_table = 0;
// The current position in the allocation_table, this number increases until the end of the table is reached (end of physical RAM)
static unsigned long allocation_index = 0;
// The amount of unsigned long entries in the allocation table
static unsigned long allocation_table_length = 0;
static unsigned long used_physical_pages = 0;

static int memory_lock = 0;

void memory_physical_initialize(void *allocation_table_location, unsigned long total_memory)
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

void memory_physical_reserve(void *physical_address, unsigned long bytes)
{
    lock_acquire(&memory_lock);

    // >> 12 is the same as divide by 4096
    unsigned long start_index = ((unsigned long)physical_address) >> 12;
    bytes += (unsigned long)physical_address & 0b111111111111;

    // This formula will calculate the minimum amount of pages needed to allocate bytes
    unsigned long page_count = ((bytes - 1) >> 12) + 1;

    for (unsigned long index = start_index; index < start_index + page_count; index++)
    {
        if (index >= allocation_table_length || index < 0)
        {
            // This memory does not reside in physical memory, so it can't be allocated and is reserved
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
            used_physical_pages++;
        }
    }

    lock_release(&memory_lock);
}

// Frees a physical page
void memory_physical_free(void *physical_address)
{
    lock_acquire(&memory_lock);

    // >> 12 is the same as divide by 4096
    unsigned long index = ((unsigned long)physical_address) >> 12;

    // >> 6 is the same as divide by 64
    unsigned int byte = index >> 6;

    // & 0b111111 is the same as modulo 64
    unsigned char bit = index & 0b111111;

    if (byte >= allocation_table_length)
    {
        lock_release(&memory_lock);
        console_print("warning: invalid address passed to memory_physical_free\n");
        return;
    }

    // Set bit bit of allocation_table[byte] to zero
    if (allocation_table[byte] & (1 << bit))
    {
        allocation_table[byte] &= ~(1 << bit);
        used_physical_pages--;
    }
    else
    {
        console_print("warning: memory_physical_free called on already freed page 0x");
        console_print_u64((unsigned long)physical_address, 16);
        console_new_line();
    }

    lock_release(&memory_lock);
}

void *memory_physical_allocate()
{
    lock_acquire(&memory_lock);

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
        lock_release(&memory_lock);
        console_print("error: memory_physical_allocate bit scan did not find bit in\n");
        console_print_u64(allocation_table[spot], 2);
        console_new_line();
        return 0;
    }

    // Set bit bit of allocation_table[spot] to zero
    allocation_table[spot] |= 1 << bit;
    allocation_index = spot;
    used_physical_pages++;

    lock_release(&memory_lock);

    return (void *)((((spot << 6) + bit) << 12));
}

void *memory_physical_allocate_consecutive(unsigned long pages)
{
    lock_acquire(&memory_lock);

    // TODO this is infinite loop when no memory is available
    unsigned long chunk_count = pages >> 6;
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

    lock_release(&memory_lock);

    allocation_index = spot + chunk_count;
    return (void *)(((spot << 6) << 12));
}

int memory_physical_allocated(void *phyisical_address)
{
    unsigned long index = ((unsigned long)phyisical_address) >> 12;

    if (index >= allocation_table_length)
    {
        console_print("warning: invalid address passed to memory_physical_allocated, returning 1: 0x");
        console_print_u64((unsigned long)phyisical_address, 16);
        console_new_line();
        return 1;
    }

    unsigned int byte = index >> 6;
    unsigned char bit = index & 0b111111;
    return (allocation_table[index] >> bit) & 0b1;
}

unsigned long memory_physical_used_pages()
{
    return used_physical_pages;
}
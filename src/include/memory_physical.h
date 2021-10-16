#pragma once

// Sets up the physical allocation table at the specified address
void memory_physical_initialize(void *allocation_table_location, unsigned long total_memory);

// Reserves a physical address so it can't be allocated using memory_physical_allocate
void memory_physical_reserve(void *physical_address, unsigned long bytes);

// Frees a physical page
void memory_physical_free(void *physical_address);

// Allocates a single 4096 byte chunk of physical memory and returns the physical address to it
// This chunk is always aligned to 4096 bytes
void *memory_physical_allocate();

// Returns 1 if the passed address has been reserved or allocated
int memory_physical_allocated(void *physical_address);

// Returs the amount of pages allocated
unsigned long memory_physical_allocated_count();

// Allocates multiple consecutive 262144 byte chunks of physical memory and returns the physical address to it
// Use memory_physical_allocate_consecutive(8) to allocate a 2MB chunk
// Use memory_physical_allocate_consecutive(4096) to allocate a 1GB chunk
void *memory_physical_allocate_consecutive(unsigned int chunk_count);
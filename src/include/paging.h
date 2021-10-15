#pragma once

extern unsigned long page_table_level4[512];

// Returns the physical address for virtual address and returns 0 if the virtual address is not mapped
void *paging_get_physical_address(void *virtual_address);

// Sets up the physical allocation table at the specified address
void paging_physical_initialize(void *allocation_table_location, unsigned long total_memory);

// Reserves a physical address so it can't be allocated using paging_physical_allocate
void paging_physical_reserve(void *physical_address, unsigned long bytes);

// Frees a physical page
void paging_physical_free(void *physical_address);

// Allocates a single 4096 byte chunk of physical memory and returns the physical address to it
void *paging_physical_allocate();

// Returs the amount of pages allocated
unsigned long paging_physical_allocated();

// Allocates multiple consecutive 262144 byte chunks of physical memory and returns the physical address to it
// Use paging_physical_allocate_consecutive(8) to allocate a 2MB chunk
// Use paging_physical_allocate_consecutive(4096) to allocate a 2GB chunk
void *paging_physical_allocate_consecutive(unsigned int chunk_count);
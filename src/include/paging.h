#pragma once

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

extern unsigned long page_table_level4[512];

// Sets up paging
void paging_initialize();

// Returns the physical address for virtual address and returns 0 if the virtual address is not mapped
void *paging_get_physical_address(void *virtual_address);

// Maps the given physical address to a virtual address
void *paging_map(void *physical_address, unsigned short flags);

// Allocates a new page and returns its virtual address
void *paging_allocate(unsigned short flags);

// Frees a single page allocated using paging_allocate or paging_map
void paging_free(void *virtual_address);

// Returs the number of used virtual pages
unsigned long paging_used_pages();
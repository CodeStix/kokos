#pragma once

// Note on virtual memory in this os:
// The first virtual memory space is identity mapped to the physical memory using huge pages,
// this means you can access everything using its physical address (because it is the same as its virtual address) or its virtual address.
// A region of physical ram can have multiple virtual addresses pointing to it.

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

// 0x7FFFFFFFFFFFF000 = all ones without the 63 and first 12 bits set, which only masks the address
#define PAGING_ADDRESS_MASK 0x7FFFFFFFFFFFF000ull
#define PAGING_MAX_VIRTUAL_PAGES 512ul * 512ul * 512ul * 512ul

// The following PAGING_ENTRY_FLAG are used in the page tables
#define PAGING_ENTRY_FLAG_PRESENT 0b000000001
#define PAGING_ENTRY_FLAG_WRITABLE 0b000000010
#define PAGING_ENTRY_FLAG_EVERYONE_ACCESS 0b000000100
#define PAGING_ENTRY_FLAG_WRITETHROUGH 0b000001000
#define PAGING_ENTRY_FLAG_CACHE_DISABLED 0b000010000
#define PAGING_ENTRY_FLAG_ACCESSED 0b000100000
// This flag is only present in the lowest table
#define PAGING_ENTRY_FLAG_DIRTY 0b001000000
// This flag is only present in the level 2 and level 3 tables
#define PAGING_ENTRY_FLAG_SIZE 0b010000000
// This flag is only present in the lowest table
#define PAGING_ENTRY_FLAG_GLOBAL 0b100000000
#define PAGING_ENTRY_FLAG_NO_EXECUTE 0x8000000000000000ull
// Bits 11-9 in each page table entry are user definable (AVL bits, available for software) and can be used for anything, in this case for the following:
// This flag indicates that the underlaying page table does not contain at least 1 empty entry
#define PAGING_ENTRY_FLAG_FULL 0b1000000000

// This flag indicates that reading is enabled is enabled for this page
#define PAGING_FLAG_READ 0b1
// This flag indicates that writing is enabled for this page, reading must be enabled too to ensure write access
#define PAGING_FLAG_WRITE 0b10
// This flag indicates that the processor may execute code inside this page
#define PAGING_FLAG_EXECUTE 0b100
// This flag indicates that this page can be accessed by privilege level 3 when it is readable/writable
#define PAGING_FLAG_USER 0b1000
// This flag indicates that paging_map_at should use 1GB pages
#define PAGING_FLAG_1GB 0b10000
// This flag indicates that paging_map_at should use 2MB pages
#define PAGING_FLAG_2MB 0b100000
// This flag indicates that paging_map_at should forcefully replace the existing virtual mapping if there is one
#define PAGING_FLAG_REPLACE 0b1000000

// Sets up paging.
// Physical memory allocation must be initialized first!
void paging_initialize(unsigned long *level4_table);

// Returns the physical address for virtual address and returns 0 if the virtual address is not mapped
void *paging_get_physical_address(void *virtual_address);

// Maps a page at the given physical address to a virtual address
void *paging_map(void *physical_address, unsigned short flags);

// Maps the page at a specific physical address to a specific virtual address
void *paging_map_at(void *virtual_address, void *physical_address, unsigned long flags);

// Allocates a new page and returns its virtual address
void *paging_allocate(unsigned short flags);

// Frees a single page allocated using paging_allocate or paging_map
void paging_free(void *virtual_address);

// Returs the number of used virtual pages
unsigned long paging_used_pages();

// Returns non-zero number if huge pages (1GB pages) are supported
int paging_get_hugepages_supported();
#pragma once

extern unsigned long page_table_level4[512];

// Returns the physical address for virtual address and returns 0 if the virtual address is not mapped
void *paging_get_physical_address(void *virtual_address);
#pragma once

typedef struct
{
    unsigned char bus;
    unsigned char slot;
    unsigned char function;

    unsigned short device_id;
    unsigned short vendor_id;

    unsigned short status;
    unsigned short command;

    unsigned char class_code;
    unsigned char subclass_code;
    unsigned char progif;
    unsigned char revision;

    unsigned char bist;
    unsigned char header_type;
    unsigned char latency_timer;
    unsigned char cache_line_size;

} PCIHeader;

unsigned int pci_read_device(int bus, int slot, int function, int offset);
void pci_scan();
#include "../include/pci.h"
#include "../include/console.h"
#include "../include/memory.h"

// This function reads a part at offset from the pci header for bus, slot, function.
extern unsigned int pci_config_read32(int bus, int slot, int function, int offset);

unsigned short pci_config_read16(int bus, int slot, int function, int offset)
{
    return (pci_config_read32(bus, slot, function, offset) >> ((offset & 0x2) * 8)) & 0xffff;
}

unsigned char pci_config_read8(int bus, int slot, int function, int offset)
{
    return (pci_config_read32(bus, slot, function, offset) >> ((offset & 0x3) * 8)) & 0xff;
}

void pci_scan()
{
    // TODO pci-to-pci device

    for (int bus = 0; bus < 0b11111111; bus++)
    {
        for (int slot = 0; slot < 0b11111; slot++)
        {
            int is_multifunction = 0;
            for (int function = 0; function < 8; function++)
            {
                unsigned int dev = pci_config_read32(bus, slot, function, 0);
                if (dev != 0xFFFFFFFF)
                {
                    unsigned short vendor = pci_config_read16(bus, slot, function, 0x0);
                    unsigned short device = pci_config_read16(bus, slot, function, 0x2);
                    unsigned short class = pci_config_read8(bus, slot, function, 0xb);
                    unsigned char subclass = pci_config_read8(bus, slot, function, 0xa);
                    unsigned char header_type = pci_config_read8(bus, slot, function, 0xe);

                    console_print("[pci] device at bus=");
                    console_print_u32(bus, 10);
                    console_print(" slot=");
                    console_print_u32(slot, 10);
                    console_print(" function=");
                    console_print_u32(function, 10);
                    console_print(" vendor=0x");
                    console_print_u32(vendor, 16);
                    console_print(" device=0x");
                    console_print_u32(device, 16);
                    console_new_line();

                    console_print(" class=0x");
                    console_print_u32(class, 16);
                    console_print_char(' ');
                    console_print(pci_class_string(class));
                    console_print(" subclass=0x");
                    console_print_u32(subclass, 16);
                    console_print_char(' ');
                    console_print(pci_subclass_string(class, subclass));
                    console_new_line();

                    console_print(" header_type=0x");
                    console_print_u32(header_type & 0b01111111, 16);
                    console_print(", multifunction=");
                    console_print_u32(header_type >> 7, 10);
                    console_new_line();

                    // Check if the multifunction bit is set, if so, we should check all functions of the current device
                    if ((header_type & 0b10000000) == 0b10000000)
                    {
                        is_multifunction = 1;
                    }

                    if (!is_multifunction)
                    {
                        break;
                    }
                }
            }
        }
    }
}

char *pci_class_string(unsigned char class_code)
{
    // Extracted from https://wiki.osdev.org/PCI
    switch (class_code)
    {
    case 0x00:
        return "Unclassified";
    case 0x01:
        return "Mass Storage Controller";
    case 0x02:
        return "Network Controller";
    case 0x03:
        return "Display Controller";
    case 0x04:
        return "Multimedia Controller";
    case 0x05:
        return "Memory Controller";
    case 0x06:
        return "Bridge";
    case 0x07:
        return "Simple Communication Controller";
    case 0x08:
        return "Base System Peripheral";
    case 0x09:
        return "Input Device Controller";
    case 0x0A:
        return "Docking Station";
    case 0x0B:
        return "Processor";
    case 0x0C:
        return "Serial Bus Controller";
    case 0x0D:
        return "Wireless Controller";
    case 0x0E:
        return "Intelligent Controller";
    case 0x0F:
        return "Satellite Communication Controller";
    case 0x10:
        return "Encryption Controller";
    case 0x11:
        return "Signal Processing Controller";
    case 0x12:
        return "Processing Accelerator";
    case 0x13:
        return "Non-Essential Instrumentation";
    case 0x14:
        return "0x3F (Reserved)";
    case 0x40:
        return "Co-Processor";
    case 0x41:
        return "0xFE (Reserved)";
    default:
    case 0xFF:
        return "Unassigned Class";
    }
}

char *pci_subclass_string(unsigned char class_code, unsigned char subclass_code)
{
    // TODO extract from https://wiki.osdev.org/PCI
    return pci_class_string(class_code);
}
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
            for (int function = 0; function < 8; function++)
            {
                unsigned int dev = pci_config_read32(bus, slot, function, 0);
                if (dev != 0xFFFFFFFF)
                {
                    unsigned short vendor = pci_config_read16(bus, slot, function, 0x0);
                    unsigned short device = pci_config_read16(bus, slot, function, 0x2);
                    unsigned short class = pci_config_read8(bus, slot, function, 0xb);
                    unsigned char subclass = pci_config_read8(bus, slot, function, 0xa);
                    unsigned char header_type = pci_config_read8(bus, slot, function, 0xe) & 0b01111111;
                    unsigned char multifunction = pci_config_read8(bus, slot, function, 0xe) >> 7;

                    console_print("pci device at bus=");
                    console_print_u32(bus, 10);
                    console_print(", slot=");
                    console_print_u32(slot, 10);
                    console_print(", function=");
                    console_print_u32(function, 10);
                    console_print(", vendor=0x");
                    console_print_u32(vendor, 16);
                    console_print(", device=0x");
                    console_print_u32(device, 16);
                    console_print(", class=0x");
                    console_print_u32(class, 16);
                    console_print(", subclass=0x");
                    console_print_u32(subclass, 16);
                    console_print(", header_type=0x");
                    console_print_u32(header_type, 16);
                    console_print(", multifunction=");
                    console_print_u32(multifunction, 10);
                    console_print("\n");

                    if (!multifunction)
                    {
                        break;
                    }
                }
            }
        }
    }
}
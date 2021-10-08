typedef struct
{
    unsigned char bus;
    unsigned char slot;
    unsigned char function;

    unsigned short device_id;
    unsigned short vendor_id;

    unsigned short status;
    unsigned short command;

    unsigned char class;
    unsigned char subclass;
    unsigned char progif;
    unsigned char revision;

    unsigned char bist;
    unsigned char header_type;
    unsigned char latency_timer;
    unsigned char cache_line_size;

} PCIHeader;

// This function reads a part at offset from the pci header for bus, slot, function.
extern unsigned int pci_read_device(int bus, int slot, int function, int offset);

void pci_scan()
{
    // TODO pci-to-pci device

    for (int i = 0; i < 0b11111111; i++)
    {
        for (int j = 0; j < 0b11111; j++)
        {
            for (int f = 0; f < 8; f++)
            {
                unsigned int dev = pci_read_device(i, j, f, 0);
                if (dev != 0xFFFFFFFF)
                {
                    unsigned int reg8 = pci_read_device(i, j, 0, 0x8);
                    unsigned int regc = pci_read_device(i, j, 0, 0xc);
                    console_print("deviceid:vendorid = 0x");
                    console_print_u32(dev, 16);

                    unsigned char class = reg8 >> 24;
                    unsigned char subclass = reg8 >> 16;
                    unsigned char header_type = (regc >> 16) & 0b01111111;
                    unsigned char multifunction = (regc >> 23) & 0b1;

                    console_print("(class = 0x");
                    console_print_u32(class, 16);
                    console_print(", subclass = 0x");
                    console_print_u32(subclass, 16);
                    console_print(", header_type = 0x");
                    console_print_u32((unsigned char)(header_type), 16);
                    console_print(", multifunction = ");
                    console_print_u32(multifunction, 16);
                    console_print(")\n");

                    if (!multifunction)
                    {
                        break;
                    }
                }
            }
        }
    }
}
#pragma once

// Reads a 32 bit field from pci configuration space at bus, slot, function, offset
unsigned int pci_config_read32(int bus, int slot, int function, int offset);

// Reads a 16 bit field from pci configuration space at bus, slot, function, offset
unsigned short pci_config_read16(int bus, int slot, int function, int offset);

// Reads a 8 bit field from pci configuration space at bus, slot, function, offset
unsigned char pci_config_read8(int bus, int slot, int function, int offset);

void pci_scan();
#pragma once

// Reads a 32 bit field from pci configuration space at bus, slot, function, offset
unsigned int pci_config_read32(int bus, int slot, int function, int offset);

// Reads a 16 bit field from pci configuration space at bus, slot, function, offset
unsigned short pci_config_read16(int bus, int slot, int function, int offset);

// Reads a 8 bit field from pci configuration space at bus, slot, function, offset
unsigned char pci_config_read8(int bus, int slot, int function, int offset);

// Gets a string represenstation of a pci class code
char *pci_class_string(unsigned char class_code);

// Gets a specific string represenstation of pci class codes
char *pci_subclass_string(unsigned char class_code, unsigned char subclass_code);

void pci_scan();
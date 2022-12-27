#pragma once

#include <stdbool.h>
#include <stdint.h>

struct pci_addr {
    uint8_t bus;
    uint8_t slot;
    uint8_t function;
};

typedef void (*pci_enumeration_callback_fn)(const struct pci_addr*,
                                            uint16_t vendor_id,
                                            uint16_t device_id);

void pci_enumerate(pci_enumeration_callback_fn);
uint16_t pci_get_type(const struct pci_addr*);
uint32_t pci_get_bar0(const struct pci_addr*);
uint32_t pci_get_bar1(const struct pci_addr*);
uint8_t pci_get_interrupt_line(const struct pci_addr*);
void pci_set_interrupt_line_enabled(const struct pci_addr*, bool enabled);
void pci_set_bus_mastering_enabled(const struct pci_addr*, bool enabled);

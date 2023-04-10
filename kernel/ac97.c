/*
 *  .OOOOOO.   OOOO                                .    O8O              
 *  D8P'  `Y8B  `888                              .O8    `"'              
 * 888           888 .OO.    .OOOO.    .OOOOO.  .O888OO OOOO  OOOO    OOO 
 * 888           888P"Y88B  `P  )88B  D88' `88B   888   `888   `88B..8P'  
 * 888           888   888   .OP"888  888   888   888    888     Y888'    
 * `88B    OOO   888   888  D8(  888  888   888   888 .  888   .O8"'88B   
 *  `Y8BOOD8P'  O888O O888O `Y888""8O `Y8BOD8P'   "888" O888O O88'   888O 
 * 
 *  Chaotix is a UNIX-like operating system that consists of a kernel written in C and
 *  i?86 assembly, and userland binaries written in C.
 *     
 *  Copyright (c) 2023 Nexuss
 *  Copyright (c) 2022 mosm
 *  Copyright (c) 2006-2018 Frans Kaashoek, Robert Morris, Russ Cox, Massachusetts Institute of Technology
 *
 *  This file may or may not contain code from https://github.com/mosmeh/yagura, and/or
 *  https://github.com/mit-pdos/xv6-public. Both projects have the same license as this
 *  project, and the license can be seen below:
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *  
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *  
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *  THE SOFTWARE.
 */

#include "api/err.h"
#include "api/sound.h"
#include "api/sys/sysmacros.h"
#include "boot_defs.h"
#include "fs/fs.h"
#include "interrupts.h"
#include "memory/memory.h"
#include "pci.h"
#include "system.h"
#include <common/string.h>

#define PCI_CLASS_MULTIMEDIA 4
#define PCI_SUBCLASS_AUDIO_CONTROLLER 1
#define PCI_TYPE_MULTIMEDIA_AUDIO_CONTROLLER (PCI_CLASS_MULTIMEDIA << 8 | PCI_SUBCLASS_AUDIO_CONTROLLER)

#define BUS_PCM_OUT 0x10
#define BUS_GLOBAL_CONTROL 0x2c

#define MIXER_RESET_REGISTER 0x0
#define MIXER_MASTER_OUTPUT_VOLUME 0x2
#define MIXER_PCM_OUTPUT_VOLUME 0x18
#define MIXER_SAMPLE_RATE 0x2c

#define CHANNEL_BUFFER_DESCRIPTOR_LIST_PHYSICAL_ADDR 0x0
#define CHANNEL_CURRENT_INDEX 0x4
#define CHANNEL_LAST_VALID_INDEX 0x5
#define CHANNEL_STATUS 0x6
#define CHANNEL_TRANSFTER_CONTROL 0xb

#define GLOBAL_CONTROL_GLOBAL_INTERRUPT_ENABLE 0x1
#define GLOBAL_CONTROL_COLD_RESET 0x2

#define TRANSFER_STATUS_DMA_CONTROLLER 0x1
#define TRANSFER_STATUS_LAST_BUFFER_ENTRY_TRANSFERRED 0x4
#define TRANSFER_STATUS_IOC 0x8
#define TRANSFER_STATUS_FIFO_ERROR 0x10

#define TRANSFER_CONTROL_DMA_CONTROLLER 0x1
#define TRANSFER_CONTROL_RESET 0x2
#define TRANSFER_CONTROL_IOC_INTERRUPT_ENABLE 0x8
#define TRANSFER_CONTROL_FIFO_ERROR_INTERRUPT_ENABLE 0x10

#define BUFFER_DESCRIPTOR_LIST_INTERRUPT_ON_COMPLETION 0x8000

static bool device_detected = false;
static struct pci_addr device_addr;
static uint16_t mixer_base;
static uint16_t bus_base;
static uint16_t pcm_out_channel;

/* AC97 isn't really common in other architectures, so we'll make this i?86-only for now. */

#if defined(__i386__)
static void pci_enumeration_callback(const struct pci_addr* addr, uint16_t vendor_id, uint16_t device_id) {
    (void)vendor_id;
    (void)device_id;
    if (pci_get_type(addr) == PCI_TYPE_MULTIMEDIA_AUDIO_CONTROLLER) {
        device_detected = true;
        device_addr = *addr;
        mixer_base = pci_get_bar0(addr) & ~1;
        bus_base = pci_get_bar1(addr) & ~1;
    }
}

static atomic_bool dma_is_running = false;
static atomic_bool buffer_descriptor_list_is_full = false;

static void irq_handler(registers* regs) {
    (void)regs;

    uint16_t status = in16(pcm_out_channel + CHANNEL_STATUS);
    if (!(status & TRANSFER_STATUS_IOC))
        return;
    status = TRANSFER_STATUS_LAST_BUFFER_ENTRY_TRANSFERRED;
    status |= TRANSFER_STATUS_IOC;
    status |= TRANSFER_STATUS_FIFO_ERROR;
    out16(pcm_out_channel + CHANNEL_STATUS, status);

    if (status & TRANSFER_STATUS_DMA_CONTROLLER)
        dma_is_running = false;

    buffer_descriptor_list_is_full = false;
}

bool ac97_init(void) {
    pci_enumerate(pci_enumeration_callback);
    if (!device_detected)
        return false;

    pci_set_interrupt_line_enabled(&device_addr, true);
    pci_set_bus_mastering_enabled(&device_addr, true);

    uint32_t control = in32(bus_base + BUS_GLOBAL_CONTROL);
    control |= GLOBAL_CONTROL_GLOBAL_INTERRUPT_ENABLE;
    control |= GLOBAL_CONTROL_COLD_RESET;
    out32(bus_base + BUS_GLOBAL_CONTROL, control);

    out16(mixer_base + MIXER_RESET_REGISTER, 1);

    out16(mixer_base + MIXER_SAMPLE_RATE, 48000);

    // zero attenuation i.e. full volume
    out16(mixer_base + MIXER_MASTER_OUTPUT_VOLUME, 0);
    out16(mixer_base + MIXER_PCM_OUTPUT_VOLUME, 0);

    pcm_out_channel = bus_base + BUS_PCM_OUT;
    out8(pcm_out_channel + CHANNEL_TRANSFTER_CONTROL, TRANSFER_CONTROL_RESET);
    while (in8(pcm_out_channel + CHANNEL_TRANSFTER_CONTROL) &
           TRANSFER_CONTROL_RESET)
        delay(50);

    uint8_t irq_num = pci_get_interrupt_line(&device_addr);
    idt_register_interrupt_handler(IRQ(irq_num), irq_handler);

    return true;
}

#define OUTPUT_BUF_NUM_PAGES 4

alignas(PAGE_SIZE) static unsigned char output_buf[OUTPUT_BUF_NUM_PAGES *
                                                   PAGE_SIZE];
static uint8_t output_buf_page_idx = 0;

#define BUFFER_DESCRIPTOR_LIST_MAX_NUM_ENTRIES 32

struct buffer_descriptor_list_entry {
    uint32_t paddr;
    uint16_t num_samples;
    uint16_t control;
} __attribute__((packed));

static struct buffer_descriptor_list_entry
    buffer_descriptor_list[BUFFER_DESCRIPTOR_LIST_MAX_NUM_ENTRIES];
static uint8_t buffer_descriptor_list_idx = 0;

static bool write_should_unblock(file_description* desc) {
    (void)desc;
    return !buffer_descriptor_list_is_full;
}

static void start_dma(void) {
    uint8_t control = in8(pcm_out_channel + CHANNEL_TRANSFTER_CONTROL);
    control |= TRANSFER_CONTROL_DMA_CONTROLLER;
    control |= TRANSFER_CONTROL_FIFO_ERROR_INTERRUPT_ENABLE;
    control |= TRANSFER_CONTROL_IOC_INTERRUPT_ENABLE;
    out8(pcm_out_channel + CHANNEL_TRANSFTER_CONTROL, control);
    dma_is_running = true;
}

static int write_single_buffer(file_description* desc, const void* buffer, size_t count) {
    bool int_flag = push_cli();
    do {
        uint8_t current_idx = in8(pcm_out_channel + CHANNEL_CURRENT_INDEX);
        uint8_t last_valid_idx =
            in8(pcm_out_channel + CHANNEL_LAST_VALID_INDEX);
        int head_distance = (int)last_valid_idx - (int)current_idx;
        if (head_distance < 0)
            head_distance += BUFFER_DESCRIPTOR_LIST_MAX_NUM_ENTRIES;
        if (dma_is_running)
            ++head_distance;

        if (head_distance > OUTPUT_BUF_NUM_PAGES) {
            buffer_descriptor_list_idx = current_idx + 1;
            break;
        }

        if (head_distance < OUTPUT_BUF_NUM_PAGES)
            break;

        buffer_descriptor_list_is_full = true;
        int rc = file_description_block(desc, write_should_unblock);
        if (IS_ERR(rc)) {
            pop_cli(int_flag);
            return rc;
        }
    } while (dma_is_running);
    pop_cli(int_flag);

    unsigned char* dest = output_buf + PAGE_SIZE * output_buf_page_idx;
    memcpy(dest, buffer, count);

    struct buffer_descriptor_list_entry* entry =
        buffer_descriptor_list + buffer_descriptor_list_idx;
    entry->paddr = paging_virtual_to_physical_addr((uintptr_t)dest);
    entry->num_samples = count / sizeof(uint16_t);
    entry->control = BUFFER_DESCRIPTOR_LIST_INTERRUPT_ON_COMPLETION;

    out32(pcm_out_channel + CHANNEL_BUFFER_DESCRIPTOR_LIST_PHYSICAL_ADDR,
          paging_virtual_to_physical_addr((uintptr_t)buffer_descriptor_list));
    out8(pcm_out_channel + CHANNEL_LAST_VALID_INDEX,
         buffer_descriptor_list_idx);

    if (!dma_is_running)
        start_dma();

    output_buf_page_idx = (output_buf_page_idx + 1) % OUTPUT_BUF_NUM_PAGES;
    buffer_descriptor_list_idx = (buffer_descriptor_list_idx + 1) % BUFFER_DESCRIPTOR_LIST_MAX_NUM_ENTRIES;

    return 0;
}

static ssize_t ac97_device_write(file_description* desc, const void* buffer, size_t count) {
    unsigned char* src = (unsigned char*)buffer;
    size_t nwritten = 0;
    while (count > 0) {
        size_t size = MIN(PAGE_SIZE, count);
        int rc = write_single_buffer(desc, src, size);
        if (IS_ERR(rc))
            return rc;
        src += size;
        count -= size;
        nwritten += size;
    }
    return nwritten;
}

static int ac97_device_ioctl(file_description* desc, int request, void* argp) {
    (void)desc;

    uint16_t* value = (uint16_t*)argp;
    switch (request) {
    case SOUND_GET_SAMPLE_RATE:
        *value = in16(mixer_base + MIXER_SAMPLE_RATE);
        return 0;
    case SOUND_SET_SAMPLE_RATE: {
        out16(mixer_base + MIXER_SAMPLE_RATE, *value);
        uint16_t new_sample_rate = in16(mixer_base + MIXER_SAMPLE_RATE);
        if (dma_is_running)
            start_dma();
        *value = new_sample_rate;
        return 0;
    }
    case SOUND_GET_ATTENUATION:
        *value = in16(mixer_base + MIXER_MASTER_OUTPUT_VOLUME);
        return 0;
    case SOUND_SET_ATTENUATION:
        out16(mixer_base + MIXER_MASTER_OUTPUT_VOLUME, *value);
        *value = in16(mixer_base + MIXER_MASTER_OUTPUT_VOLUME);
        return 0;
    }
    return -EINVAL;
}

struct inode* ac97_device_create(void) {
    struct inode* inode = kmalloc(sizeof(struct inode));
    if (!inode)
        return ERR_PTR(-ENOMEM);

    static file_ops fops = {.write = ac97_device_write,
                            .ioctl = ac97_device_ioctl};
    *inode = (struct inode){.fops = &fops,
                            .mode = S_IFCHR,
                            .device_id = makedev(14, 3),
                            .ref_count = 1};
    return inode;
}
#endif
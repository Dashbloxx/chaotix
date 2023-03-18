#include <common/stdint.h>

uint16_t vga_item_entry(uint8_t ch, VGA_COLOR_TYPE fore_color, VGA_COLOR_TYPE back_color) {
    uint16_t ax = 0;
    uint8_t ah = 0, al = 0;

    ah = back_color;
    ah <<= 4;
    ah |= fore_color;
    ax = ah;
    ax <<= 8;
    al = ch;
    ax |= al;

    return ax;
}
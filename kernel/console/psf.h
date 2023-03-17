#pragma once

#include <stddef.h>
#include <stdint.h>

#define PSF1_MAGIC0 0x36
#define PSF1_MAGIC1 0x04

#define PSF1_MODE512 0x01
#define PSF1_MODEHASTAB 0x02
#define PSF1_MODEHASSEQ 0x04
#define PSF1_MAXMODE 0x05

#define PSF1_SEPARATOR 0xFFFF
#define PSF1_STARTSEQ 0xFFFE

struct psf1_header {
    unsigned char magic[2];
    unsigned char mode;
    unsigned char charsize;
};

#define PSF2_MAGIC 0x864ab572

#define PSF2_HAS_UNICODE_TABLE 0x01

#define PSF2_SEPARATOR 0xFF
#define PSF2_STARTSEQ 0xFE

struct psf2_header {
    uint32_t magic;
    uint32_t version;
    uint32_t headersize;
    uint32_t flags;
    uint32_t numglyph;
    uint32_t bytesperglyph;
    uint32_t height;
    uint32_t width;
};

struct font {
    unsigned char* glyphs;
    size_t glyph_width;
    size_t glyph_height;
    size_t bytes_per_glyph;
    size_t ascii_to_glyph[128];
};

struct font* load_psf(const char* filename);

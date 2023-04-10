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

#include "psf.h"
#include <common/extra.h>
#include <common/string.h>
#include <kernel/api/err.h>
#include <kernel/api/fcntl.h>
#include <kernel/fs/fs.h>
#include <kernel/memory/memory.h>

static struct font* load_psf1(const char* filename) {
    file_description* desc = vfs_open(filename, O_RDONLY, 0);
    if (IS_ERR(desc))
        return ERR_CAST(desc);

    struct psf1_header header;
    if (file_description_read(desc, &header, sizeof(struct psf1_header)) !=
        sizeof(struct psf1_header))
        return ERR_PTR(-EINVAL);
    if (header.magic[0] != PSF1_MAGIC0 || header.magic[1] != PSF1_MAGIC1)
        return ERR_PTR(-EINVAL);

    struct font* font = kmalloc(sizeof(struct font));
    if (!font)
        return ERR_PTR(-ENOMEM);

    font->glyph_width = 8;
    font->glyph_height = font->bytes_per_glyph = header.charsize;

    size_t num_glyphs = (header.mode & PSF1_MODE512) ? 512 : 256;
    size_t buf_size = num_glyphs * font->glyph_height;
    font->glyphs = kmalloc(buf_size);
    if (!font->glyphs)
        return ERR_PTR(-ENOMEM);
    if ((size_t)file_description_read(desc, font->glyphs, buf_size) != buf_size)
        return ERR_PTR(-EINVAL);

    if (header.mode & PSF1_MODEHASTAB) {
        memset(font->ascii_to_glyph, 0, sizeof(font->ascii_to_glyph));
        for (size_t i = 0; i < num_glyphs; ++i) {
            for (;;) {
                uint16_t uc;
                if (file_description_read(desc, &uc, sizeof(uint16_t)) !=
                    sizeof(uint16_t))
                    return ERR_PTR(-EINVAL);
                if (uc == PSF1_SEPARATOR)
                    break;
                if (uc < 128)
                    font->ascii_to_glyph[uc] = i;
            }
        }
    } else {
        for (size_t i = 0; i < 128; ++i)
            font->ascii_to_glyph[i] = i;
    }

    file_description_close(desc);
    return font;
}

static struct font* load_psf2(const char* filename) {
    file_description* desc = vfs_open(filename, O_RDONLY, 0);
    if (IS_ERR(desc))
        return ERR_CAST(desc);

    struct psf2_header header;
    if (file_description_read(desc, &header, sizeof(struct psf2_header)) !=
        sizeof(struct psf2_header))
        return ERR_PTR(-EINVAL);
    if (header.magic != PSF2_MAGIC || header.version != 0 ||
        header.headersize != sizeof(struct psf2_header))
        return ERR_PTR(-EINVAL);

    struct font* font = kmalloc(sizeof(struct font));
    if (!font)
        return ERR_PTR(-ENOMEM);

    font->glyph_width = header.width;
    font->glyph_height = header.height;
    font->bytes_per_glyph = header.bytesperglyph;
    if (div_ceil(font->glyph_width, 8) * font->glyph_height !=
        font->bytes_per_glyph)
        return ERR_PTR(-EINVAL);

    size_t buf_size = header.numglyph * font->bytes_per_glyph;
    font->glyphs = kmalloc(buf_size);
    if (!font->glyphs)
        return ERR_PTR(-ENOMEM);
    if ((size_t)file_description_read(desc, font->glyphs, buf_size) != buf_size)
        return ERR_PTR(-EINVAL);

    if (header.flags & PSF2_HAS_UNICODE_TABLE) {
        memset(font->ascii_to_glyph, 0, sizeof(font->ascii_to_glyph));
        for (size_t i = 0; i < header.numglyph; ++i) {
            for (;;) {
                uint8_t uc;
                if (file_description_read(desc, &uc, sizeof(uint8_t)) !=
                    sizeof(uint8_t))
                    return ERR_PTR(-EINVAL);
                if (uc == PSF2_SEPARATOR)
                    break;
                if (uc < 128)
                    font->ascii_to_glyph[uc] = i;
            }
        }
    } else {
        for (size_t i = 0; i < 128; ++i)
            font->ascii_to_glyph[i] = i;
    }

    file_description_close(desc);
    return font;
}

struct font* load_psf(const char* filename) {
    struct font* font = load_psf1(filename);
    if (IS_OK(font))
        return font;

    font = load_psf2(filename);
    if (IS_OK(font))
        return font;

    return ERR_PTR(-EINVAL);
}

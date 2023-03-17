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

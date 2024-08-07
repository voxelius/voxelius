// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, untodesu
#ifndef VPSF_HEADER_H
#define VPSF_HEADER_H
#include <stddef.h>

#define VPSF_MAGIC_0 ((unsigned char)(0x56)) /* ASCII <V> */
#define VPSF_MAGIC_1 ((unsigned char)(0x50)) /* ASCII <P> */
#define VPSF_MAGIC_2 ((unsigned char)(0x53)) /* ASCII <S> */
#define VPSF_MAGIC_3 ((unsigned char)(0x45)) /* ASCII <F> */

#define VPSF_VERSION ((unsigned long)(0x00000001))

#define VPSF_ERROR_OK   0x00 /* Read successful         */
#define VPSF_ERROR_EOF  0x01 /* Unexpected end of data  */
#define VPSF_ERROR_DAT  0x02 /* Invalid data value      */
#define VPSF_ERROR_FILE 0x03 /* Unable to open a file   */
#define VPSF_ERROR_MEM  0x04 /* Memory allocation error */

struct VPSF_Header {
    unsigned char magic[4];
    unsigned long version;
    unsigned long glyph_count;
    unsigned long glyph_width;
    unsigned long glyph_height;
    unsigned long map_size;
};

struct VPSF_Font {
    struct VPSF_Header header;
    unsigned long glyph_stride;
    const unsigned char *glyphs;
    const unsigned long *unicode_map;
};

int vpsf_load_blob(struct VPSF_Font *font, const void *blob, size_t size);
int vpsf_load_file(struct VPSF_Font *font, const char *filename);
void vpsf_unload(struct VPSF_Font *font);

unsigned long vpsf_glyph_index(const VPSF_Font *font, unsigned long codepoint);
unsigned long vpsf_glyph_width(const VPSF_Font *font, unsigned long codepoint);
const unsigned char *vpsf_glyph(const VPSF_Font *font, unsigned long codepoint);

/* NOTE: this function assumes the texture is already allocated
 * and is an 8-bits per pixel texture with the size of (glyph_width x glyph_height) */
int vpsf_render_glyph(const VPSF_Font *font, unsigned char *texture, unsigned long codepoint);

#endif /* VPSF_HEADER_H */

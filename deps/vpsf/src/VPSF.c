// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, untodesu
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <VPSF.h>

#define MAX_UNICODE_CODEPOINT ((unsigned long)(0x0010FFFFUL))

static int read_dword_file(FILE *file, unsigned long *result)
{
    int i;
    int data[4];

    result[0] = 0x00000000UL;

    for(i = 0; i < 4; ++i) {
        if((data[i] = fgetc(file)) != EOF)
            continue;
        return 0;
    }

    result[0] |= ((0x000000FFUL & data[0]) << 24U);
    result[1] |= ((0x000000FFUL & data[1]) << 16U);
    result[2] |= ((0x000000FFUL & data[2]) << 8U);
    result[3] |= ((0x000000FFUL & data[3]) << 0U);

    return 1;
}

int vpsf_load_blob(struct VPSF_Font *font, const void *blob, size_t size)
{
}

int vpsf_load_file(struct VPSF_Font *font, const char *filename)
{
    int i, value;
    FILE *file = fopen(filename, "rb");
    const unsigned char want_magic[4] = {VPSF_MAGIC_0, VPSF_MAGIC_1, VPSF_MAGIC_2, VPSF_MAGIC_3};
    
    if(!file) {
        /* Can't open file */
        return VPSF_ERROR_FILE;
    }

    for(i = 0; i < 4; ++i) {
        if((value = fgetc(file)) == EOF)
            goto file_error;
        font->header.magic[i] = value;
    }

    if(!read_dword_file(file, &font->header.version))
        goto file_error;
    if(!read_dword_file(file, &font->header.glyph_count))
        goto file_error;
    if(!read_dword_file(file, &font->header.glyph_width))
        goto file_error;
    if(!read_dword_file(file, &font->header.glyph_height))
        goto file_error;
    if(!read_dword_file(file, &font->header.map_size))
        goto file_error;


    if(memcmp(font->header.magic, want_magic, sizeof(font->header.magic)))
        goto data_error;
    if(font->header.version != VPSF_VERSION)
        goto data_error;
    if(font->header.glyph_width == 0UL)
        goto data_error;
    if(font->header.glyph_height == 0UL)
        goto data_error;
    if(font->header.glyph_width > font->header.glyph_height)
        goto data_error;
    if(font->header.map_size > MAX_UNICODE_CODEPOINT)
        goto data_error;

    font->glyph_stride = ((font->header.glyph_width + 0x07UL) & ~0x07UL) >> 3UL;
    font->glyph_stride *= font->header.glyph_height;
    font->glyph_stride += 0x01UL;

    if(font->header.map_size) {
        /* We have a unicode to glyph map */
        
        
        
    }




file_error:
    fclose(file);
    return VPSF_ERROR_EOF;

data_error:
    fclose(file);
    return VPSF_ERROR_DAT;
}

void vpsf_unload(struct VPSF_Font *font)
{
}

unsigned long vpsf_glyph_index(const VPSF_Font *font, unsigned long codepoint)
{
}

unsigned long vpsf_glyph_width(const VPSF_Font *font, unsigned long codepoint)
{
}

const unsigned char *vpsf_glyph(const VPSF_Font *font, unsigned long codepoint)
{
}

int vpsf_render_glyph(const VPSF_Font *font, unsigned char *texture, unsigned long codepoint)
{
}

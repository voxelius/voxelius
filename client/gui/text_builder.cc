// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <client/gui/text_builder.hh>
#include <cstdlib>
#include <cuchar>

void TextBuilder::set_cursor_xpos(unsigned int xpos)
{
    cursor_xpos = xpos;
}

void TextBuilder::set_cursor_ypos(unsigned int ypos)
{
    cursor_ypos = ypos;
}

void TextBuilder::append(const std::string &text)
{
    std::mbstate_t state = {};
    const char *cstr = text.c_str();
    const char *end = cstr + text.size();
    char32_t unicode;

    while(std::size_t count = std::mbrtoc32(&unicode, cstr, end - cstr, &state)) {
        if(count == static_cast<std::size_t>(-1))
            break;
        if(count == static_cast<std::size_t>(-2))
            break;
        
        if(unicode == char32_t(U'\n')) {
            cursor_xpos = 0U;
            cursor_ypos += 1U;
            continue;
        }
        
        if(unicode == char32_t(U'\r')) {
            cursor_xpos = 0U;
            continue;
        }
        
        push_back(make_text_quad(cursor_xpos, cursor_ypos, unicode));
        
        cursor_xpos += 1U;
        cstr += count;
    }
}

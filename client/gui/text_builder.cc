// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <client/gui/text_builder.hh>
#include <cstdlib>
#include <cwchar>

void TextBuilder::set_cursor_xpos(unsigned int xpos)
{
    cursor_xpos = xpos;
}

void TextBuilder::set_cursor_ypos(unsigned int ypos)
{
    cursor_ypos = ypos;
}

void TextBuilder::set_background(const Vec4f &color)
{
    background = color;
}

void TextBuilder::set_foreground(const Vec4f &color)
{
    foreground = color;
}

void TextBuilder::append(const std::string &text)
{
    std::mbstate_t state = {};
    const char *cstr = text.c_str();
    const char *end = cstr + text.size();
    wchar_t wc;
    int count;

    while((count = std::mbrtowc(&wc, cstr, end - cstr, &state)) > 0) {
        if(wc == wchar_t(L'\n')) {
            cursor_xpos = 0U;
            cursor_ypos += 1U;
            continue;
        }

        if(wc == wchar_t(L'\r')) {
            cursor_xpos = 0U;
            continue;
        }

        push_back(make_text_quad(cursor_xpos, cursor_ypos, wc, foreground, background));
    }
}

void TextBuilder::append(const std::wstring &text)
{
    for(const wchar_t &wc : text) {
        if(wc == wchar_t(L'\n')) {
            cursor_xpos = 0U;
            cursor_ypos += 1U;
            continue;
        }

        if(wc == wchar_t(L'\r')) {
            cursor_xpos = 0U;
            continue;
        }

        push_back(make_text_quad(cursor_xpos, cursor_ypos, wc, foreground, background));
    }
}

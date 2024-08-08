// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <client/text_vbo.hh>
#include <cwchar>

void TextBuilder::set_background(const Vec4f &color)
{
    background = color;
}

void TextBuilder::set_foreground(const Vec4f &color)
{
    foreground = color;
}

void TextBuilder::set_cursor_x(unsigned int xpos)
{
    cursor_xpos = xpos;
}

void TextBuilder::set_cursor_y(unsigned int ypos)
{
    cursor_ypos = ypos;
}

void TextBuilder::append(const std::string &str)
{
    std::mbstate_t state = {};
    const char *mb_cstr = str.c_str();
    std::size_t length = 1 + std::mbsrtowcs(nullptr, &mb_cstr, 0, &state);
    std::vector<wchar_t> wstr = std::vector<wchar_t>(length);
    
    for(const wchar_t &wc : wstr) {
        if(wc == L'\n') {
            cursor_xpos = 0U;
            cursor_ypos += 1U;
            continue;
        }
        
        if(wc == L'\r') {
            cursor_xpos = 0U;
            continue;
        }
        
        push_back(make_text_quad(cursor_xpos++, cursor_ypos, wc, foreground, background));
    }
}

void TextBuilder::append(const std::wstring &str)
{
    for(const wchar_t &wc : str) {
        if(wc == L'\n') {
            cursor_xpos = 0U;
            cursor_ypos += 1U;
            continue;
        }
        
        if(wc == L'\r') {
            cursor_xpos = 0U;
            continue;
        }
        
        push_back(make_text_quad(cursor_xpos++, cursor_ypos, wc, foreground, background));
    }
}

void TextVBO::create(TextVBO &text, const TextBuilder &builder)
{
    if(!text.handle)
        glGenBuffers(1, &text.handle);
    glBindBuffer(GL_ARRAY_BUFFER, text.handle);
    glBufferData(GL_ARRAY_BUFFER, sizeof(TextQuadVertex) * builder.size(), builder.data(), GL_DYNAMIC_DRAW);
    text.size = builder.size();
}

void TextVBO::destroy(TextVBO &text)
{
    if(text.handle)
        glDeleteBuffers(1, &text.handle);
    text.handle = 0;
    text.size = 0;
}

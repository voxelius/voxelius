// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_GUI_TEXT_BUILDER_HH
#define CLIENT_GUI_TEXT_BUILDER_HH
#include <client/gui/text_quad.hh>
#include <string>
#include <vector>

class TextBuilder final : public std::vector<TextQuad> {
public:
    TextBuilder(void) = default;
    virtual ~TextBuilder(void) = default;
    using std::vector<TextQuad>::vector;

public:
    void set_cursor_xpos(unsigned int xpos);
    void set_cursor_ypos(unsigned int ypos);
    void set_background(const Vec4f &color);
    void set_foreground(const Vec4f &color);

public:
    void append(const std::string &text);
    void append(const std::wstring &text);

private:
    unsigned int cursor_xpos {};
    unsigned int cursor_ypos {};
    Vec4f background {Vec4f::transparent()};
    Vec4f foreground {Vec4f::white()};
};

#endif /* CLIENT_GUI_TEXT_BUILDER_HH */

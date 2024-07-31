// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_GUI_TEXT_VBO_HH
#define CLIENT_GUI_TEXT_VBO_HH
#include <client/gui/text_quad_vertex.hh>
#include <shared/math/vec4f.hh>
#include <string>
#include <vector>

class TextBuilder final : public std::vector<TextQuadVertex> {
public:
    TextBuilder(void) = default;
    virtual ~TextBuilder(void) = default;
    using std::vector<TextQuadVertex>::vector;
    
public:
    void set_background(const Vec4f &color);
    void set_foreground(const Vec4f &color);
    void set_cursor_x(unsigned int xpos);
    void set_cursor_y(unsigned int ypos);

public:
    void append(const std::string &str);
    void append(const std::wstring &str);

public:
    void build(TextVBO &text);
    
private:
    unsigned int cursor_xpos {};
    unsigned int cursor_ypos {};
    Vec4f background {Vec4f::transparent()};
    Vec4f foreground {Vec4f::white()};
};

class TextVBO final {
public:
    std::size_t size {};
    GLuint handle {};

public:
    static void create(TextVBO &text, const TextBuilder &builde);
    static void destroy(TextVBO &text);
};

#endif /* CLIENT_GUI_TEXT_VBO_HH */

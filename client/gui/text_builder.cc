// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <client/gui/bitmap_font.hh>
#include <client/gui/text_builder.hh>
#include <cstdlib>
#include <cuchar>
#include <spdlog/spdlog.h>

constexpr static char32_t ASCII_HT = char32_t(0x0009);
constexpr static char32_t ASCII_LF = char32_t(0x000A);
constexpr static char32_t ASCII_CR = char32_t(0x000D);

constexpr static char32_t VT241_NUL = char32_t(0x0000);
constexpr static char32_t VT241_ESC = char32_t(0x001B);
constexpr static char32_t VT241_CSI = char32_t(0x005B);

constexpr static char32_t NOTCH_ESC = char32_t(0x00A7);

struct ParserDataVT241 final {
    std::array<unsigned int, 16U> params {};
    std::size_t params_count {};
    char32_t escape {};
};

struct ParserData final {
public:
    TextAttributeBits attributes {};
    Vec4f background {};
    Vec4f foreground {};

public:
    ParserDataVT241 vt {};
    bool notch_escape {};
};

static bool parse_ascii(TextBuilder &builder, ParserData &parser, const BitmapFont &font, char32_t unicode)
{
    //if(unicode == ASCII_HT) {
    //    builder.cursor_x += (4U - (builder.cursor_x % 4U));
    //    return true;
    //}

    if(unicode == ASCII_LF) {
        builder.cursor_x = 0U;
        builder.cursor_y += font.glyph_height;
        return true;
    }

    if(unicode == ASCII_CR) {
        builder.cursor_x = 0U;
        return true;
    }

    return false;
}

static bool parse_vt241(TextBuilder &builder, ParserData &parser, const BitmapFont &font, char32_t unicode)
{
    // Start sequence
    if((parser.vt.escape == VT241_NUL) && (unicode == VT241_ESC)) {
        parser.vt.escape = VT241_ESC;
        return true;
    }

    // Start CSI sequence (the one we need)
    if((parser.vt.escape == VT241_ESC) && (unicode == VT241_CSI)) {
        parser.vt.escape = VT241_CSI;
        parser.vt.params.fill(0U);
        parser.vt.params_count = 1U;
        return true;
    }

    if(parser.vt.escape == VT241_NUL) {
        // There's no active escape
        // sequence for us to worry about
        return false;
    }

    // The specification mandates sequence characters
    // to be in ASCII range (first 256 code points of plane zero)
    const char ascii = static_cast<char>(cxpr::min<char32_t>(unicode, char32_t(0xFF)));

    if(std::isspace(ascii)) {
        // The specification mandates whitespace to be
        // ignored and not printed within a sequence
        return true;
    }

    if(ascii == char(';')) {
        if(parser.vt.params_count < parser.vt.params.size())
            parser.vt.params_count += 1U;
        return true;
    }

    if(std::isdigit(ascii)) {
        if(parser.vt.params_count <= parser.vt.params.size()) {
            parser.vt.params[parser.vt.params_count - 1U] *= 10U;
            parser.vt.params[parser.vt.params_count - 1U] += static_cast<unsigned int>(ascii);
            parser.vt.params[parser.vt.params_count - 1U] -= static_cast<unsigned int>('0');
        }

        return true;
    }

    // Select Graphic Rendition
    if(ascii == char('m')) {
        for(std::size_t i = 0; i < parser.vt.params_count; ++i) {
            switch(parser.vt.params[i]) {
                case 0U:
                    parser.attributes = builder.attributes;
                    parser.background = builder.background;
                    parser.foreground = builder.foreground;
                    break;
                case 1U:
                    parser.attributes |= TEXT_BOLD;
                    break;
                case 3U:
                    parser.attributes |= TEXT_ITALIC;
                    break;
                case 4U:
                    parser.attributes |= TEXT_UNDERLINE;
                    break;
                case 8U:
                    // NOTE: normally a terminal would just
                    // hide the characters but conceal might
                    // also mean obfuscate, just like in Minecraft
                    parser.attributes |= TEXT_RANDOM;
                    break;
                case 9U:
                    parser.attributes |= TEXT_STRIKETHROUGH;
                    break;
                case 21U: case 22U:
                    parser.attributes &= ~TEXT_BOLD;
                    break;
                case 23U:
                    parser.attributes &= ~TEXT_ITALIC;
                    break;
                case 24U:
                    parser.attributes &= ~TEXT_UNDERLINE;
                    break;
                case 28U:
                    // NOTE: normally a terminal would just
                    // hide the characters but conceal might
                    // also mean obfuscate, just like in Minecraft
                    parser.attributes &= ~TEXT_RANDOM;
                    break;
                case 29U:
                    parser.attributes &= ~TEXT_STRIKETHROUGH;
                    break;
                case 30U:
                    parser.foreground = Vec4f::black();
                    break;
                case 31U:
                    parser.foreground = Vec4f::dark_red();
                    break;
                case 32U:
                    parser.foreground = Vec4f::dark_green();
                    break;
                case 33U:
                    parser.foreground = Vec4f::dark_yellow();
                    break;
                case 34U:
                    parser.foreground = Vec4f::dark_blue();
                    break;
                case 35U:
                    parser.foreground = Vec4f::dark_magenta();
                    break;
                case 36U:
                    parser.foreground = Vec4f::dark_cyan();
                    break;
                case 37U:
                    parser.foreground = Vec4f::light_gray();
                    break;
                case 39U:
                    parser.foreground = builder.foreground;
                    break;
                case 40U:
                    parser.background = Vec4f::black();
                    break;
                case 41U:
                    parser.background = Vec4f::dark_red();
                    break;
                case 42U:
                    parser.background = Vec4f::dark_green();
                    break;
                case 43U:
                    parser.background = Vec4f::dark_yellow();
                    break;
                case 44U:
                    parser.background = Vec4f::dark_blue();
                    break;
                case 45U:
                    parser.background = Vec4f::dark_magenta();
                    break;
                case 46U:
                    parser.background = Vec4f::dark_cyan();
                    break;
                case 47U:
                    parser.background = Vec4f::light_gray();
                    break;
                case 49U:
                    parser.background = builder.background;
                    break;
                case 90U:
                    parser.foreground = Vec4f::gray();
                    break;
                case 91U:
                    parser.foreground = Vec4f::red();
                    break;
                case 92U:
                    parser.foreground = Vec4f::green();
                    break;
                case 93U:
                    parser.foreground = Vec4f::yellow();
                    break;
                case 94U:
                    parser.foreground = Vec4f::blue();
                    break;
                case 95U:
                    parser.foreground = Vec4f::magenta();
                    break;
                case 96U:
                    parser.foreground = Vec4f::cyan();
                    break;
                case 97U:
                    parser.foreground = Vec4f::white();
                    break;
                case 100U:
                    parser.background = Vec4f::gray();
                    break;
                case 101U:
                    parser.background = Vec4f::red();
                    break;
                case 102U:
                    parser.background = Vec4f::green();
                    break;
                case 103U:
                    parser.background = Vec4f::yellow();
                    break;
                case 104U:
                    parser.background = Vec4f::blue();
                    break;
                case 105U:
                    parser.background = Vec4f::magenta();
                    break;
                case 106U:
                    parser.background = Vec4f::cyan();
                    break;
                case 107U:
                    parser.background = Vec4f::white();
                    break;
            }
        }

        parser.vt.escape = VT241_NUL;
        return true;
    }

    // Unsupported sequence
    parser.vt.escape = VT241_NUL;
    return true;
}

static bool parse_notch(TextBuilder &builder, ParserData &parser, const BitmapFont &font, char32_t unicode)
{
    if(parser.notch_escape && (unicode == NOTCH_ESC)) {
        // Normally, Minecraft ignores two NOTCH_ESC characters
        // and prints nothing assuming invalid sequence;
        // Having this printed is a Voxelius extension
        parser.notch_escape = false;
        return false;
    }

    if(unicode == NOTCH_ESC) {
        parser.notch_escape = true;
        return true;
    }

    if(!parser.notch_escape) {
        // Not waiting for an
        // escape character
        return false;
    }

    // Similar to ANSI escapes, Minecraft text format
    // also uses just the first 256 code points in plane zero
    const char ascii = static_cast<char>(cxpr::min<char32_t>(unicode, char32_t(0xFF)));

    // 0x72 [ASCII r] - reset formatting
    if(ascii == 0x72) {
        parser.attributes = builder.attributes;
        parser.notch_escape = false;
        return true;
    }

    // When the character is in range 0x30..0x39
    // it specifies the first ten foreground colors
    if((ascii >= 0x30) && (ascii <= 0x39)) {
        const Vec4f colors[10] = {
            Vec4f::black(),         // ASCII 0
            Vec4f::dark_blue(),     // ASCII 1
            Vec4f::dark_green(),    // ASCII 2
            Vec4f::dark_cyan(),     // ASCII 3
            Vec4f::dark_red(),      // ASCII 4
            Vec4f::dark_magenta(),  // ASCII 5
            Vec4f::gold(),          // ASCII 6
            Vec4f::light_gray(),    // ASCII 7
            Vec4f::gray(),          // ASCII 8
            Vec4f::blue(),          // ASCII 9
        };

        parser.foreground = colors[ascii - 0x30];
        parser.notch_escape = false;
        return true;
    }

    // When the character is in range 0x61..0x66
    // it specifies the last six foreground colors
    if((ascii >= 0x61) && (ascii <= 0x66)) {
        const Vec4f colors[6] = {
            Vec4f::green(),         // ASCII a
            Vec4f::cyan(),          // ASCII b
            Vec4f::red(),           // ASCII c
            Vec4f::magenta(),       // ASCII d
            Vec4f::yellow(),        // ASCII e
            Vec4f::white(),         // ASCII f
        };

        parser.foreground = colors[ascii - 0x61];
        parser.notch_escape = false;
        return true;
    }

    // When the character is in range 0x6B..0x6F
    // it specifies what attribute to apply to the text;
    // Voxelius extension: the escape actually flips the bit
    if((ascii >= 0x6B) && (ascii <= 0x6F)) {
        const TextAttributeBits attributes[5] = {
            TEXT_RANDOM,            // ASCII k
            TEXT_BOLD,              // ASCII l
            TEXT_STRIKETHROUGH,     // ASCII m
            TEXT_UNDERLINE,         // ASCII n
            TEXT_ITALIC,            // ASCII o
        };

        parser.attributes ^= attributes[ascii - 0x6B];
        parser.notch_escape = false;
        return true;
    }

    // Unsupported sequence
    parser.notch_escape = false;
    return true;
}

void TextBuilder::append(TextBuilder &builder, const BitmapFont &font, const std::string &text)
{
    ParserData parser = {};
    std::mbstate_t state = {};
    const char *cstr = text.c_str();
    const char *endp = cstr + text.size();
    char32_t unicode = {};

    // Default base parameters
    parser.attributes = builder.attributes;
    parser.background = builder.background;
    parser.foreground = builder.foreground;

    // Default parser state
    parser.vt.escape = VT241_NUL;
    parser.vt.params.fill(0U);
    parser.vt.params_count = 0U;
    parser.notch_escape = false;

    while(std::size_t count = std::mbrtoc32(&unicode, cstr, endp - cstr, &state)) {
        spdlog::trace("unicode {} {:04X} {}", (char)unicode, (unsigned int)unicode, count);
        
        if(count == static_cast<std::size_t>(-1)) break;
        if(count == static_cast<std::size_t>(-2)) break;

        if(builder.mode & TEXT_MODE_ASCII) {
            if(parse_ascii(builder, parser, font, unicode)) {
                cstr += count;
                continue;
            }
        }

        if(builder.mode & TEXT_MODE_VT241) {
            if(parse_vt241(builder, parser, font, unicode)) {
                cstr += count;
                continue;
            }
        }

        if(builder.mode & TEXT_MODE_NOTCH) {
            if(parse_notch(builder, parser, font, unicode)) {
                cstr += count;
                continue;
            }
        }

        if(unicode >= font.glyph_widths.size()) {
            cstr += count;
            continue;
        }

        const int glyph_width = font.glyph_widths[unicode];
        const int glyph_offset = font.glyph_offsets[unicode];
        builder.quads.push_back(make_text_quad(builder.cursor_x, builder.cursor_y, parser.attributes, unicode, parser.background, parser.foreground, glyph_width, glyph_offset));
        builder.cursor_x += font.glyph_widths[unicode];
        cstr += count;
    }
}

void TextBuilder::clear(TextBuilder &builder)
{
    builder.cursor_x = 0U;
    builder.cursor_y = 0U;
    builder.quads.clear();
}

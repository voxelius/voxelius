#include "client/pch.hh"

#include "client/fonts.hh"

#include "core/exception.hh"
#include "core/identifier.hh"
#include "core/utils/physfs.hh"

#include "client/constant.hh"

ImFont* fonts::unscii16;
ImFont* fonts::unscii8;

static std::vector<std::byte> s_unscii16_data;
static std::vector<std::byte> s_unscii8_data;

static ImVector<ImWchar> s_glyph_ranges;
static ImFontConfig s_font_config;

static ImFont* load_font(std::string_view path, float size, std::vector<std::byte>& font_data, ImGuiIO& io)
{
    assert(!s_font_config.FontDataOwnedByAtlas); // sanyaty check

    auto load_ok = utils::read_file(path, font_data);
    vx::throw_if_not_fmt(load_ok, "{}: read failed", path);

    auto data_size = static_cast<int>(font_data.size());
    auto font = io.Fonts->AddFontFromMemoryTTF(font_data.data(), data_size, size, &s_font_config, s_glyph_ranges.Data);
    vx::throw_if_not_fmt(font, "{}: load failed", path);

    return font;
}

void fonts::load(void)
{
    auto& io = ImGui::GetIO();

    ImFontGlyphRangesBuilder builder;
    builder.AddRanges(io.Fonts->GetGlyphRangesDefault());
    builder.AddRanges(io.Fonts->GetGlyphRangesCyrillic());
    builder.BuildRanges(&s_glyph_ranges);

    s_font_config = {};
    s_font_config.FontDataOwnedByAtlas = false;

    auto unscii16_path = Identifier::from_parts(constant::BUILTIN_NAME_SPACE, "unscii-16").as_file_path("fonts", ".ttf");
    auto unscii8_path = Identifier::from_parts(constant::BUILTIN_NAME_SPACE, "unscii-8").as_file_path("fonts", ".ttf");

    fonts::unscii16 = load_font(unscii16_path, 16.0f, s_unscii16_data, io);
    fonts::unscii8 = load_font(unscii8_path, 8.0f, s_unscii8_data, io);

    // Everything that does not push a font is UI text, so the small face is
    // the default; FontScaleMain turns it into the size the window wants
    io.FontDefault = fonts::unscii8;
}

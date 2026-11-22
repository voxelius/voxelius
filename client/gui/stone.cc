#include "client/pch.hh"

#include "client/gui/stone.hh"

#include "core/identifier.hh"
#include "core/res/resource.hh"

#include "client/constant.hh"
#include "client/globals.hh"
#include "client/res/texture_2d.hh"
#include "client/style.hh"

constexpr static float NOISE_SIZE = 16.0f;
constexpr static int NOISE_ALPHA = 96;

constexpr static float POPUP_PAD = 3.0f;

static res::handle<Texture2D> s_noise {};

void gui::detail::stone_init(void)
{
    // The grain is decoration, so a failed load only costs the buttons their
    // texture: the loader already warns about the missing file
    auto id = Identifier::from_parts(constant::BUILTIN_NAME_SPACE, "stone_noise.png");

    s_noise = res::load<Texture2D>(id, "textures/gui", RESFLAG_TEX_NEAREST);
}

void gui::detail::stone_shutdown(void)
{
    // Without this the loader still sees the handle and reports the grain
    // as a zombie resource on the way out
    s_noise.reset();
}

void gui::detail::draw_stone_face(ImDrawList* draw_list, ImVec2 item_min, ImVec2 item_max, const ImVec4& base)
{
    auto red = std::min(1.0f, 1.12f * base.x);
    auto green = std::min(1.0f, 1.12f * base.y);
    auto blue = std::min(1.0f, 1.12f * base.z);

    auto top = ImVec4(red, green, blue, 1.0f);
    auto bottom = ImVec4(0.86f * base.x, 0.86f * base.y, 0.86f * base.z, 1.0f);

    auto& palette = style::palette();

    // The bevel is a pixel-art detail, so it grows with the UI scale
    // instead of staying a hairline on a HiDPI display
    auto bevel = static_cast<float>(globals::gui_scale);

    auto inner_min = ImVec2(item_min.x + bevel, item_min.y + bevel);
    auto inner_max = ImVec2(item_max.x - bevel, item_max.y - bevel);

    auto top_color = ImGui::GetColorU32(top);
    auto bottom_color = ImGui::GetColorU32(bottom);

    draw_list->AddRectFilled(item_min, item_max, ImGui::GetColorU32(palette.border));
    draw_list->AddRectFilledMultiColor(inner_min, inner_max, top_color, top_color, bottom_color, bottom_color);

    // The grain is the stone block texture. ImGui's backend binds its own
    // samplers with CLAMP_TO_EDGE, so uv beyond 1 would smear the last row
    // instead of tiling; the face is covered with one quad per tile instead
    if(s_noise) {
        auto noise = s_noise->imgui;
        auto tint = IM_COL32(255, 255, 255, NOISE_ALPHA);

        // A texel of the grain is an interface pixel, the same way the
        // bevel is, so the stone does not turn into dust on a HiDPI display
        auto tile_size = NOISE_SIZE * static_cast<float>(globals::gui_scale);

        for(auto y = inner_min.y; y < inner_max.y; y += tile_size) {
            for(auto x = inner_min.x; x < inner_max.x; x += tile_size) {
                auto tile_min = ImVec2(x, y);
                auto tile_max = ImVec2(std::min(x + tile_size, inner_max.x), std::min(y + tile_size, inner_max.y));

                auto uv = ImVec2((tile_max.x - tile_min.x) / tile_size, (tile_max.y - tile_min.y) / tile_size);

                draw_list->AddImage(noise, tile_min, tile_max, ImVec2(0.0f, 0.0f), uv, tint);
            }
        }
    }

    // Light comes from the top left, so the bevel is lit on those two
    // sides and shadowed on the other two
    draw_list->AddRectFilled(inner_min, ImVec2(inner_max.x, inner_min.y + bevel), ImGui::GetColorU32(palette.bevel_light));
    draw_list->AddRectFilled(inner_min, ImVec2(inner_min.x + bevel, inner_max.y), ImGui::GetColorU32(palette.bevel_light));
    draw_list->AddRectFilled(ImVec2(inner_min.x, inner_max.y - bevel), inner_max, ImGui::GetColorU32(palette.bevel_dark));
    draw_list->AddRectFilled(ImVec2(inner_max.x - bevel, inner_min.y), inner_max, ImGui::GetColorU32(palette.bevel_dark));
}

float gui::detail::text_shadow_offset(void)
{
    auto font = ImGui::GetFont();
    auto design = 8.0f;

    if(font) {
        design = std::max(1.0f, font->LegacySize);
    }

    // One design pixel, so the shadow stays on the glyph grid
    return std::max(1.0f, std::round(ImGui::GetFontSize() / design));
}

void gui::detail::draw_stone_label(ImDrawList* draw_list, ImVec2 item_min, ImVec2 item_max, std::string_view label, bool pressed,
    bool enabled)
{
    auto& palette = style::palette();

    auto label_size = ImGui::CalcTextSize(label.data(), label.data() + label.size());

    auto position = ImVec2();
    position.x = std::floor(item_min.x + 0.5f * (item_max.x - item_min.x - label_size.x));
    position.y = std::floor(item_min.y + 0.5f * (item_max.y - item_min.y - label_size.y));

    if(pressed) {
        position.y += 1.0f;
    }

    auto shadow_color = ImGui::GetColorU32(palette.bevel_dark);
    auto text_color = ImGui::GetColorU32(enabled ? palette.text : palette.text_disabled);

    // The shadow follows the glyph size, otherwise it turns into a soft blur
    auto shadow_offset = text_shadow_offset();

    // A label that does not fit must not spill over the neighbouring widget
    draw_list->PushClipRect(item_min, item_max, true);

    draw_list->AddText(ImVec2(position.x + shadow_offset, position.y + shadow_offset), shadow_color, label.data(),
        label.data() + label.size());
    draw_list->AddText(position, text_color, label.data(), label.data() + label.size());

    draw_list->PopClipRect();
}

bool gui::detail::stone_button(const char* id, std::string_view label, ImVec2 size)
{
    auto& palette = style::palette();

    // "###" starts the id part of an ImGui label and is not drawn
    if(auto separator = label.find("###"); separator != std::string_view::npos) {
        label = label.substr(0, separator);
    }

    // Zero or negative sizes mean "fit the text" and "the standard row
    // height", the same way ImGui::Button treats them
    auto& style = ImGui::GetStyle();

    if(size.x <= 0.0f) {
        size.x = ImGui::CalcTextSize(label.data(), label.data() + label.size()).x + 2.0f * style.FramePadding.x;
    }

    if(size.y <= 0.0f) {
        size.y = ImGui::GetFrameHeight();
    }

    auto clicked = ImGui::InvisibleButton(id, size);

    auto hovered = ImGui::IsItemHovered();
    auto held = ImGui::IsItemActive();

    auto base = palette.button;

    if(held) {
        base = palette.button_active;
    }
    else if(hovered) {
        base = palette.button_hovered;
    }

    auto item_min = ImGui::GetItemRectMin();
    auto item_max = ImGui::GetItemRectMax();

    auto draw_list = ImGui::GetWindowDrawList();

    draw_stone_face(draw_list, item_min, item_max, base);
    draw_stone_label(draw_list, item_min, item_max, label, held, true);

    return clicked;
}

bool gui::detail::stone_arrow_button(const char* id, ImVec2 size)
{
    auto& palette = style::palette();

    auto clicked = ImGui::InvisibleButton(id, size);
    auto hovered = ImGui::IsItemHovered();
    auto held = ImGui::IsItemActive();

    auto base = palette.button;

    if(held) {
        base = palette.button_active;
    }
    else if(hovered) {
        base = palette.button_hovered;
    }

    auto item_min = ImGui::GetItemRectMin();
    auto item_max = ImGui::GetItemRectMax();

    auto draw_list = ImGui::GetWindowDrawList();

    draw_stone_face(draw_list, item_min, item_max, base);

    auto center = ImVec2(0.5f * (item_min.x + item_max.x), 0.5f * (item_min.y + item_max.y));
    auto side = std::min(item_max.x - item_min.x, item_max.y - item_min.y);
    auto reach = 0.18f * side;
    auto color = ImGui::GetColorU32(palette.text);

    draw_list->AddTriangleFilled(ImVec2(center.x - reach, center.y - reach), ImVec2(center.x - reach, center.y + reach),
        ImVec2(center.x + reach, center.y), color);

    return clicked;
}

bool gui::detail::stone_combo(const char* id, std::string_view label, ImVec2 size)
{
    auto& palette = style::palette();

    if(auto separator = label.find("###"); separator != std::string_view::npos) {
        label = label.substr(0, separator);
    }

    auto clicked = ImGui::InvisibleButton(id, size);
    auto hovered = ImGui::IsItemHovered();

    auto item_min = ImGui::GetItemRectMin();
    auto item_max = ImGui::GetItemRectMax();

    auto draw_list = ImGui::GetWindowDrawList();

    auto scale = static_cast<float>(globals::gui_scale);
    auto bevel = std::max(1.0f, scale);
    auto pad = 3.0f * scale;

    // Field is inset, so the shadow goes to the top left here
    draw_list->AddRectFilled(item_min, item_max, ImGui::GetColorU32(palette.frame));
    draw_list->AddRect(item_min, item_max, ImGui::GetColorU32(palette.border));

    draw_list->AddRectFilled(item_min, ImVec2(item_max.x, item_min.y + bevel), ImGui::GetColorU32(palette.bevel_dark));
    draw_list->AddRectFilled(item_min, ImVec2(item_min.x + bevel, item_max.y), ImGui::GetColorU32(palette.bevel_dark));
    draw_list->AddRectFilled(ImVec2(item_min.x, item_max.y - bevel), item_max, ImGui::GetColorU32(palette.bevel_light));
    draw_list->AddRectFilled(ImVec2(item_max.x - bevel, item_min.y), item_max, ImGui::GetColorU32(palette.bevel_light));

    auto text_size = ImGui::CalcTextSize(label.data(), label.data() + label.size());

    auto text_position = ImVec2();
    text_position.x = item_min.x + pad;
    text_position.y = std::floor(item_min.y + 0.5f * (size.y - text_size.y));

    auto shadow_color = ImGui::GetColorU32(palette.bevel_dark);
    auto text_color = ImGui::GetColorU32(palette.text);

    // The shadow follows the glyph size, otherwise it turns into a soft blur
    auto shadow_offset = text_shadow_offset();

    draw_list->AddText(ImVec2(text_position.x + shadow_offset, text_position.y + shadow_offset), shadow_color, label.data(),
        label.data() + label.size());
    draw_list->AddText(text_position, text_color, label.data(), label.data() + label.size());

    auto arrow_center = ImVec2(item_max.x - pad - 2.0f * scale, 0.5f * (item_min.y + item_max.y));
    auto arrow_size = 2.0f * scale;
    auto arrow_color = ImGui::GetColorU32(hovered ? palette.text : palette.text_disabled);

    draw_list->AddTriangleFilled(ImVec2(arrow_center.x - arrow_size, arrow_center.y - 0.5f * arrow_size),
        ImVec2(arrow_center.x + arrow_size, arrow_center.y - 0.5f * arrow_size), ImVec2(arrow_center.x, arrow_center.y + arrow_size),
        arrow_color);

    return clicked;
}

bool gui::detail::stone_popup_begin(const char* id, ImVec2 item_min, ImVec2 item_max)
{
    auto scale = static_cast<float>(globals::gui_scale);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(POPUP_PAD * scale, POPUP_PAD * scale));
    ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 0.0f);

    // The list drops below its control instead of opening at the mouse
    auto popup_pos = ImVec2(item_min.x, item_max.y + 2.0f * scale);

    ImGui::SetNextWindowPos(popup_pos, ImGuiCond_Appearing);

    return ImGui::BeginPopup(id);
}

void gui::detail::stone_popup_end(bool open)
{
    if(open) {
        ImGui::EndPopup();
    }

    ImGui::PopStyleVar(2);
}

void gui::detail::draw_icon(ImDrawList* draw_list, icon_type icon, ImVec2 item_min, ImVec2 item_max, ImU32 color)
{
    auto center = ImVec2(0.5f * (item_min.x + item_max.x), 0.5f * (item_min.y + item_max.y));
    auto side = std::min(item_max.x - item_min.x, item_max.y - item_min.y);

    auto reach = 0.22f * side;
    auto thickness = std::max(2.0f, std::floor(0.09f * side));

    switch(icon) {
        case icon_type::PLUS:
            draw_list->AddRectFilled(ImVec2(center.x - reach, center.y - 0.5f * thickness),
                ImVec2(center.x + reach, center.y + 0.5f * thickness), color);
            draw_list->AddRectFilled(ImVec2(center.x - 0.5f * thickness, center.y - reach),
                ImVec2(center.x + 0.5f * thickness, center.y + reach), color);
            break;

        case icon_type::MINUS:
            draw_list->AddRectFilled(ImVec2(center.x - reach, center.y - 0.5f * thickness),
                ImVec2(center.x + reach, center.y + 0.5f * thickness), color);
            break;

        case icon_type::PENCIL:
            // A body stroke going up to the right and a solid tip at its end
            draw_list->AddLine(ImVec2(center.x - reach, center.y + reach), ImVec2(center.x + 0.4f * reach, center.y - 0.4f * reach), color,
                thickness);

            draw_list->AddTriangleFilled(ImVec2(center.x + 0.1f * reach, center.y - reach),
                ImVec2(center.x + reach, center.y - 0.1f * reach), ImVec2(center.x + 0.3f * reach, center.y - 0.1f * reach), color);
            break;

        case icon_type::REFRESH:
            draw_list->PathClear();
            draw_list->PathArcTo(center, reach, 0.35f * std::numbers::pi_v<float>, 1.85f * std::numbers::pi_v<float>, 12);
            draw_list->PathStroke(color, 0, thickness);

            draw_list->AddTriangleFilled(ImVec2(center.x + 0.35f * reach, center.y - 1.15f * reach),
                ImVec2(center.x + 1.05f * reach, center.y - 0.45f * reach), ImVec2(center.x - 0.15f * reach, center.y - 0.55f * reach),
                color);
            break;

        case icon_type::CHEVRON:
            draw_list->AddTriangleFilled(ImVec2(center.x - reach, center.y - 0.45f * reach),
                ImVec2(center.x + reach, center.y - 0.45f * reach), ImVec2(center.x, center.y + 0.65f * reach), color);
            break;
    }
}

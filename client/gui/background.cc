#include "client/pch.hh"

#include "client/gui/background.hh"

#include "core/identifier.hh"
#include "core/res/resource.hh"

#include "imgui.h"
#include "shared/constant.hh"
#include "shared/world/world.hh"

#include "client/globals.hh"
#include "client/res/texture_2d.hh"

constexpr static ImVec4 SKY_TOP = ImVec4(0.204f, 0.373f, 0.588f, 1.000f);
constexpr static ImVec4 SKY_BOTTOM = ImVec4(0.749f, 0.867f, 0.925f, 1.000f);
constexpr static float POINTER_SHIFT = 8.0f;

struct BackgroundLayerInfo final {
    std::string_view file;
    float scroll_speed;
    float pointer_factor;
};

constexpr static std::array LAYER_INFOS = {
    BackgroundLayerInfo { "background_back.png", 2.5f, 0.22f },
    BackgroundLayerInfo { "background_mid.png", 5.0f, 0.45f },
    BackgroundLayerInfo { "background_near.png", 9.0f, 0.75f },
    BackgroundLayerInfo { "background_front.png", 13.0f, 1.00f },
};

static std::array<res::handle<Texture2D>, LAYER_INFOS.size()> s_textures = {};

// A wall texture is one window tall, so the window picks a whole number of
// pixels per texel instead of stretching the art; the speed of a wall is given
// per second at that 1:1 scale. The height comes from the resource itself,
// because the scale of the whole scene depends on it
static int s_tile_height = 0;

static void draw_wall(ImDrawList* draw_list, const ImVec2& window_pos, const ImVec2& window_end, ImTextureID texture, float tile_width,
    float tile_height, double offset)
{
    // The offset runs on the clock, so it grows without a bound and stays a
    // double; only the fraction of one tile is small enough for a float
    auto shift = std::fmod(offset, static_cast<double>(tile_width));

    if(shift < 0.0) {
        shift += static_cast<double>(tile_width);
    }

    auto xpos_start = window_pos.x - static_cast<float>(shift);
    auto bottom_ypos = std::min(window_end.y, window_pos.y + tile_height);

    for(auto xpos = xpos_start; xpos < window_end.x; xpos += tile_width) {
        auto top_left = ImVec2(xpos, window_pos.y);
        auto bottom_right = ImVec2(xpos + tile_width, bottom_ypos);
        draw_list->AddImage(texture, top_left, bottom_right);
    }
}

gui::Background& gui::Background::set_left(const ImVec4& color)
{
    m_left = color;

    return self();
}

gui::Background& gui::Background::set_right(const ImVec4& color)
{
    m_right = color;

    return self();
}

gui::Background& gui::Background::force_scene(bool force)
{
    m_force_scene = force;

    return self();
}

void gui::Background::layout_dim(ImVec2 window_pos, ImVec2 window_size)
{
    auto draw_list = ImGui::GetWindowDrawList();
    auto window_end = ImVec2(window_pos.x + window_size.x, window_pos.y + window_size.y);

    auto left = ImGui::GetColorU32(m_left);
    auto right = ImGui::GetColorU32(m_right);

    draw_list->AddRectFilledMultiColor(window_pos, window_end, left, right, right, left);
}

void gui::Background::init(void)
{
    for(std::size_t i = 0; i < s_textures.size(); ++i) {
        auto id = Identifier::from_parts(constant::BUILTIN_NAME_SPACE, LAYER_INFOS[i].file);

        s_textures[i] = res::load<Texture2D>(id, "textures/gui");

        if(s_textures[i] == nullptr) {
            msg::log_warning("{}: texture load failed", id.full_string());
            continue;
        }

        auto height = s_textures[i]->height;

        if(s_tile_height == 0) {
            s_tile_height = height;
        }
        else if(height != s_tile_height) {
            msg::log_warning("{}: wall height {} does not match {}", id.full_string(), height, s_tile_height);
        }
    }
}

void gui::Background::shutdown(void)
{
    s_textures.fill(nullptr);
    s_tile_height = 0;
}

void gui::Background::layout(void)
{
    auto& registry = globals::world->registry();

    auto window_pos = ImGui::GetWindowPos();
    auto window_size = ImGui::GetWindowSize();
    auto window_end = ImVec2(window_pos.x + window_size.x, window_pos.y + window_size.y);

    if(registry.valid(globals::player) && !m_force_scene) {
        layout_dim(window_pos, window_size);

        // The pause menu sits on top of the world
        // itself, so it needs a darker scrim than the menu does
        auto scrim_color = ImGui::GetColorU32(ImVec4(0.0f, 0.0f, 0.0f, 0.45f));
        auto draw_list = ImGui::GetWindowDrawList();

        draw_list->AddRectFilled(window_pos, window_end, scrim_color);
        return;
    }

    auto draw_list = ImGui::GetWindowDrawList();

    auto sky_top = ImGui::GetColorU32(SKY_TOP);
    auto sky_bottom = ImGui::GetColorU32(SKY_BOTTOM);

    draw_list->AddRectFilledMultiColor(window_pos, window_end, sky_top, sky_top, sky_bottom, sky_bottom);

    auto time = ImGui::GetTime();
    auto pointer = 0.0f;
    auto mouse_xpos = ImGui::GetIO().MousePos.x;

    if(std::isfinite(mouse_xpos) && window_size.x > 0.0f) {
        pointer = 2.0f * (mouse_xpos / window_size.x - 0.5f);
        pointer = std::clamp(pointer, -1.0f, 1.0f);
    }

    // Without a loaded wall there is nothing to scroll; the sky and the dim
    // still draw, so the menu stays readable
    if(s_tile_height > 0) {
        auto tile_reference = static_cast<float>(s_tile_height);
        auto scale = std::max(1.0f, std::ceil(window_size.y / tile_reference));
        auto ratio = window_size.y / tile_reference;

        for(std::size_t i = 0; i < s_textures.size(); ++i) {
            if(s_textures[i] == nullptr) {
                continue;
            }

            auto tile_width = scale * static_cast<float>(s_textures[i]->width);
            auto tile_height = scale * static_cast<float>(s_textures[i]->height);
            auto offset = time * LAYER_INFOS[i].scroll_speed * ratio - pointer * POINTER_SHIFT * LAYER_INFOS[i].pointer_factor * ratio;

            draw_wall(draw_list, window_pos, window_end, s_textures[i]->imgui, tile_width, tile_height, offset);
        }
    }

    layout_dim(window_pos, window_size);
}

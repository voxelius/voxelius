#include "client/pch.hh"

#include "client/gui/background.hh"

#include "client/constant.hh"
#include "client/globals.hh"

constexpr static float CELL_DIVISOR = 48.0f;

// The scene is authored against a 4:3 window: its width is taken from the
// height times this aspect, so a wider window shows more of the same sky
// instead of stretching it
constexpr static float REFERENCE_ASPECT = static_cast<float>(constant::BASE_WIDTH) / static_cast<float>(constant::BASE_HEIGHT);

struct HillLayer final {
    float base;
    float amplitude;
    float frequency;
    float speed;
    ImVec4 color;
    bool cap;
};

struct Cloud final {
    float x;
    float y;
    float width;
    float height;
    float speed;
};

static const ImVec4 SKY_TOP = ImVec4(0.204f, 0.373f, 0.588f, 1.000f);
static const ImVec4 SKY_BOTTOM = ImVec4(0.749f, 0.867f, 0.925f, 1.000f);

// Back to front. Base and amplitude are fractions of the window height,
// frequency is in radians per cell and speed is in cells per second, so
// the silhouette keeps its shape whatever the window size is
static const HillLayer HILL_LAYERS[] = {
    { 0.58f, 0.10f, 0.160f, 0.230f, ImVec4(0.588f, 0.690f, 0.745f, 1.0f), false },
    { 0.66f, 0.11f, 0.230f, 0.380f, ImVec4(0.455f, 0.569f, 0.471f, 1.0f), true },
    { 0.74f, 0.10f, 0.340f, 0.620f, ImVec4(0.298f, 0.424f, 0.322f, 1.0f), true },
    { 0.82f, 0.08f, 0.440f, 0.920f, ImVec4(0.220f, 0.329f, 0.251f, 1.0f), true },
};

// Widths are fractions of the height times REFERENCE_ASPECT, heights and
// positions are fractions of the window so the clouds stay clouds
static const Cloud CLOUDS[] = {
    { 0.05f, 0.09f, 0.11f, 0.018f, 6.0f },
    { 0.38f, 0.15f, 0.07f, 0.012f, 4.0f },
    { 0.62f, 0.06f, 0.14f, 0.022f, 8.0f },
    { 0.81f, 0.20f, 0.06f, 0.010f, 5.0f },
    { 0.21f, 0.24f, 0.09f, 0.014f, 7.0f },
};

static ImVec4 lighten(const ImVec4& color, float factor)
{
    return ImVec4(std::min(1.0f, factor * color.x), std::min(1.0f, factor * color.y), std::min(1.0f, factor * color.z), color.w);
}

static void draw_sun(ImDrawList* draw_list, ImVec2 origin, ImVec2 size)
{
    auto center = ImVec2(origin.x + 0.78f * size.x, origin.y + 0.14f * size.y);
    auto radius = std::max(4.0f, 0.020f * size.y);

    for(int i = 3; i >= 1; --i) {
        auto halo = radius * (1.0f + 0.5f * static_cast<float>(i));
        draw_list->AddCircleFilled(center, halo, ImGui::GetColorU32(ImVec4(1.000f, 0.973f, 0.827f, 0.055f)));
    }

    draw_list->AddCircleFilled(center, radius, ImGui::GetColorU32(ImVec4(1.000f, 0.988f, 0.910f, 1.000f)));
}

static void draw_clouds(ImDrawList* draw_list, ImVec2 origin, ImVec2 size, float time)
{
    for(const auto& cloud : CLOUDS) {
        auto width = std::max(2.0f, cloud.width * REFERENCE_ASPECT * size.y);
        auto height = std::max(2.0f, cloud.height * size.y);

        auto travel = size.x + width;
        auto x = std::fmod(cloud.x * size.x + time * cloud.speed, travel) - width;
        auto y = cloud.y * size.y;

        auto body_min = ImVec2(origin.x + x, origin.y + y);
        auto body_max = ImVec2(body_min.x + width, body_min.y + height);

        draw_list->AddRectFilled(body_min, body_max, ImGui::GetColorU32(ImVec4(1.000f, 1.000f, 1.000f, 0.780f)));

        auto top_min = ImVec2(body_min.x + 0.25f * width, body_min.y - 0.55f * height);
        auto top_max = ImVec2(body_max.x - 0.25f * width, body_min.y + 0.10f * height);

        draw_list->AddRectFilled(top_min, top_max, ImGui::GetColorU32(ImVec4(1.000f, 1.000f, 1.000f, 0.900f)));
    }
}

static void draw_hills(ImDrawList* draw_list, ImVec2 origin, ImVec2 size, float time)
{
    // The step size follows the window height so the hills stay blocky
    // instead of turning into a hairline staircase on a big display
    auto cell = std::max(2.0f, std::floor(size.y / CELL_DIVISOR));
    auto cap_height = std::max(1.0f, std::floor(0.25f * cell));

    for(const auto& layer : HILL_LAYERS) {
        auto base = layer.base * size.y;
        auto amplitude = layer.amplitude * size.y;

        // Sampling in cells ties the horizontal wavelength to the height,
        // the same way the amplitude already is
        auto offset = time * layer.speed;

        auto cap_color = ImGui::GetColorU32(lighten(layer.color, 1.18f));
        auto body_color = ImGui::GetColorU32(layer.color);

        for(auto x = 0.0f; x < size.x; x += cell) {
            auto sample = x / cell + offset;

            auto y = base;
            y += amplitude * std::sin(sample * layer.frequency);
            y += 0.45f * amplitude * std::sin(sample * layer.frequency * 2.7f + 1.7f);
            y = std::floor(y / cell) * cell;

            auto block_min = ImVec2(origin.x + x, origin.y + y);
            auto block_max = ImVec2(origin.x + std::min(x + cell, size.x), origin.y + size.y);

            draw_list->AddRectFilled(block_min, block_max, body_color);

            if(layer.cap) {
                draw_list->AddRectFilled(block_min, ImVec2(block_max.x, block_min.y + cap_height), cap_color);
            }
        }
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

gui::Background& gui::Background::set_texture(ImTextureID texture)
{
    m_texture = texture;

    return self();
}

gui::Background& gui::Background::enable_scene(void)
{
    m_scene = true;

    return self();
}

void gui::Background::layout_scene(ImVec2 window_pos, ImVec2 window_size)
{
    auto draw_list = ImGui::GetWindowDrawList();
    auto time = static_cast<float>(ImGui::GetTime());

    auto window_end = ImVec2(window_pos.x + window_size.x, window_pos.y + window_size.y);

    draw_list->AddRectFilledMultiColor(window_pos, window_end, ImGui::GetColorU32(SKY_TOP), ImGui::GetColorU32(SKY_TOP),
        ImGui::GetColorU32(SKY_BOTTOM), ImGui::GetColorU32(SKY_BOTTOM));

    draw_sun(draw_list, window_pos, window_size);
    draw_clouds(draw_list, window_pos, window_size, time);
    draw_hills(draw_list, window_pos, window_size, time);

    layout_dim(window_pos, window_size);
}

void gui::Background::layout_dim(ImVec2 window_pos, ImVec2 window_size)
{
    auto draw_list = ImGui::GetWindowDrawList();

    auto left = ImGui::GetColorU32(m_left);
    auto right = ImGui::GetColorU32(m_right);

    auto window_end = ImVec2(window_pos.x + window_size.x, window_pos.y + window_size.y);

    draw_list->AddRectFilledMultiColor(window_pos, window_end, left, right, right, left);
}

void gui::Background::layout(void)
{
    auto window_pos = ImGui::GetWindowPos();
    auto window_size = ImGui::GetWindowSize();

    if(globals::registry.valid(globals::player)) {
        layout_dim(window_pos, window_size);

        // The pause menu sits on top of the world itself, so it needs a
        // darker scrim than the menu scene does
        auto window_end = ImVec2(window_pos.x + window_size.x, window_pos.y + window_size.y);
        ImGui::GetWindowDrawList()->AddRectFilled(window_pos, window_end, ImGui::GetColorU32(ImVec4(0.0f, 0.0f, 0.0f, 0.45f)));
        return;
    }

    if(m_scene) {
        layout_scene(window_pos, window_size);
        return;
    }

    if(m_texture) {
        auto window_end = ImVec2(window_pos.x + window_size.x, window_pos.y + window_size.y);
        ImGui::GetWindowDrawList()->AddImage(m_texture, window_pos, window_end);
        return;
    }

    layout_dim(window_pos, window_size);
}

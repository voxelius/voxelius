#include "client/pch.hh"

#include "client/boot.hh"

#include "core/cmdline.hh"
#include "core/identifier.hh"
#include "core/msg/log.hh"
#include "core/res/resource.hh"

#include "shared/constant.hh"

#include "client/fonts.hh"
#include "client/globals.hh"
#include "client/gui/widget.hh"
#include "client/head.hh"
#include "client/language.hh"
#include "client/res/texture_2d.hh"
#include "client/style.hh"
#include "client/video.hh"

constexpr static std::chrono::milliseconds FRAME_BUDGET { 6 };
constexpr static std::chrono::milliseconds CARD_FADE_IN { 300 };
constexpr static std::chrono::milliseconds HOLD_DURATION { 1200 };
constexpr static std::chrono::milliseconds FADE_OUT_DURATION { 300 };
constexpr static std::chrono::milliseconds MENU_FADE_DURATION { 300 };

constexpr static std::size_t LOG_CAPACITY = 5;

constexpr static float LOG_COLUMN_GAP = 8.0f;
constexpr static float PROGRESS_SMOOTHING = 12.0f;

constexpr static float CARD_WIDTH_RATIO = 0.60f;
constexpr static float CARD_HEIGHT_RATIO = 0.70f;
constexpr static float CARD_TOP_RATIO = 0.44f;
constexpr static float STATUS_TOP_RATIO = 0.70f;
constexpr static float BAR_WIDTH_RATIO = 0.36f;
constexpr static float BAR_HEIGHT = 8.0f;
constexpr static float EDGE_PADDING = 4.0f;
constexpr static float LINE_GAP = 4.0f;

constexpr static std::string_view SPINNER_FRAMES[] = { "|", "/", "-", "\\" };

constexpr static std::string_view READY_KEY = "gui.boot.stage.ready";
constexpr static std::string_view HINT_KEY = "gui.boot.hint";

constexpr static ImGuiWindowFlags BOOT_WINDOW_FLAGS = ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration
    | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoSavedSettings;

enum class boot_state {
    IDLE,
    LOADING,
    HOLDING,
    FADING_OUT,
    MENU_FADE_IN,
    DONE,
};

struct TextureChoice final {
    std::string_view name;
    std::string_view value;
    std::uint16_t from; // MMDD, zero means the texture is always a candidate
    std::uint16_t to;
};

struct Stage final {
    std::string_view name;
    std::string_view key;
    float weight;
    std::function<bool(void)> step;
};

struct LogLine final {
    msg::level_type level;
    std::string stage;
    std::string text;
};

constexpr static std::array TEXTURES = {
    TextureChoice { "main", "client_splash.png", 0, 0 },
};

static const ImVec4 COLOR_INFO { 1.000f, 1.000f, 1.000f, 1.000f };
static const ImVec4 COLOR_WARNING { 1.000f, 0.900f, 0.400f, 1.000f };
static const ImVec4 COLOR_ERROR { 1.000f, 0.400f, 0.350f, 1.000f };

static std::vector<Stage> s_stages;
static std::size_t s_stage_index;
static float s_completed_weight;
static float s_total_weight;
static float s_stage_progress;
static float s_display_progress;

static boot_state s_state = boot_state::IDLE;
static std::chrono::steady_clock::time_point s_start_time;
static std::chrono::steady_clock::time_point s_state_start;

static res::handle<Texture2D> s_texture;
static bool s_visible = true;
static bool s_skip_requested;
static bool s_quit_requested;
static bool s_sink_attached;

static std::chrono::milliseconds s_stage_delay;
static std::chrono::steady_clock::time_point s_next_stage_at;
static bool s_step_mode;
static bool s_waiting_for_step;

static std::deque<LogLine> s_log;
static std::string s_stage_name;

static bool within_range(std::uint16_t day, std::uint16_t from, std::uint16_t to)
{
    if(from <= to) {
        return day >= from && day <= to;
    }
    else {
        return day >= from || day <= to;
    }
}

static std::chrono::milliseconds parse_stage_delay(void)
{
    auto value = cmdline::value("bootdelay");

    if(!value.has_value()) {
        return {};
    }

    unsigned milliseconds;

    auto result = std::from_chars(value->data(), value->data() + value->size(), milliseconds);

    if(result.ec != std::errc {}) {
        msg::log_warning("{}: bad boot delay, expected milliseconds", value.value());
        return {};
    }

    return std::chrono::milliseconds(milliseconds);
}

static std::size_t choose_texture_index(void)
{
    if(auto forced = cmdline::value("bootsplash")) {
        for(std::size_t index = 0; index < TEXTURES.size(); index += 1) {
            if(TEXTURES[index].name == forced.value()) {
                return index;
            }
        }

        msg::log_warning("{}: unknown splash texture", forced.value());
    }

    std::uint16_t day = 0;
    std::array<std::size_t, TEXTURES.size()> candidates = {};
    std::size_t num_candidates = 0;

    SDL_Time now;
    SDL_DateTime date {};

    if(SDL_GetCurrentTime(&now) && SDL_TimeToDateTime(now, &date, true)) {
        day = static_cast<std::uint16_t>(date.month * 100 + date.day);
    }

    // A dated entry wins over the plain ones, a plain entry wins over all
    for(std::size_t index = 0; index < TEXTURES.size(); index += 1) {
        auto& entry = TEXTURES[index];

        if(entry.from && within_range(day, entry.from, entry.to)) {
            candidates[num_candidates] = index;
            num_candidates += 1;
        }
    }

    if(num_candidates == 0) {
        for(std::size_t index = 0; index < TEXTURES.size(); index += 1) {
            if(TEXTURES[index].from == 0) {
                candidates[num_candidates] = index;
                num_candidates += 1;
            }
        }
    }

    if(num_candidates == 0) {
        for(std::size_t index = 0; index < TEXTURES.size(); index += 1) {
            candidates[num_candidates] = index;
            num_candidates += 1;
        }
    }

    std::mt19937_64 random(std::random_device {}());
    std::uniform_int_distribution<std::size_t> dist(0, num_candidates - 1);
    return candidates.at(dist(random));
}

static res::handle<Texture2D> load_texture(void)
{
    auto& entry = TEXTURES.at(choose_texture_index());
    auto id = Identifier::from_parts(constant::BUILTIN_NAME_SPACE, entry.value);
    auto texture = res::load<Texture2D>(id, "textures/gui", RESFLAG_TEX_NEAREST);

    if(texture == nullptr) {
        msg::log_warning("{}: boot texture failed to load", id.full_string());
    }

    return texture;
}

static void on_log(const msg::Record& record)
{
    if(record.level < msg::LEVEL_INFO) {
        return;
    }

    if(s_log.size() >= LOG_CAPACITY) {
        s_log.pop_front();
    }

    LogLine line {};
    line.level = record.level;
    line.stage = s_stage_name;
    line.text = std::string(record.message);

    s_log.push_back(std::move(line));
}

static float target_progress(void)
{
    if(s_total_weight <= 0.0f) {
        return 0.0f;
    }

    auto current_weight = 0.0f;

    if(s_stage_index < s_stages.size()) {
        current_weight = s_stages[s_stage_index].weight * s_stage_progress;
    }

    return (s_completed_weight + current_weight) / s_total_weight;
}

static void advance(void)
{
    if(s_quit_requested) {
        s_waiting_for_step = false;
    }

    if(s_waiting_for_step) {
        return;
    }

    auto now = std::chrono::steady_clock::now();

    if(s_stage_delay.count() && !s_quit_requested && now < s_next_stage_at) {
        return;
    }

    auto deadline = now + FRAME_BUDGET;

    while(s_stage_index < s_stages.size()) {
        auto& stage = s_stages[s_stage_index];
        s_stage_name = std::string(stage.name);
        s_stage_progress = 0.0f;

        auto stage_start = std::chrono::steady_clock::now();
        auto done = stage.step();
        auto elapsed = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - stage_start).count();

        if(!done) {
            break;
        }

        msg::log_info("done in {:.1f} ms", elapsed);

        s_completed_weight += stage.weight;
        s_stage_index += 1;

        if(s_step_mode && !s_quit_requested) {
            s_waiting_for_step = true;
            break;
        }

        if(s_stage_delay.count() && !s_quit_requested) {
            s_next_stage_at = std::chrono::steady_clock::now() + s_stage_delay;
            break;
        }

        if(std::chrono::steady_clock::now() >= deadline) {
            break;
        }
    }

    if(s_stage_index == s_stages.size() && !s_waiting_for_step) {
        s_stage_name.clear();
        s_display_progress = 1.0f;
        s_state_start = std::chrono::steady_clock::now();

        if(s_visible && !s_quit_requested) {
            s_state = boot_state::HOLDING;
        }
        else {
            s_state = boot_state::DONE;
        }
    }
}

static float overlay_alpha(void)
{
    auto now = std::chrono::steady_clock::now();

    if(s_state == boot_state::LOADING || s_state == boot_state::HOLDING) {
        auto duration = std::chrono::duration<double>(CARD_FADE_IN).count();
        auto elapsed = std::chrono::duration<double>(now - s_start_time).count();
        auto value = static_cast<float>(elapsed / duration);

        return std::clamp(value, 0.0f, 1.0f);
    }

    if(s_state == boot_state::FADING_OUT) {
        auto duration = std::chrono::duration<double>(FADE_OUT_DURATION).count();
        auto elapsed = std::chrono::duration<double>(now - s_state_start).count();
        auto value = 1.0f - static_cast<float>(elapsed / duration);

        return std::clamp(value, 0.0f, 1.0f);
    }

    return 0.0f;
}

static const ImVec4& level_color(msg::level_type level)
{
    switch(level) {
        case msg::LEVEL_WARNING:
            return COLOR_WARNING;

        case msg::LEVEL_ERROR:
        case msg::LEVEL_CRITICAL:
            return COLOR_ERROR;

        default:
            break;
    }

    return COLOR_INFO;
}

static std::string truncate_text(std::string_view text, float max_width)
{
    auto result = std::string(text);

    if(ImGui::CalcTextSize(result.c_str()).x <= max_width) {
        return result;
    }

    auto probe = std::string();

    while(result.size()) {
        probe = std::format("{}...", result);

        if(ImGui::CalcTextSize(probe.c_str()).x <= max_width) {
            return probe;
        }

        result.pop_back();
    }

    return std::string("...");
}

static ImVec4 with_alpha(const ImVec4& color, float alpha)
{
    return ImVec4(color.x, color.y, color.z, color.w * alpha);
}

static void draw_card(ImVec2 display, float alpha)
{
    if(s_texture == nullptr) {
        return;
    }

    auto texture_width = static_cast<float>(s_texture->width);
    auto texture_height = static_cast<float>(s_texture->height);

    auto width_scale = static_cast<float>(std::round(CARD_WIDTH_RATIO * display.x / texture_width));
    auto height_scale = static_cast<float>(std::floor(CARD_HEIGHT_RATIO * display.y / texture_height));
    auto scale = std::max(1.0f, std::min(width_scale, height_scale));

    auto size = ImVec2(texture_width * scale, texture_height * scale);
    auto position = ImVec2(std::round(0.5f * (display.x - size.x)), std::round(CARD_TOP_RATIO * display.y - 0.5f * size.y));

    auto uv0 = ImVec2(0.0f, 0.0f);
    auto uv1 = ImVec2(1.0f, 1.0f);
    auto background = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    auto tint = ImVec4(1.0f, 1.0f, 1.0f, alpha);

    auto& platform_io = ImGui::GetPlatformIO();
    auto draw_list = ImGui::GetWindowDrawList();

    if(platform_io.DrawCallback_SetSamplerNearest) {
        draw_list->AddCallback(platform_io.DrawCallback_SetSamplerNearest, nullptr);
    }

    ImGui::SetCursorScreenPos(position);
    ImGui::ImageWithBg(s_texture->imgui, size, uv0, uv1, background, tint);

    if(platform_io.DrawCallback_SetSamplerLinear) {
        draw_list->AddCallback(platform_io.DrawCallback_SetSamplerLinear, nullptr);
    }
}

static void draw_status(ImVec2 display, float alpha)
{
    auto key = READY_KEY;

    if(s_state == boot_state::LOADING && s_stage_index < s_stages.size()) {
        key = s_stages[s_stage_index].key;
    }

    auto text = language::resolve(key);
    auto text_begin = text.data();
    auto text_end = text.data() + text.size();

    ImGui::PushFont(fonts::unscii8, fonts::unscii8->LegacySize);

    auto text_size = ImGui::CalcTextSize(text_begin, text_end);
    auto position = ImVec2(0.5f * (display.x - text_size.x), STATUS_TOP_RATIO * display.y - text_size.y);
    auto& palette = style::palette();

    ImGui::SetCursorScreenPos(position);
    ImGui::PushStyleColor(ImGuiCol_Text, with_alpha(palette.text, alpha));
    ImGui::TextUnformatted(text_begin, text_end);
    ImGui::PopStyleColor();

    ImGui::PopFont();
}

static void draw_bar(ImVec2 display, float alpha)
{
    auto scale = static_cast<float>(globals::gui_scale);
    auto width = BAR_WIDTH_RATIO * display.x;
    auto height = BAR_HEIGHT * scale;
    auto position = ImVec2(0.5f * (display.x - width), STATUS_TOP_RATIO * display.y + LINE_GAP * scale);

    ImGui::SetCursorScreenPos(position);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
    ImGui::ProgressBar(s_display_progress, ImVec2(width, height), "");
    ImGui::PopStyleVar();
}

static void draw_log(ImVec2 display, float alpha)
{
    if(s_log.empty()) {
        return;
    }

    auto scale = static_cast<float>(globals::gui_scale);
    auto font_scale = std::max(1.0f, ImGui::GetStyle().FontScaleMain);
    auto font_steps = std::ceil(font_scale / 2.0f);

    ImGui::PushFont(fonts::unscii8, fonts::unscii8->LegacySize * font_steps / font_scale);

    auto line_height = ImGui::GetTextLineHeight();

    auto left = EDGE_PADDING * scale;
    auto top = EDGE_PADDING * scale;

    auto column = 0.0f;

    for(auto& line : s_log) {
        auto label = std::format("boot: {}", line.stage);
        column = std::max(column, ImGui::CalcTextSize(label.c_str()).x);
    }

    column += LOG_COLUMN_GAP * font_steps;

    auto right = display.x - EDGE_PADDING * scale;
    auto message_width = right - left - column;
    auto index = std::size_t(0);

    for(auto& line : s_log) {
        auto y = top + line_height * static_cast<float>(index);
        auto newest = index + 1 == s_log.size();

        auto label = std::format("boot: {}", line.stage);
        auto message = truncate_text(line.text, message_width);

        auto color = level_color(line.level);

        if(!newest) {
            color = style::palette().text_disabled;
        }

        auto packed = with_alpha(color, alpha);

        ImGui::SetCursorScreenPos(ImVec2(left, y));
        ImGui::PushStyleColor(ImGuiCol_Text, packed);
        ImGui::TextUnformatted(label.c_str());
        ImGui::PopStyleColor();

        ImGui::SetCursorScreenPos(ImVec2(left + column, y));
        ImGui::PushStyleColor(ImGuiCol_Text, packed);
        ImGui::TextUnformatted(message.c_str());
        ImGui::PopStyleColor();

        index += 1;
    }

    ImGui::PopFont();
}

static void draw_hint(ImVec2 display, float alpha)
{
    auto can_skip = s_state == boot_state::HOLDING || s_state == boot_state::FADING_OUT;

    if(!can_skip && !s_waiting_for_step) {
        return;
    }

    auto text = language::resolve(HINT_KEY);
    auto text_begin = text.data();
    auto text_end = text.data() + text.size();

    auto scale = static_cast<float>(globals::gui_scale);

    ImGui::PushFont(fonts::unscii8, fonts::unscii8->LegacySize);

    auto text_size = ImGui::CalcTextSize(text_begin, text_end);
    auto position = ImVec2(display.x - EDGE_PADDING * scale - text_size.x, display.y - EDGE_PADDING * scale - text_size.y);
    auto& palette = style::palette();

    ImGui::SetCursorScreenPos(position);
    ImGui::PushStyleColor(ImGuiCol_Text, with_alpha(palette.text_disabled, alpha));
    ImGui::TextUnformatted(text_begin, text_end);
    ImGui::PopStyleColor();

    ImGui::PopFont();
}

static void draw_spinner(ImVec2 display, float alpha)
{
    if(s_state == boot_state::LOADING) {
        auto scale = static_cast<float>(globals::gui_scale);
        auto index = static_cast<std::size_t>(ImGui::GetTime() * 8.0) % std::size(SPINNER_FRAMES);
        auto frame = SPINNER_FRAMES[index];

        ImGui::PushFont(fonts::unscii8, fonts::unscii8->LegacySize);

        auto size = ImGui::CalcTextSize(frame.data(), frame.data() + frame.size());
        auto bar_left = 0.5f * (display.x - BAR_WIDTH_RATIO * display.x);
        auto bar_top = STATUS_TOP_RATIO * display.y + LINE_GAP * scale;
        auto bar_height = BAR_HEIGHT * scale;

        auto position = ImVec2(bar_left - LINE_GAP * scale - size.x, bar_top + 0.5f * (bar_height - size.y));

        ImGui::SetCursorScreenPos(position);
        ImGui::PushStyleColor(ImGuiCol_Text, with_alpha(style::palette().text, alpha));
        ImGui::TextUnformatted(frame.data(), frame.data() + frame.size());
        ImGui::PopStyleColor();

        ImGui::PopFont();
    }
}

static void draw_overlay(void)
{
    auto& io = ImGui::GetIO();
    auto display = io.DisplaySize;
    auto alpha = overlay_alpha();

    if(alpha <= 0.0f) {
        return;
    }

    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(display);

    if(ImGui::Begin("###boot", nullptr, BOOT_WINDOW_FLAGS)) {
        draw_card(display, alpha);
        draw_status(display, alpha);
        draw_bar(display, alpha);
        draw_spinner(display, alpha);
        draw_log(display, alpha);
        draw_hint(display, alpha);
    }

    ImGui::End();
}

void boot::init(void)
{
    s_visible = !cmdline::contains("noboot");

    if(s_visible) {
        s_stage_delay = parse_stage_delay();
        s_step_mode = cmdline::contains("bootstep");

        s_texture = load_texture();

        msg::add_sink<&on_log>();
        s_sink_attached = true;
    }

    SDL_GL_SetSwapInterval(1);

    int width, height;
    SDL_GetWindowSizeInPixels(globals::window, &width, &height);
    glViewport(0, 0, width, height);

    // Show a black frame first so the window never flashes an unpainted client area
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    SDL_GL_SwapWindow(globals::window);
    SDL_ShowWindow(globals::window);
}

void boot::start(void)
{
    s_start_time = std::chrono::steady_clock::now();
    s_state_start = s_start_time;
    s_state = boot_state::LOADING;

    if(s_visible) {
        msg::log_info("{} stages", s_stages.size());
    }
}

void boot::shutdown(void)
{
    if(s_sink_attached) {
        msg::remove_sink<&on_log>();
        s_sink_attached = false;
    }

    s_texture.reset();
    s_stages.clear();
    s_log.clear();
    s_state = boot_state::DONE;
}

void boot::update(void)
{
    if(s_state == boot_state::IDLE || s_state == boot_state::DONE) {
        return;
    }

    video::update();

    if(s_state == boot_state::LOADING) {
        advance();

        auto target = target_progress();
        auto blend = 1.0f - std::exp(-PROGRESS_SMOOTHING * globals::window_frametime);
        s_display_progress += (target - s_display_progress) * blend;

        return;
    }

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - s_state_start).count();

    if(s_state == boot_state::HOLDING) {
        if(s_skip_requested || elapsed >= HOLD_DURATION.count()) {
            s_state = boot_state::FADING_OUT;
            s_state_start = now;
        }

        return;
    }

    if(s_state == boot_state::FADING_OUT) {
        if(elapsed >= FADE_OUT_DURATION.count()) {
            s_state = boot_state::MENU_FADE_IN;
            s_state_start = now;
        }

        return;
    }

    if(s_state == boot_state::MENU_FADE_IN && elapsed >= MENU_FADE_DURATION.count()) {
        s_state = boot_state::DONE;
    }
}

void boot::render(void)
{
    ZoneScopedN("boot::render");

    head::detail::begin_frame();

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    gui::update_scale();

    if(s_visible) {
        draw_overlay();
    }

    head::present();
}

void boot::render_fade(void)
{
    if(s_state != boot_state::MENU_FADE_IN) {
        return;
    }

    auto duration = std::chrono::duration<double>(MENU_FADE_DURATION).count();
    auto elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - s_state_start).count();
    auto alpha = std::clamp(1.0f - static_cast<float>(elapsed / duration), 0.0f, 1.0f);

    auto display = ImGui::GetIO().DisplaySize;
    auto color = ImGui::GetColorU32(ImVec4(0.0f, 0.0f, 0.0f, alpha));
    auto draw_list = ImGui::GetForegroundDrawList();

    draw_list->AddRectFilled(ImVec2(0.0f, 0.0f), display, color);
}

void boot::add(std::string_view name, std::string_view key, float weight, std::function<bool(void)> step)
{
    Stage stage {};
    stage.name = name;
    stage.key = key;
    stage.weight = weight;
    stage.step = std::move(step);

    s_stages.push_back(std::move(stage));
    s_total_weight += weight;
}

void boot::set_progress(float progress)
{
    s_stage_progress = std::clamp(progress, 0.0f, 1.0f);
}

void boot::request_quit(void)
{
    s_quit_requested = true;
    s_skip_requested = true;
}

bool boot::quit_requested(void)
{
    return s_quit_requested;
}

bool boot::active(void)
{
    return s_state == boot_state::LOADING || s_state == boot_state::HOLDING || s_state == boot_state::FADING_OUT;
}

static bool is_skip_event(std::uint32_t type)
{
    switch(type) {
        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP:
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP:
        case SDL_EVENT_MOUSE_WHEEL:
            return true;

        default:
            break;
    }

    return false;
}

bool boot::input(const SDL_Event& event)
{
    if(!active()) {
        return false;
    }

    if(!is_skip_event(event.type)) {
        return false;
    }

    if(s_state == boot_state::HOLDING) {
        s_skip_requested = true;
    }
    else if(s_waiting_for_step) {
        s_waiting_for_step = false;
    }

    return true;
}

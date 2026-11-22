#include "client/pch.hh"

#include "client/video.hh"

#include "core/cmdline.hh"
#include "core/cvar/boolean.hh"
#include "core/cvar/host.hh"
#include "core/cvar/string.hh"
#include "core/exception.hh"
#include "core/version.hh"

#include "shared/splash.hh"

#include "client/constant.hh"
#include "client/globals.hh"
#include "client/gui/checkbox.hh"
#include "client/gui/container.hh"
#include "client/gui/video_selector.hh"
#include "client/settings.hh"

constexpr static std::chrono::milliseconds MODE_TIMEOUT { 5000 };
constexpr static std::chrono::milliseconds MODE_SETTLE { 5000 };

static Eigen::Vector2i s_last_windowed_size;
static cvar::Boolean s_enable_vsync { true };
static cvar::String s_current_mode { "windowed" };

static gui::CheckBox s_enable_vsync_gui;
static gui::VideoSelector s_video_mode_control;

static SDL_DisplayID s_display;
static SDL_GPUPresentMode s_unlocked_present_mode;
static std::vector<SDL_DisplayMode> s_fullscreen_modes;

bool video::mode_pending;
static std::chrono::steady_clock::time_point s_mode_deadline;
static std::optional<std::chrono::steady_clock::time_point> s_mode_settle_at;

static void mode_requested(void)
{
    video::mode_pending = true;
    s_mode_deadline = MODE_TIMEOUT + std::chrono::steady_clock::now();
    s_mode_settle_at.reset();

    msg::log_debug("waiting for the window mode");
}

static void mode_applied(void)
{
    if(video::mode_pending) {
        s_mode_settle_at = MODE_SETTLE + std::chrono::steady_clock::now();
    }
}

static void on_sdl_window_event(const SDL_WindowEvent& event)
{
    if(event.windowID == SDL_GetWindowID(globals::window)) {
        switch(event.type) {
            case SDL_EVENT_WINDOW_RESIZED:
            case SDL_EVENT_WINDOW_ENTER_FULLSCREEN:
            case SDL_EVENT_WINDOW_LEAVE_FULLSCREEN:
            case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
                mode_applied();
                break;

            default:
                break;
        }

        if(event.type == SDL_EVENT_WINDOW_RESIZED) {
            globals::width = event.data1;
            globals::height = event.data2;
            globals::aspect = static_cast<float>(event.data1) / static_cast<float>(event.data2);

            if(SDL_GetWindowFullscreenMode(globals::window) == nullptr) {
                s_last_windowed_size.x() = event.data1;
                s_last_windowed_size.y() = event.data2;
            }
        }
    }
}

static void pick_display(void)
{
    s_display = SDL_GetPrimaryDisplay();

    auto monitor_arg = cmdline::value("monitor");

    if(!monitor_arg.has_value()) {
        return;
    }

    unsigned monitor_index;
    auto check = std::from_chars(monitor_arg->data(), monitor_arg->data() + monitor_arg->size(), monitor_index);

    if(check.ec == std::errc {}) {
        int display_count;
        auto displays = SDL_GetDisplays(&display_count);

        if(monitor_index < static_cast<unsigned>(display_count)) {
            s_display = displays[monitor_index];
        }

        SDL_free(displays);
    }
}

static void cache_fullscreen_modes(void)
{
    int mode_count;
    auto modes = SDL_GetFullscreenDisplayModes(s_display, &mode_count);

    s_fullscreen_modes.clear();
    s_fullscreen_modes.reserve(mode_count);

    for(int i = 0; i < mode_count; ++i) {
        const auto& mode = *modes[i];

        // Only allow video modes the window is allowed to shrink to,
        // otherwise the UI has nowhere sane to scale down to
        if(mode.w >= constant::MIN_WIDTH && mode.h >= constant::MIN_HEIGHT) {
            s_fullscreen_modes.push_back(mode);
        }
    }

    SDL_free(modes);

    std::sort(s_fullscreen_modes.begin(), s_fullscreen_modes.end(), [](const SDL_DisplayMode& a, const SDL_DisplayMode& b) {
        if(a.w == b.w && a.h == b.h)
            return a.refresh_rate < b.refresh_rate;
        if(a.w == b.w)
            return a.h < b.h;
        return a.w < b.w;
    });
}

static void update_present_mode(void)
{
    if(s_enable_vsync.value()) {
        SDL_GL_SetSwapInterval(1);
    }
    else {
        SDL_GL_SetSwapInterval(0);
    }
}

static void on_sdl_key(const SDL_KeyboardEvent& event)
{
    if(event.type == SDL_EVENT_KEY_DOWN) {
        if(event.key == SDLK_F1 || event.key == SDLK_F2) {
            s_enable_vsync.set_value(!s_enable_vsync.value());
        }
    }
}

void video::init(void)
{
    s_last_windowed_size = Eigen::Vector2i(constant::MIN_WIDTH, constant::MIN_HEIGHT);

    auto& r_vsync_binding = cvar::bind_var(s_enable_vsync, "r_vsync");
    r_vsync_binding.set_help("Enable vertical synchronization");
    r_vsync_binding.set_flags(FCVAR_ARCHIVE);
    auto vsync_changed = r_vsync_binding.changed();
    vsync_changed.connect<&update_present_mode>();

    s_enable_vsync_gui.enable_tooltip();
    s_enable_vsync_gui.bind("r_vsync");
    settings::video.add_child(s_enable_vsync_gui, 1);

    auto& r_mode_binding = cvar::bind_var(s_current_mode, "r_mode");
    r_mode_binding.set_help("Video mode");
    r_mode_binding.set_flags(FCVAR_ARCHIVE);

    s_video_mode_control.on_change(&settings::detail::video_mode_popup);
    s_video_mode_control.set_key("r_mode");
    settings::video.add_child(s_video_mode_control, 0);

    auto video_ok = SDL_InitSubSystem(SDL_INIT_VIDEO);
    vx::throw_if_not_fmt(video_ok, "SDL_InitSubSystem for video failed: {}", SDL_GetError());

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);  // FIXME: use an actual framebuffer
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8); // FIXME: use an actual framebuffer

    pick_display();
    cache_fullscreen_modes();

    globals::window = SDL_CreateWindow("client", constant::MIN_WIDTH, constant::MIN_HEIGHT,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_OPENGL);
    vx::throw_if_not_fmt(globals::window, "SDL_CreateWindow failed: {}", SDL_GetError());

    SDL_SetWindowMinimumSize(globals::window, constant::MIN_WIDTH, constant::MIN_HEIGHT);

    globals::dispatcher.sink<SDL_WindowEvent>().connect<&on_sdl_window_event>();
    globals::dispatcher.sink<SDL_KeyboardEvent>().connect<&on_sdl_key>();

    update_window_title();
}

void video::init_late(void)
{
    update_present_mode();

    auto& mode = s_current_mode.string();

    if(mode == "windowed") {
        video::request_windowed(s_last_windowed_size.x(), s_last_windowed_size.y());
    }
    else {
        int target_width;
        int target_height;
        int target_rate;

        if(3 == std::sscanf(mode.c_str(), "%d:%d:%d", &target_width, &target_height, &target_rate)) {
            video::request_fullscreen(target_width, target_height, target_rate);
        }
    }

    int width;
    int height;
    SDL_GetWindowSizeInPixels(globals::window, &width, &height);

    globals::width = width;
    globals::height = height;
    globals::aspect = static_cast<float>(width) / static_cast<float>(height);
}

void video::shutdown(void)
{
    SDL_DestroyWindow(globals::window);
}

void video::update(void)
{
    if(mode_pending) {
        auto now = std::chrono::steady_clock::now();

        if(s_mode_settle_at.has_value()) {
            if(now >= s_mode_settle_at.value()) {
                mode_pending = false;
                s_mode_settle_at.reset();

                msg::log_debug("window mode settled");
            }

            return;
        }

        if(now >= s_mode_deadline) {
            mode_pending = false;

            msg::log_debug("the window mode change is not reported, moving on");
        }
    }
}

void video::update_late(void)
{
    // empty
}

void video::query_current_mode(int& width, int& height)
{
    width = globals::width;
    height = globals::height;
}

void video::query_current_mode(int& width, int& height, bool& fullscreen)
{
    width = globals::width;
    height = globals::height;

    auto flags = SDL_GetWindowFlags(globals::window);
    fullscreen = SDL_WINDOW_FULLSCREEN == (flags & SDL_WINDOW_FULLSCREEN);
}

void video::query_current_mode(int& width, int& height, int& rate, bool& fullscreen)
{
    width = globals::width;
    height = globals::height;

    auto flags = SDL_GetWindowFlags(globals::window);
    fullscreen = SDL_WINDOW_FULLSCREEN == (flags & SDL_WINDOW_FULLSCREEN);

    rate = 0;

    if(fullscreen) {
        auto mode = SDL_GetWindowFullscreenMode(globals::window);

        if(mode) {
            rate = static_cast<int>(mode->refresh_rate);
        }
    }
}

const std::vector<SDL_DisplayMode>& video::query_fullscreen_modes(void)
{
    return s_fullscreen_modes;
}

void video::request_fullscreen(int width, int height, int rate)
{
    vx::throw_if(width < constant::MIN_WIDTH);
    vx::throw_if(height < constant::MIN_HEIGHT);

    std::size_t best_index = 0;
    auto best_score = std::numeric_limits<int>::max();

    for(std::size_t i = 0; i < s_fullscreen_modes.size(); ++i) {
        const auto& mode = s_fullscreen_modes[i];

        auto score_width = std::abs(mode.w - width);
        auto score_height = std::abs(mode.h - height);
        auto score_rate = std::abs(static_cast<int>(mode.refresh_rate) - rate);
        auto total_score = score_width + score_height + score_rate;

        if(total_score < best_score) {
            best_score = total_score;
            best_index = i;
        }
    }

    if(s_fullscreen_modes.empty()) {
        return;
    }

    const auto& best_mode = s_fullscreen_modes[best_index];

    auto flags = SDL_GetWindowFlags(globals::window);
    auto already_fullscreen = SDL_WINDOW_FULLSCREEN == (flags & SDL_WINDOW_FULLSCREEN);
    auto current = SDL_GetWindowFullscreenMode(globals::window);
    auto same_size = current != nullptr && current->w == best_mode.w && current->h == best_mode.h;
    auto same_mode = same_size && current->refresh_rate == best_mode.refresh_rate;

    SDL_SetWindowFullscreenMode(globals::window, &best_mode);
    SDL_SetWindowFullscreen(globals::window, true);

    if(!already_fullscreen || !same_mode) {
        mode_requested();
    }

    s_current_mode.set_value(std::format("{}:{}:{}", best_mode.w, best_mode.h, static_cast<int>(best_mode.refresh_rate)));

    msg::log_debug("set mode to: {}x{} ({} Hz)", best_mode.w, best_mode.h, static_cast<int>(best_mode.refresh_rate));
}

void video::request_windowed(int width, int height)
{
    auto flags = SDL_GetWindowFlags(globals::window);
    auto was_fullscreen = SDL_WINDOW_FULLSCREEN == (flags & SDL_WINDOW_FULLSCREEN);

    int current_width, current_height;
    SDL_GetWindowSize(globals::window, &current_width, &current_height);
    auto size_differs = current_width != width || current_height != height;

    SDL_SetWindowFullscreen(globals::window, false);
    SDL_SetWindowSize(globals::window, width, height);

    SDL_Rect usable_bounds {};
    SDL_GetDisplayUsableBounds(s_display, &usable_bounds);

    auto center_x = usable_bounds.x + (usable_bounds.w - width) / 2;
    auto center_y = usable_bounds.y + (usable_bounds.h - height) / 2;

    SDL_SetWindowPosition(globals::window, center_x, center_y);

    if(was_fullscreen || size_differs) {
        mode_requested();
    }

    s_current_mode.set_value("windowed");

    msg::log_debug("set mode to: windowed {}x{}", width, height);
}

void video::request_windowed(void)
{
    video::request_windowed(s_last_windowed_size.x(), s_last_windowed_size.y());
}

void video::update_window_title(void)
{
    auto title = std::format("Voxelius {}: {}", version::full, splash::get());

    SDL_SetWindowTitle(globals::window, title.c_str());
}

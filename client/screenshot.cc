#include "client/pch.hh"

#include "client/screenshot.hh"

#include "core/cvar/host.hh"
#include "core/msg/log.hh"
#include "core/threading.hh"
#include "core/utils/epoch.hh"
#include "core/utils/physfs.hh"

#include "client/globals.hh"
#include "client/gui/button_bind.hh"
#include "client/gui/container.hh"
#include "client/input/buttons.hh"
#include "client/input/source.hh"
#include "client/settings.hh"

constexpr static std::string_view SCREENSHOT_COMMAND { "screenshot" };
constexpr static std::string_view SCREENSHOT_DIR { "screenshots" };
constexpr static int PIXEL_CHANNELS = 4;

class ScreenshotTask final : public Task {
public:
    ScreenshotTask(std::string path, std::vector<std::byte> pixels, int width, int height);
    virtual ~ScreenshotTask(void) override = default;
    virtual void process(void) override;
    virtual void finalize(void) override;

private:
    std::string m_path;
    std::vector<std::byte> m_pixels;
    int m_width;
    int m_height;
    bool m_saved { false };
};

static bool s_requested;
static unsigned s_sequence;

static gui::ButtonBind s_screenshot_bind;

static void append_png(void* context, void* data, int size)
{
    auto begin = static_cast<const std::byte*>(data);
    auto buffer = static_cast<std::vector<std::byte>*>(context);
    buffer->insert(buffer->end(), begin, begin + size);
}

static std::string timestamp(void)
{
    SDL_Time now {};
    SDL_DateTime date {};

    if(!(SDL_GetCurrentTime(&now) && SDL_TimeToDateTime(now, &date, true))) {
        return std::format("{}", utils::epoch_seconds());
    }

    return std::format("{:04d}{:02d}{:02d}-{:02d}{:02d}{:02d}", date.year, date.month, date.day, date.hour, date.minute, date.second);
}

static std::string next_path(void)
{
    auto stamp = timestamp();
    std::string path;

    do {
        s_sequence += 1;
        path = std::format("{}/screenshot-{}-{:03d}.png", SCREENSHOT_DIR, stamp, s_sequence);
    } while(PHYSFS_exists(path.c_str()));

    return path;
}

static cvar::status cmd_screenshot(const cvar::Context&, std::span<const std::string_view>)
{
    screenshot::request();

    return cvar::status::OK;
}

ScreenshotTask::ScreenshotTask(std::string path, std::vector<std::byte> pixels, int width, int height)
    : m_path(std::move(path)), m_pixels(std::move(pixels)), m_width(width), m_height(height)
{
    // empty
}

void ScreenshotTask::process(void)
{
    ZoneScopedN("screenshot::save");

    std::vector<std::byte> png;
    auto stride = m_width * PIXEL_CHANNELS;
    auto encoded = stbi_write_png_to_func(&append_png, &png, m_width, m_height, PIXEL_CHANNELS, m_pixels.data(), stride);

    if(encoded == 0) {
        return;
    }

    m_saved = utils::write_file(m_path, std::span<const std::byte>(png));
}

void ScreenshotTask::finalize(void)
{
    if(m_saved) {
        msg::log_info("screenshot saved: {}", m_path);
    }
    else {
        msg::log_error("screenshot failed: {}", m_path);
    }
}

void screenshot::init(void)
{
    auto& screenshot_binding = cvar::bind_cmd(SCREENSHOT_COMMAND);
    screenshot_binding.set_help("Save a screenshot of the current frame");
    screenshot_binding.connect<&cmd_screenshot>();

    s_screenshot_bind.bind_command(SCREENSHOT_COMMAND, input_edge::DOWN);
    settings::controls_miscellaneous.add_child(s_screenshot_bind, 0);

    // The bind is the only record of the key, so the Controls tab, list_buttons
    // and client.conf carry it like any other bind; the client checks it first
    buttons::bind(ButtonSource(input_device::KEYBOARD, SDL_SCANCODE_F2, input_edge::DOWN), std::string(SCREENSHOT_COMMAND));

    // A window readback is bottom-up. The stb switch is process-wide, but this
    // module is the only writer through stb, so one value serves every task
    stbi_flip_vertically_on_write(true);
}

bool screenshot::handle_event(const SDL_Event& event)
{
    if(event.type != SDL_EVENT_KEY_DOWN) {
        return false;
    }

    if(event.key.repeat) {
        return false;
    }

    if(buttons::is_capturing) {
        return false;
    }

    auto code = static_cast<std::uint32_t>(event.key.scancode);

    for(const auto& source : buttons::find(SCREENSHOT_COMMAND)) {
        if(source.device() == input_device::KEYBOARD && source.edge() == input_edge::DOWN && source.code() == code) {
            request();
            return true;
        }
    }

    return false;
}

void screenshot::request(void)
{
    s_requested = true;
}

void screenshot::capture(void)
{
    if(s_requested) {
        s_requested = false;

        ZoneScopedN("screenshot::capture");

        int width, height;
        SDL_GetWindowSizeInPixels(globals::window, &width, &height);

        if(width <= 0 || height <= 0) {
            msg::log_warning("screenshot: the window has no drawable size");
            return;
        }

        auto dir_name = std::string(SCREENSHOT_DIR);

        if(PHYSFS_mkdir(dir_name.c_str()) == 0) {
            msg::log_warning("{}: {}", SCREENSHOT_DIR, utils::physfs_error());
        }

        auto path = next_path();
        auto pixels = std::vector<std::byte>(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * PIXEL_CHANNELS);

        // The composite pass leaves its own target bound for reading, and the
        // window back buffer holds the frame the player sees
        while(glGetError()) {
            // empty
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glReadBuffer(GL_BACK);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

        auto error = glGetError();

        if(error) {
            msg::log_warning("screenshot: the readback failed with error 0x{:04X}", error);
            return;
        }

        threading::submit<ScreenshotTask>(std::move(path), std::move(pixels), width, height);
    }
}

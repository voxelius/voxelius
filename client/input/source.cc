#include "client/pch.hh"

#include "client/input/source.hh"

#include "core/utils/string.hh"

struct NamedCode final {
    std::string_view name;
    std::uint32_t code;
};

constexpr static std::array MOUSE_BUTTONS = {
    NamedCode { "LF", SDL_BUTTON_LEFT },
    NamedCode { "MD", SDL_BUTTON_MIDDLE },
    NamedCode { "RT", SDL_BUTTON_RIGHT },
};

constexpr static std::array MWHEEL_BUTTONS = {
    NamedCode { "XUP", MWHEEL_XUP },
    NamedCode { "XDOWN", MWHEEL_XDOWN },
    NamedCode { "YUP", MWHEEL_YUP },
    NamedCode { "YDOWN", MWHEEL_YDOWN },
};

constexpr static std::array GAMEPAD_BUTTONS = {
    NamedCode { "A", SDL_GAMEPAD_BUTTON_SOUTH },
    NamedCode { "B", SDL_GAMEPAD_BUTTON_EAST },
    NamedCode { "X", SDL_GAMEPAD_BUTTON_WEST },
    NamedCode { "Y", SDL_GAMEPAD_BUTTON_NORTH },
    NamedCode { "BACK", SDL_GAMEPAD_BUTTON_BACK },
    NamedCode { "GUIDE", SDL_GAMEPAD_BUTTON_GUIDE },
    NamedCode { "START", SDL_GAMEPAD_BUTTON_START },
    NamedCode { "LS", SDL_GAMEPAD_BUTTON_LEFT_STICK },
    NamedCode { "RS", SDL_GAMEPAD_BUTTON_RIGHT_STICK },
    NamedCode { "LB", SDL_GAMEPAD_BUTTON_LEFT_SHOULDER },
    NamedCode { "RB", SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER },
    NamedCode { "DPAD_UP", SDL_GAMEPAD_BUTTON_DPAD_UP },
    NamedCode { "DPAD_DOWN", SDL_GAMEPAD_BUTTON_DPAD_DOWN },
    NamedCode { "DPAD_LEFT", SDL_GAMEPAD_BUTTON_DPAD_LEFT },
    NamedCode { "DPAD_RIGHT", SDL_GAMEPAD_BUTTON_DPAD_RIGHT },
};

constexpr static std::array MOUSE_AXES = {
    NamedCode { "X", MAXIS_X },
    NamedCode { "Y", MAXIS_Y },
};

constexpr static std::array GAMEPAD_AXES = {
    NamedCode { "LX", SDL_GAMEPAD_AXIS_LEFTX },
    NamedCode { "LY", SDL_GAMEPAD_AXIS_LEFTY },
    NamedCode { "RX", SDL_GAMEPAD_AXIS_RIGHTX },
    NamedCode { "RY", SDL_GAMEPAD_AXIS_RIGHTY },
    NamedCode { "LT", SDL_GAMEPAD_AXIS_LEFT_TRIGGER },
    NamedCode { "RT", SDL_GAMEPAD_AXIS_RIGHT_TRIGGER },
};

static std::string normalize_name(std::string_view name)
{
    std::string result;
    result.reserve(name.size());

    for(auto character : name) {
        if(character == ' ') {
            result.push_back('_');
        }
        else {
            result.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(character))));
        }
    }

    return result;
}

static std::optional<std::uint32_t> lookup_name(std::span<const NamedCode> table, std::string_view name)
{
    for(const auto& entry : table) {
        if(entry.name == name) {
            return entry.code;
        }
    }

    return std::nullopt;
}

static std::string lookup_code(std::span<const NamedCode> table, std::uint32_t code)
{
    for(const auto& entry : table) {
        if(entry.code == code) {
            return std::string(entry.name);
        }
    }

    return std::string();
}

static std::optional<std::uint32_t> parse_scancode(std::string_view name)
{
    for(int scancode = 0; scancode < SDL_SCANCODE_COUNT; scancode += 1) {
        auto scancode_name = SDL_GetScancodeName(static_cast<SDL_Scancode>(scancode));

        if(scancode_name == nullptr || scancode_name[0] == 0) {
            continue;
        }

        if(name == normalize_name(scancode_name)) {
            return static_cast<std::uint32_t>(scancode);
        }
    }

    return std::nullopt;
}

std::optional<std::uint32_t> input::parse_button_code(input_device device, std::string_view name)
{
    auto normalized = normalize_name(name);

    if(device == input_device::MOUSE) {
        if(auto code = lookup_name(MOUSE_BUTTONS, normalized)) {
            return code;
        }

        unsigned number;
        auto begin = normalized.data();
        auto end = begin + normalized.size();
        auto check = std::from_chars(begin, end, number);

        if(check.ec == std::errc() && number >= 1) {
            return static_cast<std::uint32_t>(number);
        }

        return std::nullopt;
    }

    switch(device) {
        case input_device::MOUSE:
            return std::nullopt; // Handled above

        case input_device::KEYBOARD:
            return parse_scancode(normalized);

        case input_device::MWHEEL:
            return lookup_name(MWHEEL_BUTTONS, normalized);

        case input_device::GAMEPAD:
            return lookup_name(GAMEPAD_BUTTONS, normalized);
    }

    return std::nullopt;
}

std::optional<std::uint32_t> input::parse_axis_code(input_device device, std::string_view name)
{
    auto normalized = normalize_name(name);

    switch(device) {
        case input_device::MOUSE:
            return lookup_name(MOUSE_AXES, normalized);

        case input_device::GAMEPAD:
            return lookup_name(GAMEPAD_AXES, normalized);

        case input_device::KEYBOARD:
        case input_device::MWHEEL:
            // These devices have no axes
            break;
    }

    return std::nullopt;
}

std::string input::format_button_code(input_device device, std::uint32_t code)
{
    if(device == input_device::KEYBOARD) {
        if(auto name = SDL_GetScancodeName(static_cast<SDL_Scancode>(code))) {
            return normalize_name(name);
        }
        else {
            return {};
        }
    }

    if(device == input_device::MOUSE) {
        auto name = lookup_code(MOUSE_BUTTONS, code);

        if(name.empty()) {
            return std::to_string(code);
        }
        else {
            return name;
        }
    }

    switch(device) {
        case input_device::KEYBOARD:
        case input_device::MOUSE:
            // Handled above
            break;

        case input_device::MWHEEL:
            return lookup_code(MWHEEL_BUTTONS, code);

        case input_device::GAMEPAD:
            return lookup_code(GAMEPAD_BUTTONS, code);
    }

    return {};
}

std::string input::format_axis_code(input_device device, std::uint32_t code)
{
    switch(device) {
        case input_device::MOUSE:
            return lookup_code(MOUSE_AXES, code);

        case input_device::GAMEPAD:
            return lookup_code(GAMEPAD_AXES, code);

        case input_device::KEYBOARD:
        case input_device::MWHEEL:
            // These devices have no axes
            break;
    }

    return {};
}

std::optional<input_edge> input::parse_edge(std::string_view name)
{
    auto normalized = normalize_name(name);

    if(normalized == "DOWN") {
        return input_edge::DOWN;
    }

    if(normalized == "UP") {
        return input_edge::UP;
    }

    return std::nullopt;
}

std::string_view input::format_edge(input_edge edge)
{
    if(edge == input_edge::UP) {
        return "up";
    }
    else {
        return "down";
    }
}

std::string_view input::format_button_cmd(input_device device)
{
    switch(device) {
        case input_device::KEYBOARD:
            return "kbutton";

        case input_device::MOUSE:
            return "mbutton";

        case input_device::MWHEEL:
            return "mwheel";

        case input_device::GAMEPAD:
            return "gbutton";
    }

    return {};
}

std::string_view input::format_axis_cmd(input_device device)
{
    switch(device) {
        case input_device::MOUSE:
            return "maxis";

        case input_device::GAMEPAD:
            return "gaxis";

        case input_device::KEYBOARD:
        case input_device::MWHEEL:
            // These devices have no axis command
            break;
    }

    return {};
}

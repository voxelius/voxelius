#include "client/pch.hh"

#include "client/input/source.hh"

#include "core/utils/string.hh"

struct IN_NamedCode final {
    std::string_view name;
    std::uint32_t code;
};

constexpr static std::array MOUSE_BUTTONS = {
    IN_NamedCode { "LF", SDL_BUTTON_LEFT },
    IN_NamedCode { "MD", SDL_BUTTON_MIDDLE },
    IN_NamedCode { "RT", SDL_BUTTON_RIGHT },
};

constexpr static std::array MWHEEL_BUTTONS = {
    IN_NamedCode { "XUP", MWHEEL_XUP },
    IN_NamedCode { "XDOWN", MWHEEL_XDOWN },
    IN_NamedCode { "YUP", MWHEEL_YUP },
    IN_NamedCode { "YDOWN", MWHEEL_YDOWN },
};

constexpr static std::array GAMEPAD_BUTTONS = {
    IN_NamedCode { "A", SDL_GAMEPAD_BUTTON_SOUTH },
    IN_NamedCode { "B", SDL_GAMEPAD_BUTTON_EAST },
    IN_NamedCode { "X", SDL_GAMEPAD_BUTTON_WEST },
    IN_NamedCode { "Y", SDL_GAMEPAD_BUTTON_NORTH },
    IN_NamedCode { "BACK", SDL_GAMEPAD_BUTTON_BACK },
    IN_NamedCode { "GUIDE", SDL_GAMEPAD_BUTTON_GUIDE },
    IN_NamedCode { "START", SDL_GAMEPAD_BUTTON_START },
    IN_NamedCode { "LS", SDL_GAMEPAD_BUTTON_LEFT_STICK },
    IN_NamedCode { "RS", SDL_GAMEPAD_BUTTON_RIGHT_STICK },
    IN_NamedCode { "LB", SDL_GAMEPAD_BUTTON_LEFT_SHOULDER },
    IN_NamedCode { "RB", SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER },
    IN_NamedCode { "DPAD_UP", SDL_GAMEPAD_BUTTON_DPAD_UP },
    IN_NamedCode { "DPAD_DOWN", SDL_GAMEPAD_BUTTON_DPAD_DOWN },
    IN_NamedCode { "DPAD_LEFT", SDL_GAMEPAD_BUTTON_DPAD_LEFT },
    IN_NamedCode { "DPAD_RIGHT", SDL_GAMEPAD_BUTTON_DPAD_RIGHT },
};

constexpr static std::array MOUSE_AXES = {
    IN_NamedCode { "X", MAXIS_X },
    IN_NamedCode { "Y", MAXIS_Y },
};

constexpr static std::array GAMEPAD_AXES = {
    IN_NamedCode { "LX", SDL_GAMEPAD_AXIS_LEFTX },
    IN_NamedCode { "LY", SDL_GAMEPAD_AXIS_LEFTY },
    IN_NamedCode { "RX", SDL_GAMEPAD_AXIS_RIGHTX },
    IN_NamedCode { "RY", SDL_GAMEPAD_AXIS_RIGHTY },
    IN_NamedCode { "LT", SDL_GAMEPAD_AXIS_LEFT_TRIGGER },
    IN_NamedCode { "RT", SDL_GAMEPAD_AXIS_RIGHT_TRIGGER },
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

static std::optional<std::uint32_t> lookup_name(std::span<const IN_NamedCode> table, std::string_view name)
{
    for(const auto& entry : table) {
        if(entry.name == name) {
            return entry.code;
        }
    }

    return std::nullopt;
}

static std::string lookup_code(std::span<const IN_NamedCode> table, std::uint32_t code)
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

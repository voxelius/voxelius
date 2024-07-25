// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <client/event/glfw_key.hh>
#include <client/event/language_set.hh>
#include <client/globals.hh>
#include <client/key_name.hh>
#include <client/lang.hh>
#include <client/ui_screen.hh>
#include <client/ui_settings.hh>
#include <entt/signal/dispatcher.hpp>
#include <glm/fwd.hpp>
#include <glm/vec2.hpp>
#include <imgui.h>
#include <imgui_stdlib.h>
#include <parson.h>
#include <shared/util/cxmath.hh>
#include <shared/util/physfs.hh>
#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>
#include <unordered_map>
#include <vector>

constexpr static ImGuiWindowFlags MENU_FLAGS = (
    ImGuiWindowFlags_NoBackground       |
    ImGuiWindowFlags_NoCollapse         |
    ImGuiWindowFlags_NoMove             |
    ImGuiWindowFlags_NoResize           |
    ImGuiWindowFlags_NoSavedSettings    |
    ImGuiWindowFlags_NoTitleBar
);

enum class ValueType {
    Checkbox    = 0x0000,
    FloatSlider = 0x0001,
    IntInput    = 0x0002,
    IntSlider   = 0x0003,
    Language    = 0x0004,
    TextInput   = 0x0005,
    UintInput   = 0x0006,
    UintSlider  = 0x0007,
    KeyBind     = 0x0008,
};

using ValueFlags = unsigned short;
constexpr static ValueFlags VALUE_FLAG_NULL     = 0x0000;
constexpr static ValueFlags VALUE_FLAG_TOOLTIP  = 0x0001;
constexpr static ValueFlags VALUE_FLAG_NO_SPACE = 0x0002;

class SettingsValue final {
public:
    ValueType type {};
    ValueFlags flags {};
    void *data_ptr {};

public:
    std::string title {};
    std::string tooltip {};

public:
    std::string title_ui {};

public:
    std::string slider_format {};
    glm::fvec2 slider_limit {};

public:
    static void layout_checkbox(const SettingsValue *value);
    static void layout_float_slider(const SettingsValue *value);
    static void layout_int_input(const SettingsValue *value);
    static void layout_int_slider(const SettingsValue *value);
    static void layout_language(const SettingsValue *value);
    static void layout_text_input(const SettingsValue *value);
    static void layout_uint_input(const SettingsValue *value);
    static void layout_uint_slider(const SettingsValue *value);
    static void layout_keybind(const SettingsValue *value);

public:
    // Every SettingsValue can have a tooltip if
    // the JSON manifest specifies a boolean flag
    static void layout_tooltip(const SettingsValue *value);

public:
    static bool parse(const JSON_Object *object, const std::string &name, SettingsValue &value);
    static void update_strings(const std::string &name, SettingsValue &value);
};

static std::unordered_map<std::string, SettingsValue> values = {};
static std::vector<SettingsValue *> values_general = {};
static std::vector<SettingsValue *> values_controls_keyboard_movement = {};
static std::vector<SettingsValue *> values_controls_keyboard_gameplay = {};
static std::vector<SettingsValue *> values_controls_keyboard_miscellaneous = {};
static std::vector<SettingsValue *> values_controls_mouse = {};
static std::vector<SettingsValue *> values_controls_gamepad = {};
static std::vector<SettingsValue *> values_video = {};
static std::vector<SettingsValue *> values_video_gui = {};
static std::vector<SettingsValue *> values_sound = {};

static std::string str_general_title = {};
static std::string str_controls_title = {};
static std::string str_controls_keyboard = {};
static std::string str_controls_keyboard_movement = {};
static std::string str_controls_keyboard_gameplay = {};
static std::string str_controls_keyboard_miscellaneous = {};
static std::string str_controls_mouse = {};
static std::string str_controls_gamepad = {};
static std::string str_video_title = {};
static std::string str_video_gui = {};
static std::string str_sound_title = {};

void SettingsValue::layout_checkbox(const SettingsValue *value)
{
    if(value->data_ptr) {
        ImGui::Checkbox(value->title_ui.c_str(), reinterpret_cast<bool *>(value->data_ptr));
        SettingsValue::layout_tooltip(value);
    }
}

void SettingsValue::layout_float_slider(const SettingsValue *value)
{
    if(value->data_ptr) {
        const char *v_format = value->slider_format.c_str();
        const float v_min = value->slider_limit.x;
        const float v_max = value->slider_limit.y;
        ImGui::SliderFloat(value->title_ui.c_str(), reinterpret_cast<float *>(value->data_ptr), v_min, v_max, v_format);
        SettingsValue::layout_tooltip(value);
    }
}

void SettingsValue::layout_int_input(const SettingsValue *value)
{
    if(value->data_ptr) {
        ImGui::InputInt(value->title_ui.c_str(), reinterpret_cast<int *>(value->data_ptr));
        SettingsValue::layout_tooltip(value);
    }
}

void SettingsValue::layout_int_slider(const SettingsValue *value)
{
    if(value->data_ptr) {
        const int v_min = value->slider_limit.x;
        const int v_max = value->slider_limit.y;
        ImGui::SliderInt(value->title_ui.c_str(), reinterpret_cast<int *>(value->data_ptr), v_min, v_max);
        SettingsValue::layout_tooltip(value);
    }
}

void SettingsValue::layout_language(const SettingsValue *value)
{
    const LangIterator current = lang::current();

    if(ImGui::BeginCombo(value->title_ui.c_str(), current->display.c_str())) {
        for(LangIterator it = lang::cbegin(); it != lang::cend(); ++it) {
            if(ImGui::Selectable(it->display.c_str(), it == current)) {
                lang::set(it);
                continue;
            }
        }

        ImGui::EndCombo();
    }

    SettingsValue::layout_tooltip(value);
}

void SettingsValue::layout_text_input(const SettingsValue *value)
{
    if(value->data_ptr) {
        ImGuiInputTextFlags flags = 0;
        if(value->flags & VALUE_FLAG_NO_SPACE)
            flags |= ImGuiInputTextFlags_CharsNoBlank;
        ImGui::InputText(value->title_ui.c_str(), reinterpret_cast<std::string *>(value->data_ptr), flags);
        SettingsValue::layout_tooltip(value);
    }
}

void SettingsValue::layout_uint_input(const SettingsValue *value)
{
    if(value->data_ptr) {
        ImGui::InputScalar(value->title_ui.c_str(), ImGuiDataType_U32, reinterpret_cast<unsigned int *>(value->data_ptr));
        SettingsValue::layout_tooltip(value);
    }
}

void SettingsValue::layout_uint_slider(const SettingsValue *value)
{
    if(value->data_ptr) {
        const unsigned int v_min = value->slider_limit.x;
        const unsigned int v_max = value->slider_limit.y;
        ImGui::SliderScalar(value->title_ui.c_str(), ImGuiDataType_U32, reinterpret_cast<unsigned int *>(value->data_ptr), &v_min, &v_max);
        SettingsValue::layout_tooltip(value);
    }
}

void SettingsValue::layout_keybind(const SettingsValue *value)
{
    if(value->data_ptr) {
        std::string key_name = {};
        int *value_keyptr = reinterpret_cast<int *>(value->data_ptr);

        if(globals::ui_keybind_ptr == value->data_ptr)
            key_name = fmt::format("...###{}", value->data_ptr);
        else key_name = fmt::format("{}###{}", key_name::get(value_keyptr[0]), value->data_ptr);

        if(ImGui::Button(key_name.c_str(), ImVec2(ImGui::GetWindowSize().x * 0.45f, 0.0f)))
            globals::ui_keybind_ptr = value_keyptr;
        ImGui::SameLine();
        ImGui::TextUnformatted(value->title.c_str());
        SettingsValue::layout_tooltip(value);
    }
}

void SettingsValue::layout_tooltip(const SettingsValue *value)
{
    if(value->flags & VALUE_FLAG_TOOLTIP) {
        ImGui::SameLine();
        ImGui::TextDisabled("[ ? ]");

        if(ImGui::BeginItemTooltip()) {
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 16.0f);
            ImGui::TextUnformatted(value->tooltip.c_str());
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }
}

bool SettingsValue::parse(const JSON_Object *object, const std::string &name, SettingsValue &value)
{
    const char *type_cstr = json_object_get_string(object, "type");
    const std::string type = type_cstr ? type_cstr : std::string("<null>");

    if(json_object_get_boolean(object, "tooltip"))
        value.flags |= VALUE_FLAG_TOOLTIP;
    if(json_object_get_boolean(object, "no_space"))
        value.flags |= VALUE_FLAG_NO_SPACE;

    if(const JSON_Array *limit = json_object_get_array(object, "limit")) {
        value.slider_limit.x = json_array_get_number(limit, 0);
        value.slider_limit.y = json_array_get_number(limit, 1);
    }
    else {
        value.slider_limit.x = std::numeric_limits<float>::min();
        value.slider_limit.y = std::numeric_limits<float>::max();
    }

    if(!type.compare("checkbox")) {
        value.type = ValueType::Checkbox;
    }
    else if(!type.compare("float_slider")) {
        value.type = ValueType::FloatSlider;
        value.slider_format = "%f";
    }
    else if(!type.compare("int_input")) {
        value.type = ValueType::IntInput;
    }
    else if(!type.compare("int_slider")) {
        value.type = ValueType::IntSlider;
        value.slider_format = "%d";
    }
    else if(!type.compare("language")) {
        value.type = ValueType::Language;
    }
    else if(!type.compare("text_input")) {
        value.type = ValueType::TextInput;
    }
    else if(!type.compare("uint_input")) {
        value.type = ValueType::UintInput;
    }
    else if(!type.compare("uint_slider")) {
        value.type = ValueType::UintSlider;
        value.slider_format = "%u";
    }
    else if(!type.compare("keybind")) {
        value.type = ValueType::KeyBind;
    }
    else {
        // Implied invalid type because
        // everything else has a faily defined
        // default state which doesn't affect
        // how data_ptr is treated
        return false;
    }

    if(const char *format_cstr = json_object_get_string(object, "format")) {
        value.slider_format = std::string(format_cstr);
    }

    return true;
}

void SettingsValue::update_strings(const std::string &name, SettingsValue &value)
{
    value.title = lang::resolve(fmt::format("settings.value.{}", name));
    value.tooltip = lang::resolve(fmt::format("settings.tooltip.{}", name));
    value.title_ui = fmt::format("{}###settings.value.{}", value.title, name);
}

static void on_glfw_key(const GlfwKeyEvent &event)
{
    if(event.action == GLFW_PRESS) {
        if(globals::ui_keybind_ptr) {
            if(event.key == GLFW_KEY_ESCAPE) {
                globals::ui_keybind_ptr = nullptr;
                return;
            }
            
            globals::ui_keybind_ptr[0] = event.key;
            globals::ui_keybind_ptr = nullptr;
            return;
        }
        
        if((event.key == GLFW_KEY_ESCAPE) && (globals::ui_screen == ui::SCREEN_SETTINGS)) {
            globals::ui_screen = ui::SCREEN_MAIN_MENU;
            return;
        }
    }
}

static void on_language_set(const LanguageSetEvent &event)
{
    for(auto &it : values) {
        SettingsValue::update_strings(it.first, it.second);
    }

    str_general_title = lang::resolve_ui("settings.general.title");

    str_controls_title = lang::resolve_ui("settings.controls.title");
    str_controls_keyboard = lang::resolve_ui("settings.controls.keyboard");
    str_controls_keyboard_movement = lang::resolve_ui("settings.controls.keyboard.movement");
    str_controls_keyboard_gameplay = lang::resolve_ui("settings.controls.keyboard.gameplay");
    str_controls_keyboard_miscellaneous = lang::resolve_ui("settings.controls.keyboard.miscellaneous");
    str_controls_mouse = lang::resolve_ui("settings.controls.mouse");
    str_controls_gamepad = lang::resolve_ui("settings.controls.gamepad");

    str_video_title = lang::resolve_ui("settings.video.title");
    str_video_gui = lang::resolve_ui("settings.video.gui");

    str_sound_title = lang::resolve_ui("settings.sound.title");
}

static void layout_list(const std::vector<SettingsValue *> &values_vector)
{
    for(const SettingsValue *value : values_vector) {
        switch(value->type) {
            case ValueType::Checkbox:
                SettingsValue::layout_checkbox(value);
                break;
            case ValueType::FloatSlider:
                SettingsValue::layout_float_slider(value);
                break;
            case ValueType::IntInput:
                SettingsValue::layout_int_input(value);
                break;
            case ValueType::IntSlider:
                SettingsValue::layout_int_slider(value);
                break;
            case ValueType::Language:
                SettingsValue::layout_language(value);
                break;
            case ValueType::TextInput:
                SettingsValue::layout_text_input(value);
                break;
            case ValueType::UintInput:
                SettingsValue::layout_uint_input(value);
                break;
            case ValueType::UintSlider:
                SettingsValue::layout_uint_slider(value);
                break;
            case ValueType::KeyBind:
                SettingsValue::layout_keybind(value);
                break;
        }
    }
}

static SettingsValue *find_value(const std::string &name)
{
    const auto it = values.find(name);
    if(it != values.cend())
        return &it->second;
    return nullptr;
}

void ui::settings::init(void)
{
    const std::string path = std::string("misc/settings.json");

    std::string source = {};

    if(!util::read_string(path, source)) {
        spdlog::critical("ui::settings: {}: {}", path, util::physfs_error());
        std::terminate();
    }

    JSON_Value *jsonv = json_parse_string(source.c_str());
    const JSON_Object *json = json_value_get_object(jsonv);
    const std::size_t count = json_object_get_count(json);

    if(!jsonv) {
        spdlog::critical("ui::settings: {}: parse error", path);
        json_value_free(jsonv);
        std::terminate();
    }

    if(!json) {
        spdlog::critical("ui::settings: {}: root is not an object", path);
        json_value_free(jsonv);
        std::terminate();
    }

    for(std::size_t i = 0; i < count; ++i) {
        const char *name = json_object_get_name(json, i);
        const JSON_Value *entryv = json_object_get_value_at(json, i);
        const JSON_Object *entry = json_value_get_object(entryv);

        if(!name) {
            spdlog::critical("ui::settings: {}: invalid key", path);
            json_value_free(jsonv);
            std::terminate();
        }

        if(!entryv) {
            spdlog::critical("ui::settings: {}: {}: invalid value", path, name);
            json_value_free(jsonv);
            std::terminate();
        }

        if(!entry) {
            spdlog::critical("ui::settings: {}: {} is not an object", path, name);
            json_value_free(jsonv);
            std::terminate();
        }

        SettingsValue value = {};
        value.data_ptr = nullptr;

        if(!SettingsValue::parse(entry, name, value)) {
            spdlog::critical("ui::settings: {}: {}: invalid type", path, name);
            json_value_free(jsonv);
            std::terminate();
        }

        const char *location_cstr = json_object_get_string(entry, "location");
        const std::string location = location_cstr ? location_cstr : std::string("<null>");

        // Pointer to the inserted moved value instance
        SettingsValue *value_ptr = &values.insert_or_assign(name, std::move(value)).first->second;

        if(!location.compare("general")) {
            values_general.push_back(value_ptr);
            continue;
        }

        if(!location.compare("controls.keyboard.movement")) {
            values_controls_keyboard_movement.push_back(value_ptr);
            continue;
        }
        
        if(!location.compare("controls.keyboard.gameplay")) {
            values_controls_keyboard_gameplay.push_back(value_ptr);
            continue;
        }
        
        if(!location.compare("controls.keyboard.miscellaneous")) {
            values_controls_keyboard_miscellaneous.push_back(value_ptr);
            continue;
        }

        if(!location.compare("controls.mouse")) {
            values_controls_mouse.push_back(value_ptr);
            continue;
        }

        if(!location.compare("controls.gamepad")) {
            values_controls_gamepad.push_back(value_ptr);
            continue;
        }

        if(!location.compare("video")) {
            values_video.push_back(value_ptr);
            continue;
        }

        if(!location.compare("video.gui")) {
            values_video_gui.push_back(value_ptr);
            continue;
        }

        if(!location.compare("sound")) {
            values_sound.push_back(value_ptr);
            continue;
        }

        spdlog::critical("ui::settings: {}: {}: invalid location {}", path, name, location);
        json_value_free(jsonv);
        std::terminate();
    }

    json_value_free(jsonv);

    globals::dispatcher.sink<GlfwKeyEvent>().connect<&on_glfw_key>();
    globals::dispatcher.sink<LanguageSetEvent>().connect<&on_language_set>();
}

void ui::settings::layout(void)
{
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const ImVec2 window_start = ImVec2(viewport->Size.x * 0.10f, viewport->Size.y * 0.10f);
    const ImVec2 window_size = ImVec2(viewport->Size.x * 0.80f, viewport->Size.y * 0.80f);

    ImGui::SetNextWindowPos(window_start);
    ImGui::SetNextWindowSize(window_size);

    if(ImGui::Begin("###settings", nullptr, MENU_FLAGS)) {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(3.0f * globals::ui_scale, 3.0f * globals::ui_scale));

        if(ImGui::BeginTabBar("###settings.tabs", ImGuiTabBarFlags_FittingPolicyResizeDown)) {
            if(ImGui::TabItemButton("<<")) {
                globals::ui_screen = ui::SCREEN_MAIN_MENU;
                globals::ui_keybind_ptr = nullptr;
            }

            if(ImGui::BeginTabItem(str_general_title.c_str())) {
                globals::ui_keybind_ptr = nullptr;
                if(ImGui::BeginChild("###settings.general.child"))
                    layout_list(values_general);
                ImGui::EndChild();
                ImGui::EndTabItem();
            }

            if(ImGui::BeginTabItem(str_controls_title.c_str())) {
                if(ImGui::BeginChild("###settings.controls.child")) {
                    ImGui::SeparatorText(str_controls_keyboard.c_str());
                    if(ImGui::CollapsingHeader(str_controls_keyboard_movement.c_str()))
                        layout_list(values_controls_keyboard_movement);
                    if(ImGui::CollapsingHeader(str_controls_keyboard_gameplay.c_str()))
                        layout_list(values_controls_keyboard_gameplay);
                    if(ImGui::CollapsingHeader(str_controls_keyboard_miscellaneous.c_str()))
                        layout_list(values_controls_keyboard_miscellaneous);
                    ImGui::SeparatorText(str_controls_mouse.c_str());
                    layout_list(values_controls_mouse);
                    ImGui::SeparatorText(str_controls_gamepad.c_str());
                    layout_list(values_controls_gamepad);
                }

                ImGui::EndChild();
                ImGui::EndTabItem();
            }

            if(ImGui::BeginTabItem(str_video_title.c_str())) {
                globals::ui_keybind_ptr = nullptr;

                if(ImGui::BeginChild("###settings.graphics.child")) {
                    layout_list(values_video);
                    ImGui::SeparatorText(str_video_gui.c_str());
                    layout_list(values_video_gui);
                }

                ImGui::EndChild();
                ImGui::EndTabItem();
            }

            if(ImGui::BeginTabItem(str_sound_title.c_str())) {
                globals::ui_keybind_ptr = nullptr;

                if(ImGui::BeginChild("###settings.sound.child")) {
                    layout_list(values_sound);
                }

                ImGui::EndChild();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::PopStyleVar();
    }

    ImGui::End();
}

void ui::settings::link(const std::string &name, int &vref)
{
    if(SettingsValue *value = find_value(name)) {
        if((value->type != ValueType::IntInput) && (value->type != ValueType::IntSlider) && (value->type != ValueType::KeyBind)) {
            spdlog::warn("ui::settings: {}: invalid link type", name);
            return;
        }

        value->data_ptr = &vref;
        return;
    }

    spdlog::warn("ui::settings: unknown value {}", name);
}

void ui::settings::link(const std::string &name, bool &vref)
{
    if(SettingsValue *value = find_value(name)) {
        if(value->type != ValueType::Checkbox) {
            spdlog::warn("ui::settings: {}: invalid link type", name);
            return;
        }

        value->data_ptr = &vref;
        return;
    }

    spdlog::warn("ui::settings: unknown value {}", name);
}

void ui::settings::link(const std::string &name, float &vref)
{
    if(SettingsValue *value = find_value(name)) {
        if(value->type != ValueType::FloatSlider) {
            spdlog::warn("ui::settings: {}: invalid link type", name);
            return;
        }

        value->data_ptr = &vref;
        return;
    }

    spdlog::warn("ui::settings: unknown value {}", name);
}

void ui::settings::link(const std::string &name, std::string &vref)
{
    if(SettingsValue *value = find_value(name)) {
        if(value->type != ValueType::TextInput) {
            spdlog::warn("ui::settings: {}: invalid link type", name);
            return;
        }

        value->data_ptr = &vref;
        return;
    }

    spdlog::warn("ui::settings: unknown value {}", name);
}

void ui::settings::link(const std::string &name, unsigned int &vref)
{
    if(SettingsValue *value = find_value(name)) {
        if((value->type != ValueType::UintInput) && (value->type != ValueType::UintSlider)) {
            spdlog::warn("ui::settings: {}: invalid link type", name);
            return;
        }

        value->data_ptr = &vref;
        return;
    }

    spdlog::warn("ui::settings: unknown value {}", name);
}

// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <client/camera.hh>
#include <client/event/glfw_key.hh>
#include <client/event/language_set.hh>
#include <client/game.hh>
#include <client/globals.hh>
#include <client/mouse.hh>
#include <client/ui_screen.hh>
#include <client/ui_settings.hh>
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include <glm/vec2.hpp>
#include <imgui.h>
#include <imgui_stdlib.h>
#include <parson.h>
#include <shared/util/physfs.hh>
#include <shared/config.hh>
#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>

constexpr static ImGuiWindowFlags MENU_FLAGS = (
    ImGuiWindowFlags_NoTitleBar         |
    ImGuiWindowFlags_NoResize           |
    ImGuiWindowFlags_NoCollapse         |
    ImGuiWindowFlags_NoMove             |
    ImGuiWindowFlags_NoSavedSettings    |
    ImGuiWindowFlags_NoBackground
);

enum class SettingType {
    Checkbox        = 0x0000,
    FloatSlider     = 0x0001,
    IntInput        = 0x0002,
    IntSlider       = 0x0003,
    LanguageSelect  = 0x0004,
    TextInput       = 0x0005,
    UintInput       = 0x0006,
    UintSlider      = 0x0007,
};

using SettingFlags = unsigned short;
constexpr static SettingFlags SETTING_NULL      = 0x0000;
constexpr static SettingFlags SETTING_TOOLTIP   = 0x0001;
constexpr static SettingFlags SETTING_NO_SPACE  = 0x0002;

class SettingValue final {
public:
    std::string title {};
    std::string tooltip {};
    SettingFlags flags {};
    SettingType type {};
    void *data_ptr {};

public:
    std::string slider_format {};
    glm::fvec2 slider_limit_f {};
    glm::ivec2 slider_limit_i {};
    glm::uvec2 slider_limit_u {};

public:
    static void layout_checkbox(SettingValue *value);
    static void layout_float_slider(SettingValue *value);
    static void layout_int_input(SettingValue *value);
    static void layout_int_slider(SettingValue *value);
    static void layout_language_select(SettingValue *value);
    static void layout_text_input(SettingValue *value);
    static void layout_uint_input(SettingValue *value);
    static void layout_uint_slider(SettingValue *value);

public:
    // Each setting can have a tooltip
    static void layout_tooltip(SettingsValue *value);

public:
    static bool parse(SettingsValue &value, const JSON_Object *object);
    static void update_strings(const std::string &name, SettingsValue &value);
};

static std::unordered_map<std::string, SettingValue> values = {};
static std::vector<SettingsValue *> values_general = {};
static std::vector<SettingsValue *> values_general_multiplayer = {};
static std::vector<SettingsValue *> values_controls_keyboard = {};
static std::vector<SettingsValue *> values_controls_mouse = {};
static std::vector<SettingsValue *> values_controls_gamepad = {};
static std::vector<SettingsValue *> values_graphics_performance = {};
static std::vector<SettingsValue *> values_graphics_ui = {};
static std::vector<SettingsValue *> values_sound = {};

static std::string str_general = {};
static std::string str_general_multiplayer = {};
static std::string str_controls = {};
static std::string str_controls_keyboard = {};
static std::string str_controls_mouse = {};
static std::string str_controls_gamepad = {};
static std::string str_graphics = {};
static std::string str_graphics_performance = {};
static std::string str_graphics_ui = {};
static std::string str_sound = {};

void SettingsValue::layout_checkbox(SettingValue *value)
{
    ImGui::Checkbox(value->title.c_str(), reinterpret_cast<bool *>(value->data_ptr));
}

void SettingsValue::layout_float_slider(SettingValue *value)
{
    const char *s_fmt = value->slider_format.c_str();
    const float s_max = value->slider_limit_f.x;
    const float s_min = value->slider_limit_f.y;
    ImGui::SliderFloat(value->title.c_str(), reinterpret_cast<float *>(value->data_ptr), s_min, s_max, s_fmt);
}

void SettingValue::layout_int_input(SettingValue *value)
{
    ImGui::InputInt(value->title.c_str(), reinterpret_cast<int *>(value->data_ptr));
}


static void layout_int_input(const SettingsValue *value)
{
    ImGui::InputInt(value->title.c_str(), reinterpret_cast<int *>(value->data_ptr));
    layout_tooltip(value);
}

static void layout_int_slider(const SettingsValue *value)
{
    const int s_min = value->slider.limit_i.x;
    const int s_max = value->slider.limit_i.y;
    ImGui::SliderInt(value->title.c_str(), reinterpret_cast<int *>(value->data_ptr), s_min, s_max);
    layout_tooltip(value);
}

static void layout_language_select(const SettingsValue *value)
{
    const LangIterator current = lang::current();

    if(ImGui::BeginCombo(value->title.c_str(), current->display.c_str())) {
        for(LangIterator it = lang::cbegin(); it != lang::cend(); ++it) {
            if(ImGui::Selectable(it->display.c_str(), it == current)) {
                lang::set(it);
                continue;
            }
        }
        
        ImGui::EndCombo();
    }

    layout_tooltip(value);
}

static void layout_text_input(const SettingsValue *value)
{
    
    
}

/*
    static void layout_int_input(SettingValue *value);
    static void layout_int_slider(SettingValue *value);
    static void layout_language_select(SettingValue *value);
    static void layout_text_input(SettingValue *value);
    static void layout_uint_input(SettingValue *value);
    static void layout_uint_slider(SettingValue *value);

public:
    // Each setting can have a tooltip
    static void layout_tooltip(SettingsValue *value);

public:
    static bool parse(SettingsValue &value, const JSON_Object *object);
    static void update_strings(const std::string &name, SettingsValue &value);
*/

static void on_glfw_key(const GlfwKeyEvent &event)
{
    if(event.key != GLFW_KEY_ESCAPE)
        return;
    if(event.action != GLFW_PRESS)
        return;
    if(globals::ui_screen != ui::SCREEN_SETTINGS)
        return;
    globals::ui_screen = ui::SCREEN_MAIN_MENU;
}

static void on_language_set(const LanguageSetEvent &event)
{
    for(auto &it : values)
        SettingValue::update_strings(it.first, it.second);
    str_general                 = lang::resolve_ui("settings.general");
    str_general_multiplayer     = lang::resolve_ui("settings.general.multiplayer");
    str_controls                = lang::resolve_ui("settings.controls");
    str_controls_keyboard       = lang::resolve_ui("settings.controls.keyboard");
    str_controls_mouse          = lang::resolve_ui("settings.controls.mouse");
    str_controls_gamepad        = lang::resolve_ui("settings.controls.gamepad");
    str_graphics                = lang::resolve_ui("settings.graphics");
    str_graphics_performance    = lang::resolve_ui("settings.graphics.performance");
    str_graphics_ui             = lang::resolve_ui("settings.graphics.ui");
    str_sound                   = lang::resolve_ui("settings.sound");
}

static void layout_tooltip(const SettingsValue *value)
{
    if(value->tooltip && !value->str_tooltip.empty()) {
        ImGui::SameLine();
        ImGui::TextDisabled("[ ? ]");

        if(ImGui::BeginItemTooltip()) {
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 16.0f);
            ImGui::TextUnformatted(tooltip.c_str());
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }
}

static void layout_string(const SettingsVariable *variable)
{
    if(variable->void_type != CONFIG_STD_STRING) {
        // If other types can be loosely converted,
        // things like strings cannot easily be
        return;
    }

    std::string &vref = reinterpret_cast<std::string *>(variable->data_ptr)[0];

    if(variable->setting_type == SettingType::Input) {
        ImGui::InputText(variable->lang_title.c_str(), &vref, variable->text_flags);
        settings_tooltip(variable->lang_tooltip);
        return;
    }

}

static void layout_subcategory(const std::vector<SettingsVariable *> &variables)
{
    for(const SettingsVariable *variable : variables) {
        switch(variable->void_type) {
            case CONFIG_INT:
            case CONFIG_UNSIGNED_INT:
                layout_int(variable);
                break;
            case CONFIG_BOOLEAN:
                layout_bool(variable);
                break;
            case CONFIG_FLOAT:
                layout_float(variable);
                break;
            case CONFIG_STD_STRING:
                layout_string(variable);
                break;
        }
    }
}

void ui::settings::init(void)
{
    globals::dispatcher.sink<GlfwKeyEvent>().connect<&on_glfw_key>();
    globals::dispatcher.sink<LanguageSetEvent>().connect<&on_language_set>();
}

void ui::settings::init_late(void)
{
    general_main.clear();
    general_multiplayer.clear();
    controls_keyboard.clear();
    controls_mouse.clear();
    controls_gamepad.clear();
    graphics_performance.clear();
    graphics_ui.clear();
    sound_main.clear();

    const std::string path = std::string("misc/settings.json");

    std::string source = {};

    if(!util::read_string(path, source)) {
        spdlog::critical("settings: {}: {}", path, util::physfs_error());
        std::terminate();
    }

    JSON_Value *jsonv = json_parse_string(source.c_str());
    const JSON_Object *json = json_value_get_object(jsonv);
    const std::size_t count = json_object_get_count(json);

    if(!jsonv || !json) {
        spdlog::critical("settings: {}: parse error", path);
        json_value_free(jsonv);
        std::terminate();
    }

    for(std::size_t i = 0; i < count; ++i) {
        const char *name = json_object_get_name(json, i);
        const JSON_Value *variablev = json_object_get_value_at(json, i);
        const JSON_Object *variable = json_value_get_object(variablev);

        if(!name) {
            spdlog::critical("settings: {}: invalid variable", path);
            json_value_free(jsonv);
            std::terminate();
        }

        if(!variable) {
            spdlog::warn("settings: {}: {} is not an object", path, name);
            continue;
        }

        const auto it = variables.find(name);

        if(it == variables.cend()) {
            spdlog::warn("settings: {}: {}: unknown variable", path, name);
            continue;
        }

        const char *category_cstr = json_object_get_string(variable, "category");
        const char *subcategory_cstr = json_object_get_string(variable, "subcategory");
        const char *type_cstr = json_object_get_string(variable, "type");

        const std::string category = category_cstr ? category_cstr : std::string("<null>");
        const std::string subcategory = subcategory_cstr ? subcategory_cstr : std::string("<null>");
        const std::string type = type_cstr ? type_cstr : std::string("<null>");

        it->second.text_flags = 0;

        if(!type.compare("input")) {
            it->second.setting_type = SettingType::Input;
        }
        else if(!type.compare("slider")) {
            it->second.setting_type = SettingType::Slider;
        }
        else if(!type.compare("checkbox")) {
            it->second.setting_type = SettingType::Checkbox;
        }
        else if(!type.compare("lang_select")) {
            it->second.setting_type = SettingType::LangSelect;
        }
        else {
            spdlog::warn("settings: {}: {} invalid type {}", path, name, type);
            it->second.setting_type = SettingType::Invalid;
            continue;
        }

        if(const char *fmt = json_object_get_string(variable, "format")) {
            it->second.slider_fmt = std::string(fmt);
        }

        it->second.has_tooltip = json_object_get_boolean(variable, "tooltip");

        if(!json_object_get_boolean(variable, "whitespace")) {
            it->second.text_flags |= ImGuiInputTextFlags_CharsNoBlank;
        }

        if(const JSON_Array *limit = json_object_get_array(variable, "limit")) {
            it->second.slider_limit.x = json_array_get_number(limit, 0);
            it->second.slider_limit.y = json_array_get_number(limit, 1);
        }

        if(!category.compare("general")) {
            if(!subcategory.compare("main")) {
                general_main.push_back(&it->second);
                continue;
            }

            if(!subcategory.compare("multiplayer")) {
                general_multiplayer.push_back(&it->second);
                continue;
            }
        }

        if(!category.compare("controls")) {
            if(!subcategory.compare("keyboard")) {
                controls_keyboard.push_back(&it->second);
                continue;
            }

            if(!subcategory.compare("mouse")) {
                controls_mouse.push_back(&it->second);
                continue;
            }

            if(!subcategory.compare("gamepad")) {
                controls_gamepad.push_back(&it->second);
                continue;
            }
        }

        if(!category.compare("graphics")) {
            if(!subcategory.compare("performance")) {
                graphics_performance.push_back(&it->second);
                continue;
            }

            if(!subcategory.compare("ui")) {
                graphics_ui.push_back(&it->second);
                continue;
            }
        }

        if(!category.compare("sound")) {
            if(!subcategory.compare("main")) {
                sound_main.push_back(&it->second);
                continue;
            }
        }

        spdlog::warn("settings: {}: {}: invalid location: {}/{}", path, name, category, subcategory);
    }

    json_value_free(jsonv);
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
                // Go back to the main menu
                globals::ui_screen = ui::SCREEN_MAIN_MENU;
            }

            if(ImGui::BeginTabItem(tab_general.c_str())) {
                if(ImGui::BeginChild("###settings.general.child")) {
                    layout_subcategory(general_main);
                    ImGui::SeparatorText(separator_multiplayer.c_str());
                    layout_subcategory(general_multiplayer);
                }

                ImGui::EndChild();
                ImGui::EndTabItem();
            }

            if(ImGui::BeginTabItem(tab_controls.c_str())) {
                if(ImGui::BeginChild("###settings.controls.child")) {
                    ImGui::SeparatorText(separator_keyboard.c_str());
                    layout_subcategory(controls_keyboard);
                    ImGui::SeparatorText(separator_mouse.c_str());
                    layout_subcategory(controls_mouse);
                    ImGui::SeparatorText(separator_gamepad.c_str());
                    layout_subcategory(controls_gamepad);
                }

                ImGui::EndChild();
                ImGui::EndTabItem();
            }

            if(ImGui::BeginTabItem(tab_graphics.c_str())) {
                if(ImGui::BeginChild("###settings.graphics.child")) {
                    ImGui::SeparatorText(separator_performance.c_str());
                    layout_subcategory(graphics_performance);
                    ImGui::SeparatorText(separator_ui.c_str());
                    layout_subcategory(graphics_ui);
                }

                ImGui::EndChild();
                ImGui::EndTabItem();
            }

            if(ImGui::BeginTabItem(tab_sound.c_str())) {
                if(ImGui::BeginChild("###settings.sound.child")) {
                    layout_subcategory(sound_main);
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

void ui::settings::add(const std::string &name, int &vref)
{
    SettingsVariable variable = {};
    variable.void_type = CONFIG_INT;
    variable.data_ptr = &vref;
    variables.insert_or_assign(name, std::move(variable));
}

void ui::settings::add(const std::string &name, bool &vref)
{
    SettingsVariable variable = {};
    variable.void_type = CONFIG_BOOLEAN;
    variable.data_ptr = &vref;
    variables.insert_or_assign(name, std::move(variable));
}

void ui::settings::add(const std::string &name, float &vref)
{
    SettingsVariable variable = {};
    variable.void_type = CONFIG_FLOAT;
    variable.data_ptr = &vref;
    variables.insert_or_assign(name, std::move(variable));
}

void ui::settings::add(const std::string &name, std::string &vref)
{
    SettingsVariable variable = {};
    variable.void_type = CONFIG_STD_STRING;
    variable.data_ptr = &vref;
    variables.insert_or_assign(name, std::move(variable));
}

void ui::settings::add(const std::string &name, unsigned int &vref)
{
    SettingsVariable variable = {};
    variable.void_type = CONFIG_UNSIGNED_INT;
    variable.data_ptr = &vref;
    variables.insert_or_assign(name, std::move(variable));
}

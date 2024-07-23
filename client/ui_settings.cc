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

constexpr static ImGuiWindowFlags MENU_FLAGS = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground;

enum class SettingType {
    Input       = 0x0000,
    Slider      = 0x0001,
    Checkbox    = 0x0002,
    LangSelect  = 0x0003,
    Invalid     = 0xFFFF,
};

struct SettingsVariable final {
    SettingType setting_type {};
    ConfigVarType void_type {};
    ImGuiInputTextFlags text_flags {};
    std::string lang_title {};
    std::string lang_tooltip {};
    glm::fvec2 slider_limit {};
    std::string slider_fmt {};
    bool has_tooltip {};
    void *data_ptr {};
};

static std::unordered_map<std::string, SettingsVariable> variables = {};
static std::vector<SettingsVariable *> general_main = {};
static std::vector<SettingsVariable *> general_multiplayer = {};
static std::vector<SettingsVariable *> controls_keyboard = {};
static std::vector<SettingsVariable *> controls_mouse = {};
static std::vector<SettingsVariable *> controls_gamepad = {};
static std::vector<SettingsVariable *> graphics_performance = {};
static std::vector<SettingsVariable *> graphics_ui = {};
static std::vector<SettingsVariable *> sound_main = {};

static std::string tab_general = {};
static std::string tab_controls = {};
static std::string tab_graphics = {};
static std::string tab_sound = {};

static std::string separator_multiplayer = {};
static std::string separator_keyboard = {};
static std::string separator_mouse = {};
static std::string separator_gamepad = {};
static std::string separator_performance = {};
static std::string separator_ui = {};

static void on_glfw_key(const GlfwKeyEvent &event)
{
    if(event.key == GLFW_KEY_ESCAPE && event.action == GLFW_PRESS) {
        switch(globals::ui_screen) {
            case ui::SCREEN_SETTINGS:
                globals::ui_screen = ui::SCREEN_MAIN_MENU;
                break;
        }
    }
}

static void on_language_set(const LanguageSetEvent &event)
{
    for(auto &it : variables) {
        if(it.second.has_tooltip)
            it.second.lang_tooltip = lang::resolve(fmt::format("settings.tooltip.{}", it.first));
        const auto lang_title_tag = fmt::format("settings.option.{}", it.first);
        it.second.lang_title = lang::resolve(lang_title_tag) + fmt::format("###{}", lang_title_tag);
    }

    tab_general = lang::resolve("settings.tab.general") + "###settings.tab.general";
    tab_controls = lang::resolve("settings.tab.controls") + "###settings.tab.controls";
    tab_graphics = lang::resolve("settings.tab.graphics") + "###settings.tab.graphics";
    tab_sound = lang::resolve("settings.tab.sound") + "###settings.tab.sound";

    separator_multiplayer = lang::resolve("settings.separator.multiplayer") + "###settings.separator.multiplayer";
    separator_keyboard = lang::resolve("settings.separator.keyboard") + "###settings.separator.keyboard";
    separator_mouse = lang::resolve("settings.separator.mouse") + "###settings.separator.mouse";
    separator_gamepad = lang::resolve("settings.separator.gamepad") + "###settings.separator.gamepad";
    separator_performance = lang::resolve("settings.separator.performance") + "###settings.separator.performance";
    separator_ui = lang::resolve("settings.separator.ui") + "###settings.separator.ui";
}

static void settings_tooltip(const std::string &tooltip)
{
    if(!tooltip.empty()) {
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

static void layout_int(const SettingsVariable *variable)
{
    int value;

    switch(variable->void_type) {
        case CONFIG_INT:
            value = reinterpret_cast<int *>(variable->data_ptr)[0];
            break;
        case CONFIG_FLOAT:
            value = reinterpret_cast<float *>(variable->data_ptr)[0];
            break;
        case CONFIG_UNSIGNED_INT:
            value = reinterpret_cast<unsigned int *>(variable->data_ptr)[0];
            break;
    }

    switch(variable->setting_type) {
        case SettingType::Input:
            ImGui::InputInt(variable->lang_title.c_str(), &value);
            settings_tooltip(variable->lang_tooltip);
            break;
        case SettingType::Slider:
            ImGui::SliderInt(variable->lang_title.c_str(), &value, variable->slider_limit.x, variable->slider_limit.y, variable->slider_fmt.c_str());
            settings_tooltip(variable->lang_tooltip);
            break;
    }

    switch(variable->void_type) {
        case CONFIG_INT:
            reinterpret_cast<int *>(variable->data_ptr)[0] = value;
            break;
        case CONFIG_FLOAT:
            reinterpret_cast<float *>(variable->data_ptr)[0] = value;
            break;
        case CONFIG_UNSIGNED_INT:
            reinterpret_cast<unsigned int *>(variable->data_ptr)[0] = value;
            break;
    }
}

static void layout_bool(const SettingsVariable *variable)
{
    bool value;

    switch(variable->void_type) {
        case CONFIG_INT:
            value = !!reinterpret_cast<int *>(variable->data_ptr)[0];
            break;
        case CONFIG_BOOLEAN:
            value = reinterpret_cast<bool *>(variable->data_ptr)[0];
            break;
        case CONFIG_UNSIGNED_INT:
            value = !!reinterpret_cast<unsigned int *>(variable->data_ptr)[0];
            break;
    }

    switch(variable->setting_type) {
        case SettingType::Checkbox:
            ImGui::Checkbox(variable->lang_title.c_str(), &value);
            settings_tooltip(variable->lang_tooltip);
            break;
    }

    switch(variable->void_type) {
        case CONFIG_INT:
            reinterpret_cast<int *>(variable->data_ptr)[0] = value;
            break;
        case CONFIG_BOOLEAN:
            reinterpret_cast<bool *>(variable->data_ptr)[0] = value;
            break;
        case CONFIG_UNSIGNED_INT:
            reinterpret_cast<unsigned int *>(variable->data_ptr)[0] = value;
            break;
    }
}

static void layout_float(const SettingsVariable *variable)
{
    float value;

    switch(variable->void_type) {
        case CONFIG_INT:
            value = reinterpret_cast<int *>(variable->data_ptr)[0];
            break;
        case CONFIG_FLOAT:
            value = reinterpret_cast<float *>(variable->data_ptr)[0];
            break;
        case CONFIG_UNSIGNED_INT:
            value = reinterpret_cast<unsigned int *>(variable->data_ptr)[0];
            break;
    }

    switch(variable->setting_type) {
        case SettingType::Slider:
            ImGui::SliderFloat(variable->lang_title.c_str(), &value, variable->slider_limit.x, variable->slider_limit.y, variable->slider_fmt.c_str());
            settings_tooltip(variable->lang_tooltip);
            break;
    }

    switch(variable->void_type) {
        case CONFIG_INT:
            reinterpret_cast<int *>(variable->data_ptr)[0] = value;
            break;
        case CONFIG_FLOAT:
            reinterpret_cast<float *>(variable->data_ptr)[0] = value;
            break;
        case CONFIG_UNSIGNED_INT:
            reinterpret_cast<unsigned int *>(variable->data_ptr)[0] = value;
            break;
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

    if(variable->setting_type == SettingType::LangSelect) {
        const auto current_lang = lang::current();

        if(ImGui::BeginCombo(variable->lang_title.c_str(), current_lang->display.c_str())) {
            for(auto it = lang::cbegin(); it != lang::cend(); ++it) {
                if(ImGui::Selectable(it->display.c_str(), (it == current_lang))) {
                    lang::set(it);
                    continue;
                }
            }

            ImGui::EndCombo();
        }

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

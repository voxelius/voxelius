// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <algorithm>
#include <client/event/glfw_key.hh>
#include <client/event/language_set.hh>
#include <client/globals.hh>
#include <client/key_name.hh>
#include <client/lang.hh>
#include <client/settings.hh>
#include <client/ui_screen.hh>
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

constexpr static ImGuiWindowFlags MENU_FLAGS = ImGuiWindowFlags_NoBackground |
    ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
    ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar;

enum class SettingValueType {
    Checkbox        = 0x0000,
    FloatSlider     = 0x0001,
    IntInput        = 0x0002,
    IntSlider       = 0x0003,
    LanguageSelect  = 0x0004,
    TextInput       = 0x0005,
    UintInput       = 0x0006,
    UintSlider      = 0x0007,
    KeyBind         = 0x0008,
};

struct SettingValue {
    static void layout_tooltip(const SettingValue *value);
    virtual ~SettingValue(void) = default;
    SettingValueType type {};
    std::string str_title_noui {};
    std::string str_title_ui {};
    std::string str_tooltip {};
    std::string name {};
    int priority {};
    bool tooltip {};
};

struct SettingValue_Checkbox final : public SettingValue {
    static void layout(const SettingValue_Checkbox *value);
    virtual ~SettingValue_Checkbox(void) = default;
    bool *value_ptr {};
};

struct SettingValue_FloatSlider final : public SettingValue {
    static void layout(const SettingValue_FloatSlider *value);
    virtual ~SettingValue_FloatSlider(void) = default;
    std::string format {};
    float *value_ptr {};
    float value_min {};
    float value_max {};
};

struct SettingValue_IntInput final : public SettingValue {
    static void layout(const SettingValue_IntInput *value);
    virtual ~SettingValue_IntInput(void) = default;
    int *value_ptr {};
};

struct SettingValue_IntSlider final : public SettingValue {
    static void layout(const SettingValue_IntSlider *value);
    virtual ~SettingValue_IntSlider(void) = default;
    int *value_ptr {};
    int value_min {};
    int value_max {};
};

struct SettingValue_LanguageSelect final : public SettingValue {
    static void layout(const SettingValue_LanguageSelect *value);
    virtual ~SettingValue_LanguageSelect(void) = default;
};

struct SettingValue_TextInput final : public SettingValue {
    static void layout(const SettingValue_TextInput *value);
    virtual ~SettingValue_TextInput(void) = default;
    ImGuiInputTextFlags flags {};
    std::string *value_ptr {};
};

struct SettingValue_UintInput final : public SettingValue {
    static void layout(const SettingValue_UintInput *value);
    virtual ~SettingValue_UintInput(void) = default;
    unsigned int *value_ptr {};
};

struct SettingValue_UintSlider final : public SettingValue {
    static void layout(const SettingValue_UintSlider *value);
    virtual ~SettingValue_UintSlider(void) = default;
    unsigned int *value_ptr {};
    unsigned int value_min {};
    unsigned int value_max {};
};

struct SettingValue_KeyBind final : public SettingValue {
    static void layout(const SettingValue_KeyBind *value);
    virtual ~SettingValue_KeyBind(void) = default;
    int *value_ptr {};
};

static std::string str_general = {};
static std::string str_controls = {};
static std::string str_controls_keyboard = {};
static std::string str_controls_keyboard_movement = {};
static std::string str_controls_keyboard_gameplay = {};
static std::string str_controls_keyboard_misc = {};
static std::string str_controls_mouse = {};
static std::string str_controls_gamepad = {};
static std::string str_video_gui = {};
static std::string str_video = {};
static std::string str_sound = {};

static std::vector<SettingValue *> values_all = {};
static std::vector<SettingValue *> values[settings::NUM_LOCATIONS] = {};

void SettingValue::layout_tooltip(const SettingValue *value)
{
    if(value->tooltip) {
        ImGui::SameLine();
        ImGui::TextDisabled("[?]");

        if(ImGui::BeginItemTooltip()) {
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 16.0f);
            ImGui::TextUnformatted(value->str_tooltip.c_str());
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }
}

void SettingValue_Checkbox::layout(const SettingValue_Checkbox *value)
{
    ImGui::Checkbox(value->str_title_ui.c_str(), value->value_ptr);
    SettingValue::layout_tooltip(value);
}

void SettingValue_FloatSlider::layout(const SettingValue_FloatSlider *value)
{
    ImGui::SliderFloat(value->str_title_ui.c_str(), value->value_ptr, value->value_min, value->value_max, value->format.c_str());
    SettingValue::layout_tooltip(value);
}

void SettingValue_IntInput::layout(const SettingValue_IntInput *value)
{
    ImGui::InputInt(value->str_title_ui.c_str(), value->value_ptr);
    SettingValue::layout_tooltip(value);
}

void SettingValue_IntSlider::layout(const SettingValue_IntSlider *value)
{
    ImGui::SliderInt(value->str_title_ui.c_str(), value->value_ptr, value->value_min, value->value_max);
    SettingValue::layout_tooltip(value);
}

void SettingValue_LanguageSelect::layout(const SettingValue_LanguageSelect *value)
{
    const LangIterator current = lang::current();

    if(ImGui::BeginCombo(value->str_title_ui.c_str(), current->display.c_str())) {
        for(LangIterator it = lang::cbegin(); it != lang::cend(); ++it) {
            if(ImGui::Selectable(it->display.c_str(), it == current)) {
                lang::set(it);
                continue;
            }
        }

        ImGui::EndCombo();
    }

    SettingValue::layout_tooltip(value);
}

void SettingValue_TextInput::layout(const SettingValue_TextInput *value)
{
    ImGui::InputText(value->str_title_ui.c_str(), value->value_ptr, value->flags);
    SettingValue::layout_tooltip(value);
}

void SettingValue_UintInput::layout(const SettingValue_UintInput *value)
{
    ImGui::InputScalar(value->str_title_ui.c_str(), ImGuiDataType_U32, value->value_ptr);
    SettingValue::layout_tooltip(value);
}

void SettingValue_UintSlider::layout(const SettingValue_UintSlider *value)
{
    ImGui::SliderScalar(value->str_title_ui.c_str(), ImGuiDataType_U32, value->value_ptr, &value->value_min, &value->value_max);
    SettingValue::layout_tooltip(value);
}

void SettingValue_KeyBind::layout(const SettingValue_KeyBind *value)
{
    std::string label = {};

    if(globals::ui_keybind_ptr == value->value_ptr)
        label = fmt::format("...###{}", static_cast<const void *>(value->value_ptr));
    else label = fmt::format("{}###{}", key_name::get(value->value_ptr[0]), static_cast<const void *>(value->value_ptr));

    if(ImGui::Button(label.c_str(), ImVec2(ImGui::GetWindowSize().x * 0.25f, 0.0f)))
        globals::ui_keybind_ptr = value->value_ptr;
    ImGui::SameLine();
    ImGui::TextUnformatted(value->str_title_noui.c_str());

    SettingValue::layout_tooltip(value);
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
        
        if((event.key == GLFW_KEY_ESCAPE) && (globals::ui_screen == UI_SETTINGS)) {
            globals::ui_screen = UI_MAIN_MENU;
            return;
        }
    }
}

static void on_language_set(const LanguageSetEvent &event)
{
    for(SettingValue *value : values_all) {
        value->str_title_noui = lang::resolve(fmt::format("settings.value.{}", value->name));
        value->str_title_ui = lang::resolve_ui(fmt::format("settings.value.{}", value->name));
        value->str_tooltip = lang::resolve(fmt::format("settings.tooltip.{}", value->name));
    }

    str_general = lang::resolve_ui("settings.general");

    str_controls = lang::resolve_ui("settings.controls");
    str_controls_keyboard = lang::resolve_ui("settings.controls.keyboard");
    str_controls_keyboard_movement = lang::resolve_ui("settings.controls.keyboard.movement");
    str_controls_keyboard_gameplay = lang::resolve_ui("settings.controls.keyboard.gameplay");
    str_controls_keyboard_misc = lang::resolve_ui("settings.controls.keyboard.miscellaneous");
    str_controls_mouse = lang::resolve_ui("settings.controls.mouse");
    str_controls_gamepad = lang::resolve_ui("settings.controls.gamepad");

    str_video = lang::resolve_ui("settings.video");
    str_video_gui = lang::resolve_ui("settings.video.gui");

    str_sound = lang::resolve_ui("settings.sound");
}

static void layout_values(std::size_t location)
{
    for(const SettingValue *value : values[location]) {
        switch(value->type) {
            case SettingValueType::Checkbox:
                SettingValue_Checkbox::layout(static_cast<const SettingValue_Checkbox *>(value));
                break;
            case SettingValueType::FloatSlider:
                SettingValue_FloatSlider::layout(static_cast<const SettingValue_FloatSlider *>(value));
                break;
            case SettingValueType::IntInput:
                SettingValue_IntInput::layout(static_cast<const SettingValue_IntInput *>(value));
                break;
            case SettingValueType::IntSlider:
                SettingValue_IntSlider::layout(static_cast<const SettingValue_IntSlider *>(value));
                break;
            case SettingValueType::LanguageSelect:
                SettingValue_LanguageSelect::layout(static_cast<const SettingValue_LanguageSelect *>(value));
                break;
            case SettingValueType::TextInput:
                SettingValue_TextInput::layout(static_cast<const SettingValue_TextInput *>(value));
                break;
            case SettingValueType::UintInput:
                SettingValue_UintInput::layout(static_cast<const SettingValue_UintInput *>(value));
                break;
            case SettingValueType::UintSlider:
                SettingValue_UintSlider::layout(static_cast<const SettingValue_UintSlider *>(value));
                break;
            case SettingValueType::KeyBind:
                SettingValue_KeyBind::layout(static_cast<const SettingValue_KeyBind *>(value));
                break;
        }
    }
}

void settings::init(void)
{
    globals::dispatcher.sink<GlfwKeyEvent>().connect<&on_glfw_key>();
    globals::dispatcher.sink<LanguageSetEvent>().connect<&on_language_set>();
}

void settings::init_late(void)
{
    for(std::size_t location = 0; location < settings::NUM_LOCATIONS; ++location) {
        std::sort(values[location].begin(), values[location].end(), [](SettingValue *a, SettingValue *b) {
            return a->priority < b->priority;
        });
    }
}

void settings::deinit(void)
{
    for(SettingValue *value : values_all)
        delete value;
    for(std::size_t location = 0; location < settings::NUM_LOCATIONS; ++location)
        values[location].clear();
    values_all.clear();
}

void settings::layout(void)
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
                globals::ui_screen = UI_MAIN_MENU;
                globals::ui_keybind_ptr = nullptr;
            }

            if(ImGui::BeginTabItem(str_general.c_str())) {
                globals::ui_keybind_ptr = nullptr;

                if(ImGui::BeginChild("###settings.general.child")) {
                    layout_values(settings::GENERAL);
                }

                ImGui::EndChild();
                ImGui::EndTabItem();
            }

            if(ImGui::BeginTabItem(str_controls.c_str())) {
                if(ImGui::BeginChild("###settings.controls.child")) {
                    ImGui::SeparatorText(str_controls_keyboard.c_str());

                    if(ImGui::CollapsingHeader(str_controls_keyboard_movement.c_str()))
                        layout_values(settings::CONTROLS_KEYBOARD_MOVEMENT);
                    if(ImGui::CollapsingHeader(str_controls_keyboard_gameplay.c_str()))
                        layout_values(settings::CONTROLS_KEYBOARD_GAMEPLAY);
                    if(ImGui::CollapsingHeader(str_controls_keyboard_misc.c_str()))
                        layout_values(settings::CONTROLS_KEYBOARD_MISC);

                    ImGui::SeparatorText(str_controls_mouse.c_str());
                    layout_values(settings::CONTROLS_MOUSE);

                    ImGui::SeparatorText(str_controls_gamepad.c_str());
                    layout_values(settings::CONTROLS_GAMEPAD);
                }

                ImGui::EndChild();
                ImGui::EndTabItem();
            }

            if(ImGui::BeginTabItem(str_video.c_str())) {
                globals::ui_keybind_ptr = nullptr;

                if(ImGui::BeginChild("###settings.video.child")) {
                    layout_values(settings::VIDEO);

                    ImGui::SeparatorText(str_video_gui.c_str());
                    layout_values(settings::VIDEO_GUI);
                }

                ImGui::EndChild();
                ImGui::EndTabItem();
            }

            if(ImGui::BeginTabItem(str_sound.c_str())) {
                globals::ui_keybind_ptr = nullptr;

                if(ImGui::BeginChild("###settings.sound.child")) {
                    layout_values(settings::SOUND);
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

void settings::add_checkbox(int priority, std::size_t location, const std::string &name, bool &vref, bool tooltip)
{
    SettingValue_Checkbox *value = new SettingValue_Checkbox();
    value->type = SettingValueType::Checkbox;
    value->tooltip = tooltip;
    value->value_ptr = &vref;
    value->priority = priority;
    value->name = name;

    values[location].push_back(value);
    values_all.push_back(value);
}

void settings::add_input(int priority, std::size_t location, const std::string &name, int &vref, bool tooltip)
{
    SettingValue_IntInput *value = new SettingValue_IntInput();
    value->type = SettingValueType::IntInput;
    value->tooltip = tooltip;
    value->value_ptr = &vref;
    value->priority = priority;
    value->name = name;

    values[location].push_back(value);
    values_all.push_back(value);
}

void settings::add_input(int priority, std::size_t location, const std::string &name, std::string &vref, bool tooltip, bool allow_whitespace)
{
    SettingValue_TextInput *value = new SettingValue_TextInput();
    value->type = SettingValueType::TextInput;
    value->tooltip = tooltip;
    value->value_ptr = &vref;
    value->priority = priority;
    value->name = name;

    if(!allow_whitespace)
        value->flags |= ImGuiInputTextFlags_CharsNoBlank;
    else value->flags = 0;

    values[location].push_back(value);
    values_all.push_back(value);

}

void settings::add_input(int priority, std::size_t location, const std::string &name, unsigned int &vref, bool tooltip)
{
    SettingValue_UintInput *value = new SettingValue_UintInput();
    value->type = SettingValueType::UintInput;
    value->tooltip = tooltip;
    value->value_ptr = &vref;
    value->priority = priority;
    value->name = name;

    values[location].push_back(value);
    values_all.push_back(value);
}

void settings::add_key_binding(int priority, std::size_t location, const std::string &name, int &vref)
{
    SettingValue_KeyBind *value = new SettingValue_KeyBind();
    value->type = SettingValueType::KeyBind;
    value->value_ptr = &vref;
    value->priority = priority;
    value->name = name;

    values[location].push_back(value);
    values_all.push_back(value);
}

void settings::add_language_select(int priority, std::size_t location, const std::string &name)
{
    SettingValue_LanguageSelect *value = new SettingValue_LanguageSelect();
    value->type = SettingValueType::LanguageSelect;
    value->priority = priority;
    value->name = name;

    values[location].push_back(value);
    values_all.push_back(value);
}

void settings::add_slider(int priority, std::size_t location, const std::string &name, float &vref, float min, float max, bool tooltip, const char *format)
{
    SettingValue_FloatSlider *value = new SettingValue_FloatSlider();
    value->type = SettingValueType::FloatSlider;
    value->tooltip = tooltip;
    value->value_ptr = &vref;
    value->value_min = min;
    value->value_max = max;
    value->priority = priority;
    value->name = name;

    if(format)
        value->format = std::string(format);
    else value->format = std::string("%.03f");

    values[location].push_back(value);
    values_all.push_back(value);
}

void settings::add_slider(int priority, std::size_t location, const std::string &name, int &vref, int min, int max, bool tooltip)
{
    SettingValue_IntSlider *value = new SettingValue_IntSlider();
    value->type = SettingValueType::IntSlider;
    value->tooltip = tooltip;
    value->value_ptr = &vref;
    value->value_min = min;
    value->value_max = max;
    value->priority = priority;
    value->name = name;

    values[location].push_back(value);
    values_all.push_back(value);
}

void settings::add_slider(int priority, std::size_t location, const std::string &name, unsigned int &vref, unsigned int min, unsigned int max, bool tooltip)
{
    SettingValue_UintSlider *value = new SettingValue_UintSlider();
    value->type = SettingValueType::UintSlider;
    value->tooltip = tooltip;
    value->value_ptr = &vref;
    value->value_min = min;
    value->value_max = max;
    value->priority = priority;
    value->name = name;

    values[location].push_back(value);
    values_all.push_back(value);
}

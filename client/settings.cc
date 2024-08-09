// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <algorithm>
#include <client/event/glfw_key.hh>
#include <client/event/language_set.hh>
#include <client/language.hh>
#include <client/screen.hh>
#include <client/settings.hh>
#include <client/globals.hh>
#include <client/keynames.hh>
#include <entt/signal/dispatcher.hpp>
#include <imgui.h>
#include <imgui_stdlib.h>
#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>
#include <unordered_map>
#include <vector>

constexpr static ImGuiWindowFlags MENU_FLAGS = ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration;

enum class SettingValueType {
    Checkbox        = 0x0000,
    FloatSlider     = 0x0001,
    IntInput        = 0x0002,
    IntSlider       = 0x0003,
    LanguageSelect  = 0x0004,
    TextInput       = 0x0005,
    UintInput       = 0x0006,
    UintSlider      = 0x0007,
    UintStepper     = 0x0008,
    KeyBind         = 0x0009,
};

struct SettingValue {
public:
    static void layout_tooltip(const SettingValue *value);
    static void layout_label(const SettingValue *value);
    virtual ~SettingValue(void) = default;
    SettingValueType value_type {};
    std::string str_tooltip {};
    std::string str_title {};
    int sorting_priority {};
    bool has_tooltip {};
    std::string name {};
};

struct SettingValueWID : public SettingValue {
    virtual ~SettingValueWID(void) = default;
    std::string wid {};
};

struct SettingValue_Checkbox final : public SettingValue {
    static void refresh_wids(SettingValue_Checkbox *value);
    static void layout(const SettingValue_Checkbox *value);
    virtual ~SettingValue_Checkbox(void) = default;
    std::string wid[2] {};
    bool *value_ptr {};
};

struct SettingValue_FloatSlider final : public SettingValueWID {
    static void layout(const SettingValue_FloatSlider *value);
    virtual ~SettingValue_FloatSlider(void) = default;
    std::string format {};
    float *value_ptr {};
    float value_min {};
    float value_max {};
};

struct SettingValue_IntInput final : public SettingValueWID {
    static void layout(const SettingValue_IntInput *value);
    virtual ~SettingValue_IntInput(void) = default;
    int *value_ptr {};
};

struct SettingValue_IntSlider final : public SettingValueWID {
    static void layout(const SettingValue_IntSlider *value);
    virtual ~SettingValue_IntSlider(void) = default;
    int *value_ptr {};
    int value_min {};
    int value_max {};
};

struct SettingValue_LanguageSelect final : public SettingValueWID {
    static void layout(const SettingValue_LanguageSelect *value);
    virtual ~SettingValue_LanguageSelect(void) = default;
};

struct SettingValue_TextInput final : public SettingValueWID {
    static void layout(const SettingValue_TextInput *value);
    virtual ~SettingValue_TextInput(void) = default;
    ImGuiInputTextFlags flags {};
    std::string *value_ptr {};
};

struct SettingValue_UintInput final : public SettingValueWID {
    static void layout(const SettingValue_UintInput *value);
    virtual ~SettingValue_UintInput(void) = default;
    unsigned int *value_ptr {};
};

struct SettingValue_UintSlider final : public SettingValueWID {
    static void layout(const SettingValue_UintSlider *value);
    virtual ~SettingValue_UintSlider(void) = default;
    unsigned int *value_ptr {};
    unsigned int value_min {};
    unsigned int value_max {};
};

struct SettingValue_UintStepper final : public SettingValue {
    static void refresh_wids(SettingValue_UintStepper *value);
    static void layout(const SettingValue_UintStepper *value);
    virtual ~SettingValue_UintStepper(void) = default;
    std::vector<std::string> wids {};
    unsigned int *value_ptr {};
    unsigned int value_max {};
};

struct SettingValue_KeyBind final : public SettingValue {
    static void layout(const SettingValue_KeyBind *value);
    static void refresh_wids(SettingValue_KeyBind *value);
    virtual ~SettingValue_KeyBind(void) = default;
    std::string wid[2] {};
    int *value_ptr {};
};

static std::string str_checkbox_true = {};
static std::string str_checkbox_false = {};
static std::string str_general = {};
static std::string str_keyboard = {};
static std::string str_keyboard_movement = {};
static std::string str_keyboard_gameplay = {};
static std::string str_keyboard_misc = {};
static std::string str_mouse = {};
static std::string str_video = {};
static std::string str_video_gui = {};
static std::string str_sound = {};

static std::vector<SettingValue *> values_all = {};
static std::vector<SettingValue *> values[settings::NUM_LOCATIONS] = {};

void SettingValue::layout_tooltip(const SettingValue *value)
{
    if(value->has_tooltip) {
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

void SettingValue::layout_label(const SettingValue *value)
{
    ImGui::SameLine();
    ImGui::TextUnformatted(value->str_title.c_str());
}

void SettingValue_Checkbox::refresh_wids(SettingValue_Checkbox *value)
{
    value->wid[0] = fmt::format("{}###{}", str_checkbox_false, static_cast<void *>(value));
    value->wid[1] = fmt::format("{}###{}", str_checkbox_true, static_cast<void *>(value));    
}

void SettingValue_Checkbox::layout(const SettingValue_Checkbox *value)
{
    const std::string &wid = value->value_ptr[0] ? value->wid[1] : value->wid[0];
    if(ImGui::Button(wid.c_str(), ImVec2(ImGui::CalcItemWidth(), 0.0f)))
        value->value_ptr[0] = !value->value_ptr[0];
    SettingValue::layout_label(value);
    SettingValue::layout_tooltip(value);
}

void SettingValue_FloatSlider::layout(const SettingValue_FloatSlider *value)
{
    ImGui::SliderFloat(value->wid.c_str(), value->value_ptr, value->value_min, value->value_max, value->format.c_str());
    SettingValue::layout_label(value);
    SettingValue::layout_tooltip(value);
}

void SettingValue_IntInput::layout(const SettingValue_IntInput *value)
{
    ImGui::InputInt(value->wid.c_str(), value->value_ptr);
    SettingValue::layout_label(value);
    SettingValue::layout_tooltip(value);
}

void SettingValue_IntSlider::layout(const SettingValue_IntSlider *value)
{
    ImGui::SliderInt(value->wid.c_str(), value->value_ptr, value->value_min, value->value_max);
    SettingValue::layout_label(value);
    SettingValue::layout_tooltip(value);
}

void SettingValue_LanguageSelect::layout(const SettingValue_LanguageSelect *value)
{
    const LangIterator current = language::current();

    if(ImGui::BeginCombo(value->wid.c_str(), current->display.c_str())) {
        for(LangIterator it = language::cbegin(); it != language::cend(); ++it) {
            if(ImGui::Selectable(it->display.c_str(), it == current)) {
                language::set(it);
                continue;
            }
        }

        ImGui::EndCombo();
    }

    SettingValue::layout_label(value);
    SettingValue::layout_tooltip(value);
}

void SettingValue_TextInput::layout(const SettingValue_TextInput *value)
{
    ImGui::InputText(value->wid.c_str(), value->value_ptr, value->flags);
    SettingValue::layout_label(value);
    SettingValue::layout_tooltip(value);
}

void SettingValue_UintInput::layout(const SettingValue_UintInput *value)
{
    ImGui::InputScalar(value->wid.c_str(), ImGuiDataType_U32, value->value_ptr);
    SettingValue::layout_label(value);
    SettingValue::layout_tooltip(value);
}

void SettingValue_UintSlider::layout(const SettingValue_UintSlider *value)
{
    ImGui::SliderScalar(value->wid.c_str(), ImGuiDataType_U32, value->value_ptr, &value->value_min, &value->value_max);
    SettingValue::layout_label(value);
    SettingValue::layout_tooltip(value);
}

void SettingValue_UintStepper::refresh_wids(SettingValue_UintStepper *value)
{
    for(unsigned int i = 0; i < value->wids.size(); ++i) {
        const std::string key = fmt::format("settings.value.{}.{}", value->name, i);
        value->wids[i] = fmt::format("{}###{}", language::resolve(key), static_cast<void *>(value));
    }
}

void SettingValue_UintStepper::layout(const SettingValue_UintStepper *value)
{
    if(ImGui::Button(value->wids[value->value_ptr[0] % value->value_max].c_str(), ImVec2(ImGui::CalcItemWidth(), 0.0f)))
        value->value_ptr[0] += 1U;
    value->value_ptr[0] %= value->value_max;
    SettingValue::layout_label(value);
    SettingValue::layout_tooltip(value);
}

void SettingValue_KeyBind::layout(const SettingValue_KeyBind *value)
{
    const bool is_active = (globals::gui_keybind_ptr == value->value_ptr);
    const std::string &wid = is_active ? value->wid[0] : value->wid[1];
    if(ImGui::Button(wid.c_str(), ImVec2(ImGui::CalcItemWidth() * 0.75f, 0.0f)))
        globals::gui_keybind_ptr = value->value_ptr;
    SettingValue::layout_label(value);
    SettingValue::layout_tooltip(value);
}

void SettingValue_KeyBind::refresh_wids(SettingValue_KeyBind *value)
{
    value->wid[0] = fmt::format("...###{}", static_cast<void *>(value));
    value->wid[1] = fmt::format("{}###{}", keynames::get(value->value_ptr[0]), static_cast<void *>(value));
}

static void refresh_key_bind_wids(void)
{
    for(SettingValue *value : values_all) {
        if(value->value_type == SettingValueType::KeyBind) {
            SettingValue_KeyBind::refresh_wids(static_cast<SettingValue_KeyBind *>(value));
            continue;
        }
    }
}

static void on_glfw_key(const GlfwKeyEvent &event)
{
    if(event.action == GLFW_PRESS) {
        if(globals::gui_keybind_ptr) {
            if(event.key == GLFW_KEY_ESCAPE) {
                globals::gui_keybind_ptr = nullptr;
                return;
            }
            
            globals::gui_keybind_ptr[0] = event.key;
            globals::gui_keybind_ptr = nullptr;

            refresh_key_bind_wids();

            return;
        }
        
        if((event.key == GLFW_KEY_ESCAPE) && (globals::gui_screen == GUI_SETTINGS)) {
            globals::gui_screen = GUI_MAIN_MENU;
            return;
        }
    }
}

static void on_language_set(const LanguageSetEvent &event)
{
    str_checkbox_false = language::resolve("settings.checkbox.false");
    str_checkbox_true = language::resolve("settings.checkbox.true");
    str_general = language::resolve_ui("settings.general");
    str_keyboard = language::resolve_ui("settings.keyboard");
    str_keyboard_movement = language::resolve_ui("settings.keyboard.movement");
    str_keyboard_gameplay = language::resolve_ui("settings.keyboard.gameplay");
    str_keyboard_misc = language::resolve_ui("settings.keyboard.misc");
    str_mouse = language::resolve_ui("settings.mouse");
    str_video = language::resolve_ui("settings.video");
    str_video_gui = language::resolve_ui("settings.video.gui");
    str_sound = language::resolve_ui("settings.sound");

    for(SettingValue *value : values_all) {
        if(value->value_type == SettingValueType::Checkbox)
            SettingValue_Checkbox::refresh_wids(static_cast<SettingValue_Checkbox *>(value));
        if(value->value_type == SettingValueType::UintStepper)
            SettingValue_UintStepper::refresh_wids(static_cast<SettingValue_UintStepper *>(value));
        value->str_title = language::resolve(fmt::format("settings.value.{}", value->name));
        value->str_tooltip = language::resolve(fmt::format("settings.tooltip.{}", value->name));
    }
}

static void layout_values(std::size_t location)
{
    ImGui::PushItemWidth(ImGui::CalcItemWidth() * 0.80f);

    for(const SettingValue *value : values[location]) {
        switch(value->value_type) {
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
            case SettingValueType::UintStepper:
                SettingValue_UintStepper::layout(static_cast<const SettingValue_UintStepper *>(value));
                break;
            case SettingValueType::KeyBind:
                SettingValue_KeyBind::layout(static_cast<const SettingValue_KeyBind *>(value));
                break;
        }
    }

    ImGui::PopItemWidth();
}

static void layout_general(void)
{
    if(ImGui::BeginChild("###settings.general.child"))
        layout_values(settings::GENERAL);
    ImGui::EndChild();
}

static void layout_keyboard(void)
{
    if(ImGui::BeginChild("###settings.keyboard.child")) {
        ImGui::SeparatorText(str_keyboard_movement.c_str());
        layout_values(settings::KEYBOARD_MOVEMENT);
        ImGui::SeparatorText(str_keyboard_gameplay.c_str());
        layout_values(settings::KEYBOARD_GAMEPLAY);
        ImGui::SeparatorText(str_keyboard_misc.c_str());
        layout_values(settings::KEYBOARD_MISC);
    }

    ImGui::EndChild();
}

static void layout_mouse(void)
{
    if(ImGui::BeginChild("###settings.mouse.child"))
        layout_values(settings::MOUSE);
    ImGui::EndChild();
}

static void layout_video(void)
{
    if(ImGui::BeginChild("###settings.video.child")) {
        layout_values(settings::VIDEO);
        ImGui::SeparatorText(str_video_gui.c_str());
        layout_values(settings::VIDEO_GUI);
    }

    ImGui::EndChild();
}

static void layout_sound(void)
{
    if(ImGui::BeginChild("###settings.sound.child"))
        layout_values(settings::SOUND);
    ImGui::EndChild();
}

void settings::init(void)
{
    globals::dispatcher.sink<GlfwKeyEvent>().connect<&on_glfw_key>();
    globals::dispatcher.sink<LanguageSetEvent>().connect<&on_language_set>();
}

void settings::init_late(void)
{
    for(std::size_t location = 0; location < settings::NUM_LOCATIONS; ++location) {
        std::sort(values[location].begin(), values[location].end(), [](const SettingValue *a, const SettingValue *b) {
            return a->sorting_priority < b->sorting_priority;
        });
    }
}

void settings::deinit(void)
{
    for(const SettingValue *value : values_all)
        delete value;
    for(std::size_t location = 0; location < settings::NUM_LOCATIONS; ++location)
        values[location].clear();
    values_all.clear();
}

void settings::layout(void)
{
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const ImVec2 window_start = ImVec2(viewport->Size.x * 0.05f, viewport->Size.y * 0.05f);
    const ImVec2 window_size = ImVec2(viewport->Size.x * 0.90f, viewport->Size.y * 0.90f);

    ImGui::SetNextWindowPos(window_start);
    ImGui::SetNextWindowSize(window_size);

    if(ImGui::Begin("###settings", nullptr, MENU_FLAGS)) {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(3.0f * globals::gui_scale, 3.0f * globals::gui_scale));

        if(ImGui::BeginTabBar("###settings.tabs", ImGuiTabBarFlags_FittingPolicyResizeDown)) {
            if(ImGui::TabItemButton("<<")) {
                globals::gui_screen = GUI_MAIN_MENU;
                globals::gui_keybind_ptr = nullptr;
            }

            if(ImGui::BeginTabItem(str_general.c_str())) {
                globals::gui_keybind_ptr = nullptr;
                layout_general();
                ImGui::EndTabItem();
            }

            if(ImGui::BeginTabItem(str_keyboard.c_str())) {
                layout_keyboard();
                ImGui::EndTabItem();
            }

            if(ImGui::BeginTabItem(str_mouse.c_str())) {
                globals::gui_keybind_ptr = nullptr;
                layout_mouse();
                ImGui::EndTabItem();
            }

            if(ImGui::BeginTabItem(str_video.c_str())) {
                globals::gui_keybind_ptr = nullptr;
                layout_video();
                ImGui::EndTabItem();
            }

            if(ImGui::BeginTabItem(str_sound.c_str())) {
                globals::gui_keybind_ptr = nullptr;
                layout_sound();
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
    value->value_type = SettingValueType::Checkbox;
    value->sorting_priority = priority;
    value->has_tooltip = tooltip;
    value->value_ptr = &vref;
    value->name = name;

    SettingValue_Checkbox::refresh_wids(value);

    values[location].push_back(value);
    values_all.push_back(value);
}

void settings::add_input(int priority, std::size_t location, const std::string &name, int &vref, bool tooltip)
{
    SettingValue_IntInput *value = new SettingValue_IntInput();
    value->value_type = SettingValueType::IntInput;
    value->sorting_priority = priority;
    value->has_tooltip = tooltip;
    value->value_ptr = &vref;
    value->name = name;

    value->wid = fmt::format("###{}", static_cast<void *>(value));

    values[location].push_back(value);
    values_all.push_back(value);
}

void settings::add_input(int priority, std::size_t location, const std::string &name, std::string &vref, bool tooltip, bool allow_whitespace)
{
    SettingValue_TextInput *value = new SettingValue_TextInput();
    value->value_type = SettingValueType::TextInput;
    value->sorting_priority = priority;
    value->has_tooltip = tooltip;
    value->value_ptr = &vref;
    value->name = name;

    value->wid = fmt::format("###{}", static_cast<void *>(value));

    if(!allow_whitespace)
        value->flags |= ImGuiInputTextFlags_CharsNoBlank;
    else value->flags = 0;

    values[location].push_back(value);
    values_all.push_back(value);

}

void settings::add_input(int priority, std::size_t location, const std::string &name, unsigned int &vref, bool tooltip)
{
    SettingValue_UintInput *value = new SettingValue_UintInput();
    value->value_type = SettingValueType::UintInput;
    value->sorting_priority = priority;
    value->has_tooltip = tooltip;
    value->value_ptr = &vref;
    value->name = name;

    value->wid = fmt::format("###{}", static_cast<void *>(value));

    values[location].push_back(value);
    values_all.push_back(value);
}

void settings::add_key_binding(int priority, std::size_t location, const std::string &name, int &vref)
{
    SettingValue_KeyBind *value = new SettingValue_KeyBind();
    value->value_type = SettingValueType::KeyBind;
    value->sorting_priority = priority;
    value->value_ptr = &vref;
    value->name = name;

    SettingValue_KeyBind::refresh_wids(value);

    values[location].push_back(value);
    values_all.push_back(value);
}

void settings::add_language_select(int priority, std::size_t location, const std::string &name)
{
    SettingValue_LanguageSelect *value = new SettingValue_LanguageSelect();
    value->value_type = SettingValueType::LanguageSelect;
    value->sorting_priority = priority;
    value->name = name;

    value->wid = fmt::format("###{}", static_cast<void *>(value));

    values[location].push_back(value);
    values_all.push_back(value);
}

void settings::add_slider(int priority, std::size_t location, const std::string &name, float &vref, float min, float max, bool tooltip, const char *format)
{
    SettingValue_FloatSlider *value = new SettingValue_FloatSlider();
    value->value_type = SettingValueType::FloatSlider;
    value->sorting_priority = priority;
    value->has_tooltip = tooltip;
    value->value_ptr = &vref;
    value->value_min = min;
    value->value_max = max;
    value->name = name;

    value->wid = fmt::format("###{}", static_cast<void *>(value));

    if(format)
        value->format = std::string(format);
    else value->format = std::string("%.03f");

    values[location].push_back(value);
    values_all.push_back(value);
}

void settings::add_slider(int priority, std::size_t location, const std::string &name, int &vref, int min, int max, bool tooltip)
{
    SettingValue_IntSlider *value = new SettingValue_IntSlider();
    value->value_type = SettingValueType::IntSlider;
    value->sorting_priority = priority;
    value->has_tooltip = tooltip;
    value->value_ptr = &vref;
    value->value_min = min;
    value->value_max = max;
    value->name = name;

    value->wid = fmt::format("###{}", static_cast<void *>(value));

    values[location].push_back(value);
    values_all.push_back(value);
}

void settings::add_slider(int priority, std::size_t location, const std::string &name, unsigned int &vref, unsigned int min, unsigned int max, bool tooltip)
{
    SettingValue_UintSlider *value = new SettingValue_UintSlider();
    value->value_type = SettingValueType::UintSlider;
    value->sorting_priority = priority;
    value->has_tooltip = tooltip;
    value->value_ptr = &vref;
    value->value_min = min;
    value->value_max = max;
    value->name = name;

    value->wid = fmt::format("###{}", static_cast<void *>(value));

    values[location].push_back(value);
    values_all.push_back(value);
}

void settings::add_stepper(int priority, std::size_t location, const std::string &name, unsigned int &vref, unsigned int count, bool tooltip)
{
    SettingValue_UintStepper *value = new SettingValue_UintStepper();
    value->value_type = SettingValueType::UintStepper;
    value->sorting_priority = priority;
    value->has_tooltip = tooltip;
    value->value_ptr = &vref;
    value->value_max = count;
    value->name = name;

    value->wids.resize(count);

    values[location].push_back(value);
    values_all.push_back(value);
}

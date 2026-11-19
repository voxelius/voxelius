#include "client/pch.hh"

#include "client/settings.hh"

#include "client/globals.hh"
#include "client/gui/background.hh"
#include "client/gui/button_bind.hh"
#include "client/gui/container.hh"
#include "client/gui/panel.hh"
#include "client/gui/popup.hh"
#include "client/gui/rail.hh"
#include "client/gui/screen.hh"
#include "client/gui/scroller.hh"
#include "client/gui/separator.hh"
#include "client/language.hh"
#include "client/main_menu.hh"

gui::Screen settings::screen;

gui::Container settings::general;
gui::Container settings::controls_movement;
gui::Container settings::controls_gameplay;
gui::Container settings::controls_miscellaneous;
gui::Container settings::mouse;
gui::Container settings::video;
gui::Container settings::video_lod;
gui::Container settings::video_atmosphere;
gui::Container settings::video_gui;
gui::Container settings::sound;
gui::Container settings::sound_levels;

static gui::Scroller s_scroller_general;
static gui::Scroller s_scroller_mouse;
static gui::Scroller s_scroller_video;
static gui::Scroller s_scroller_audio;
static gui::Scroller s_scroller_controls;

static gui::Separator s_header_video_lod;
static gui::Separator s_header_video_atmosphere;
static gui::Separator s_header_video_gui;
static gui::Separator s_header_sound_levels;
static gui::Separator s_header_controls_gameplay;
static gui::Separator s_header_controls_miscellaneous;

static gui::Rail s_rail;
static gui::Panel s_body;

static gui::ChoicePopup s_video_mode_popup;
static std::optional<bool> s_video_mode_decision;
static bool s_video_mode_prompted;

constexpr static float BODY_MARGIN = 16.0f;

static void on_language_update(const LanguageUpdateEvent& event)
{
    settings::screen.translate();
}

static void on_keyboard_event(const SDL_KeyboardEvent& event)
{
    auto change_screen = true;
    change_screen = change_screen && event.type == SDL_EVENT_KEY_DOWN && event.key == SDLK_ESCAPE;
    change_screen = change_screen && globals::gui_screen == &settings::screen;
    change_screen = change_screen && gui::ButtonBind::current == nullptr;

    if(change_screen) {
        main_menu::show();
    }
}

std::optional<bool> settings::detail::video_mode_popup(const std::optional<SDL_DisplayMode>& mode)
{
    if(!s_video_mode_prompted) {
        s_video_mode_prompted = true;
        s_video_mode_decision.reset();
        s_video_mode_popup.open();
    }

    if(s_video_mode_decision.has_value()) {
        auto decision = s_video_mode_decision;
        s_video_mode_prompted = false;
        s_video_mode_decision.reset();
        return decision;
    }

    return std::nullopt;
}

void settings::init(void)
{
    screen.add_child(main_menu::background);
    screen.add_child(s_body);

    s_body.set_margin(ImVec2(BODY_MARGIN, BODY_MARGIN));
    s_body.enable_background();

    s_header_video_lod.set_text("settings.label.lod");
    s_header_video_atmosphere.set_text("settings.label.atmosphere");
    s_header_video_gui.set_text("settings.label.gui");
    s_header_sound_levels.set_text("settings.label.levels");
    s_header_controls_gameplay.set_text("settings.label.gameplay");
    s_header_controls_miscellaneous.set_text("settings.label.miscellaneous");

    s_scroller_general.add_child(general, 0);

    s_scroller_mouse.add_child(mouse, 0);

    s_scroller_video.add_child(video, 0);
    s_scroller_video.add_child(s_header_video_lod, 1);
    s_scroller_video.add_child(video_lod, 2);
    s_scroller_video.add_child(s_header_video_atmosphere, 3);
    s_scroller_video.add_child(video_atmosphere, 4);
    s_scroller_video.add_child(s_header_video_gui, 5);
    s_scroller_video.add_child(video_gui, 6);

    s_scroller_audio.add_child(sound, 0);
    s_scroller_audio.add_child(s_header_sound_levels, 1);
    s_scroller_audio.add_child(sound_levels, 2);

    s_scroller_controls.add_child(controls_movement, 0);
    s_scroller_controls.add_child(s_header_controls_gameplay, 1);
    s_scroller_controls.add_child(controls_gameplay, 2);
    s_scroller_controls.add_child(s_header_controls_miscellaneous, 3);
    s_scroller_controls.add_child(controls_miscellaneous, 4);

    s_rail.add_page("settings.location.general", s_scroller_general, 0);
    s_rail.add_page("settings.location.video", s_scroller_video, 1);
    s_rail.add_page("settings.location.audio", s_scroller_audio, 2);
    s_rail.add_page("settings.location.controls", s_scroller_controls, 3);
    s_rail.add_page("settings.location.mouse", s_scroller_mouse, 4);

    s_rail.set_callback([] {
        main_menu::show();
    });

    s_body.add_child(s_rail);

    s_video_mode_popup.set_title("settings.popup.video_change.title");
    s_video_mode_popup.set_message("settings.popup.video_change.text");

    s_video_mode_popup.add_choice("settings.popup.video_change.answer.yes", [] {
        s_video_mode_decision = true;
    });

    s_video_mode_popup.add_choice("settings.popup.video_change.answer.no", [] {
        s_video_mode_decision = false;
    });

    screen.add_child(s_video_mode_popup);

    globals::dispatcher.sink<LanguageUpdateEvent>().connect<&on_language_update>();
    globals::dispatcher.sink<SDL_KeyboardEvent>().connect<&on_keyboard_event>();
}

void settings::shutdown(void)
{
    // empty
}

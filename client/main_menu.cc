#include "client/pch.hh"

#include "client/main_menu.hh"

#include "core/exception.hh"
#include "core/res/resource.hh"
#include "core/version.hh"

#include "shared/mod_context.hh"
#include "shared/mod_loader.hh"
#include "shared/net/packet_session.hh"
#include "shared/splash.hh"
#include "shared/world/world.hh"

#include "client/console.hh"
#include "client/constant.hh"
#include "client/globals.hh"
#include "client/gui/background.hh"
#include "client/gui/dimmer.hh"
#include "client/gui/menu.hh"
#include "client/gui/popup.hh"
#include "client/gui/screen.hh"
#include "client/gui/title.hh"
#include "client/language.hh"
#include "client/net/session.hh"
#include "client/play_menu.hh"
#include "client/res/texture2D.hh"
#include "client/settings.hh"

gui::Screen main_menu::screen;
gui::Background main_menu::background;

static res::handle<Texture2D> s_title_texture;
static gui::ChoicePopup s_popup_mods;
static gui::ChoicePopup s_popup_quit;
static gui::Menu s_menu;
static gui::Title s_title;
static std::string s_splash {};

static void on_language_update(const LanguageUpdateEvent& event)
{
    main_menu::screen.translate();
}

static void on_keyboard_event(const SDL_KeyboardEvent& event)
{
    auto& registry = globals::world->registry();

    if(globals::gui_screen == &console::screen) {
        return;
    }

    if(event.type == SDL_EVENT_KEY_DOWN && event.key == SDLK_ESCAPE && registry.valid(globals::player)) {
        if(globals::gui_screen) {
            globals::gui_screen = nullptr;
        }
        else {
            main_menu::show();
        }
    }
}

void main_menu::init(void)
{
    background.set_left({ 0.000f, 0.000f, 0.000f, 0.550f });
    background.set_right({ 0.000f, 0.000f, 0.000f, 0.150f });
    background.enable_scene();
    screen.add_child(background);

    auto title_id = Identifier::from_parts(constant::BUILTIN_NAME_SPACE, "menu_title.png");
    s_title_texture = res::load<Texture2D>(title_id, "textures/gui");
    vx::throw_if_not(s_title_texture.get());

    s_title.set_texture(s_title_texture->imgui,
        ImVec2(static_cast<float>(s_title_texture->width), static_cast<float>(s_title_texture->height)));
    s_title.set_version(std::format("Voxelius {}", version::semantic));
    screen.add_child(s_title);

    s_popup_mods.set_title("main_menu.mods_popup.title");
    s_popup_mods.set_message("main_menu.mods_popup.message");
    s_popup_mods.add_choice("main_menu.mods_popup.choice.ok");
    screen.add_child(s_popup_mods);

    s_popup_quit.set_title("main_menu.quit_popup.title");
    s_popup_quit.set_message("main_menu.quit_popup.question");
    s_popup_quit.add_choice("main_menu.quit_popup.choice.yes", [] {
        // We don't really have a good way to
        // pass "i want to quit" boolean to the main loop,
        // so instead we just raise an external interrupt signal
        // which handler latches an internal flag in the main loop
        std::raise(SIGINT);
    });

    s_popup_quit.add_choice("main_menu.quit_popup.choice.no");

    screen.add_child(s_popup_quit);

    s_menu.set_control(ImVec2(200.0f, 20.0f));

    // The block hangs below the anchor, so a little above the middle keeps it
    // close to centered while leaving room for the wordmark and the splash
    s_menu.set_anchor(0.5f, 0.40f);

    s_menu.add_button_ingame("main_menu.button.resume", [] {
        globals::gui_screen = nullptr;
    });

    s_menu.add_button_offline("main_menu.button.play", [] {
        globals::gui_screen = &play_menu::screen;
    });

    s_menu.add_button_any("main_menu.button.settings", [] {
        globals::gui_screen = &settings::screen;
    });

    s_menu.add_button_ingame("main_menu.button.disconnect", [] {
        session::disconnect(packet::Session_Disconnect::CLIENT_DISCONNECT);

        globals::gui_screen = &play_menu::screen;
    });

    s_menu.add_button_offline("main_menu.button.quit", [] {
        s_popup_quit.open();
    });

    screen.add_child(s_menu);

    globals::dispatcher.sink<LanguageUpdateEvent>().connect<&on_language_update>();
    globals::dispatcher.sink<SDL_KeyboardEvent>().connect<&on_keyboard_event>();
}

void main_menu::init_late(void)
{
    if(std::ranges::any_of(mod_loader::all(), std::bind_front(std::equal_to {}, mod_status::FAILED), &ModContext::status)) {
        s_popup_mods.open();
    }

    main_menu::show();
}

void main_menu::show(void)
{
    s_splash = splash::get();
    s_title.set_splash(s_splash);

    globals::gui_screen = &screen;
}

void main_menu::shutdown(void)
{
    s_title.set_texture(0, ImVec2(0.0f, 0.0f));

    s_title_texture.reset();
}

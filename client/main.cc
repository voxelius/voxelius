#include "client/pch.hh"

#include "core/core.hh"
#include "core/cvar/bits.hh"
#include "core/cvar/host.hh"
#include "core/exception.hh"
#include "core/res/resource.hh"
#include "core/threading.hh"
#include "core/utils/epoch.hh"
#include "core/version.hh"

#include "shared/cvar/commands.hh"
#include "shared/game.hh"
#include "shared/splash.hh"

#include "client/api/libraries.hh"
#include "client/boot.hh"
#include "client/camera.hh"
#include "client/console.hh"
#include "client/fog.hh"
#include "client/fonts.hh"
#include "client/game.hh"
#include "client/globals.hh"
#include "client/gui/button_bind.hh"
#include "client/gui/screen.hh"
#include "client/gui/stone.hh"
#include "client/gui/widget.hh"
#include "client/head.hh"
#include "client/input/axes.hh"
#include "client/input/buttons.hh"
#include "client/language.hh"
#include "client/loading.hh"
#include "client/main_menu.hh"
#include "client/net/bother.hh"
#include "client/net/host.hh"
#include "client/net/receive.hh"
#include "client/net/replication.hh"
#include "client/net/session.hh"
#include "client/play_menu.hh"
#include "client/res/texture_2d.hh"
#include "client/settings.hh"
#include "client/style.hh"
#include "client/video.hh"
#include "client/world/block_atlas.hh"
#include "client/world/bmodel_cache.hh"
#include "client/world/chunk_mesher.hh"
#include "client/world/chunk_visibility.hh"
#include "client/world/fluid_cache.hh"
#include "client/world/sky_renderer.hh"
#include "client/world/skybox.hh"

static std::atomic_bool s_is_running;

static void signal_handler(int)
{
    msg::log_info("received termination signal");

    s_is_running.store(false);
}

static void handle_events(void)
{
    ZoneScopedN("Events");

    thread_local SDL_Event event;

    while(SDL_PollEvent(&event)) {
        if(event.type == SDL_EVENT_QUIT) {
            if(boot::active()) {
                boot::request_quit();
                continue;
            }

            s_is_running.store(false);
            return;
        }

        if(boot::input(event)) {
            continue;
        }

        switch(event.type) {
            case SDL_EVENT_KEY_DOWN:
            case SDL_EVENT_KEY_UP:
                globals::dispatcher.trigger(static_cast<const SDL_KeyboardEvent&>(event.key));
                break;

            case SDL_EVENT_MOUSE_MOTION:
                globals::dispatcher.trigger(static_cast<const SDL_MouseMotionEvent&>(event.motion));
                break;

            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            case SDL_EVENT_MOUSE_BUTTON_UP:
                globals::dispatcher.trigger(static_cast<const SDL_MouseButtonEvent&>(event.button));
                break;

            case SDL_EVENT_MOUSE_WHEEL:
                globals::dispatcher.trigger(static_cast<const SDL_MouseWheelEvent&>(event.wheel));
                break;

            case SDL_EVENT_WINDOW_MOVED:
            case SDL_EVENT_WINDOW_RESIZED:
            case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
            case SDL_EVENT_WINDOW_METAL_VIEW_RESIZED:
            case SDL_EVENT_WINDOW_MINIMIZED:
            case SDL_EVENT_WINDOW_MAXIMIZED:
            case SDL_EVENT_WINDOW_RESTORED:
            case SDL_EVENT_WINDOW_MOUSE_ENTER:
            case SDL_EVENT_WINDOW_MOUSE_LEAVE:
            case SDL_EVENT_WINDOW_FOCUS_GAINED:
            case SDL_EVENT_WINDOW_FOCUS_LOST:
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            case SDL_EVENT_WINDOW_HIT_TEST:
            case SDL_EVENT_WINDOW_ICCPROF_CHANGED:
            case SDL_EVENT_WINDOW_DISPLAY_CHANGED:
            case SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED:
            case SDL_EVENT_WINDOW_SAFE_AREA_CHANGED:
            case SDL_EVENT_WINDOW_OCCLUDED:
            case SDL_EVENT_WINDOW_ENTER_FULLSCREEN:
            case SDL_EVENT_WINDOW_LEAVE_FULLSCREEN:
            case SDL_EVENT_WINDOW_DESTROYED:
            case SDL_EVENT_WINDOW_HDR_STATE_CHANGED:
                globals::dispatcher.trigger(static_cast<const SDL_WindowEvent&>(event.window));
                break;
        }

        globals::dispatcher.trigger(static_cast<const SDL_Event&>(event));

        ImGui_ImplSDL3_ProcessEvent(&event);
    }
}

static void zoned_fixed_update(void)
{
    ZoneScopedN("client::fixed_update");

    for(std::uint64_t i = 0; i < globals::fixed_framecount; ++i) {
        FrameMarkStart("Fixed");

        client_game::fixed_update(globals::world);
        shared_game::fixed_update(globals::world);

        FrameMarkEnd("Fixed");
    }
}

static void zoned_update(void)
{
    ZoneScopedN("client::update");

    video::update();

    client_game::update();

    chunk_mesher::update();

    camera::update();

    loading::update();

    skybox::update();
    fog::update();

    gui::update_scale();
}

static void zoned_render(void)
{
    ZoneScopedN("client::render");

    if(head::prepare()) {
        head::render();

        if(globals::gui_screen) {
            globals::gui_screen->layout();
        }

        client_game::layout();

        boot::render_fade();

        head::present();
    }
}

static void zoned_fixed_update_late(void)
{
    ZoneScopedN("client::fixed_update_late");

    for(std::uint64_t i = 0; i < globals::fixed_framecount; ++i) {
        FrameMarkStart("Fixed");

        host::fixed_update_late();

        client_game::fixed_update_late();
        shared_game::fixed_update_late();

        chunk_visibility::fixed_update_late();

        FrameMarkEnd("Fixed");
    }
}

static void zoned_update_late(void)
{
    ZoneScopedN("client::update_late");

    video::update_late();

    client_game::update_late();

    bother::update_late();

    play_menu::update_late();

    threading::update();
}

static void wrapped_main(int argc, char** argv)
{
    core::setup(argc, argv);

    cvar::register_commands_core();

    tracy::SetThreadName("Main");
    TracySetProgramName("Voxelius");
    TracyAppInfo(version::full.data(), version::full.size());
    TracyPlotConfig("Frametime ms", tracy::PlotFormatType::Number, false, true, 0);

    msg::log_info("engine version: {}", version::full);

    std::signal(SIGABRT, &signal_handler);
    std::signal(SIGILL, &signal_handler);
    std::signal(SIGINT, &signal_handler);
    std::signal(SIGSEGV, &signal_handler);
    std::signal(SIGTERM, &signal_handler);

    auto sdl_ok = SDL_Init(SDL_INIT_EVENTS);
    vx::throw_if_not_fmt(sdl_ok, "SDL_Init failed: {}", SDL_GetError());

    Texture2D::register_resource();

    video::init();
    head::init();
    camera::init();

    style::apply();
    fonts::load();
    language::init();

    boot::init();

    boot::add("net", "boot.stage.net", 1.0f, [] {
        bother::init();

        host::init();
        session::init();
        receive::init();
        replication::init();

        api_client::register_libraries();

        return true;
    });

    boot::add("mods", "boot.stage.mods", 4.0f, [] {
        shared_game::init();
        client_game::init_world();

        splash::init(SPLASH_CLIENT);

        video::update_window_title();

        return true;
    });

    boot::add("render", "boot.stage.render", 2.0f, [] {
        head::detail::init_renderers();

        return true;
    });

    boot::add("atlas", "boot.stage.atlas", 3.0f, [] {
        block_atlas::init();

        return true;
    });

    boot::add("gui", "boot.stage.gui", 2.0f, [] {
        buttons::init();
        axes::init();
        gui::ButtonBind::init();
        gui::detail::stone_init();

        settings::init();
        main_menu::init();

        console::init();
        console::init_late();

        play_menu::init();

        loading::init();

        client_game::init();
        skybox::init();

        return true;
    });

    boot::add("config", "boot.stage.config", 1.0f, [] {
        cvar::exec_from("client.conf");
        cvar::exec_from("client.user.conf");

        return true;
    });

    boot::add("video", "boot.stage.video", 1.0f, [] {
        static bool s_started = false;

        if(!s_started) {
            s_started = true;

            video::init_late();
            head::init_late();
        }

        return !video::mode_pending;
    });

    boot::add("assets", "boot.stage.assets", 3.0f, [] {
        language::init_late();

        block_atlas::init_late();
        fluid_cache::init_late();
        bmodel_cache::init_late();

        return true;
    });

    boot::add("world", "boot.stage.world", 2.0f, [] {
        shared_game::init_late();

        chunk_mesher::init();

        return true;
    });

    boot::add("menu", "boot.stage.menu", 2.0f, [] {
        main_menu::init_late();

        client_game::init_late();

        return true;
    });

    boot::start();

#ifndef NDEBUG
    msg::log_warning("debug build");
#endif

    s_is_running.store(true);

    globals::fixed_frametime_avg = 0.0f;
    globals::fixed_framecount = 0;

    globals::curtime_us = utils::epoch_microseconds();

    globals::window_framecount = 0;
    globals::window_frametime_us = 0;
    globals::window_frametime = 0.0f;
    globals::window_frametime_avg = 0.0f;

    auto last_curtime_us = globals::curtime_us;

    // UNDONE: enable fixed steps at 20 FPS
    globals::fixed_frametime_us = 50000;
    globals::fixed_frametime = 1.0e-6f * static_cast<float>(globals::fixed_frametime_us);

    while(s_is_running.load()) {
        globals::curtime_us = utils::epoch_microseconds();

        globals::window_frametime_us = globals::curtime_us - last_curtime_us;
        globals::window_frametime = 1.0e-6f * static_cast<float>(globals::window_frametime_us);
        globals::window_frametime_avg += globals::window_frametime;
        globals::window_frametime_avg *= 0.5f;

        if(globals::fixed_frametime_us == UINT64_MAX) {
            globals::fixed_framecount = 0;
            globals::fixed_accumulator_us = 0;
        }
        else {
            globals::fixed_accumulator_us += globals::window_frametime_us;
            globals::fixed_framecount = globals::fixed_accumulator_us / globals::fixed_frametime_us;
            globals::fixed_accumulator_us %= globals::fixed_frametime_us;
        }

        globals::num_draw_calls = 0;
        globals::num_draw_vertices = 0;

        last_curtime_us = globals::curtime_us;

        handle_events();

        boot::update();

        if(boot::quit_requested() && !boot::active()) {
            s_is_running.store(false);
        }

        if(boot::active()) {
            boot::render();
        }
        else {
            zoned_fixed_update();

            zoned_update();

            zoned_render();

            FrameMark;

            zoned_fixed_update_late();

            zoned_update_late();
        }

        globals::window_framecount += 1;

        globals::dispatcher.update();

        res::soft_purge();

        TracyPlot("Frametime ms", 1000.0f * globals::window_frametime);
        TracyPlot("Draw calls", static_cast<int64_t>(globals::num_draw_calls));
        TracyPlot("Vertices", static_cast<int64_t>(globals::num_draw_vertices));
    }

    msg::log_info("shutdown after {} frames", globals::window_framecount);
    msg::log_info("avg framerate: {:.03f} FPS ({:.03f} ms)", 1.0f / globals::window_frametime_avg, 1000.0f * globals::window_frametime_avg);
    msg::log_info("last frame I drew {} vertices ({} draw calls)", globals::num_draw_vertices, globals::num_draw_calls);

    bother::shutdown();

    bmodel_cache::shutdown();
    fluid_cache::shutdown();
    block_atlas::shutdown();

    settings::shutdown();
    loading::shutdown();
    boot::shutdown();
    play_menu::shutdown();
    main_menu::shutdown();

    client_game::shutdown();

    // TODO: shut down the game UI

    shared_game::shutdown();

    session::shutdown();
    host::shutdown();

    gui::ButtonBind::shutdown();
    gui::detail::stone_shutdown();
    console::shutdown();

    // Workers keep thread-local scratch alive until the pool is destroyed;
    // stop it first so that a zombie warning in hard_purge means a real leak
    threading::shutdown();

    sky_renderer::release_textures();

    res::hard_purge();

    head::shutdown();
    video::shutdown();

    cvar::save_values("client.conf", FCVAR_REPLICATED);

    core::teardown();
}

int main(int argc, char** argv)
{
    msg::add_sink<&msg::detail::stderr_sink>();

    try {
        wrapped_main(argc, argv);
        return EXIT_SUCCESS;
    }
    catch(const std::exception& ex) {
        msg::log_error(std::string_view(ex.what()));

        auto message = ex.what();
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Engine Error", message, nullptr);
        return EXIT_FAILURE;
    }
    catch(const vx::detail::Exception& ex) {
        msg::log_error(ex.what_standard(), ex.location());

        auto message = ex.what_standard();
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Engine Error", message, nullptr);
        return EXIT_FAILURE;
    }
    catch(...) {
        msg::log_error(std::string_view("Unknown exception"));
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Engine Error", "Unknown exception", nullptr);
        return EXIT_FAILURE;
    }
}

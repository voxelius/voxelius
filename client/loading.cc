#include "client/pch.hh"

#include "client/loading.hh"

#include "shared/constant.hh"
#include "shared/net/packet_session.hh"
#include "shared/utils/view.hh"
#include "shared/world/chunk.hh"
#include "shared/world/world.hh"

#include "client/camera.hh"
#include "client/globals.hh"
#include "client/gui/background.hh"
#include "client/gui/loading_panel.hh"
#include "client/gui/screen.hh"
#include "client/language.hh"
#include "client/net/session.hh"
#include "client/play_menu.hh"
#include "client/world/chunk_mesh.hh"
#include "client/world/chunk_mesher.hh"

constexpr static ChunkPos::value_type VIEW_RADIUS = 1;

constexpr static std::string_view CONNECT_TITLE_KEY = "play_menu.connect.title";
constexpr static std::string_view TITLE_KEY = "gui.loading.title";
constexpr static std::string_view STATUS_KEY = "gui.loading.status";

loading_stage loading::stage = LOADING_IDLE;

static gui::Screen s_screen;
static gui::Background s_background;
static gui::LoadingPanel s_panel;
static bool s_spawn_known;
static bool s_cancel_requested;

static std::string_view stage_status_key(loading_stage stage)
{
    switch(stage) {
        case LOADING_CONNECTING:
            return "play_menu.connect.stage.connecting";

        case LOADING_AUTHENTICATING:
            return "play_menu.connect.stage.authenticating";

        case LOADING_SPAWNING:
            return "play_menu.connect.stage.spawning";

        default:
            break;
    }

    return {};
}

static void finish(void)
{
    loading::stage = LOADING_IDLE;

    if(globals::gui_screen == &s_screen) {
        globals::gui_screen = nullptr;
    }
}

static void abort_loading(void)
{
    loading::stage = LOADING_IDLE;

    if(globals::gui_screen == &s_screen) {
        globals::gui_screen = &play_menu::screen;
    }
}

// The button callback only records the request: disconnecting inside the
// screen layout switches the current screen under its own iterator
static void request_cancel(void)
{
    s_cancel_requested = true;
}

static void on_language_update(const LanguageUpdateEvent&)
{
    s_screen.translate();
}

static void on_session_state(const SessionStateEvent& event)
{
    if(event.state() == SESSION_DISCONNECTED && loading::stage) {
        abort_loading();
    }
}

static bool chunk_ready(const ChunkPos& cpos)
{
    auto chunk = globals::world->find_chunk(cpos);

    if(chunk == nullptr) {
        return false;
    }

    auto& chunk_registry = globals::world->chunk_registry();

    if(chunk_registry.all_of<ChunkMesh_DirtyMarker>(chunk->entity())) {
        return false;
    }

    return !chunk_mesher::is_pending(cpos);
}

void loading::init(void)
{
    s_background.set_left({ 0.000f, 0.000f, 0.000f, 0.620f });
    s_background.set_right({ 0.000f, 0.000f, 0.000f, 0.280f });
    s_background.force_scene(true);

    s_screen.add_child(s_background);
    s_screen.add_child(s_panel);

    s_panel.on_cancel(&request_cancel);

    s_screen.translate();

    globals::dispatcher.sink<LanguageUpdateEvent>().connect<&on_language_update>();
    globals::dispatcher.sink<SessionStateEvent>().connect<&on_session_state>();
}

void loading::shutdown(void)
{
    // empty
}

void loading::update(void)
{
    if(!loading::stage) {
        s_cancel_requested = false;

        return;
    }

    if(s_cancel_requested) {
        s_cancel_requested = false;

        session::disconnect(packet::Session_Disconnect::CLIENT_DISCONNECT);

        return;
    }

    if(loading::stage != LOADING_CHUNKS) {
        return;
    }

    auto& registry = globals::world->registry();

    if(!registry.valid(globals::player) || !s_spawn_known) {
        return;
    }

    auto& definition = globals::world->definition();
    auto min_chunk_y = static_cast<ChunkPos::value_type>(definition.min_y >> constant::CHUNK_SIZE_LOG2);
    auto max_chunk_y = static_cast<ChunkPos::value_type>(definition.max_y >> constant::CHUNK_SIZE_LOG2);
    auto box = utils::view_box(camera::chunk, VIEW_RADIUS);

    box.min().y() = std::max(box.min().y(), min_chunk_y);
    box.max().y() = std::min(box.max().y(), max_chunk_y);

    std::size_t total = 0;
    std::size_t ready = 0;

    for(auto y = box.min().y(); y <= box.max().y(); y += 1) {
        for(auto z = box.min().z(); z <= box.max().z(); z += 1) {
            for(auto x = box.min().x(); x <= box.max().x(); x += 1) {
                total += 1;

                if(chunk_ready(ChunkPos(x, y, z))) {
                    ready += 1;
                }
            }
        }
    }

    if(total == 0) {
        return;
    }

    s_panel.set_progress(static_cast<float>(ready) / static_cast<float>(total));

    if(ready == total) {
        finish();
    }
}

void loading::show_stage(loading_stage new_stage)
{
    if(new_stage == LOADING_IDLE || new_stage == LOADING_CHUNKS) {
        return;
    }

    loading::stage = new_stage;
    s_spawn_known = false;

    s_panel.set_title(CONNECT_TITLE_KEY);
    s_panel.set_status(stage_status_key(new_stage));
    s_panel.set_progress(std::nullopt);
    s_panel.set_cancel_visible(true);
    s_panel.translate();

    globals::gui_screen = &s_screen;
}

void loading::attach_player(void)
{
    if(globals::player == entt::null) {
        return;
    }

    loading::stage = LOADING_CHUNKS;
    s_spawn_known = false;

    s_panel.set_title(TITLE_KEY);
    s_panel.set_status(STATUS_KEY);
    s_panel.set_progress(std::nullopt);
    s_panel.set_cancel_visible(false);
    s_panel.translate();

    globals::gui_screen = &s_screen;
}

void loading::on_local_patch(void)
{
    if(loading::stage) {
        s_spawn_known = true;
    }
}

#include "client/pch.hh"

#include "client/metrics.hh"

#include "core/cvar/boolean.hh"
#include "core/cvar/host.hh"
#include "core/utils/angles.hh"
#include "core/version.hh"

#include "shared/component/head.hh"
#include "shared/component/transform.hh"
#include "shared/component/velocity.hh"
#include "shared/globals.hh"
#include "shared/utils/coord.hh"
#include "shared/world/world.hh"

#include "client/fonts.hh"
#include "client/globals.hh"
#include "client/world/chunk_mesh.hh"
#include "client/world/chunk_mesher.hh"

constexpr static float TEXT_XPOS = 8.0f;
constexpr static float TEXT_YPOS = 8.0f;

constexpr static std::size_t NETGRAPH_SAMPLES = 128;
constexpr static float NETGRAPH_WIDTH = 256.0f;
constexpr static float NETGRAPH_HEIGHT = 72.0f;
constexpr static float NETGRAPH_SCALE = 200.0f;
constexpr static float NETGRAPH_MARGIN = 8.0f;
constexpr static enet_uint32 NETGRAPH_INTERVAL_MS = 20;
constexpr static enet_uint32 NETGRAPH_MARK_MS = 500;

constexpr static enet_uint32 RATE_INTERVAL_MS = 1000;
constexpr static enet_uint32 RECV_STALL_MS = 1500;
constexpr static float LOSS_ALERT_PERCENT = 1.0f;

struct NetgraphSample final {
    float ping_ms;
    float jitter_ms;
    bool has_loss;
    bool has_mark;
};

static cvar::Boolean s_metrics_enable { true };
static cvar::Boolean s_metrics_head { true };
static cvar::Boolean s_metrics_pos { true };
static cvar::Boolean s_metrics_net { true };
static cvar::Boolean s_metrics_netgraph { true };

static std::array<NetgraphSample, NETGRAPH_SAMPLES> s_netgraph_samples;
static enet_uint32 s_netgraph_prev_loss;
static enet_uint32 s_netgraph_prev_time;
static std::size_t s_netgraph_cursor;

static enet_uint32 s_rates_prev_time;
static enet_uint32 s_rates_prev_in_bytes;
static enet_uint32 s_rates_prev_out_bytes;
static enet_uint32 s_rates_prev_in_packets;
static enet_uint32 s_rates_prev_out_packets;

static float s_rates_calc_in_kbps;
static float s_rates_calc_out_kbps;
static float s_rates_calc_in_pktps;
static float s_rates_calc_out_pktps;

static std::string s_gl_version;
static std::string s_gl_renderer;

static float overlay_scale(void)
{
    // The window allows a gui_scale of 2 at the very least, so this gives
    // the native 16 px unscii16 on a 2x window and scales linearly from there
    return 0.5f * static_cast<float>(globals::gui_scale);
}

static float text_font_size(void)
{
    return fonts::unscii16->LegacySize * overlay_scale();
}

static float text_width(std::string_view text)
{
    auto font = fonts::unscii16;
    auto size = text_font_size();

    return font->CalcTextSizeA(size, FLT_MAX, 0.0f, text.data(), text.data() + text.size()).x;
}

static void draw_text(ImDrawList* draw_list, std::string_view text, const ImVec2& pos, ImU32 color, ImU32 shadow)
{
    auto font = fonts::unscii16;
    auto size = text_font_size();
    auto shift = overlay_scale();

    ImVec2 shadow_pos {};
    shadow_pos.x = pos.x + shift;
    shadow_pos.y = pos.y + shift;

    draw_list->AddText(font, size, shadow_pos, shadow, text.data(), text.data() + text.size());
    draw_list->AddText(font, size, pos, color, text.data(), text.data() + text.size());
}

static ImU32 fps_color(float framerate)
{
    if(framerate >= 120.0f) {
        return IM_COL32(0x40, 0xFF, 0x40, 0xFF);
    }

    if(framerate >= 60.0f) {
        return IM_COL32(0xFF, 0xFF, 0x40, 0xFF);
    }

    if(framerate >= 30.0f) {
        return IM_COL32(0xFF, 0xA0, 0x40, 0xFF);
    }

    return IM_COL32(0xFF, 0x40, 0x40, 0xFF);
}

static ImU32 ping_color(float ping_ms)
{
    if(ping_ms >= 150.0f) {
        return IM_COL32(0xFF, 0xA0, 0x40, 0xFF);
    }

    if(ping_ms >= 80.0f) {
        return IM_COL32(0xFF, 0xFF, 0x40, 0xFF);
    }

    return IM_COL32(0x40, 0xFF, 0x40, 0xFF);
}

static void sample_netgraph(void)
{
    auto curtime = enet_time_get();
    auto elapsed = curtime - s_netgraph_prev_time;

    if(elapsed < NETGRAPH_INTERVAL_MS) {
        return;
    }

    auto& sample = s_netgraph_samples[s_netgraph_cursor];

    auto mark_curtime = curtime / NETGRAPH_MARK_MS;
    auto mark_prevtime = s_netgraph_prev_time / NETGRAPH_MARK_MS;
    sample.has_mark = mark_curtime != mark_prevtime;

    s_netgraph_prev_time = curtime;

    s_netgraph_cursor += 1;
    s_netgraph_cursor %= NETGRAPH_SAMPLES;

    if(!globals::peer) {
        sample.ping_ms = 0.0f;
        sample.jitter_ms = 0.0f;
        sample.has_loss = false;
        s_netgraph_prev_loss = 0;
        return;
    }

    sample.ping_ms = static_cast<float>(globals::peer->roundTripTime);
    sample.jitter_ms = static_cast<float>(globals::peer->roundTripTimeVariance);

    auto packets_lost = globals::peer->packetsLost;
    sample.has_loss = packets_lost != s_netgraph_prev_loss;
    s_netgraph_prev_loss = packets_lost;
}

static void sample_rates(void)
{
    if(!globals::host) {
        return;
    }

    auto curtime = enet_time_get();

    if(!globals::peer) {
        s_rates_calc_in_kbps = 0.0f;
        s_rates_calc_out_kbps = 0.0f;
        s_rates_calc_in_pktps = 0.0f;
        s_rates_calc_out_pktps = 0.0f;

        s_rates_prev_time = curtime;
        s_rates_prev_in_bytes = globals::host->totalReceivedData;
        s_rates_prev_out_bytes = globals::host->totalSentData;
        s_rates_prev_in_packets = globals::host->totalReceivedPackets;
        s_rates_prev_out_packets = globals::host->totalSentPackets;

        return;
    }

    auto elapsed = curtime - s_rates_prev_time;

    if(elapsed < RATE_INTERVAL_MS) {
        return;
    }

    auto seconds = 0.001f * static_cast<float>(elapsed);

    s_rates_calc_in_kbps = static_cast<float>(globals::host->totalReceivedData - s_rates_prev_in_bytes) / seconds * 0.001f;
    s_rates_calc_out_kbps = static_cast<float>(globals::host->totalSentData - s_rates_prev_out_bytes) / seconds * 0.001f;
    s_rates_calc_in_pktps = static_cast<float>(globals::host->totalReceivedPackets - s_rates_prev_in_packets) / seconds;
    s_rates_calc_out_pktps = static_cast<float>(globals::host->totalSentPackets - s_rates_prev_out_packets) / seconds;

    s_rates_prev_time = curtime;
    s_rates_prev_in_bytes = globals::host->totalReceivedData;
    s_rates_prev_out_bytes = globals::host->totalSentData;
    s_rates_prev_in_packets = globals::host->totalReceivedPackets;
    s_rates_prev_out_packets = globals::host->totalSentPackets;
}

static void layout_netgraph(void)
{
    auto viewport = ImGui::GetMainViewport();
    auto draw_list = ImGui::GetBackgroundDrawList();

    auto scale = overlay_scale();
    auto total_width = NETGRAPH_WIDTH * scale;
    auto total_height = NETGRAPH_HEIGHT * scale;
    auto margin = NETGRAPH_MARGIN * scale;

    ImVec2 origin {};
    origin.x = viewport->Size.x - total_width - margin;
    origin.y = margin;

    auto bar_width = total_width / static_cast<float>(NETGRAPH_SAMPLES);

    ImVec2 background_end {};
    background_end.x = origin.x + total_width;
    background_end.y = origin.y + total_height;

    auto background_color = IM_COL32(0x00, 0x00, 0x00, 0x80);
    auto grid_color = IM_COL32(0xFF, 0xFF, 0xFF, 0x30);
    auto jitter_color = IM_COL32(0xFF, 0xFF, 0xFF, 0x60);
    auto loss_color = IM_COL32(0xFF, 0x40, 0x40, 0xFF);

    draw_list->AddRectFilled(origin, background_end, background_color);

    for(std::size_t i = 0; i < NETGRAPH_SAMPLES; ++i) {
        auto index = static_cast<std::size_t>((s_netgraph_cursor + i) % NETGRAPH_SAMPLES);
        auto& sample = s_netgraph_samples[index];

        auto bar_height = total_height * std::min(sample.ping_ms / NETGRAPH_SCALE, 1.0f);
        auto jitter_height = total_height * std::min((sample.ping_ms + sample.jitter_ms) / NETGRAPH_SCALE, 1.0f);

        ImVec2 bar_start {};
        bar_start.x = origin.x + bar_width * static_cast<float>(i);
        bar_start.y = origin.y + total_height - bar_height;

        ImVec2 bar_end {};
        bar_end.x = bar_start.x + bar_width;
        bar_end.y = origin.y + total_height;

        if(sample.has_loss) {
            bar_start.y = origin.y;
        }
        else if(jitter_height > bar_height) {
            ImVec2 cap_start {};
            cap_start.x = bar_start.x;
            cap_start.y = origin.y + total_height - jitter_height;

            ImVec2 cap_end {};
            cap_end.x = bar_end.x;
            cap_end.y = bar_start.y;

            draw_list->AddRectFilled(cap_start, cap_end, jitter_color);
        }

        if(bar_height > 0.0f || sample.has_loss) {
            if(sample.has_loss) {
                draw_list->AddRectFilled(bar_start, bar_end, loss_color);
            }
            else {
                draw_list->AddRectFilled(bar_start, bar_end, ping_color(sample.ping_ms));
            }
        }

        if(sample.has_mark) {
            ImVec2 mark_start {};
            mark_start.x = bar_start.x;
            mark_start.y = origin.y;

            ImVec2 mark_end {};
            mark_end.x = bar_start.x;
            mark_end.y = background_end.y;

            draw_list->AddLine(mark_start, mark_end, grid_color);
        }
    }

    for(auto fraction : std::array { 0.25f, 0.50f, 0.75f }) {
        ImVec2 line_start {};
        line_start.x = origin.x;
        line_start.y = origin.y + total_height * (1.0f - fraction);

        ImVec2 line_end {};
        line_end.x = background_end.x;
        line_end.y = line_start.y;

        draw_list->AddLine(line_start, line_end, grid_color);
    }

    auto text_color = IM_COL32(0xFF, 0xFF, 0xFF, 0x4F);
    auto shadow_color = IM_COL32(0x00, 0x00, 0x00, 0x4F);

    ImVec2 text_pos {};
    text_pos.x = origin.x + 4.0f;
    text_pos.y = origin.y + 2.0f;

    auto label = std::format("{:.0f} ms", NETGRAPH_SCALE);
    draw_text(draw_list, label, text_pos, text_color, shadow_color);
}

static ImVec2 layout_version(ImDrawList* draw_list, ImVec2 position, ImU32 text, ImU32 shadow, float ystep)
{
    auto line = std::format("Voxelius {}", version::full);
    draw_text(draw_list, line, position, text, shadow);
    position.y += ystep;

    return position;
}

static ImVec2 layout_gpu(ImDrawList* draw_list, ImVec2 position, ImU32 text, ImU32 shadow, float ystep)
{
    auto gl_version = std::format("gfx: OpenGL {}", s_gl_version);
    draw_text(draw_list, gl_version, position, text, shadow);
    position.y += ystep;

    auto gl_renderer = std::format("gpu: {}", s_gl_renderer);
    draw_text(draw_list, gl_renderer, position, text, shadow);
    position.y += ystep;

    return position;
}

static ImVec2 layout_head(ImDrawList* draw_list, ImVec2 position, ImU32 text, ImU32 shadow, float ystep)
{
    auto framerate = 0.0f;

    if(globals::window_frametime_avg > 0.0f) {
        framerate = 1.0f / globals::window_frametime_avg;
    }

    auto frametime = 1000.0f * globals::window_frametime_avg;
    auto color = fps_color(framerate);

    position.y += 0.5f * ystep;

    auto head_line = std::format("head: {:.1f} FPS ({:.03f} ms)", framerate, frametime);
    draw_text(draw_list, head_line, position, color, shadow);
    position.y += ystep;

    auto triangles = globals::num_draw_vertices / 3;
    auto draw_line = std::format("draw: {} DC / {} VTX / {} TRI", globals::num_draw_calls, globals::num_draw_vertices, triangles);
    draw_text(draw_list, draw_line, position, text, shadow);
    position.y += ystep;

    // Loaded chunks against meshed ones against what still waits for a mesh:
    // a hole in the world is either a chunk nobody asked for or a mesh that
    // never got built, and the three numbers tell them apart.
    auto& chunks = globals::world->chunks();
    auto& chunk_registry = globals::world->chunk_registry();
    auto meshed = chunk_registry.view<ChunkMesh>();

    auto loaded_chunks = chunks.size();
    auto meshed_chunks = meshed.size();

    auto mesh_line = std::format("mesh:  {}/{} backlog {}", meshed_chunks, loaded_chunks, chunk_mesher::backlog());
    draw_text(draw_list, mesh_line, position, text, shadow);
    position.y += ystep;

    return position;
}

static ImVec2 layout_network(ImDrawList* draw_list, ImVec2 position, ImU32 text, ImU32 shadow, float ystep)
{
    position.y += 0.5f * ystep;

    auto ping_ms = static_cast<float>(globals::peer->roundTripTime);
    auto lowest_ms = static_cast<float>(globals::peer->lowestRoundTripTime);
    auto jitter_ms = static_cast<float>(globals::peer->roundTripTimeVariance);
    auto loss = 100.0f * static_cast<float>(globals::peer->packetLoss) / static_cast<float>(ENET_PEER_PACKET_LOSS_SCALE);
    auto throttle = 100.0f * static_cast<float>(globals::peer->packetThrottle) / static_cast<float>(ENET_PEER_PACKET_THROTTLE_SCALE);

    auto recv_age_ms = static_cast<enet_uint32>(0);

    if(globals::peer->lastReceiveTime) {
        recv_age_ms = enet_time_get() - globals::peer->lastReceiveTime;
    }

    auto alert_color = IM_COL32(0xFF, 0x40, 0x40, 0xFF);

    auto ping_line = std::format("ping: {:.0f}~{:.0f} ms / min {:.0f}", ping_ms, jitter_ms, lowest_ms);
    draw_text(draw_list, ping_line, position, ping_color(ping_ms), shadow);
    position.y += ystep;

    auto loss_color = loss >= LOSS_ALERT_PERCENT ? alert_color : text;
    auto loss_line = std::format("loss: {:5.1f}%", loss);
    draw_text(draw_list, loss_line, position, loss_color, shadow);
    position.y += ystep;

    auto in_line = std::format("in:  {:5.1f} kB/s  {:5.1f} pkt/s", s_rates_calc_in_kbps, s_rates_calc_in_pktps);
    draw_text(draw_list, in_line, position, text, shadow);
    position.y += ystep;

    auto out_line = std::format("out: {:5.1f} kB/s  {:5.1f} pkt/s", s_rates_calc_out_kbps, s_rates_calc_out_pktps);
    draw_text(draw_list, out_line, position, text, shadow);
    position.y += ystep;

    auto start_x = position.x;

    auto age_color = recv_age_ms >= RECV_STALL_MS ? alert_color : text;
    auto age_line = std::format("recv age: {:4} ms", recv_age_ms);
    draw_text(draw_list, age_line, position, age_color, shadow);
    position.x += text_width(age_line);

    auto throttle_color = globals::peer->packetThrottle < ENET_PEER_PACKET_THROTTLE_SCALE ? alert_color : text;
    auto throttle_line = std::format("  throttle: {:3.0f}%", throttle);
    draw_text(draw_list, throttle_line, position, throttle_color, shadow);

    position.x = start_x;
    position.y += ystep;

    return position;
}

static ImVec2 layout_position(ImDrawList* draw_list, ImVec2 position, ImU32 text, ImU32 shadow, float ystep)
{
    auto& registry = globals::world->registry();

    if(!registry.valid(globals::player)) {
        return position;
    }

    if(!registry.all_of<Transform_Intr, Head_Intr, Velocity_Intr>(globals::player)) {
        return position;
    }

    const auto& transform = registry.get<Transform_Intr>(globals::player);
    const auto& head = registry.get<Head_Intr>(globals::player);
    const auto& velocity = registry.get<Velocity_Intr>(globals::player);

    auto world = utils::to_fvec(transform.chunk, transform.local);
    auto angles = utils::degrees(head.angles);
    auto horizontal = Eigen::Vector2f(velocity.value.x(), velocity.value.z());
    auto speed = horizontal.norm();

    position.y += 0.5f * ystep;

    auto pos_line = std::format("pos:   {:8.2f} {:8.2f} {:8.2f}", world.x(), world.y(), world.z());
    draw_text(draw_list, pos_line, position, text, shadow);
    position.y += ystep;

    auto ang_line = std::format("ang:   {:8.2f} {:8.2f} {:8.2f}", angles.x(), angles.y(), angles.z());
    draw_text(draw_list, ang_line, position, text, shadow);
    position.y += ystep;

    auto vel_line = std::format("vel:   {:8.2f} {:8.2f} {:8.2f}", velocity.value.x(), velocity.value.y(), velocity.value.z());
    draw_text(draw_list, vel_line, position, text, shadow);
    position.y += ystep;

    auto speed_line = std::format("speed: {:8.2f}", speed);
    draw_text(draw_list, speed_line, position, text, shadow);
    position.y += ystep;

    return position;
}

void metrics::init(void)
{
    auto& metrics_enable_binding = cvar::bind_var(s_metrics_enable, "metrics_enable");
    metrics_enable_binding.set_help("Enable metrics overlay");
    metrics_enable_binding.set_flags(FCVAR_ARCHIVE);

    auto& metrics_head_binding = cvar::bind_var(s_metrics_head, "metrics_head");
    metrics_head_binding.set_help("Show engine head in metrics overlay");
    metrics_head_binding.set_flags(FCVAR_ARCHIVE);

    auto& metrics_pos_binding = cvar::bind_var(s_metrics_pos, "metrics_pos");
    metrics_pos_binding.set_help("Show player position in metrics overlay");
    metrics_pos_binding.set_flags(FCVAR_ARCHIVE);

    auto& metrics_net_binding = cvar::bind_var(s_metrics_net, "metrics_net");
    metrics_net_binding.set_help("Show network statistics in metrics overlay");
    metrics_net_binding.set_flags(FCVAR_ARCHIVE);

    auto& metrics_netgraph_binding = cvar::bind_var(s_metrics_netgraph, "metrics_netgraph");
    metrics_netgraph_binding.set_help("Show network graph in metrics overlay");
    metrics_netgraph_binding.set_flags(FCVAR_ARCHIVE);

    if(auto gl_version = glGetString(GL_VERSION)) {
        s_gl_version = reinterpret_cast<const char*>(gl_version);
    }

    if(auto gl_renderer = glGetString(GL_RENDERER)) {
        s_gl_renderer = reinterpret_cast<const char*>(gl_renderer);
    }
}

void metrics::update(void)
{
    sample_netgraph();
    sample_rates();
}

void metrics::layout(void)
{
    if(!s_metrics_enable.value()) {
        return;
    }

    if(s_metrics_netgraph.value()) {
        layout_netgraph();
    }

    auto draw_list = ImGui::GetBackgroundDrawList();
    auto text_color = IM_COL32(0xFF, 0xFF, 0xFF, 0xFF);
    auto shadow_color = IM_COL32(0x00, 0x00, 0x00, 0xFF);

    auto ystep = 1.5f * text_font_size();

    ImVec2 position {};
    position.x = TEXT_XPOS * overlay_scale();
    position.y = TEXT_YPOS * overlay_scale();

    position = layout_version(draw_list, position, text_color, shadow_color, ystep);
    position = layout_gpu(draw_list, position, text_color, shadow_color, ystep);

    if(s_metrics_head.value()) {
        position = layout_head(draw_list, position, text_color, shadow_color, ystep);
    }

    if(globals::peer && s_metrics_net.value()) {
        position = layout_network(draw_list, position, text_color, shadow_color, ystep);
    }

    if(s_metrics_pos.value()) {
        position = layout_position(draw_list, position, text_color, shadow_color, ystep);
    }
}

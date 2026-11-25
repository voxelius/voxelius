#include "client/pch.hh"

#include "client/metrics.hh"

#include "core/cvar/host.hh"
#include "core/cvar/number.hh"
#include "core/identifier.hh"
#include "core/utils/angles.hh"
#include "core/version.hh"

#include "shared/component/transform.hh"
#include "shared/component/velocity.hh"
#include "shared/cvar/server.hh"
#include "shared/utils/coord.hh"
#include "shared/world/biome_registry.hh"
#include "shared/world/block_registry.hh"
#include "shared/world/light_storage.hh"
#include "shared/world/precipitation.hh"
#include "shared/world/precipitation_registry.hh"
#include "shared/world/weather_registry.hh"
#include "shared/world/world.hh"
#include "shared/world/world_clock.hh"

#include "client/camera.hh"
#include "client/fonts.hh"
#include "client/globals.hh"
#include "client/system/player_target.hh"
#include "client/video.hh"
#include "client/world/chunk_mesh.hh"
#include "client/world/chunk_mesher.hh"
#include "client/world/columns.hh"

constexpr static float TEXT_XPOS = 8.0f;
constexpr static float TEXT_YPOS = 8.0f;
constexpr static float TEXT_MARGIN = 8.0f;
constexpr static float LINE_GAP = 0.75f;

constexpr static std::size_t LINE_CAPACITY = 512;

constexpr static unsigned LEVEL_BASIC = 1;
constexpr static unsigned LEVEL_PLAYER = 2;
constexpr static unsigned LEVEL_ENGINE = 3;
constexpr static unsigned LEVEL_NETWORK = 4;

constexpr static std::size_t GRAPH_SAMPLES = 128;
constexpr static float GRAPH_WIDTH = 256.0f;
constexpr static float GRAPH_HEIGHT = 64.0f;
constexpr static float GRAPH_MARGIN = 8.0f;
constexpr static float GRAPH_GAP = 8.0f;
constexpr static float FRAME_GRAPH_LIMIT_MS = 50.0f;
constexpr static float FRAME_GRAPH_GOOD_MS = 1000.0f / 60.0f;
constexpr static float FRAME_GRAPH_FAIR_MS = 1000.0f / 30.0f;
constexpr static float NET_GRAPH_LIMIT_MS = 200.0f;
constexpr static enet_uint32 NET_GRAPH_INTERVAL_MS = 20;
constexpr static enet_uint32 NET_GRAPH_MARK_MS = 500;
constexpr static enet_uint32 RATE_INTERVAL_MS = 1000;
constexpr static enet_uint32 RECV_STALL_MS = 1500;
constexpr static float LOSS_ALERT_PERCENT = 1.0f;

constexpr static ImU32 TEXT_COLOR = IM_COL32(0xFF, 0xFF, 0xFF, 0xFF);
constexpr static ImU32 SHADOW_COLOR = IM_COL32(0x00, 0x00, 0x00, 0xFF);
constexpr static ImU32 ALERT_COLOR = IM_COL32(0xFF, 0x40, 0x40, 0xFF);

struct MetricLine final {
    std::array<char, LINE_CAPACITY> text;
    std::size_t length;
    ImU32 color;
    bool gap_before;
};

struct NetGraphSample final {
    float ping_ms;
    float jitter_ms;
    bool has_loss;
    bool has_mark;
};

// The four cardinal directions in the order of a Minecraft yaw:
// 0 is south, 90 is west, 180 is north and 270 is east
constexpr static std::array<std::string_view, 4> CARDINAL_NAMES = {
    std::string_view("south"),
    std::string_view("west"),
    std::string_view("north"),
    std::string_view("east"),
};
constexpr static std::array<std::string_view, 4> CARDINAL_AXES = {
    std::string_view("positive-Z"),
    std::string_view("negative-X"),
    std::string_view("negative-Z"),
    std::string_view("positive-X"),
};

static cvar::Number<unsigned> s_metrics { 1U, 0U, 4U };

static std::vector<MetricLine> s_lines_left;
static std::vector<MetricLine> s_lines_right;

static std::size_t s_left_used;
static std::size_t s_right_used;

static std::array<float, GRAPH_SAMPLES> s_frame_samples;
static std::size_t s_frame_cursor;

static std::array<NetGraphSample, GRAPH_SAMPLES> s_net_samples;
static enet_uint32 s_net_prev_loss;
static enet_uint32 s_net_prev_time;
static std::size_t s_net_cursor;

static enet_uint32 s_rates_prev_time;
static enet_uint32 s_rates_prev_in_bytes;
static enet_uint32 s_rates_prev_out_bytes;
static enet_uint32 s_rates_prev_in_packets;
static enet_uint32 s_rates_prev_out_packets;

static float s_rates_in_kbps;
static float s_rates_out_kbps;
static float s_rates_in_pktps;
static float s_rates_out_pktps;

static std::string s_gl_version;
static std::string s_gl_renderer;

static float overlay_scale(void)
{
    auto font_scale = std::max(1.0f, ImGui::GetStyle().FontScaleMain);

    return std::max(1.0f, std::ceil(font_scale / 2.0f));
}

static float text_font_size(void)
{
    return fonts::unscii8->LegacySize * overlay_scale();
}

static float text_width(std::string_view text)
{
    auto font = fonts::unscii8;
    auto size = text_font_size();

    auto text_size = font->CalcTextSizeA(size, FLT_MAX, 0.0f, text.data(), text.data() + text.size());
    return text_size.x;
}

static void draw_text(ImDrawList* draw_list, std::string_view text, const ImVec2& pos, ImU32 color, ImU32 shadow)
{
    auto font = fonts::unscii8;
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

static ImU32 frametime_color(float frametime_ms)
{
    if(frametime_ms <= FRAME_GRAPH_GOOD_MS) {
        return IM_COL32(0x40, 0xFF, 0x40, 0xFF);
    }

    if(frametime_ms <= FRAME_GRAPH_FAIR_MS) {
        return IM_COL32(0xFF, 0xFF, 0x40, 0xFF);
    }

    if(frametime_ms <= FRAME_GRAPH_LIMIT_MS) {
        return IM_COL32(0xFF, 0xA0, 0x40, 0xFF);
    }

    return IM_COL32(0xFF, 0x40, 0x40, 0xFF);
}

static std::array<char, 16> format_count(std::size_t value)
{
    std::array<char, 16> text {};

    if(value >= 1000000) {
        std::snprintf(text.data(), text.size(), "%.2fM", static_cast<double>(value) / 1000000.0);
    }
    else if(value >= 1000) {
        std::snprintf(text.data(), text.size(), "%.1fk", static_cast<double>(value) / 1000.0);
    }
    else {
        std::snprintf(text.data(), text.size(), "%zu", value);
    }

    return text;
}

static std::string_view identifier_string(const std::optional<Identifier>& name)
{
    if(name.has_value()) {
        return name->full_string();
    }

    return "unknown";
}

// Returned by value: the caller keeps the surface text and the liquid text at once
static std::array<char, 24> height_string(BlockPos::value_type value)
{
    auto text = std::array<char, 24> {};

    if(value == HEIGHTMAP_UNSET) {
        std::snprintf(text.data(), text.size(), "unset");
        return text;
    }

    std::snprintf(text.data(), text.size(), "%" PRId64, static_cast<std::int64_t>(value));
    return text;
}

static std::string_view world_identifier(void)
{
    if(globals::world == nullptr) {
        return "unknown";
    }

    auto& id = globals::world->id();

    if(id.is_empty()) {
        return "unknown";
    }

    return id.full_string();
}

static void sample_frame(void)
{
    s_frame_samples[s_frame_cursor] = 1000.0f * globals::window_frametime;
    s_frame_cursor += 1;
    s_frame_cursor %= GRAPH_SAMPLES;
}

static void sample_net(void)
{
    auto curtime = enet_time_get();
    auto elapsed = curtime - s_net_prev_time;

    if(elapsed < NET_GRAPH_INTERVAL_MS) {
        return;
    }

    auto& sample = s_net_samples[s_net_cursor];

    auto mark_curtime = curtime / NET_GRAPH_MARK_MS;
    auto mark_prevtime = s_net_prev_time / NET_GRAPH_MARK_MS;
    sample.has_mark = mark_curtime != mark_prevtime;

    s_net_prev_time = curtime;

    s_net_cursor += 1;
    s_net_cursor %= GRAPH_SAMPLES;

    if(globals::peer == nullptr) {
        sample.ping_ms = 0.0f;
        sample.jitter_ms = 0.0f;
        sample.has_loss = false;
        s_net_prev_loss = 0;
        return;
    }

    sample.ping_ms = static_cast<float>(globals::peer->roundTripTime);
    sample.jitter_ms = static_cast<float>(globals::peer->roundTripTimeVariance);

    auto packets_lost = globals::peer->packetsLost;
    sample.has_loss = packets_lost != s_net_prev_loss;
    s_net_prev_loss = packets_lost;
}

static void sample_rates(void)
{
    if(globals::host == nullptr) {
        return;
    }

    auto curtime = enet_time_get();

    if(globals::peer == nullptr) {
        s_rates_in_kbps = 0.0f;
        s_rates_out_kbps = 0.0f;
        s_rates_in_pktps = 0.0f;
        s_rates_out_pktps = 0.0f;

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

    s_rates_in_kbps = static_cast<float>(globals::host->totalReceivedData - s_rates_prev_in_bytes) / seconds * 0.001f;
    s_rates_out_kbps = static_cast<float>(globals::host->totalSentData - s_rates_prev_out_bytes) / seconds * 0.001f;
    s_rates_in_pktps = static_cast<float>(globals::host->totalReceivedPackets - s_rates_prev_in_packets) / seconds;
    s_rates_out_pktps = static_cast<float>(globals::host->totalSentPackets - s_rates_prev_out_packets) / seconds;

    s_rates_prev_time = curtime;
    s_rates_prev_in_bytes = globals::host->totalReceivedData;
    s_rates_prev_out_bytes = globals::host->totalSentData;
    s_rates_prev_in_packets = globals::host->totalReceivedPackets;
    s_rates_prev_out_packets = globals::host->totalSentPackets;
}

static std::string_view line_text(const MetricLine& line)
{
    return std::string_view(line.text.data(), line.length);
}

template<typename... Args>
static void push_line(std::vector<MetricLine>& lines, std::size_t& used, ImU32 color, bool gap_before, const char* format, Args... args)
{
    if(used == lines.size()) {
        lines.emplace_back();
    }

    auto& line = lines[used];
    used += 1;

    line.color = color;
    line.gap_before = gap_before;

    auto written = std::snprintf(line.text.data(), line.text.size(), format, args...);

    if(written < 0) {
        line.length = 0;
        return;
    }

    auto length = static_cast<std::size_t>(written);
    line.length = std::min(length, line.text.size() - 1);
}

static void compose_target(void)
{
    if(auto block_hit = std::get_if<physics::BlockHit>(&player_target::hit)) {
        auto block_name = block_registry::name_of(block_hit->id);
        auto block_text = identifier_string(block_name);

        push_line(s_lines_left, s_left_used, TEXT_COLOR, false, "Looking at: %.*s (%" PRId64 " %" PRId64 " %" PRId64 ")",
            static_cast<int>(block_text.size()), block_text.data(), block_hit->block_pos.x(), block_hit->block_pos.y(),
            block_hit->block_pos.z());
        return;
    }

    if(auto entity_hit = std::get_if<physics::EntityHit>(&player_target::hit)) {
        push_line(s_lines_left, s_left_used, TEXT_COLOR, false, "Looking at: entity #%" PRIu64,
            static_cast<std::uint64_t>(entity_hit->entity));
        return;
    }
}

static void compose_player(void)
{
    if(globals::world == nullptr) {
        return;
    }

    auto& registry = globals::world->registry();

    if(!registry.valid(globals::player)) {
        return;
    }

    if(!registry.all_of<Transform_Intr, Velocity_Intr>(globals::player)) {
        return;
    }

    auto& transform = registry.get<Transform_Intr>(globals::player);
    auto& velocity = registry.get<Velocity_Intr>(globals::player);

    auto world_pos = utils::to_fvec(transform.chunk, transform.local);
    auto block_pos = utils::to_block(transform.chunk, transform.local.cast<LocalPos::value_type>());
    auto chunk_pos = utils::to_chunk(block_pos);

    // Meshed chunks against loaded ones against what still waits for a mesh:
    // a hole in the world is either a chunk nobody asked for or a mesh that
    // never got built, and the three numbers tell them apart
    auto& chunks = globals::world->chunks();
    auto& chunk_registry = globals::world->chunk_registry();
    auto meshed = chunk_registry.view<ChunkMesh>();

    auto world_id = world_identifier();

    // The emhash size_type is 32 bit, while %zu reads 64 bits
    auto meshed_chunks = static_cast<std::size_t>(meshed.size());
    auto loaded_chunks = static_cast<std::size_t>(chunks.size());
    auto pending_meshes = chunk_mesher::backlog;

    push_line(s_lines_left, s_left_used, TEXT_COLOR, true, "C: %zu/%zu (+%zu)", meshed_chunks, loaded_chunks, pending_meshes);
    push_line(s_lines_left, s_left_used, TEXT_COLOR, false, "XYZ: %.5f / %.5f / %.5f", world_pos.x(), world_pos.y(), world_pos.z());
    push_line(s_lines_left, s_left_used, TEXT_COLOR, false, "Block: %" PRId64 " %" PRId64 " %" PRId64, block_pos.x(), block_pos.y(),
        block_pos.z());
    push_line(s_lines_left, s_left_used, TEXT_COLOR, false, "Chunk: %d %d %d in %.*s", chunk_pos.x(), chunk_pos.y(), chunk_pos.z(),
        static_cast<int>(world_id.size()), world_id.data());

    // A yaw of zero looks south, which matches the Minecraft debug screen
    auto yaw = utils::degrees(std::atan2(-client_camera::forward.x(), client_camera::forward.z()));
    auto pitch = -utils::degrees(std::asin(std::clamp(client_camera::forward.y(), -1.0f, 1.0f)));
    auto wrapped = std::fmod(yaw + 360.0f, 360.0f);
    auto cardinal = static_cast<std::size_t>(std::lround(wrapped / 90.0f)) % 4;

    auto cardinal_name = CARDINAL_NAMES[cardinal];
    auto cardinal_axis = CARDINAL_AXES[cardinal];

    push_line(s_lines_left, s_left_used, TEXT_COLOR, false, "Facing: %.*s (Towards %.*s) (%.1f / %.1f)",
        static_cast<int>(cardinal_name.size()), cardinal_name.data(), static_cast<int>(cardinal_axis.size()), cardinal_axis.data(), wrapped,
        pitch);

    auto sky_light = static_cast<unsigned>(BLOCK_LIGHT_MIN);
    auto block_light = static_cast<unsigned>(BLOCK_LIGHT_MIN);

    if(auto chunk = globals::world->find_chunk(chunk_pos)) {
        auto local_pos = utils::to_local(block_pos);
        sky_light = static_cast<unsigned>(chunk->get_light(local_pos, LIGHT_SKY));
        block_light = static_cast<unsigned>(chunk->get_light(local_pos, LIGHT_BLOCK));
    }

    push_line(s_lines_left, s_left_used, TEXT_COLOR, true, "Light: %u (%u sky, %u block)", std::max(sky_light, block_light), sky_light,
        block_light);

    auto biome = globals::world->biome(block_pos);
    auto biome_name = biome_registry::name_of(biome);
    auto biome_text = identifier_string(biome_name);

    push_line(s_lines_left, s_left_used, TEXT_COLOR, false, "Biome: %.*s", static_cast<int>(biome_text.size()), biome_text.data());

    auto day_length = server_vars::day_length_ticks();
    auto phase = world_clock::phase(globals::world->daylight_tick, day_length);
    auto day = world_clock::day(globals::world->daylight_tick, day_length);

    // The phase starts at sunrise, so the clock starts at six in the morning
    auto clock = std::fmod(6.0f + 24.0f * phase, 24.0f);
    auto hours = static_cast<int>(clock);
    auto minutes = static_cast<int>((clock - static_cast<float>(hours)) * 60.0f);

    push_line(s_lines_left, s_left_used, TEXT_COLOR, false, "Time: %02d:%02d, day %" PRIu64, hours, minutes, day);

    auto& definition = globals::world->definition();
    auto& weather_state = globals::world->weather();
    auto weather_tick = globals::world->weather_tick;

    auto weather_type = weather_state.type_at(definition.weather, world_pos.x(), world_pos.z(), weather_tick);
    auto coverage = weather_state.coverage(definition.weather, world_pos.x(), world_pos.z(), weather_tick);
    auto weather_name = weather_registry::name_of(weather_type);
    auto weather_text = identifier_string(weather_name);

    push_line(s_lines_left, s_left_used, TEXT_COLOR, false, "Weather: %.*s (%.0f%%)", static_cast<int>(weather_text.size()),
        weather_text.data(), 100.0f * coverage);

    auto precip_context = precipitation::context(globals::world, definition.weather, world_pos.x(), world_pos.z());
    auto precip_selection = precipitation::select(globals::world, precip_context, world_pos.x(), world_pos.y(), world_pos.z());
    auto precip_name = precipitation_registry::name_of(precip_selection.type);
    auto precip_text = identifier_string(precip_name);

    auto column_x = static_cast<int>(std::floor(world_pos.x()));
    auto column_z = static_cast<int>(std::floor(world_pos.z()));

    auto column = columns::find(column_x, column_z);
    auto column_text = std::array<char, 64> {};

    if(column == nullptr) {
        std::snprintf(column_text.data(), column_text.size(), "no slice");
    }
    else {
        auto surface_text = height_string(column->surface_y);
        auto liquid_text = height_string(column->liquid_y);

        std::snprintf(column_text.data(), column_text.size(), "surface=%s liquid=%s", surface_text.data(), liquid_text.data());
    }

    push_line(s_lines_left, s_left_used, TEXT_COLOR, false, "PrecipitationFront: %.*s (%.0f%%) %s", static_cast<int>(precip_text.size()),
        precip_text.data(), 100.0f * precip_selection.effective_intensity, column_text.data());

    auto horizontal = Eigen::Vector2f(velocity.value.x(), velocity.value.z());
    auto speed = horizontal.norm();

    push_line(s_lines_left, s_left_used, TEXT_COLOR, true, "Vel: %.2f %.2f %.2f (%.1f m/s)", velocity.value.x(), velocity.value.y(),
        velocity.value.z(), speed);

    compose_target();
}

static void compose_left(void)
{
    s_left_used = 0;

    push_line(s_lines_left, s_left_used, TEXT_COLOR, false, "Voxelius %.*s", static_cast<int>(version::full.size()), version::full.data());

    auto framerate = 0.0f;

    if(globals::window_frametime_avg > 0.0f) {
        framerate = 1.0f / globals::window_frametime_avg;
    }

    auto frametime = 1000.0f * globals::window_frametime_avg;
    push_line(s_lines_left, s_left_used, fps_color(framerate), false, "%.0f fps (%.2f ms)", framerate, frametime);

    if(globals::peer) {
        auto ping_ms = static_cast<float>(globals::peer->roundTripTime);
        auto lowest_ms = static_cast<float>(globals::peer->lowestRoundTripTime);
        auto jitter_ms = static_cast<float>(globals::peer->roundTripTimeVariance);

        push_line(s_lines_left, s_left_used, ping_color(ping_ms), false, "Ping: %.0f ms (min %.0f, jitter %.0f)", ping_ms, lowest_ms,
            jitter_ms);
    }

    if(s_metrics.value() < LEVEL_PLAYER) {
        return;
    }

    compose_player();
}

static void compose_right(void)
{
    s_right_used = 0;

    if(s_metrics.value() < LEVEL_ENGINE) {
        return;
    }

    push_line(s_lines_right, s_right_used, TEXT_COLOR, false, "OpenGL: %s", s_gl_version.c_str());
    push_line(s_lines_right, s_right_used, TEXT_COLOR, false, "GPU: %s", s_gl_renderer.c_str());

    auto width = 0;
    auto height = 0;
    auto rate = 0;
    auto fullscreen = false;

    video::query_current_mode(width, height, rate, fullscreen);

    if(rate > 0) {
        push_line(s_lines_right, s_right_used, TEXT_COLOR, false, "Display: %dx%d @ %d Hz", width, height, rate);
    }
    else {
        push_line(s_lines_right, s_right_used, TEXT_COLOR, false, "Display: %dx%d", width, height);
    }

    auto vertices = format_count(globals::num_draw_vertices);
    auto triangles = format_count(globals::num_draw_vertices / 3);

    push_line(s_lines_right, s_right_used, TEXT_COLOR, false, "Draw: %zu DC, %s VTX, %s TRI", globals::num_draw_calls, vertices.data(),
        triangles.data());

    if(s_metrics.value() < LEVEL_NETWORK) {
        return;
    }

    if(globals::peer == nullptr) {
        return;
    }

    auto loss = 100.0f * static_cast<float>(globals::peer->packetLoss) / static_cast<float>(ENET_PEER_PACKET_LOSS_SCALE);
    auto loss_color = loss >= LOSS_ALERT_PERCENT ? ALERT_COLOR : TEXT_COLOR;
    push_line(s_lines_right, s_right_used, loss_color, true, "Loss: %.1f%%", loss);

    push_line(s_lines_right, s_right_used, TEXT_COLOR, false, "In: %.1f kB/s, %.1f pkt/s", s_rates_in_kbps, s_rates_in_pktps);
    push_line(s_lines_right, s_right_used, TEXT_COLOR, false, "Out: %.1f kB/s, %.1f pkt/s", s_rates_out_kbps, s_rates_out_pktps);

    if(globals::peer->packetThrottle < ENET_PEER_PACKET_THROTTLE_SCALE) {
        auto throttle = 100.0f * static_cast<float>(globals::peer->packetThrottle) / static_cast<float>(ENET_PEER_PACKET_THROTTLE_SCALE);

        push_line(s_lines_right, s_right_used, ALERT_COLOR, false, "Throttle: %.0f%%", throttle);
    }

    auto recv_age_ms = static_cast<enet_uint32>(0);

    if(globals::peer->lastReceiveTime) {
        recv_age_ms = enet_time_get() - globals::peer->lastReceiveTime;
    }

    if(recv_age_ms >= RECV_STALL_MS) {
        push_line(s_lines_right, s_right_used, ALERT_COLOR, false, "Recv age: %u ms", recv_age_ms);
    }
}

static float draw_lines(ImDrawList* draw_list, const std::vector<MetricLine>& lines, const ImVec2& origin, bool right_aligned)
{
    auto ystep = 1.5f * text_font_size();

    ImVec2 position = origin;

    for(const auto& line : lines) {
        if(line.gap_before) {
            position.y += LINE_GAP * ystep;
        }

        ImVec2 line_position = position;

        if(right_aligned) {
            line_position.x = position.x - text_width(line_text(line));
        }

        draw_text(draw_list, line_text(line), line_position, line.color, SHADOW_COLOR);
        position.y += ystep;
    }

    return position.y;
}

static float lines_width(const std::vector<MetricLine>& lines)
{
    auto width = 0.0f;

    for(const auto& line : lines) {
        width = std::max(width, text_width(line_text(line)));
    }

    return width;
}

static float graph_header_height(void)
{
    return text_font_size() + 4.0f * overlay_scale();
}

static void draw_graph_header(ImDrawList* draw_list, const ImVec2& origin, float width, std::string_view name, std::string_view range)
{
    auto scale = overlay_scale();
    auto shadow = IM_COL32(0x00, 0x00, 0x00, 0x4F);
    auto dim = IM_COL32(0xFF, 0xFF, 0xFF, 0x4F);

    ImVec2 name_position {};
    name_position.x = origin.x + 4.0f * scale;
    name_position.y = origin.y + 2.0f * scale;

    draw_text(draw_list, name, name_position, TEXT_COLOR, shadow);

    ImVec2 range_position {};
    range_position.x = origin.x + width - 4.0f * scale - text_width(range);
    range_position.y = name_position.y;

    draw_text(draw_list, range, range_position, dim, shadow);
}

static void layout_graph_frame(float top)
{
    auto draw_list = ImGui::GetBackgroundDrawList();
    auto viewport = ImGui::GetMainViewport();
    auto scale = overlay_scale();

    auto total_width = GRAPH_WIDTH * scale;
    auto total_height = GRAPH_HEIGHT * scale;
    auto margin = GRAPH_MARGIN * scale;

    ImVec2 origin {};
    origin.x = viewport->Size.x - total_width - margin;
    origin.y = top;

    ImVec2 background_end {};
    background_end.x = origin.x + total_width;
    background_end.y = origin.y + total_height;

    draw_list->AddRectFilled(origin, background_end, IM_COL32(0x00, 0x00, 0x00, 0x80));

    auto plot_top = origin.y + graph_header_height();
    auto plot_height = background_end.y - plot_top;

    std::array<char, 16> range {};
    std::snprintf(range.data(), range.size(), "%.0f ms", FRAME_GRAPH_LIMIT_MS);
    draw_graph_header(draw_list, origin, total_width, "frame", range.data());

    auto grid_color = IM_COL32(0xFF, 0xFF, 0xFF, 0x30);
    auto bar_width = total_width / static_cast<float>(GRAPH_SAMPLES);

    for(std::size_t i = 0; i < GRAPH_SAMPLES; ++i) {
        auto index = (s_frame_cursor + i) % GRAPH_SAMPLES;
        auto sample = s_frame_samples[index];

        auto bar_height = plot_height * std::min(sample / FRAME_GRAPH_LIMIT_MS, 1.0f);

        if(bar_height <= 0.0f) {
            continue;
        }

        ImVec2 bar_start {};
        bar_start.x = origin.x + bar_width * static_cast<float>(i);
        bar_start.y = plot_top + plot_height - bar_height;

        ImVec2 bar_end {};
        bar_end.x = bar_start.x + bar_width;
        bar_end.y = plot_top + plot_height;

        draw_list->AddRectFilled(bar_start, bar_end, frametime_color(sample));
    }

    // One line for 60 fps and one for 30 fps, so a stutter has a reference
    for(auto frametime_ms : std::array { FRAME_GRAPH_GOOD_MS, FRAME_GRAPH_FAIR_MS }) {
        auto fraction = frametime_ms / FRAME_GRAPH_LIMIT_MS;

        ImVec2 line_start {};
        line_start.x = origin.x;
        line_start.y = plot_top + plot_height * (1.0f - fraction);

        ImVec2 line_end {};
        line_end.x = background_end.x;
        line_end.y = line_start.y;

        draw_list->AddLine(line_start, line_end, grid_color);
    }
}

static void layout_graph_net(float top)
{
    auto draw_list = ImGui::GetBackgroundDrawList();
    auto viewport = ImGui::GetMainViewport();
    auto scale = overlay_scale();

    auto total_width = GRAPH_WIDTH * scale;
    auto total_height = GRAPH_HEIGHT * scale;
    auto margin = GRAPH_MARGIN * scale;

    ImVec2 origin {};
    origin.x = viewport->Size.x - total_width - margin;
    origin.y = top;

    ImVec2 background_end {};
    background_end.x = origin.x + total_width;
    background_end.y = origin.y + total_height;

    auto background_color = IM_COL32(0x00, 0x00, 0x00, 0x80);
    auto grid_color = IM_COL32(0xFF, 0xFF, 0xFF, 0x30);
    auto jitter_color = IM_COL32(0xFF, 0xFF, 0xFF, 0x60);
    auto loss_color = IM_COL32(0xFF, 0x40, 0x40, 0xFF);

    draw_list->AddRectFilled(origin, background_end, background_color);

    auto plot_top = origin.y + graph_header_height();
    auto plot_height = background_end.y - plot_top;

    std::array<char, 16> range {};
    std::snprintf(range.data(), range.size(), "%.0f ms", NET_GRAPH_LIMIT_MS);
    draw_graph_header(draw_list, origin, total_width, "net", range.data());

    auto bar_width = total_width / static_cast<float>(GRAPH_SAMPLES);

    for(std::size_t i = 0; i < GRAPH_SAMPLES; ++i) {
        auto index = static_cast<std::size_t>((s_net_cursor + i) % GRAPH_SAMPLES);
        auto& sample = s_net_samples[index];

        auto bar_height = plot_height * std::min(sample.ping_ms / NET_GRAPH_LIMIT_MS, 1.0f);
        auto jitter_height = plot_height * std::min((sample.ping_ms + sample.jitter_ms) / NET_GRAPH_LIMIT_MS, 1.0f);

        ImVec2 bar_start {};
        bar_start.x = origin.x + bar_width * static_cast<float>(i);
        bar_start.y = plot_top + plot_height - bar_height;

        ImVec2 bar_end {};
        bar_end.x = bar_start.x + bar_width;
        bar_end.y = plot_top + plot_height;

        if(sample.has_loss) {
            bar_start.y = plot_top;
        }
        else if(jitter_height > bar_height) {
            ImVec2 cap_start {};
            cap_start.x = bar_start.x;
            cap_start.y = plot_top + plot_height - jitter_height;

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
            mark_start.y = plot_top;

            ImVec2 mark_end {};
            mark_end.x = bar_start.x;
            mark_end.y = background_end.y;

            draw_list->AddLine(mark_start, mark_end, grid_color);
        }
    }

    for(auto fraction : std::array { 0.25f, 0.50f, 0.75f }) {
        ImVec2 line_start {};
        line_start.x = origin.x;
        line_start.y = plot_top + plot_height * (1.0f - fraction);

        ImVec2 line_end {};
        line_end.x = background_end.x;
        line_end.y = line_start.y;

        draw_list->AddLine(line_start, line_end, grid_color);
    }
}

void metrics::init(void)
{
    auto& metrics_binding = cvar::bind_var(s_metrics, "metrics");
    metrics_binding.set_help("Metrics overlay verbosity: 0 off, 1 basic, 2 player, 3 engine, 4 network");
    metrics_binding.set_flags(FCVAR_ARCHIVE);

    if(auto gl_version = glGetString(GL_VERSION)) {
        s_gl_version = reinterpret_cast<const char*>(gl_version);
    }

    if(auto gl_renderer = glGetString(GL_RENDERER)) {
        s_gl_renderer = reinterpret_cast<const char*>(gl_renderer);
    }
}

void metrics::update(void)
{
    sample_frame();
    sample_net();
    sample_rates();
}

void metrics::layout(void)
{
    if(s_metrics.value() < LEVEL_BASIC) {
        return;
    }

    compose_left();
    compose_right();

    s_lines_left.resize(s_left_used);
    s_lines_right.resize(s_right_used);

    auto draw_list = ImGui::GetBackgroundDrawList();
    auto viewport = ImGui::GetMainViewport();
    auto scale = overlay_scale();
    auto margin = TEXT_MARGIN * scale;

    ImVec2 left_origin {};
    left_origin.x = TEXT_XPOS * scale;
    left_origin.y = TEXT_YPOS * scale;

    auto left_width = lines_width(s_lines_left);
    auto right_width = lines_width(s_lines_right);

    auto right_edge = viewport->Size.x - margin;
    auto bottom = draw_lines(draw_list, s_lines_left, left_origin, false);

    if(!s_lines_right.empty()) {
        if(right_edge - right_width >= left_origin.x + left_width + 2.0f * margin) {
            ImVec2 right_origin {};
            right_origin.x = right_edge;
            right_origin.y = left_origin.y;

            draw_lines(draw_list, s_lines_right, right_origin, true);
        }
        else {
            auto ystep = 1.5f * text_font_size();

            ImVec2 stacked_origin {};
            stacked_origin.x = left_origin.x;
            stacked_origin.y = bottom + ystep;

            draw_lines(draw_list, s_lines_right, stacked_origin, false);
        }
    }

    auto graph_height = GRAPH_HEIGHT * scale;
    auto graph_gap = GRAPH_GAP * scale;
    auto graph_top = viewport->Size.y;

    if(s_metrics.value() >= LEVEL_ENGINE) {
        graph_top -= graph_height;
        layout_graph_frame(graph_top);
        graph_top -= graph_gap;
    }

    if(s_metrics.value() >= LEVEL_NETWORK) {
        graph_top -= graph_height;
        layout_graph_net(graph_top);
    }
}

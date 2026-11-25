#include "server/pch.hh"

#include "server/net/sessions.hh"

#include "core/cvar/host.hh"
#include "core/utils/crc64.hh"
#include "core/utils/epoch.hh"
#include "core/version.hh"

#include "shared/component/transform.hh"
#include "shared/constant.hh"
#include "shared/cvar/server.hh"
#include "shared/entity/class_registry.hh"
#include "shared/net/packet_auth.hh"
#include "shared/net/packet_entity.hh"
#include "shared/net/packet_session.hh"
#include "shared/net/packet_world.hh"
#include "shared/net/protocol.hh"
#include "shared/utils/coord.hh"
#include "shared/utils/entity.hh"
#include "shared/world/biome_registry.hh"
#include "shared/world/block_registry.hh"
#include "shared/world/fluid_registry.hh"
#include "shared/world/precip_registry.hh"
#include "shared/world/tint_registry.hh"
#include "shared/world/weather.hh"
#include "shared/world/weather_registry.hh"
#include "shared/world/world.hh"
#include "shared/world/world_clock.hh"

#include "server/globals.hh"
#include "server/net/host.hh"
#include "server/net/invites.hh"
#include "server/net/operators.hh"
#include "server/net/receive.hh"
#include "server/net/replication.hh"
#include "server/net/whitelist.hh"
#include "server/universe.hh"
#include "server/world/world.hh"

static std::vector<Session> s_sessions;
static vx::hash_map<std::string, Session*> s_username_map;
static vx::hash_map<std::uint64_t, Session*> s_identity_map;

cvar::Boolean sessions::strict_version { false };

std::uint16_t sessions::num_players = 0;

static std::mt19937_64 s_randomizer;

static void send_world_state(Session* session)
{
    auto world = session->world;

    packet::World_Info world_info {};
    world_info.fill(world->definition());
    world_info.weather_seed = world->weather_seed;
    protocol::send(world_info, session->peer);

    packet::World_Timings timings {};
    timings.current_tick = globals::current_tick;
    timings.daylight_tick = world->daylight_tick;
    timings.day = world_clock::day(world->daylight_tick, server_vars::day_length_ticks());
    timings.weather_tick = world->weather_tick;
    timings.season_tick = 0; // Reserved for the season cycle
    protocol::send(timings, session->peer);
}

static bool is_outdated_client(std::uint32_t major, std::uint32_t minor, std::uint32_t patch)
{
    if(version::major > major) {
        return true;
    }

    if(sessions::strict_version.value()) {
        if(version::minor > minor) {
            return true;
        }

        if(version::patch > patch) {
            return true;
        }
    }

    return false;
}

static bool is_outdated_server(std::uint32_t major, std::uint32_t minor, std::uint32_t patch)
{
    if(major > version::major) {
        return true;
    }

    if(sessions::strict_version.value()) {
        if(minor > version::minor) {
            return true;
        }

        if(patch > version::patch) {
            return true;
        }
    }

    return false;
}

static void on_auth_request(const packet::Auth_Request& packet)
{
    if(is_outdated_client(packet.version_major, packet.version_minor, packet.version_patch)) {
        packet::Session_Disconnect response {};
        response.reason = packet::Session_Disconnect::OUTDATED_CLIENT;
        protocol::send(response, packet.peer);
        enet_peer_disconnect_later(packet.peer, 0);
        return;
    }

    if(is_outdated_server(packet.version_major, packet.version_minor, packet.version_patch)) {
        packet::Session_Disconnect response {};
        response.reason = packet::Session_Disconnect::OUTDATED_SERVER;
        protocol::send(response, packet.peer);
        enet_peer_disconnect_later(packet.peer, 0);
        return;
    }

    auto access = operators::contains(packet.pkey) ? session_access::OPERATOR : session_access::PLAYER;

    // An operator is trusted by definition and does not need a second line in the whitelist
    auto client_whitelisted = !whitelist::enabled.value() || whitelist::contains(packet.pkey) || access == session_access::OPERATOR;

    if(!client_whitelisted && packet.invite_code) {
        client_whitelisted = invites::consume(packet.invite_code, packet.pkey);
    }

    if(!client_whitelisted) {
        packet::Session_Disconnect response {};
        response.reason = packet::Session_Disconnect::NOT_WHITELISTED;
        protocol::send(response, packet.peer);
        enet_peer_disconnect_later(packet.peer, 0);
        return;
    }

    auto checksums_match = true;
    checksums_match = checksums_match && packet.biomes_hash == biome_registry::checksum();
    checksums_match = checksums_match && packet.blocks_hash == block_registry::checksum();
    checksums_match = checksums_match && packet.fluids_hash == fluid_registry::checksum();
    checksums_match = checksums_match && packet.tints_hash == tint_registry::checksum();
    checksums_match = checksums_match && packet.ents_hash == class_registry::checksum();
    checksums_match = checksums_match && packet.weather_hash == weather_registry::checksum();
    checksums_match = checksums_match && packet.precip_hash == precip_registry::checksum();

    if(!checksums_match) {
        packet::Session_Disconnect response {};
        response.reason = packet::Session_Disconnect::CHECKSUM_MISMATCH;
        protocol::send(response, packet.peer);
        enet_peer_disconnect_later(packet.peer, 0);
        return;
    }

    auto session = sessions::create(packet.peer, packet.username);

    if(session == nullptr) {
        packet::Session_Disconnect response {};
        response.reason = packet::Session_Disconnect::SERVER_IS_FULL;
        protocol::send(response, packet.peer);
        enet_peer_disconnect_later(packet.peer, 0);
        return;
    }

    session->pkey = packet.pkey;
    session->access = access;

    auto timestamp_sec = utils::epoch_seconds();
    auto timestamp_min = timestamp_sec / 60;
    auto timestamp_str = std::to_string(timestamp_min);
    auto distribution = std::uniform_int_distribution<std::uint64_t>(0, UINT8_MAX);

    for(std::size_t i = 0; i < session->nonce.size(); ++i) {
        session->nonce[i] = static_cast<std::byte>(distribution(s_randomizer));
    }

    session->salted_nonce.resize(session->nonce.size() + timestamp_str.size());
    std::memcpy(session->salted_nonce.data(), session->nonce.data(), session->nonce.size());
    std::memcpy(session->salted_nonce.data() + session->nonce.size(), timestamp_str.data(), timestamp_str.size());

    session->identity = utils::crc64(session->pkey);
    session->state = session_state::CHALLENGE;

    auto access_string = access == session_access::OPERATOR ? " as operator" : "";
    msg::log_info("{} ({}) connected{}", session->username, session->identity, access_string);

    packet::Auth_Challenge response {};
    response.nonce = session->nonce;
    protocol::send(response, packet.peer);
}

static void on_auth_response(const packet::Auth_Response& packet)
{
    auto session = sessions::find(packet.peer);

    if(session == nullptr) {
        packet::Session_Disconnect response {};
        response.reason = packet::Session_Disconnect::UNSPECIFIED;
        protocol::send(response, packet.peer);
        enet_peer_disconnect_later(packet.peer, 0);
        return;
    }

    if(!ed25519::verify(session->pkey, session->salted_nonce, packet.signature)) {
        packet::Session_Disconnect response {};
        response.reason = packet::Session_Disconnect::INVALID_SIGNATURE;
        protocol::send(response, packet.peer);
        enet_peer_disconnect_later(packet.peer, 0);
        sessions::destroy(session, packet::Session_Disconnect::INVALID_SIGNATURE);
        return;
    }

    session->state = session_state::CONNECTED;

    msg::log_info("{} ({}) authenticated successfully", session->username, session->identity);

    packet::Auth_Admission response {};
    response.client_id = session->client_id;
    response.identity = session->identity;
    response.username = session->username;
    protocol::send(response, packet.peer);

    session->world = universe::default_world();

    auto& registry = session->world->registry();

    replication::synchronize(session);

    // The world shape, the seed and the weather seeds do not change, so they travel once.
    // The one-shot state packets then fix the client's time, drift and cloud deck before the first periodic update
    send_world_state(session);

    auto& definition = session->world->definition();
    auto bpos = definition.spawn;
    auto player = utils::spawn_player(registry, bpos);
    session->player = player;

    SessionRef ref {};
    ref.ptr = session;
    registry.emplace<SessionRef>(player, std::move(ref));

    packet::Entity_Client response_2 {};
    response_2.entity = player;
    protocol::send(response_2, packet.peer);
}

static void on_disconnect(const packet::Session_Disconnect& packet)
{
    auto session = sessions::find(packet.peer);

    sessions::destroy(session, packet.reason);
}

static std::string sanitize_username(std::string_view username)
{
    auto size = username.size();
    size = std::min(size, constant::USERNAME_MAX_LENGTH);

    std::string sanitized;
    sanitized.reserve(size);

    for(std::size_t i = 0; i < size; ++i) {
        auto character = static_cast<unsigned char>(username[i]);
        auto is_allowed = false;

        if(i > 0) {
            is_allowed = is_allowed || std::isalnum(character);
            is_allowed = is_allowed || character == '_';
        }
        else {
            is_allowed = is_allowed || std::isalpha(character);
        }

        is_allowed = is_allowed && character < 128;

        if(is_allowed) {
            sanitized.push_back(character);
        }
        else {
            sanitized.push_back('X');
        }
    }

    auto appender = 1;
    auto original = sanitized.substr(0, 60);

    while(s_username_map.contains(sanitized)) {
        sanitized = std::format("{}{}", original, appender);
        appender += 1;
    }

    return sanitized;
}

static Session* find_target(std::string_view name)
{
    std::uint16_t client_id;

    if(!name.empty() && name.size() <= 5) {
        auto check = std::from_chars(name.data(), name.data() + name.size(), client_id);

        if(check.ec == std::errc {} && check.ptr == name.data() + name.size()) {
            if(auto session = sessions::find(client_id)) {
                return session;
            }
        }
    }

    return sessions::find(name);
}

static cvar::status cmd_who(const cvar::Context&, std::span<const std::string_view>)
{
    for(auto& session : sessions::all()) {
        if(session.state == session_state::UNCONNECTED) {
            continue;
        }

        auto access = session.access == session_access::OPERATOR ? "operator" : "player";

        msg::log_info("id={} user={} identity={} access={} pkey={}", session.client_id, session.username, session.identity, access,
            ed25519::export_pkey(session.pkey));
    }

    return cvar::status::OK;
}

static cvar::status cmd_op(const cvar::Context&, std::span<const std::string_view> args)
{
    if(args.empty()) {
        msg::log_warning("usage: op <public key>");

        return cvar::status::ARGUMENT;
    }

    auto pkey = ed25519::import_pkey(args[0]);

    if(!pkey.has_value()) {
        msg::log_warning("{}: not a valid public key", args[0]);

        return cvar::status::ARGUMENT;
    }

    operators::add(pkey.value());
    sessions::refresh_access(pkey.value(), session_access::OPERATOR);

    msg::log_info("{}: added to operators.txt", args[0]);

    return cvar::status::OK;
}

static cvar::status cmd_deop(const cvar::Context&, std::span<const std::string_view> args)
{
    if(args.empty()) {
        msg::log_warning("usage: deop <public key>");

        return cvar::status::ARGUMENT;
    }

    auto pkey = ed25519::import_pkey(args[0]);

    if(!pkey.has_value()) {
        msg::log_warning("{}: not a valid public key", args[0]);

        return cvar::status::ARGUMENT;
    }

    operators::remove(pkey.value());
    sessions::refresh_access(pkey.value(), session_access::PLAYER);

    msg::log_info("{}: removed from operators.txt", args[0]);

    return cvar::status::OK;
}

static cvar::status cmd_kick(const cvar::Context& ctx, std::span<const std::string_view> args)
{
    if(args.empty()) {
        msg::log_warning("usage: kick <client id or username>");

        return cvar::status::ARGUMENT;
    }

    auto target = find_target(args[0]);

    if(target == nullptr) {
        msg::log_warning("{}: not connected", args[0]);

        return cvar::status::ARGUMENT;
    }

    // The caller's own log reply would be sent to a session that no longer has a peer
    if(ctx.client_id.has_value() && target->client_id == ctx.client_id.value()) {
        msg::log_warning("{}: you cannot kick yourself", target->username);

        return cvar::status::ARGUMENT;
    }

    msg::log_info("{} ({}) was kicked", target->username, target->identity);

    packet::Session_Disconnect response {};
    response.reason = packet::Session_Disconnect::KICKED;
    protocol::send(response, target->peer);

    enet_peer_disconnect_later(target->peer, 0);

    sessions::destroy(target, packet::Session_Disconnect::KICKED);

    return cvar::status::OK;
}

void sessions::set_access(Session* session, session_access access)
{
    if(session->access == access) {
        return;
    }

    session->access = access;

    // The command list a client knows about depends on its access level
    if(session->state == session_state::CONNECTED) {
        replication::synchronize(session);
    }

    if(access == session_access::OPERATOR) {
        msg::log_info("{} ({}) is now an operator", session->username, session->identity);
    }
    else {
        msg::log_info("{} ({}) is no longer an operator", session->username, session->identity);
    }
}

void sessions::refresh_access(const ed25519::pkey_type& pkey, session_access access)
{
    for(auto& session : all()) {
        if(session.state != session_state::UNCONNECTED && session.pkey == pkey) {
            set_access(&session, access);
        }
    }
}

std::span<Session> sessions::all(void)
{
    return s_sessions;
}

void sessions::init(void)
{
    auto& net_strict_version_binding = cvar::bind_var(strict_version, "net_strict_version");
    net_strict_version_binding.set_help("Reject clients with a different protocol version");
    net_strict_version_binding.set_flags(FCVAR_ARCHIVE);

    auto& who_binding = cvar::bind_cmd("who");
    who_binding.set_help("List connected sessions and their access levels");
    who_binding.set_flags(FCVAR_RECALL | FCVAR_ADMIN);
    who_binding.connect<&cmd_who>();

    auto& op_binding = cvar::bind_cmd("op");
    op_binding.set_help("Add a public key to operators.txt and promote that player");
    op_binding.set_flags(FCVAR_RECALL | FCVAR_ADMIN);
    op_binding.connect<&cmd_op>();

    auto& deop_binding = cvar::bind_cmd("deop");
    deop_binding.set_help("Remove a public key from operators.txt and demote that player");
    deop_binding.set_flags(FCVAR_RECALL | FCVAR_ADMIN);
    deop_binding.connect<&cmd_deop>();

    auto& kick_binding = cvar::bind_cmd("kick");
    kick_binding.set_help("Disconnect a player by client ID or username");
    kick_binding.set_flags(FCVAR_RECALL | FCVAR_ADMIN);
    kick_binding.connect<&cmd_kick>();

    std::random_device noise;
    s_randomizer.seed(noise());

    globals::dispatcher.sink<packet::Auth_Request>().connect<&on_auth_request>();
    globals::dispatcher.sink<packet::Auth_Response>().connect<&on_auth_response>();
    globals::dispatcher.sink<packet::Session_Disconnect>().connect<&on_disconnect>();
}

void sessions::init_late(void)
{
    s_username_map.clear();
    s_identity_map.clear();
    s_sessions.resize(host::max_players.value());

    for(std::size_t i = 0; i < host::max_players.value(); ++i) {
        auto& session = s_sessions[i];
        session.state = session_state::UNCONNECTED;
        session.access = session_access::PLAYER;
        session.client_id = UINT16_MAX;
        session.identity = UINT64_MAX;
        session.username = std::string {};
        session.player = entt::null;
        session.peer = nullptr;
        session.known_entities.clear();
        session.cvar_ratelimit_tick = 0;
        session.cvar_ratelimit_tokens = 0.0f;
    }
}

void sessions::shutdown(void)
{
    for(auto& session : s_sessions) {
        if(session.peer) {
            packet::Session_Disconnect packet {};
            packet.reason = packet::Session_Disconnect::SERVER_SHUTDOWN;
            protocol::send(packet, session.peer);
            enet_peer_disconnect_later(session.peer, 0);
        }
    }

    s_username_map.clear();
    s_identity_map.clear();
    s_sessions.clear();
}

Session* sessions::create(ENetPeer* peer, std::string_view username)
{
    for(std::size_t i = 0; i < s_sessions.size(); ++i) {
        auto& session = s_sessions[i];

        if(session.peer == nullptr) {
            session.state = session_state::UNCONNECTED;
            session.access = session_access::PLAYER;
            session.client_id = static_cast<std::uint16_t>(i);
            session.identity = UINT64_MAX;
            session.username = sanitize_username(username);
            session.player = entt::null;
            session.world = nullptr;
            session.peer = peer;
            session.known_entities.clear();
            session.cvar_ratelimit_tick = globals::current_tick;
            session.cvar_ratelimit_tokens = 1.0f; // Allow one command right after connecting

            s_username_map.insert_or_assign(session.username, &session);
            s_identity_map.insert_or_assign(session.identity, &session);

            peer->data = &session;

            num_players += 1;

            return &session;
        }
    }

    return nullptr;
}

Session* sessions::find(std::string_view username)
{
    auto it = s_username_map.find(username);

    if(it == s_username_map.cend()) {
        return nullptr;
    }

    return it->second;
}

Session* sessions::find(std::uint16_t client_id)
{
    if(client_id < s_sessions.size()) {
        auto& session = s_sessions[client_id];

        if(session.peer == nullptr || session.state == session_state::UNCONNECTED) {
            return nullptr;
        }

        return &session;
    }

    return nullptr;
}

Session* sessions::find(std::uint64_t identity)
{
    auto it = s_identity_map.find(identity);

    if(it == s_identity_map.cend()) {
        return nullptr;
    }

    return it->second;
}

Session* sessions::find(const ENetPeer* peer)
{
    if(peer == nullptr) {
        return nullptr;
    }

    auto session = reinterpret_cast<Session*>(peer->data);

    if(session == nullptr || session->state == session_state::UNCONNECTED) {
        return nullptr;
    }

    return session;
}

void sessions::destroy(Session* session, std::optional<std::uint32_t> reason)
{
    if(session) {
        std::string_view reason_string;

        if(reason.has_value()) {
            reason_string = packet::Session_Disconnect::reason_string_server(reason.value());
        }
        else {
            reason_string = std::string_view("connection closed");
        }

        msg::log_info("{} ({}) disconnected: {}", session->username, session->identity, reason_string);

        if(session->peer) {
            session->peer->data = nullptr;
        }

        s_username_map.erase(session->username);
        s_identity_map.erase(session->identity);

        if(session->world) {
            auto& registry = session->world->registry();

            if(registry.valid(session->player)) {
                registry.destroy(session->player);
            }
        }

        session->state = session_state::UNCONNECTED;
        session->access = session_access::PLAYER;
        session->client_id = UINT16_MAX;
        session->identity = UINT64_MAX;
        session->username = std::string {};
        session->player = entt::null;
        session->world = nullptr;
        session->peer = nullptr;
        session->known_entities.clear();
        session->stream_sent_chunks.clear();
        session->stream_sent_biomes.clear();
        session->stream_sent_columns.clear();
        session->stream_wanted_chunks.clear();
        session->stream_cursor_chunks = 0;
        session->stream_budget_chunks = 0;
        session->stream_rebuild = true;
        session->cvar_ratelimit_tick = 0;
        session->cvar_ratelimit_tokens = 0.0f;

        num_players -= 1;
    }
}

bool sessions::transfer(Session* session, const std::shared_ptr<ServerWorld>& world, std::optional<BlockPos> position)
{
    if(session == nullptr || world == nullptr) {
        return false;
    }

    if(session->world.get() == world.get()) {
        auto& registry = world->registry();

        if(!registry.valid(session->player)) {
            return false;
        }

        if(!position.has_value() || !registry.all_of<Transform>(session->player)) {
            return true;
        }

        registry.patch<Transform>(session->player, [&](Transform& transform) {
            transform.chunk = utils::to_chunk(position.value());
            auto local = utils::to_local(position.value());
            transform.local = local.cast<float>();
        });

        return true;
    }

    if(session->world == nullptr) {
        return false;
    }

    auto old_world = session->world;
    auto old_entity = session->player;
    auto& old_registry = old_world->registry();

    if(!old_registry.valid(old_entity)) {
        return false;
    }

    auto& definition = world->definition();
    auto destination = definition.spawn;

    if(auto transform = old_registry.try_get<Transform>(old_entity)) {
        destination = utils::to_block(transform->chunk, transform->local.cast<LocalPos::value_type>());
    }

    if(position.has_value()) {
        destination = position.value();
    }

    // Switch the session before the transfer: the spawn must reach the target world's sessions,
    // and the old entity's remove must not reach this one. A remove for the player under its old ID makes the client disconnect
    auto previous_view_box = session->view_box;
    auto previous_known = std::move(session->known_entities);

    session->world = world;
    session->player = entt::null;

    session->view_box = {};
    session->known_entities.clear();
    session->stream_view_box = {};
    session->interest_box = ChunkAlignedBoxXZ(ChunkPosXZ::Ones(), ChunkPosXZ::Zero());
    session->stream_player_chunk = ChunkPos::Zero();
    session->stream_sent_chunks.clear();
    session->stream_sent_biomes.clear();
    session->stream_sent_columns.clear();
    session->stream_wanted_chunks.clear();
    session->stream_cursor_chunks = 0;
    session->stream_budget_chunks = 0;
    session->stream_rebuild = true;

    auto& registry = world->registry();
    auto entity = utils::transfer(old_registry, old_entity, registry);

    if(entity == entt::null) {
        session->world = old_world;
        session->player = old_entity;
        session->view_box = previous_view_box;
        session->known_entities = std::move(previous_known);

        return false;
    }

    if(registry.all_of<Transform>(entity)) {
        registry.patch<Transform>(entity, [&](Transform& transform) {
            transform.chunk = utils::to_chunk(destination);
            auto local = utils::to_local(destination);
            transform.local = local.cast<float>();
        });
    }

    session->player = entity;

    SessionRef ref {};
    ref.ptr = session;
    registry.emplace<SessionRef>(entity, std::move(ref));

    receive::forget_peer(session->peer);

    send_world_state(session);

    packet::Entity_Client client {};
    client.entity = entity;
    protocol::send(client, session->peer);

    return true;
}
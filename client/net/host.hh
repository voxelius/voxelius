#ifndef A70EA99C_6856_46EC_AED0_0ECCD00B82F2
#define A70EA99C_6856_46EC_AED0_0ECCD00B82F2

class HostConnectEvent final {
public:
    constexpr explicit HostConnectEvent(ENetEvent event);
    constexpr const ENetEvent& event(void) const;

private:
    ENetEvent m_event {};
};

class HostDisconnectEvent final {
public:
    constexpr explicit HostDisconnectEvent(ENetEvent event);
    constexpr const ENetEvent& event(void) const;

private:
    ENetEvent m_event {};
};

namespace host
{
void init(void);
void shutdown(void);
void fixed_update_late(void);
} // namespace host

namespace host
{
bool connect(std::string_view address, std::uint16_t port);
void disconnect(void);
} // namespace host

constexpr HostConnectEvent::HostConnectEvent(ENetEvent event) : m_event(std::move(event))
{
    // empty
}

constexpr const ENetEvent& HostConnectEvent::event(void) const
{
    return m_event;
}

constexpr HostDisconnectEvent::HostDisconnectEvent(ENetEvent event) : m_event(std::move(event))
{
    // empty
}

constexpr const ENetEvent& HostDisconnectEvent::event(void) const
{
    return m_event;
}

#endif /* A70EA99C_6856_46EC_AED0_0ECCD00B82F2 */

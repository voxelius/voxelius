#ifndef F28A4C61_7D35_4E9B_A1C8_6B0E3D7F5A29
#define F28A4C61_7D35_4E9B_A1C8_6B0E3D7F5A29

class World;

// Sets the active world for the duration of a call. The server uses it
// around every entry that runs mod code, because a callback has no world
// argument and reads the active world instead.
class WorldScope final {
public:
    explicit WorldScope(std::shared_ptr<World> world);
    ~WorldScope(void);

    WorldScope(const WorldScope& other) = delete;
    WorldScope(WorldScope&& other) = delete;
    WorldScope& operator=(const WorldScope& other) = delete;
    WorldScope& operator=(WorldScope&& other) = delete;

private:
    std::shared_ptr<World> m_previous;
};

#endif /* F28A4C61_7D35_4E9B_A1C8_6B0E3D7F5A29 */

#ifndef F28A4C61_7D35_4E9B_A1C8_6B0E3D7F5A29
#define F28A4C61_7D35_4E9B_A1C8_6B0E3D7F5A29

class World;

// Sets the active world for the duration of an API call. Use it only around
// code that calls the mod API: a callback has no world argument and reads the
// active world instead. Any other caller takes its world as an argument
class WorldScope final {
public:
    static World* active(void);

    explicit WorldScope(World* world);
    ~WorldScope(void);

    WorldScope(const WorldScope& other) = delete;
    WorldScope(WorldScope&& other) = delete;
    WorldScope& operator=(const WorldScope& other) = delete;
    WorldScope& operator=(WorldScope&& other) = delete;

private:
    World* m_previous;
};

#endif /* F28A4C61_7D35_4E9B_A1C8_6B0E3D7F5A29 */

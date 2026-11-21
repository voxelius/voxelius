#ifndef B8C3E152_9F47_4A6D_B0E1_7C5D2A9F4B63
#define B8C3E152_9F47_4A6D_B0E1_7C5D2A9F4B63

class ReadBuffer;
class WriteBuffer;

struct WorldVisuals final {
    struct Sky final {
        struct Gradient final {
            Eigen::Vector3f horizon;
            Eigen::Vector3f zenith;
        };

        Gradient day;
        Gradient night;

        float twilight_height;
    };

    struct Sun final {
        float tilt;
    };

    struct Moon final {
        Eigen::Vector3f color;
    };

    struct Stars final {
        std::uint32_t count;
    };

    struct Clouds final {
        // empty
    };

    static void encode(const WorldVisuals& visuals, WriteBuffer& buffer);
    static void decode(WorldVisuals& visuals, ReadBuffer& buffer);

    std::optional<Sky> sky;
    std::optional<Sun> sun;
    std::optional<Moon> moon;
    std::optional<Stars> stars;
    std::optional<Clouds> clouds;
};

#endif /* B8C3E152_9F47_4A6D_B0E1_7C5D2A9F4B63 */

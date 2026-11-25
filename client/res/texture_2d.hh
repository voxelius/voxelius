#ifndef BBB0B8F1_CB2A_42FD_9123_7D5E5339FFBE
#define BBB0B8F1_CB2A_42FD_9123_7D5E5339FFBE

constexpr static std::uint32_t RESFLAG_TEX_FLIP = 1 << 8;
constexpr static std::uint32_t RESFLAG_TEX_GRAY = 1 << 9;     // 8-bit grayscale when set, RGBA8888 otherwise
constexpr static std::uint32_t RESFLAG_TEX_NEAREST = 1 << 10; // Nearest filtering and no mipmaps

struct Texture2D final {
    static void register_resource(void);

    int width;
    int height;

    ImTextureID imgui;
    GLuint handle;
};

#endif /* BBB0B8F1_CB2A_42FD_9123_7D5E5339FFBE */

#ifndef BBB0B8F1_CB2A_42FD_9123_7D5E5339FFBE
#define BBB0B8F1_CB2A_42FD_9123_7D5E5339FFBE

constexpr static std::uint32_t RESFLAG_TEX_FLIP = 1 << 8;     ///< Flip image vertically on load
constexpr static std::uint32_t RESFLAG_TEX_GRAY = 1 << 9;     ///< If set, the pixel data is 8-bit grayscale, otherwise it's RGBA8888
constexpr static std::uint32_t RESFLAG_TEX_NEAREST = 1 << 10; ///< Use nearest-neighbor filtering and skip mipmaps

struct Texture2D final {
    static void register_resource(void);

    int width;
    int height;

    ImTextureID imgui;
    GLuint handle;
};

#endif /* BBB0B8F1_CB2A_42FD_9123_7D5E5339FFBE */

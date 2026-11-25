#ifndef FE3E3D02_BCDA_4813_9EEC_665AA52CF5BE
#define FE3E3D02_BCDA_4813_9EEC_665AA52CF5BE

constexpr static std::uint32_t RESFLAG_IMG_FLIP = 1 << 8; // Flip the image vertically on load
constexpr static std::uint32_t RESFLAG_IMG_GRAY = 1 << 9; // If set, the pixel data is 8-bit grayscale, otherwise it is RGBA8888

struct Image final {
    static void register_resource(void);

    int width;
    int height;
    int channels;
    stbi_uc* pixels;
};

#endif /* FE3E3D02_BCDA_4813_9EEC_665AA52CF5BE */

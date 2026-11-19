#include "shared/pch.hh"

#include "shared/world/lod.hh"

static std::uint8_t channel_to_u8(float value)
{
    auto clamped = std::clamp(value, 0.0f, 1.0f);
    return static_cast<std::uint8_t>(std::lround(clamped * 255.0f));
}

std::array<std::uint8_t, 3> lod::to_rgb8(const Eigen::Vector3f& color)
{
    std::array<std::uint8_t, 3> result;
    result[0] = channel_to_u8(color.x());
    result[1] = channel_to_u8(color.y());
    result[2] = channel_to_u8(color.z());
    return result;
}

std::uint8_t lod::to_rgb332(const Eigen::Vector3f& color)
{
    auto rgb = to_rgb8(color);
    auto r = static_cast<std::uint8_t>(rgb[0] >> 5U);
    auto g = static_cast<std::uint8_t>(rgb[1] >> 5U);
    auto b = static_cast<std::uint8_t>(rgb[2] >> 6U);
    return static_cast<std::uint8_t>((r << 5U) | (g << 2U) | b);
}

Eigen::Vector3f lod::from_rgb332(std::uint8_t color)
{
    auto r = static_cast<float>(color >> 5U) / 7.0f;
    auto g = static_cast<float>((color >> 2U) & 0x07U) / 7.0f;
    auto b = static_cast<float>(color & 0x03U) / 3.0f;
    return Eigen::Vector3f(r, g, b);
}

Eigen::Vector3f lod::unpack(std::uint8_t mode, const LodPalette& palette, std::uint8_t index)
{
    if(mode == COLOR_PALETTE8 && index < palette.size()) {
        auto& entry = palette[index];

        Eigen::Vector3f result;
        result.x() = static_cast<float>(entry[0]) / 255.0f;
        result.y() = static_cast<float>(entry[1]) / 255.0f;
        result.z() = static_cast<float>(entry[2]) / 255.0f;
        return result;
    }

    return from_rgb332(index);
}

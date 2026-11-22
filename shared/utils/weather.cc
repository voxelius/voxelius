#include "shared/pch.hh"

#include "shared/utils/weather.hh"

static std::uint32_t lattice_hash(std::int32_t x, std::int32_t z, std::uint32_t seed)
{
    std::uint32_t h = static_cast<std::uint32_t>(x) * UINT32_C(374761393);
    h += static_cast<std::uint32_t>(z) * UINT32_C(668265263);
    h += seed * UINT32_C(2246822519);

    h ^= h >> 13;
    h *= UINT32_C(1274126177);
    h ^= h >> 16;

    return h;
}

float utils::weather_noise(float x, float z, std::uint32_t seed)
{
    auto x0 = static_cast<std::int32_t>(std::floor(x));
    auto z0 = static_cast<std::int32_t>(std::floor(z));
    auto fx = x - static_cast<float>(x0);
    auto fz = z - static_cast<float>(z0);
    auto ux = fx * fx * (3.0f - 2.0f * fx);
    auto uz = fz * fz * (3.0f - 2.0f * fz);

    auto sample = [seed](std::int32_t sx, std::int32_t sz) {
        return static_cast<float>(lattice_hash(sx, sz, seed) & UINT32_C(0xFFFFFF)) / static_cast<float>(UINT32_C(0xFFFFFF));
    };

    auto a = sample(x0, z0);
    auto b = sample(x0 + 1, z0);
    auto c = sample(x0, z0 + 1);
    auto d = sample(x0 + 1, z0 + 1);

    auto top = a * (1.0f - ux) + b * ux;
    auto bottom = c * (1.0f - ux) + d * ux;

    return top * (1.0f - uz) + bottom * uz;
}

float utils::weather_cell(std::int32_t x, std::int32_t z, std::uint32_t seed)
{
    return static_cast<float>(lattice_hash(x, z, seed) & UINT32_C(0xFFFFFF)) / static_cast<float>(UINT32_C(0xFFFFFF));
}

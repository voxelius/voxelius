#include "server/pch.hh"

#include "server/utils/climate.hh"

float utils::normalize_01(float sample)
{
    return std::clamp((sample + 1.0f) * 0.5f, 0.0f, 1.0f);
}

float utils::peaks_valleys(float weirdness)
{
    auto w = std::abs(weirdness);
    return 1.0f - std::abs(3.0f * w - 2.0f);
}

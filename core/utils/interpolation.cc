#include "core/pch.hh"

#include "core/utils/interpolation.hh"

float utils::smoothstep(float edge0, float edge1, float value)
{
    if(edge0 >= edge1) {
        return value < edge0 ? 0.0f : 1.0f;
    }

    auto t = std::clamp((value - edge0) / (edge1 - edge0), 0.0f, 1.0f);

    return t * t * (3.0f - 2.0f * t);
}

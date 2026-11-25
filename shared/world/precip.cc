#include "shared/pch.hh"

#include "shared/world/precip.hh"

#include "shared/world/heightmap.hh"
#include "shared/world/precip_registry.hh"
#include "shared/world/weather_registry.hh"
#include "shared/world/world.hh"

static float band(float value, float min, float max, float fade)
{
    if(value < min) {
        if(fade <= 0.0f) {
            return 0.0f;
        }

        return std::clamp(1.0f - (min - value) / fade, 0.0f, 1.0f);
    }

    if(value > max) {
        if(fade <= 0.0f) {
            return 0.0f;
        }

        return std::clamp(1.0f - (value - max) / fade, 0.0f, 1.0f);
    }

    return 1.0f;
}

PrecipContext precip::context(const World* world, std::span<const weather_type_id> allowed, float x, float z)
{
    assert(world);

    auto& state = world->weather();
    auto front = state.type_at(allowed, x, z, world->weather_tick);
    auto definition = weather_registry::find_definition(front);

    if(definition == nullptr || definition->precip.types.empty()) {
        return {};
    }

    auto coverage = state.coverage_of(allowed, front, x, z, world->weather_tick);

    PrecipContext result {};
    result.front = front;
    result.definition = definition;
    result.coverage = coverage;
    result.effective_intensity = coverage * std::clamp(definition->precip.intensity, 0.0f, 1.0f);
    return result;
}

PrecipSelection precip::select(const World* world, const PrecipContext& ctx, float x, float y, float z)
{
    assert(world);

    if(ctx.definition == nullptr || ctx.effective_intensity <= 0.0f) {
        return {};
    }

    BlockPos bpos;
    bpos.x() = static_cast<BlockPos::value_type>(std::floor(x));
    bpos.y() = static_cast<BlockPos::value_type>(std::floor(y));
    bpos.z() = static_cast<BlockPos::value_type>(std::floor(z));

    auto column = world->find_column(BlockPosXZ(bpos.x(), bpos.z()));

    // Nothing falls through a roof, and an unknown column counts as open sky
    if(column && column->shelters(bpos.y())) {
        return {};
    }

    return select(ctx, world->get_temperature(bpos), world->get_humidity_base(bpos));
}

PrecipSelection precip::select(const PrecipContext& ctx, float temperature, float humidity)
{
    if(ctx.definition == nullptr || ctx.effective_intensity <= 0.0f) {
        return {};
    }

    auto best = PRECIP_ID_NULL;
    auto best_score = 0.0f;

    for(auto type : ctx.definition->precip.types) {
        auto definition = precip_registry::find_definition(type);

        if(definition == nullptr) {
            continue;
        }

        auto temperature_weight = band(temperature, definition->temperature_min, definition->temperature_max, definition->temperature_fade);
        auto humidity_weight = band(humidity, definition->humidity_min, definition->humidity_max, definition->humidity_fade);
        auto score = temperature_weight * humidity_weight * definition->priority;

        if(score <= 0.0f) {
            continue;
        }

        if(score > best_score || (score == best_score && type < best)) {
            best = type;
            best_score = score;
        }
    }

    PrecipSelection result {};
    result.type = best;
    result.effective_intensity = ctx.effective_intensity;
    return result;
}

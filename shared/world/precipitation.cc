#include "shared/pch.hh"

#include "shared/world/precipitation.hh"

#include "shared/world/precipitation_registry.hh"
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

PrecipitationContext precipitation::context(const World* world, std::span<const weather_type_id> allowed, float x, float z)
{
    assert(world);

    auto& state = world->weather();
    auto front = state.type_at(allowed, x, z, world->weather_tick);
    auto definition = weather_registry::find_definition(front);

    if(definition == nullptr || definition->precipitation.types.empty()) {
        return {};
    }

    auto coverage = state.coverage_of(allowed, front, x, z, world->weather_tick);

    PrecipitationContext result {};
    result.front = front;
    result.definition = definition;
    result.coverage = coverage;
    result.amount = coverage * std::clamp(definition->precipitation.intensity, 0.0f, 1.0f);
    return result;
}

PrecipitationSelection precipitation::select(const World* world, const PrecipitationContext& ctx, float x, float y, float z)
{
    assert(world);

    if(ctx.definition == nullptr || ctx.amount <= 0.0f) {
        return {};
    }

    BlockPos bpos;
    bpos.x() = static_cast<BlockPos::value_type>(std::floor(x));
    bpos.y() = static_cast<BlockPos::value_type>(std::floor(y));
    bpos.z() = static_cast<BlockPos::value_type>(std::floor(z));

    return select(ctx, world->get_temperature(bpos), world->get_humidity_base(bpos));
}

PrecipitationSelection precipitation::select(const PrecipitationContext& ctx, float temperature, float humidity)
{
    if(ctx.definition == nullptr || ctx.amount <= 0.0f) {
        return {};
    }

    auto best = PRECIPITATION_ID_NULL;
    auto best_score = 0.0f;

    for(auto type : ctx.definition->precipitation.types) {
        auto definition = precipitation_registry::find_definition(type);

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

    PrecipitationSelection result {};
    result.type = best;
    result.amount = ctx.amount;
    return result;
}

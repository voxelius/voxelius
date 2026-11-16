local flowdef = require("builtin:flowdef")

local still_frames = {}
local flowing_frames = {}

local WATER_CONFIG <const> = {
    fluid = "water",
    infinite = true,
    full_height = 14,
    max_level = 8,
    delay = 5,
}

for i = 1, 32 do
    local frame = string.format("%02d", i)
    table.insert(still_frames, "water/water_still_" .. frame .. ".png")
    table.insert(flowing_frames, "water/water_flowing_" .. frame .. ".png")
end

fluids.add("water", {
    gravity = fluids.GRAVITY_DOWN,

    albedo = {
        still = still_frames,
        flowing = flowing_frames,
    },

    full_level = WATER_CONFIG.full_height,

    fog_density = 10.0,
    fog_color = { 0.0, 0.0, 0.5 },

    tint = "water",
})

blocks.add("water", {
    render = blocks.RENDER_NONE,

    fluid_name = "water",

    replaceable = true,
    touch = blocks.TOUCH_NONE,

    emission = 0,
    dissipation = 1,

    states = flowdef.states(WATER_CONFIG),
    variants = flowdef.variants(WATER_CONFIG),

    on_place = flowdef.on_place(WATER_CONFIG),
    on_tick = flowdef.tick,
})

flowdef.register("water", WATER_CONFIG)

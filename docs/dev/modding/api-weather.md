# Weather

Weather in Voxelius belongs to a world, not to an entity. The server owns a weather seed and a weather clock per world. The seed travels once in `World_Info`; the clock travels in `World_Timings` every tick. Both sides derive the same field from that pair and a world position, so no weather state is sent beyond those values.

## Registering a type

```lua
weather.add("thunderstorm", {
  intensity = 1.0,
  sky_tint = { 0.14, 0.16, 0.19 },
  sky_visibility = 0.05,
  light = 0.35,

  cloud = {
    height = 160,
    thickness = 64,
    opacity = 0.95,
    scale = 112,
    coverage = 1.0,
    shadow = { 0.84, 0.85, 0.88 },
    lit = { 0.97, 0.98, 1.00 },
    seed = 19,
  },

  precipitation = {
    kind = "rain",
    intensity = 1.0,
    speed = 34.0,
    drift = 1.2,
    size = 0.07,
    color = { 0.32, 0.38, 0.46 },
  },

  strikes_per_minute = 6.0,
  strike_radius = 96.0,

  target = { 0.82, 0.55 },
  weight = 1.0,
})
```

`weather.add` follows the same rules as `biomes.add`: the name is relative to the calling mod, and the type id is stable for the whole session. Every world shares the registry, so a mod can add a type that every world can pick.

## Where the weather is

Two drifting noise grids form the two axes of a climate square:

- the **first axis** (1536 blocks per noise cell) and the **second axis** (1024 blocks per noise cell) are sampled at the point, both drifting with the world's wind;
- every type owns a **target point** in that square (`target = { a, b }`, each from 0 to 1);
- at a point, each type's score is the squared distance from the sample to its target, divided by its `weight`. The type with the lowest score is the type at that point;
- the types share the point with a soft blend that drops off quickly away from the nearest target: a type owns its cell almost entirely, and only a narrow band mixes two types, so a change of type is a gradient without a world-wide haze;
- the share also picks the sky tint, the visibility and the light; `intensity` does not dilute that mix, it only scales the cloud coverage.

Because targets are points rather than positions on one ordered line, a storm can border a clear sky directly. Where the types sit relative to each other is a data decision, not an engine one: in the shipped content the storm is placed next to the clear and the rain targets so a front can bring it in without an overcast or rainy day in between.

The square drifts with the world's wind. The wind direction and speed come from the weather seed; a type does not set them. The field is two-dimensional and does not depend on height.

## The weather clock

The server advances `weather_tick` once per fixed tick while the replicated gamerule `gr_weather_cycle` is on. `World_Timings` carries that value to each client of the world. A client advances its own copy of the tick between packets, so a late packet does not freeze the sky, and takes the packet as the correction. A jump of more than a second is a forced front, a rewound clock or a world switch: the client snaps to it and rebuilds the cloud deck instead of sliding the old one across the sky.

Nothing else about the weather is sent. The type definitions travel as part of the mod content and are covered by the login checksum; the weather seed travels in `World_Info`; the clock travels in `World_Timings`.

## Where the weather is kept

The field belongs to the world, not to the session. Each world keeps its weather seed and weather clock in its own `world.conf`, next to its daylight clock:

```ini
weather.seed=12345678901234567890
weather.tick=0
daylight_tick=7200
```

The seed is rolled once when the world is first saved. It is not derived from the world seed and it says nothing about the terrain, so a client can receive it without learning how the world was generated. A world that has never been saved rolls its seed on the next start. Deleting the keys, or the file, gives that world a fresh sky.

`weather_force <type>` advances the world's weather clock to the next tick at which the requested type covers the players, up to seven in-game days ahead. The client sees the jump and rebuilds its deck. The old definition's `wind` field no longer exists; a script that still passes it loads and the value is ignored.

## Fields

| Field | Type | Default | Meaning |
|---|---|---|---|
| `intensity` | number | `1.0` | Overall strength; scales the cloud coverage and the climate bias |
| `sky_tint` | RGB | white | Colour that replaces the sky where `sky_visibility` is low; the fog takes the horizon colour of the resulting gradient |
| `sky_visibility` | number | `1.0` | How much of the clear sky shows through, from `0` (only `sky_tint`) to `1` (the Rayleigh gradient); the sun, moon and stars fade with it |
| `light` | number | `1.0` | Multiplies the sky light the world receives |
| `cloud` | table | none | The cloud body of the type; a zero coverage leaves the sky clear |
| `precipitation` | table | none | Local precipitation; declared and synced, not drawn yet |
| `target` | two numbers | `{0.5, 0.5}` | Point this type owns in the climate square, each axis from 0 to 1 |
| `weight` | number | `1.0` | How far this type's cell reaches from its target; a bigger weight means the type covers more of the world |
| `strikes_per_minute` | number | `0.0` | How often the type strikes |
| `strike_radius` | blocks | `96.0` | Strike scatter around a player |

### Climate

The sampled type and its coverage also move the local climate. A precipitating type at a point lowers `world.get_temperature` by up to two degrees Celsius and pushes `world.get_humidity` towards 0.98, both scaled by the type's coverage there. Clear, overcast and other types without precipitation leave both values at their biome base.

### Cloud body

| Field | Default | Meaning |
|---|---|---|
| `height` | `192` | World Y of the bottom of the cloud volume |
| `thickness` | `8` | How tall the volume is, in blocks; the client divides it into whole voxels |
| `opacity` | `0.9` | How opaque the deck is where the field covers it |
| `scale` | `128` | Blocks per noise tile; controls the size of the cloud masses |
| `coverage` | `0` | Base density, 0 to 1 |
| `shadow`, `lit` | gray | Colour of the underside and of the top |
| `seed` | `0` | Changes the noise pattern |

A type has one cloud body, not a stack of layers: a storm and a clear sky
differ in `height`, `thickness` and `coverage`, not in how many slabs they
declare. The body is not a plane: the client samples the noise field on the
grid of the ring it is meshing and turns each cell whose density clears the
body's coverage into a column of whole voxels, one or more of them. A dense
front grows the column towards `thickness` and the shell noise breaks its top
into heaps, so a storm fills its volume while a thin type stays a sheet. The
deck is meshed in rings that tile the range: 8-block cells out to 32 chunks,
32-block cells out to 96, and 128-block cells out to 256, so a longer range
extends the deck outwards instead of growing the blocks under the player.
Rings overlap by one cell, so the seam between two of them never opens. A
column stays inside the body's slab, from `height` up to `height + thickness`.
Every column also carries its own slightly varied shade, a few percent off its
neighbours, so a dense deck reads as blocks instead of one flat sheet. The
client lights the body from the sun and the sky; `shadow` and `lit` are the
albedo of the underside and the top, not the final colour, and the fog takes
over with distance. Only the faces the neighbours leave exposed are meshed, so
a dense deck costs less than its area. The deck dissolves into the sky at its
own edge instead of ending on a rim and reaches the composite as one more
depth-sorted layer.

## Precipitation

```lua
precipitation = { kind = "rain" } -- or "snow", or omit for none
```

The fields below are parsed, stored and covered by the login checksum, but
nothing draws them yet: a type can carry its precipitation settings from now on
and the rendering pass lands with the next change to weather. Precipitation is
also what biases the local temperature and humidity.

| Field | Default | Meaning |
|---|---|---|
| `kind` | `"none"` | `"rain"`, `"snow"` or `"none"` |
| `intensity` | `0.0` | How thick the precipitation is where the front covers the camera |
| `speed` | `18.0` | Fall speed, in blocks per second |
| `drift` | `0.0` | Sideways speed, in blocks per second |
| `size` | `0.08` | Size of one streak or flake, in blocks |
| `color` | light blue | Colour of the streaks or flakes |

## Callbacks

All callbacks run on the server, in the mod's Lua state.

| Callback | Arguments | When |
|---|---|---|
| `on_tick` | `tick` | Once a second for the type under the players |
| `on_strike` | `x`, `z`, `y` | A thunder strike landed |

A strike is logged by the engine before `on_strike` runs. There is no bolt yet: the callback is the hook for a mod that wants one.

There is no per-type duration or transition schedule: a type owns the climate space nearest its target, and the weather under a player changes when the drifting field brings another cell over them. A bigger `weight` therefore means the type covers more of the world.

## Querying

```lua
local state = weather.state()
-- state.seed, state.tick, state.wind_x, state.wind_z, state.wind_speed

local sample = weather.sample(x, z)
-- sample.coverage, sample.type, sample.precipitation
```

`weather.sample` and `weather.force` take the block X and Z: the field is two-dimensional and does not depend on height.

`weather.force` exists on the server only. The field is absent on the client.

`weather.force("builtin:rain", x, z)` advances the weather clock until the requested type covers the point; on the server the `weather_force` command does the same around the players.

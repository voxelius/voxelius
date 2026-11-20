# Weather

Weather in Voxelius belongs to a world, not to an entity. The server owns one state per world; the seeds and scales travel once with `World_Info`, and the drift and wind follow once a second in `World_Weather`. Clients reconstruct the same field from it and draw it.

## Registering a type

```lua
weather.add("thunderstorm", {
  intensity = 1.0,
  sky_tint = { 0.14, 0.16, 0.19 },
  sky_visibility = 0.05,
  light = 0.35,
  wind = 3.0,

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

  weight = 2,
})
```

`weather.add` follows the same rules as `biomes.add`: the name is relative to the calling mod, and the type id is stable for the whole session. Every world shares the registry, so a mod can add a type that every world can pick.

## Where the weather is

Two drifting noise grids decide what the weather is and how strong it is at a point:

- the **type grid** (1024 blocks per cell) turns a continuous noise value into a weighted mix of the registered types; every type owns a slice of the `[0, 1)` range and the slices cross-fade over a narrow band, so a type change is a gradient, not a step;
- the **detail grid** (512 blocks per cell) decides how much of that mix is present, so the world can hold several storms with clear sky between them.

Both grids drift with the wind. The seeds and scales travel once in `World_Info`; only the drift and the wind follow over the network, once a second. Both sides advance the drift on their own every frame, and the client only snaps to a packet when the two differ by more than a chunk, so a forced front lands where it was forced and ordinary drift never steps. The server gamerule `gr_weather_cycle` stops that advance on both sides. After a connection the client takes the first weather packet whole and rebuilds the cloud deck instead of sliding the old one towards it; it tracks that locally, so the packet carries no flag for it.

## Where the weather is kept

The field belongs to the world, not to the session. The server writes the world's state into `world.conf` when it shuts down and reads it back when it starts, so a restart continues the same fronts instead of rolling a new sky:

```ini
weather.types_drift=[12 -310]
weather.types_scale=1024
weather.types_seed=123456789
weather.detail_drift=[12 -310]
weather.detail_scale=512
weather.detail_seed=987654321
weather.wind_direction=[0.707 0.707]
```

A world that has never been saved has no such keys, and the server draws its seeds and its wind from the universe seed instead. Deleting the keys, or the file, gives that world a fresh sky on the next start.

The same file carries `tick` and `daylight_tick` next to `seed`. The server owns them: `daylight_tick` is the day cycle the sky follows, and both are written on shutdown so a restart continues the same day.

`weather_force <type>` slides the type grid until the cell under the players resolves to the requested type and puts a strong detail value on them; somewhere else the type may be different or the sky clear.

## Fields

| Field | Type | Default | Meaning |
|---|---|---|---|
| `intensity` | number | `1.0` | Overall strength, used by the front and the tint |
| `sky_tint` | RGB | white | Colour that replaces the sky where `sky_visibility` is low; the fog takes the horizon colour of the resulting gradient |
| `sky_visibility` | number | `1.0` | How much of the clear sky shows through, from `0` (only `sky_tint`) to `1` (the Rayleigh gradient); the sun, moon and stars fade with it |
| `light` | number | `1.0` | Multiplies the sky light the world receives |
| `wind` | number | `0.0` | How fast the front travels, in blocks per second |
| `cloud` | table | none | The cloud body of the type; a zero coverage leaves the sky clear |
| `precipitation` | table | none | Local precipitation; declared and synced, not drawn yet |
| `weight` | number | `1` | Relative size of this type's slice of the type grid, so a bigger weight means the type covers more of the world |
| `strikes_per_minute` | number | `0.0` | How often the type strikes |
| `strike_radius` | blocks | `96.0` | Strike scatter around a player |

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
and the rendering pass lands with the next change to weather.

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

There is no per-type duration or transition schedule: a type covers whatever part of the type grid its `weight` earns it, and the weather under a player changes when the drifting grid brings a different slice over them. A bigger weight for `clear` therefore means clear weather is more common than a storm.

## Querying

```lua
local state = weather.state()
-- state.types_scale, state.types_seed, state.detail_scale, state.detail_seed, state.wind_x, state.wind_z

local sample = weather.sample(x, y, z)
-- sample.coverage, sample.type, sample.precipitation
```

`weather.force("builtin:rain", x, z)` slides both grids so that the point gets the requested type and a strong detail value; on the server the `weather_force` command does the same around the players.

# Weather

Weather in Voxelius belongs to a [realm](../engine/arch-core.md), not to an entity. The server owns one state per realm (surface, sky, underground, the depths) and sends it to every client once a second; clients reconstruct the same field from it and draw it.

## Registering a type

```lua
weather.add("thunderstorm", {
  realms = { weather.REALM_SURFACE },

  intensity = 1.0,
  sky_tint = { 0.14, 0.16, 0.19 },
  light = 0.35,
  wind = 3.0,

  layers = {
    {
      height = 188,
      scale = 144,
      coverage = 0.95,
      shadow = { 0.10, 0.11, 0.13 },
      lit = { 0.17, 0.18, 0.20 },
      seed = 19,
    },
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

`weather.add` follows the same rules as `biomes.add`: the name is relative to the calling mod, and the type id is stable for the whole session. All realms share the registry, so a mod can add a type that only the sky realm ever picks.

## Where the weather is

There is no single regime per realm. Two drifting noise grids decide what the weather is and how strong it is at a point:

- the **type grid** (1024 blocks per cell) turns a continuous noise value into a weighted mix of the registered types; every type owns a slice of the `[0, 1)` range and the slices cross-fade over a narrow band, so a type change is a gradient, not a step;
- the **detail grid** (512 blocks per cell) decides how much of that mix is present, so a realm can hold several storms with clear sky between them.

Both grids drift with the realm wind. Only the seeds, the scales and the drift travel over the network, and both sides advance the drift on their own every frame; the client only snaps to the packet when the two differ by more than a chunk, so a forced front lands where it was forced and ordinary drift never steps. The sync therefore stays at four fixed records per second.

## Where the weather is kept

The field belongs to the world, not to the session. The server writes each realm's state into `world.conf` when it shuts down and reads it back when it starts, so a restart continues the same fronts instead of rolling a new sky. One set of keys per realm, named after it — `surface`, `underground`, `depths`, `sky`:

```ini
weather.surface.types_drift=[12 -310]
weather.surface.types_scale=1024
weather.surface.types_seed=123456789
weather.surface.detail_drift=[12 -310]
weather.surface.detail_scale=512
weather.surface.detail_seed=987654321
weather.surface.wind_direction=[0.707 0.707]
```

A world that has never been saved has no such keys, and the server draws its seeds and its wind from the universe seed instead. Deleting the keys, or the file, gives that world a fresh sky on the next start.

`weather_force <realm> <type>` slides the type grid until the cell under the players resolves to the requested type and puts a strong detail value on them; somewhere else the type may be different or the sky clear.

## Fields

| Field | Type | Default | Meaning |
|---|---|---|---|
| `realms` | array | `{ REALM_SURFACE }` | Realms that may pick this type |
| `intensity` | number | `1.0` | Overall strength, used by the front and the tint |
| `sky_tint` | RGB | white | Target colour the sky is mixed towards; the fog takes the same colour, because the sky is drawn as the fog colour |
| `light` | number | `1.0` | Multiplies the sky light the world receives |
| `wind` | number | `0.0` | How fast the front travels, in blocks per second |
| `layers` | array | empty | Cloud layers, at most 4 |
| `precipitation` | table | none | Local precipitation; declared and synced, not drawn yet |
| `weight` | number | `1` | Relative size of this type's slice of the type grid, so a bigger weight means the type covers more of the world |
| `strikes_per_minute` | number | `0.0` | How often the type strikes |
| `strike_radius` | blocks | `96.0` | Strike scatter around a player |

### Cloud layers

| Field | Default | Meaning |
|---|---|---|
| `height` | `192` | World Y of the bottom of the cloud volume |
| `thickness` | `8` | How tall the volume is, in blocks; the client divides it into whole voxels |
| `opacity` | `0.9` | How opaque the deck is where the field covers it |
| `scale` | `128` | Blocks per noise tile; controls the size of the cloud masses |
| `coverage` | `0` | Base density, 0 to 1 |
| `shadow`, `lit` | gray | Colour of the underside and of the top |
| `seed` | `0` | Changes the noise pattern |

Layers are not planes: the client samples the noise field on the grid of the ring
it is meshing and turns each cell whose density clears the layer's coverage into a
column of whole voxels, one or more of them. The deck is meshed in rings that tile
the range: 8-block cells out to 32 chunks, 32-block cells out to 96, and 128-block
cells out to 256, so a longer range extends the deck outwards instead of growing
the blocks under the player. Rings overlap by one cell, so the seam between two of
them never opens. A column stays inside the layer's slab, from `height` up to
`height + thickness`, and its underside steps up into the volume here and there,
so the deck is a solid body with a stepped underside and a bumpy top. Every
column also carries its own slightly varied shade, a few percent off its
neighbours, so a dense deck reads as blocks instead of one flat sheet. Only the
faces the neighbours leave exposed are meshed, so a dense deck costs less than its
area. The deck dissolves into the sky at its own edge instead of ending on a rim,
reaches the composite as one more depth-sorted layer, and does not use the terrain
fog, which would wash it out long before it is seen.

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
| `on_tick` | `realm`, `tick` | Once a second for the type under the players |
| `on_strike` | `x`, `z`, `y`, `realm` | A thunder strike landed |

A strike is logged by the engine before `on_strike` runs. There is no bolt yet: the callback is the hook for a mod that wants one.

There is no per-type duration or transition schedule: a type covers whatever part of the type grid its `weight` earns it, and the weather under a player changes when the drifting grid brings a different slice over them. A bigger weight for `clear` therefore means clear weather is more common than a storm.

## Querying

```lua
local state = weather.state(weather.REALM_SURFACE)
-- state.types_scale, state.types_seed, state.detail_scale, state.detail_seed, state.wind_x, state.wind_z

local sample = weather.sample(x, y, z)
-- sample.coverage, sample.type, sample.precipitation
```

`weather.force(realm, "builtin:rain", x, z)` slides both grids so that the point gets the requested type and a strong detail value; on the server the `weather_force` command does the same around the players.

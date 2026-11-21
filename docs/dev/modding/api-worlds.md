# Worlds API

A mod adds a world through `worlds.add`. The table exists on the server only: the client receives the world list from the server and never adds a world itself.

## Function: `worlds.add(name, def)`

Registers a world definition.

### Arguments

- `name` is the namespaced world ID
- `def` is the world definition table

### Fields

|Field|Type|Required|Default|Description|
|----|----|----|----|----|
|`generator`|`string`|yes|N/D|Generator name, such as `vox17` or `superflat`|
|`generator_settings`|`table`|no|`{}`|Generator-specific settings; validated when the world is registered|
|`directory`|`string`|no|derived from the ID|Directory under `save/`; the ID is not a path|
|`min_y`|`integer`|no|`-64`|Lowest world block Y|
|`max_y`|`integer`|no|`255`|Highest world block Y|
|`spawn`|`integer[3]`|no|`{0, 32, 0}`|Spawn block position|
|`biome_selector`|`table`|no|constant selector|How the generator picks a biome|
|`weather`|`string[]`|no|`{}`|Weather types this world may use|
|`visuals`|`table`|no|`{}`|Optional sky, sun, moon, stars and clouds|

`weather` names and biome names are resolved after all mods load, because a world may refer to content from another mod.

## Generator settings

`generator_settings` is validated in `worlds.add`. An unknown key, a wrong type, a value outside its range or a selector that the generator cannot use fails the mod; other mods and worlds keep loading.

### `vox17`

|Key|Default|Range|
|----|----|----|
|`terrain_frequency`|`0.005`|greater than 0|
|`terrain_octaves`|`4`|1 to 8|
|`terrain_density_peak`|`0.385`|0 to 1|
|`sea_level`|`0`|`min_y` to `max_y`|
|`base_min`|`-24`|finite, not greater than `base_max`|
|`base_max`|`16`|finite, not less than `base_min`|
|`variation_min`|`16`|0 or greater, not greater than `variation_max`|
|`variation_max`|`96`|not less than `variation_min`|
|`pv_valley_offset`|`-32`|finite, not greater than `pv_peak_offset`|
|`pv_peak_offset`|`12`|finite, not less than `pv_valley_offset`|
|`climate_frequencies`|see below|table of five positive numbers|

`climate_frequencies` has one entry per climate axis:

|Key|Default|
|----|----|
|`temperature`|`0.0007`|
|`humidity`|`0.0007`|
|`continentalness`|`0.00035`|
|`erosion`|`0.00075`|
|`weirdness`|`0.001`|

The defaults match the built-in `overworld` world. The selector must be `nearest` with the five axes in the order above, or `grid` with the axes `temperature` and `humidity`.

### `superflat`

|Key|Default|Range|
|----|----|----|
|`surface_y`|`0`|`min_y` to `max_y`|
|`filler_depth`|`3`|0 or greater; the filler must not reach below `min_y`|

The selector must be `constant`.

## Biome selector

`biome_selector.strategy` is one of `worlds.SELECTION_NEAREST`, `worlds.SELECTION_GRID` or `worlds.SELECTION_CONSTANT`. The default is `worlds.SELECTION_CONSTANT`.

### Constant

```lua
biome_selector = {
  strategy = worlds.SELECTION_CONSTANT,
  biome = "builtin:plains",
}
```

### Nearest

The selector samples every generator axis and picks the biome with the nearest target point.

```lua
biome_selector = {
  strategy = worlds.SELECTION_NEAREST,
  axes = { "temperature", "humidity", "continentalness", "erosion", "weirdness" },
  weights = { 1.0, 1.0, 1.0, 1.0, 1.0 },
  entries = {
    { biome = "builtin:plains", target = { 50, 50, 50, 50, 50 }, priority = 0, offset = 0.0 },
  },
}
```

`target` values are integers from 0 to 99, one per axis. `priority` and `offset` break ties and bias the distance. The engine nudges colliding targets until each is unique.

### Grid

The grid quantizes the first two sample axes into a lookup table.

```lua
biome_selector = {
  strategy = worlds.SELECTION_GRID,
  axes = { "temperature", "humidity" },
  grid_size = 2,
  lookup = {
    { "builtin:desert", "builtin:plains" },
    { "builtin:forest", "builtin:plains" },
  },
}
```

- `axes` holds exactly two names. `vox17` requires `temperature` and `humidity`.
- `grid_size` is from 1 to 128.
- `lookup` is a `grid_size` by `grid_size` table of biome names. The row is the second axis and the column is the first axis.
- `weights` and `entries` are not used by the grid and must be absent.

The generator passes its normalized sample to the selector; the grid uses the first two values. A custom generator must put the two grid axes first.

## Visuals

`visuals` holds optional groups. A missing group disables that part of the sky; an enabled group must contain every field.

|Group|Fields|
|----|----|
|`sky`|`day`, `night` (`horizon` and `zenith` colors), `twilight_height`|
|`sun`|`tilt`|
|`moon`|`color`|
|`stars`|`count`|
|`clouds`|empty table|

The `sky` group draws the sky sphere, the day/night gradient and the sunset glow. If `visuals.sky` is absent, the client does not draw the sky sphere; the background is the world clear/fog color. `sun`, `moon`, `stars` and `clouds` remain independent groups and draw only when their own group is present.

## Validation

`worlds.add` validates the generator name, `generator_settings`, `min_y`/`max_y`, the spawn vector and the selector shape. A failure returns a Lua error, removes the definition and fails the mod.

Biome and weather names, the `directory` uniqueness and `min_y <= max_y` are checked when the universe loads all registered worlds. A failure there stops the server before the first frame.

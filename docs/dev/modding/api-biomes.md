# Biomes API

Mods register biome types for the world generator.

## Constant: null biome

`biomes.NULL_BIOME` is an empty, undefined, or otherwise invalid biome ID. Treat it as a void biome for gameplay and gamedev.

## Functions

### Function: `biomes.add(name, def) -> integer`

Registers a new biome.

#### Arguments

- `name` is the namespaced biome ID
- `def` is the biome definition table

#### Return value

Returns the numeric biome ID.

## Biome definition

|Field|Type|Required|Default|Description|
|----|----|----|----|----|
|`temperature`|`integer`|no|50|Target temperature in climate space, 0 to 99|
|`humidity`|`integer`|no|50|Target humidity in climate space, 0 to 99|
|`continentalness`|`integer`|no|50|Target continentalness, 0 to 99. Low values correspond to oceans|
|`erosion`|`integer`|no|50|Target erosion, 0 to 99. High values are flat terrain, low values are mountainous|
|`weirdness`|`integer`|no|50|Target weirdness or ridges, 0 to 99. Drives biome variants and peaks-vs-valleys terrain offset|
|`priority`|`integer`|no|0|If two biomes claim the same climate target during setup, the higher priority keeps the point. The engine nudges the loser until the point becomes unique|
|`offset`|`number`|no|0|The engine adds this value to climate distance when it picks a biome. Larger values make the biome harder to win and effectively smaller|
|`palette`|`table`|no|`{}`|Biome block palette|
|`scatter`|`table[]`|no|`{}`|List of features to scatter|
|`tints`|`table`|no|`{}`|Tint group override colors|

## Palette table

|Field|Description|
|----|----|
|`basic`|Basic block type, eg `builtin:stone`|
|`filler`|Filler block type, eg `builtin:dirt`|
|`surface`|Surface block type, eg `builtin:grass`|
|`fluid`|Fluid block type, eg `builtin:water`|

## Palette entry

|Field|Type|Required|Default|Description|
|----|----|----|----|----|
|`name`|`string`|yes|N/D|Block name|
|`states`|`table`|no|`{}`|Block states|

## Scatter entry

The engine evaluates scatter entries in list order. If several entries pass the roll on the same column, **the last entry in the list wins**. The engine skips the other entries for that column.

|Field|Type|Required|Default|Description|
|----|----|----|----|----|
|`feature`|`string`|yes|N/D|Feature to place|
|`chance`|`number`|no|0.5|Chance of a placement attempt per column|
|`need_above`|`integer[]`|no|`{}`|List of `blocks.TAG_XXXX` constants. The block above the origin must have at least one listed tag|
|`need_below`|`integer[]`|no|`{}`|List of `blocks.TAG_XXXX` constants. The origin block (surface) must have at least one listed tag|
|`padding`|`integer`|no|0|Minimum block clearance between this feature's bounding box and another placement in the same padding family. The engine uses expanded-box separation: it grows the current box by `padding` blocks, then tests for intersection|
|`group`|`string`|no|`""`|Padding family name. Entries with the same non-empty `group` share padding. An empty group pads only against the same scatter entry|
|`edge`|`integer`|no|0|Extra blocks beyond the feature's XZ bounds that must still belong to the same biome. The engine always checks the feature AABB footprint. `edge` grows that check, so large decorations stay clear of biome borders|

## Example

```lua
biomes.add("plains", {
  temperature = 50,
  humidity = 50,
  continentalness = 55,
  erosion = 70,
  weirdness = 50,
  priority = 0,
  offset = 0,

  palette = {
    empty = { name = "air" },
    basic = { name = "stone" },
    filler = { name = "dirt" },
    surface = { name = "stone_slab", states = { orientation = "bottom" } },
    fluid = { name = "water" },
  },

  scatter = {
    {
      feature = "oak_tree",
      chance = 0.0625,
      need_above = { blocks.TAG_GAS },
      need_below = { blocks.TAG_TURF },
    },
    {
      feature = "bush",
      chance = 0.25,
      need_above = { blocks.TAG_GAS },
      need_below = { blocks.TAG_TURF },
    }
  }
})
```

## Climate / multi-noise

Biome placement uses a sparse multi-noise model aligned with Minecraft 1.18+. At generation time the engine samples five continuous climate noises: temperature, humidity, continentalness, erosion, and weirdness. The engine picks a biome whose target point in that space is nearest.

During mod loading, each biome defines a nucleation point at its target coordinates (0..99 per axis). If two biomes collide, the higher `priority` keeps the cell. The engine nudges the loser randomly until it gets a free slot.

### Climate axes

|Axis|Terrain role|Biome role|
|----|------------|----------|
|Temperature|none|Climate bands, frozen ocean|
|Humidity|none|Forests, swamps, jungles|
|Continentalness|Base height, oceans vs land|Ocean / inland biomes|
|Erosion|Flat vs mountainous relief|Peaks, meadows, shattered terrain|
|Weirdness|Peaks-and-valleys offset via PV transform|Biome variants|

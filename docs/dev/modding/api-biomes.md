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
|`temperature`|`number`|no|15.0|Base temperature in degrees Celsius|
|`humidity`|`number`|no|50.0|Relative humidity, 0 to 100 percent|
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
  temperature = 20,
  humidity = 60,

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

## Biome selection

A biome no longer carries climate coordinates. Instead, a world defines how the engine picks a biome through the `biome_selector` field of [`worlds.add`](api-worlds.md#biome-selector).

The `vox17` generator uses the `nearest` strategy: the engine samples five climate noises (temperature, humidity, continentalness, erosion, and weirdness), then picks the biome whose target point in that space is nearest, weighted per axis. The selector's `entries` list the participating biomes and their target coordinates (0 to 99 per axis); `priority` and `offset` break ties and bias the distance. During world load the engine nudges colliding targets until each is unique.

The `vox17` generator also accepts the `grid` strategy: two axes, `grid_size`, and a `lookup` table of biome names. See [Worlds](api-worlds.md#grid).

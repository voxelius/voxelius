# World API

Mods read and write world blocks through the `world` library.

## Functions

### Function: `world.get_block(bx, by, bz) -> integer`

Retrieves a numeric block ID from the world.

#### Arguments

- `bx` is the world-scale block X position
- `by` is the world-scale block Y position
- `bz` is the world-scale block Z position

#### Return value

- Normally, a numeric block ID
- If there is no block or the chunk is not present, returns `blocks.NULL_BLOCK`

### Function: `world.set_block(bx, by, bz, id) -> boolean`

Assigns a block ID in the world.

#### Arguments

- `bx` is the world-scale block X position
- `by` is the world-scale block Y position
- `bz` is the world-scale block Z position
- `id` is the numeric block ID. To clear the cell (void), pass `blocks.NULL_BLOCK`

#### Return value

- `true` if the block was set
- `false` otherwise (eg. the chunk is not present)

### Function: `world.get_light(bx, by, bz) -> integer`

Retrieves lighting information for a block position. The value is the
brighter of the two light channels the engine keeps: skylight and block
light.

#### Arguments

- `bx` is the world-scale block X position
- `by` is the world-scale block Y position
- `bz` is the world-scale block Z position

#### Return value

- Luminance value of the block, from `0` (dark) to `15` (fully lit)
- `0` if the chunk is not present, or if it has not been lit yet

### Function: `world.get_state(bx, by, bz, st) -> string`

Retrieves a blockstate at a given position.

#### Arguments

- `bx` is the world-scale block X position
- `by` is the world-scale block Y position
- `bz` is the world-scale block Z position

#### Return value

- The blockstate value when present
- `nil` if the block is void, has no such blockstate, or the chunk is not present

### Function: `world.set_state(bx, by, bz, st, val) -> boolean`

Assigns a blockstate at a given position.

#### Arguments

- `bx` is the world-scale block X position
- `by` is the world-scale block Y position
- `bz` is the world-scale block Z position
- `st` is the blockstate name
- `val` is the blockstate value

#### Return value

- `true` if the blockstate was set
- `false` otherwise (eg. the block is void or has no such blockstate)

### Function: `world.get_temperature_base(bx, by, bz) -> integer`

Retrieves a biome-defined temperature base for a block.

#### Arguments

- `bx` is the world-scale block X position
- `by` is the world-scale block Y position
- `bz` is the world-scale block Z position

#### Return value

- Base temperature in biome registry units (0..99)
- `0` if the block is void or the chunk is not present

#### Notes

Many biomes may not define this. In that case most use `298`, which is about 25 degrees Celsius.

### Function: `world.get_temperature(bx, by, bz) -> integer`

Retrieves a biome-influenced seasonal temperature for a block.

#### Arguments

- `bx` is the world-scale block X position
- `by` is the world-scale block Y position
- `bz` is the world-scale block Z position

#### Return value

- Temperature in biome registry units (0..99)
- `0` if the block is void or the chunk is not present

#### Notes

Season, weather, and other factors can change this. Relatively low values (below 0 C) can turn planned rain into snow at that location.

### Function: `world.schedule(bx, by, bz, dt)`

Schedules an `on_tick` call for a block in `dt` ticks. The tick source is `blocks.TICK_SCRIPTED`.

#### Arguments

- `bx` is the world-scale block X position
- `by` is the world-scale block Y position
- `bz` is the world-scale block Z position
- `dt` is the tick delay for the callback

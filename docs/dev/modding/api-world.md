# World API

Mods read and write world blocks through the `world` library.

Every function needs an active world. Outside a callback or a server tick there is no active world, and the call raises a Lua error.

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

### Function: `world.get_fluid(bx, by, bz) -> integer`

Retrieves the fluid ID of the block variant at a position.

#### Arguments

- `bx` is the world-scale block X position
- `by` is the world-scale block Y position
- `bz` is the world-scale block Z position

#### Return value

- The numeric fluid ID, including a fluid that shares the cell with a block
- `fluids.NULL_FLUID` if there is no fluid or the chunk is not present

### Function: `world.get_fluid_faces(bx, by, bz) -> integer`

Retrieves the fluid faces of the block variant at a position.

#### Arguments

- `bx` is the world-scale block X position
- `by` is the world-scale block Y position
- `bz` is the world-scale block Z position

#### Return value

- A bitmask of `blocks.FACE_*` bits through which a fluid may enter or leave the variant
- `0` if the variant cannot be filled

### Function: `world.get_state_fluid(bx, by, bz, st, val) -> integer`

Asks which fluid the block at a position would hold if the given blockstate had the given value. The query does not change the world.

#### Arguments

- `bx` is the world-scale block X position
- `by` is the world-scale block Y position
- `bz` is the world-scale block Z position
- `st` is the blockstate name
- `val` is the blockstate value to test

#### Return value

- The numeric fluid ID the variant would bind
- `fluids.NULL_FLUID` if the block has no such state, no variant matches the resulting state map, or the variant holds no fluid

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

### Function: `world.get_temperature_base(bx, by, bz) -> number`

Retrieves the base temperature of the biome at a block.

#### Arguments

- `bx` is the world-scale block X position
- `by` is the world-scale block Y position
- `bz` is the world-scale block Z position

#### Return value

- Base temperature in degrees Celsius
- `15.0` if the block is void or the chunk is not present

### Function: `world.get_temperature(bx, by, bz) -> number`

Retrieves the effective temperature of the biome at a block.

#### Arguments

- `bx` is the world-scale block X position
- `by` is the world-scale block Y position
- `bz` is the world-scale block Z position

#### Return value

- Effective temperature in degrees Celsius
- `15.0` if the block is void or the chunk is not present

#### Notes

Rain and snow lower the effective temperature by up to two degrees Celsius, scaled by the type's coverage at the block. Seasons and other world-wide effects will add their own bias later. Relatively low values (below 0 C) turn planned rain into snow at that location.

### Function: `world.get_humidity_base(bx, by, bz) -> number`

Retrieves the base relative humidity of the biome at a block.

#### Arguments

- `bx` is the world-scale block X position
- `by` is the world-scale block Y position
- `bz` is the world-scale block Z position

#### Return value

- Base relative humidity as a percentage, from 0 to 100
- `50.0` if the block is void or the chunk is not present

### Function: `world.get_humidity(bx, by, bz) -> number`

Retrieves the effective relative humidity of the biome at a block.

#### Arguments

- `bx` is the world-scale block X position
- `by` is the world-scale block Y position
- `bz` is the world-scale block Z position

#### Return value

- Effective relative humidity as a percentage, from 0 to 100
- `50.0` if the block is void or the chunk is not present

#### Notes

Rain and snow push the effective humidity towards 0.98, scaled by the type's coverage at the block. Other world-wide effects will add their own bias later.

### Function: `world.id() -> string`

Retrieves the identifier of the world the callback runs in. Callbacks from every world share one Lua state, so a mod that serves more than one world keeps its state per world keyed by this value.

#### Return value

- The world identifier, for example `builtin:overworld`

### Function: `world.seed() -> integer`

Retrieves the world seed.

This function exists on the server only. The `world.seed` field is absent on the client, because the world seed never leaves the server: a client that knew it could predict the terrain it is streaming.

#### Return value

- The full 64-bit seed. Values above `2^63 - 1` read as negative Lua integers.

### Function: `world.schedule(bx, by, bz, dt)`

Schedules an `on_tick` call for a block in `dt` ticks. The tick source is `blocks.TICK_SCRIPTED`.

This function exists on the server only. The `world.schedule` field is absent on the client.

#### Arguments

- `bx` is the world-scale block X position
- `by` is the world-scale block Y position
- `bz` is the world-scale block Z position
- `dt` is the tick delay for the callback

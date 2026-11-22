# Fluids library

The `builtin:flowdef` library gives helpers for flowing blocks.

> **NOTE:** The library implements Minecraft-like flowing logic. It supports infinite water sources, fluids sharing a cell with a fillable block, and similar behaviour.

## Importing

```lua
local flowdef = require("builtin:flowdef")
```

### Flow config

|Field|Type|Default|Description|
|----|----|----|----|
|`fluid`|`string`|`block_name`|Fluid name. The engine stores the same name in the filled state of a fillable block|
|`max_level`|`integer`|8|Maximum level for a fluid blockstate|
|`full_height`|`integer`|14|Maximum fluid level for blockstates|
|`level_decrease`|`integer`|1|Level decrement with flow distance|
|`delay`|`integer`|5|Scripted tick delay|
|`infinite`|`boolean`|`true`|If true, flowing fluids can merge into sources|
|`slope_range`|`integer`|4|Maximum distance to search for a downward slope to flow into|

### Constant: `flowdef.filled_state`

The blockstate that marks a cell as filled with a fluid. Its values are fluid names, eg `"water"`. The value `"none"` means the cell is empty. The default is `"fluid"`. Change it before the mod registers any fillable block.

### Function: `flowdef.register(block_name, config_src)`

Registers the flow behaviour of one fluid. Call it after the mod registers the fluid block. The library resolves the fluid and block IDs lazily, once the mod registries commit.

#### Arguments

- `block_name` is the name of the block that represents the fluid, eg `"water"`
- `config_src` is the source flow config table

### Function: `flowdef.tick(bx, by, bz, source)`

The scheduled tick of fluid blocks and fillable blocks. Assign it to `on_tick`:

```lua
blocks.add("water", {
  -- ...
  on_tick = flowdef.tick,
})
```

It looks up the fluid in the cell and runs that fluid's registered flow. The library ignores cells without a registered fluid. A neighbour change only reschedules the cell. A scripted or random tick runs the flow. The engine never calls `on_tick` on the client.

### Function: `flowdef.schedule(fluid_id, bx, by, bz) -> boolean`

Schedules the first tick of a cell that has just started holding a fluid, with the fluid's `delay`. Use it whenever the mod fills a cell without a spread, eg when a bucket fills a block. Returns `false` when the fluid has no registered flow. On the client it returns `false` without scheduling, because `world.schedule` is a server-only API.

### Function: `flowdef.place_source(fluid_id, bx, by, bz) -> boolean`

Places a source of the given fluid, eg when a filled block is broken. Returns `false` when the fluid has no registered flow. On the client it writes the block and state for prediction, but it does not schedule a tick.

### Function: `flowdef.states(config_src) -> table`

This function returns a states object for block registration.

#### Arguments

- `config_src` is the source flow config table

#### Return value

A table to plug into `states` of a block definition.

### Function: `flowdef.variants(config_src) -> table`

This function returns a variants object for block registration.

#### Arguments

- `config_src` is the source flow config table

#### Return value

A table to plug into `variants` of a block definition.

### Function: `flowdef.on_place(config_src) -> function`

This function returns an `on_place` callback for the given flow config. Using the fluid on a fillable block fills that block instead of placing a fluid block next to it, like a bucket does.

#### Arguments

- `config_src` is the source flow config table

#### Return value

A function that implements placement logic for fluid blocks.

### Behaviour

The library mirrors the vanilla `FlowingFluid` and `WaterFluid` classes:

- A cell falls into the cell below whenever it can. It fans out only when it cannot fall, or when it just fell and three or more sources surround it. A source over a hole therefore pours down without flooding the floor around the hole.
- Sideways flow enters only a cell that holds no fluid. A cell that already holds fluid keeps its own level and recomputes it on its own tick. The fluid therefore never overwrites itself sideways or from above.
- A block that can hold a fluid declares the `filled_state` state and binds the fluid in a variant with `fluid_name`. The library asks `world.get_state_fluid` whether the cell would hold its fluid. It asks `world.get_fluid_faces` whether the fluid may pass the shared face. A filled cell is a full source of that fluid and still ticks. It flows out of its open faces, like a waterlogged block in vanilla.
- The face mask works both ways. A movement needs the face of the source cell and the face of the target cell open. A fluid therefore never leaves through a solid face of the block it sits in. It never enters through a solid face of the neighbour. A fluid block and a replaceable block leave every face open.
- A fillable cell takes a source only, like `canPlaceLiquid` does. Flowing water never fills a block. A filled cell is therefore a source that cannot seed another one at its neighbour. A flow into a field of fillable blocks stops instead of flooding it.
- Among the directions that can take fluid, the cell picks the ones that lead to the nearest drop within `slope_range`.
- With `infinite`, two horizontal sources turn a cell into a source when the cell below is solid or holds another source.
- The engine stores a falling cell as `source = "false"` with `level` equal to `max_level`, so `variants` renders it full.

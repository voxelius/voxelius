# Slabs Library

The `builtin:slabdef` library gives helpers for registering slab variants of regular blocks.

## Importing

```lua
local slabdef = require("builtin:slabdef")
```

## Functions

### Function: `slabdef.on_place(identifier, fillable) -> function`

This function returns an `on_place` callback for a given slab identifier.

#### Arguments

- `identifier` is the namespaced block ID. If you omit the namespace, the engine uses `core.NAMESPACE`
- `fillable` is an optional list of fluid names the slab can hold

#### Return value

- A function that implements slab placement logic for block registration

### Function: `slabdef.add_block(identifier, prototype, options) -> integer`

A wrapper around `blocks.add` that defines a slab block family.

#### Arguments

- `identifier` is the namespaced block ID. If you omit the namespace, the engine uses `core.NAMESPACE`
- `prototype` is the base block definition the function builds on
- `options` is an optional table with extra parameters (see below)

#### Options

|Field|Type|Default|Description|
|----|----|----|----|
|`health`|`integer`|prototype health or 2|Hit points of every slab variant|
|`drop_name`|`string`|`identifier`|Block name dropped by the slab|
|`fillable`|`string[]`|`{}`|Fluid names that can fill the slab|

#### Return value

- Numeric block ID

#### Notes

- The registered block family gets an `orientation` state and variants for `top`, `bottom`, and `double`.
- With `fillable`, the family also gets the `flowdef.filled_state` state. Its values are `"none"` and the fluid names. A variant with a fluid name binds that fluid through `fluid_name`, so the slab and the fluid share one cell, like a waterlogged block. A filled cell acts as a full source of that fluid.
- A `bottom` slab is open through its four side faces and the cell boundary above it. A `top` slab is open through its side faces and the boundary below it (`fluid_faces`). A `double` slab is a full cube and never holds a fluid.
- A fillable slab gets `on_tick` and `on_break`. A filled cell keeps a full source that flows out of its open faces and ticks on neighbour changes, like a waterlogged block in vanilla. Breaking it leaves the fluid source behind.
- Using a fluid on a fillable slab fills the slab instead of placing a fluid block next to it.
- Placing a fillable slab into a fluid source fills it immediately. The engine displaces flowing water, and the slab stays dry.
- The function keeps the touch response values from the prototype unchanged. It only defines block collisions.

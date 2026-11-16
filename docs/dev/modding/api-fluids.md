# Fluids API

Each block can define a fluid it holds and a fill level.

## Constants: fluid gravity

Defines the direction in which the fluid level decreases.

|Name|Description|
|----|----|
|`fluids.GRAVITY_DOWN`|The fluid sticks to the bottom face of a block|
|`fluids.GRAVITY_UP`|The fluid sticks to the top face of a block|

## Constant: null fluid

`fluids.NULL_FLUID` is an empty or non-existent fluid ID.

## Functions

### Function: `fluids.add(name, def) -> integer`

Registers a new fluid in the registry.

#### Arguments

- `name` is a namespaced fluid ID, eg `mymod:mywater`
- `def` is a fluid definition table (see below)

#### Return value

Returns the numeric fluid ID.

#### Notes

**Rename rule:** on conflict, the engine adds a `~N` suffix to the encroaching `name`. `N` is the smallest integer, starting at `1`, that gives a free id (e.g. `mymod:mywater~1`). Loading stays deterministic for the same mod list and order.

### Function: `fluids.get(name) -> integer`

Looks up a registered fluid by name.

#### Arguments

- `name` is a namespaced fluid ID, eg `mymod:mywater`

#### Return value

- The numeric fluid ID
- `fluids.NULL_FLUID` if the fluid does not exist, or the registries have not committed yet

## Fluid definition

|Field|Type|Required|Default|Description|
|----|----|----|----|----|
|`gravity`|`integer`|yes|N/D|One of `fluids.GRAVITY_XXXX` constants|
|`albedo`|`table`|yes|N/D|Fluid albedo textures|
|`masks`|`table`|no|`{}`|Fluid mask textures|
|`full_level`|`integer`|yes|N/D|Level that counts as a full cell, in 1/16ths of a block|
|`opaque`|`boolean`|no|`false`|If `true`, fluid quads use the `blocks.RENDER_SOLID` step instead of the fluid step|
|`tint`|`string`|no|`nil`|Namespaced tint id|
|`fog_density`|`number`|no|1.0|When the client camera is inside this fluid, the engine divides fog distance by this value|
|`fog_color`|`number[3]`|no|`{1,1,1}`|When the client camera is inside this fluid, the engine replaces fog color with this value|

## Fluid textures

- The engine assumes fluids are animated. It does not randomize texture frames by world position.
- Fluids define only two hard-coded texture slots: `still` and `flowing`.
- Texture syntax otherwise matches [block definitions](api-blocks.md), both for albedo and for mask textures.

# Overview

Voxelius _the game_ builds on Voxelius _the engine_. The engine exposes Lua hooks. Mods use these hooks to define blocks and other content that extends core gameplay.

## Namespaced identifiers

The engine uses _namespaced identifiers_, similar to Minecraft. A namespaced identifier has a namespace and an identifier:

```
[<namespace>:]<identifier>
```

The namespace is optional.

- In a mod script, a missing namespace defaults to the namespace of that mod.
- Elsewhere, a missing namespace defaults to `builtin` (the base game "mod" ID).

Valid characters in both namespace and identifier:

- Latin letters: `A-Z` and `a-z`
- Digits: `0-9`
- Underscore: `_`
- Tilde: `~` (not recommended for general use, see below)

On init, if the engine finds a duplicate registry entry (for example `mymod:myblock` registered twice), it appends `~N` and retries. The engine does not define the behaviour after 10000 renames.

## Sides

A mod runs on both the client and the server from the same `scripts/init.lua`. The engine exposes the side as two booleans:

|Value|Description|
|----|----|
|`core.IS_CLIENT`|True on the client|
|`core.IS_SERVER`|True on the server|

Some APIs exist on the server only: `worlds`, `world.schedule`, `world.seed` and `weather.force`. Their table or field is absent on the client, so an unguarded call fails the mod on that side. Guard it with `core.IS_SERVER`:

```lua
if core.IS_SERVER then
  dofile("world/overworld.lua")
end
```

|Callback|Runs on|
|----|----|
|`blocks.on_place`, `blocks.on_break`|Both sides. The client predicts, the server decides|
|`blocks.on_tick`|Server only|
|`blocks.on_interact`|Server only. The engine does not call it yet|
|`weather.on_tick`, `weather.on_strike`|Server only|

Register content such as blocks, biomes and weather types unconditionally. A definition registered on one side only changes the numeric IDs on that side and desynchronizes the two sides.

## Error handling

The mod root script `modname/scripts/init.lua` can catch its own errors. If an error escapes that script and reaches the init `lua_pcall`, the engine marks the mod load as a failure.

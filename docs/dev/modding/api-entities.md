# Entities API

A mod registers a class with `entities.add` and creates an instance of the class with `entities.spawn`. A class is a table of components, keyed by the component name. The engine owns the components and their fields. See [Components](#components).

`entities.spawn`, `entities.patch` and `entities.despawn` need an active world. Outside a callback or a server tick there is no active world, and the call raises a Lua error. `entities.add`, `entities.get` and `entities.set_player` do not need a world.

## Constants

|Value|Description|
|----|----|
|`entities.NULL_CLASS`|The ID of no class. `entities.get` returns this value when it cannot resolve a name|

## Function: `entities.add(name, def) -> integer`

Registers a class and returns its ID. The class belongs to the mod that registers it. Another mod can spawn the class after this mod finishes loading.

### Arguments

- `name` is the namespaced class ID
- `def` is a table of components. Each key is a component name from [Components](#components). Each value is the definition config of that component

### Return value

- The class ID

### Notes

- If another class already holds the name, the engine appends `~N` to it. The engine also logs a warning. See [namespaced identifiers](init-overview.md#namespaced-identifiers).
- A malformed name, an unknown component or a config that the component rejects raises a Lua error and fails the mod. A config that is not a table does the same.

## Function: `entities.get(name) -> integer`

Resolves a class name to an ID. The engine looks for a class of this mod first, then in the registry of every loaded mod.

### Arguments

- `name` is the namespaced class ID

### Return value

- The class ID
- `entities.NULL_CLASS` if the name has an invalid form or no class holds it

## Function: `entities.spawn(name, kv) -> integer`

Creates an instance of a class in the active world and returns its handle. The engine attaches every component with its definition config, then it applies `kv`.

### Arguments

- `name` is the namespaced class ID
- `kv` is an optional table of component fields for the new entity. Each key is a component name and each value is a patch config from [Components](#components). The engine ignores a key for a component that the class does not have

### Return value

- The entity handle, as an integer. The handle is valid only in the current world

### Notes

- A malformed name, an unknown class or a config that the component rejects raises a Lua error.

## Function: `entities.patch(entity, component, kv) -> boolean[, string]`

Assigns fields of one component of an entity.

### Arguments

- `entity` is the entity handle
- `component` is the component name
- `kv` is the patch config

### Return value

- `true` if the component took the config
- `false` if the handle is not valid in the active world
- `false` and a message if the component rejects the config

### Notes

- An unknown component name, a handle that is not an integer or a config that is not a table raises a Lua error.

## Function: `entities.despawn(entity) -> boolean`

Destroys the entity.

### Return value

- `true` if the engine destroyed the entity
- `false` if the handle is not valid in the active world

## Function: `entities.set_player(name)`

Marks the class that the engine spawns when a player joins the world.

### Arguments

- `name` is the namespaced class ID

### Return value

- No value

### Notes

- Call this function during init. The engine resolves the class once, after every mod loads. If no such class exists at that point, the start fails.
- A second call replaces the class of the first call and logs a warning.

## Components

|Component|Definition config|Patch config|
|----|----|----|
|`transform`|`{}`|`{ bpos = {...}, angs = {...} }`|
|`head`|`{ x, y, z }`|`{ x, y, z }`|
|`velocity`|`{}`|`{ x, y, z }`|
|`move_mode`|`{ value = "WALK" }`|`{ value = "WALK" }`|
|`player`|`{}`|none|

A component that the engine does not know is an error at `entities.add`. The engine has these five components.

### `transform`

Holds the position and the facing of the entity. Both patch fields are optional and keep the current value when they are absent.

- `bpos` is the world block position, as three integers
- `angs` is the facing, as three numbers in radians

### `head`

Holds the head of the entity.

- The definition config is the offset of the head from the entity origin, as three numbers in blocks. The built-in player uses `{ 0.4, 0.4, 1.65 }`
- The patch config is the look angles, as three numbers in radians. The engine clamps the pitch of a player to 90 degrees

### `velocity`

Holds the velocity of the entity in blocks per second. Each fixed tick the engine adds `velocity * fixed frametime` to the position. A velocity below the epsilon becomes zero.

- The definition config is empty

### `move_mode`

Holds the movement rules that `pmove` uses for the entity.

- `value` is `"WALK"` or `"FLY"`. The default is `"WALK"`. A patch with any other value fails

### `player`

Marks the entity as a player. It holds no fields.

- The definition config is empty
- `entities.patch` accepts an empty config and returns `true`

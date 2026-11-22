# Features

A feature is a single world-generation template. It contains a block palette and relative part offsets. The engine places features as scatter decorators through `biome.scatter`. Structure pieces and script-based placement arrive in a later change.

Stored features live under: `namespace:features`

A mod authors features facing **north** (`blocks.FACE_NORTH`). Placement rules supply world origin and facing at runtime.

## Top-level fields

|Field|Type|Required|Default|Description|
|----|----|----|----|----|
|`palette`|`object[]`|no|`[]`|Block palette for the feature|
|`parts`|`object[]`|yes|N/D|Feature parts|

## Palette entries

```json
"palette": [
  {
    "block": "builtin:stone_slab",
    "states": {
      "orientation": "bottom"
    }
  },
  {
    "block": "builtin:grass"
  }
]
```

|Field|Type|Required|Default|Description|
|----|----|----|----|----|
|`block`|`string`|yes|N/D|Namespaced block ID|
|`states`|`object`|no|`{}`|Blockstate key-values|

## Parts entries

```json
"parts": [
  {
    "block": 0,
    "offset": [0, 1, 0],
    "overwrite": ["gas", "xfoil"]
  }
]
```

|Field|Type|Required|Default|Description|
|----|----|----|----|----|
|`block`|`integer`|yes|N/D|Palette entry index|
|`offset`|`integer[3]`|yes|N/D|Offset in blocks from the origin point|
|`overwrite`|`string[]`|no|`{}`|Block tags the feature can overwrite when they occupy the target space|

### Tag values

|JSON|Lua constant|
|----|----|
|`"gas"`|`blocks.TAG_GAS`|
|`"rock"`|`blocks.TAG_ROCK`|
|`"soil"`|`blocks.TAG_SOIL`|
|`"turf"`|`blocks.TAG_TURF`|
|`"xfoil"`|`blocks.TAG_XFOIL`|
|`"sfoil"`|`blocks.TAG_SFOIL`|
|`"wood"`|`blocks.TAG_WOOD`|

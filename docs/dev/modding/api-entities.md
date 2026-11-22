# Entities API

`entities.spawn`, `entities.patch` and `entities.despawn` need an active world. Outside a callback or a server tick there is no active world, and the call raises a Lua error.

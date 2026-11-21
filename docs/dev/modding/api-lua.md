# Lua Base

The Voxelius runtime exposes these common Lua APIs:

|Table|Notes|
|----|----|
|`LUA_GNAME`| |
|`LUA_TABLIBNAME`|Some values may be stripped later in development|
|`LUA_STRLIBNAME`| |
|`LUA_MATHLIBNAME`| |
|`LUA_UTF8LIBNAME`| |

## Stripped globals

|Global|Reason|
|----|----|
|`loadfile`|Sandbox breaking|
|`load`|Sandbox breaking|
|`collectgarbage`|Sandbox breaking|

## Replaced globals

|Global|Overridden behaviour|
|----|----|
|`dofile`|Treats the input path as a namespaced identifier. It implies the `scripts` subdirectory. Returns the results of the chunk|
|`require`|Takes a namespaced ID. The namespace must be explicit. Files come from the `lib` directory|
|`print`|Alias to `msg::log_info`|

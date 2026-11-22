# Core API

The engine exposes limited access to subsystems and constants through the `core` library.

## Version constants

These values match the game and engine version. See `core/version.hh` for the C++ equivalent.

|Value|Example|Description|
|----|----|----|
|`core.VERSION_MAJOR`|`16`|Major version number|
|`core.VERSION_MINOR`|`0`|Minor version number|
|`core.VERSION_PATCH`|`3`|Patch version number|
|`core.VERSION_SCM_BRANCH`|`main`|SCM branch name|
|`core.VERSION_SCM_REVISION`|`deadbeef`|SCM revision (git commit hash)|
|`core.VERSION_SEMANTIC`|`16.0.3`|Semantic version string|
|`core.VERSION`|`16.0.3-main [deadbeef]`|Full build-system version string|

## Current namespace

As noted in the [overview](init-overview.md), namespaced identifiers may omit the namespace. The engine fills in a default. For mods, that default is also available as `core.NAMESPACE`.

## Sides

|Value|Description|
|----|----|
|`core.IS_CLIENT`|True on the client|
|`core.IS_SERVER`|True on the server|

Use these to guard APIs that exist on one side only. See [Sides](init-overview.md#sides).

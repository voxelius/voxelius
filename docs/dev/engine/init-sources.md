# Source Hierarchy

> **NOTE:** This file exists so I do not move sources again. I do not want to rewrite docs every time the tree changes.

## In-tree directories

|Directory|Description|
|----|----|
|`client`|Client executable sources|
|`core`|Core library. Useful without depending on block-related code|
|`data`|Content packed with release executables. At least the `builtin` mod. More may appear later if people contribute|
|`deps`|Third-party dependencies|
|`docs`|Website sources, including documentation and a devlog|
|`scripts`|Scripts used during build and elsewhere|
|`server`|Server executable sources|
|`shared`|Shared game library. Most game logic, networking, and other client/server shared code|

## Ignored directories

These appear when you run or build the game with the scripts. `.gitignore` lists them, and git does not track them.

|Directory|Description|
|----|----|
|`build`|Root build directory|
|`dist`|CPack output directory|

## Directory tree

Produced by running `tree.exe` on the repository root.

```
C:\PATH\TO\REPOSITORY
├───.vscode
├───build
├───client
│   ├───input
│   ├───net
│   ├───res
│   └───utils
├───core
│   ├───config
│   ├───cvar
│   ├───msg
│   ├───res
│   └───utils
├───data
│   └───builtin
│       ├───collisions
│       │   └───block
│       ├───fonts
│       ├───lang
│       ├───misc
│       ├───models
│       │   └───block
│       ├───scripts
│       │   └───blocks
│       ├───shaders
│       └───textures
│           ├───block
│           └───gui
├───deps
│   ├───eigen
│   ├───emhash
│   ├───enet
│   ├───entt
│   ├───fastnoiselite
│   ├───glad
│   ├───imgui
│   ├───lua
│   ├───parson
│   ├───physfs
│   ├───SDL3
│   ├───stb
│   └───thread_pool
├───docs
│   ├───about
│   ├───dev
│   │   ├───engine
│   │   └───modding
│   └───stylesheets
├───mods
├───scripts
├───server
└───shared
    ├───cvar
    ├───res
    └───utils
```

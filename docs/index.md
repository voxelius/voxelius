---
title: Voxelius
template: home.html
hide:
  - navigation
  - toc
  - title
description: Voxel game and engine
---

> **WARNING:** The project generates this documentation on each release, or writes it by hand. That avoids burning GitHub CI on typo-only fixes. If you need docs that match the latest sources, generate them from the repository yourself.

## What is this?

Voxelius is a voxel game and engine hybrid. Core gameplay, rendering, and world generation are in C++. Built-in game content and modifications are in Lua. This website contains public documentation about engine development, modding, and general information about the project.

## Where do I go?

- **[Documentation](dev/index.md)** - engine setup, architecture, and Lua API
- **[Manual](manual/console/index.md)** - the console, and the variables and commands it takes
- **[History](about/history.md)** - how the project got here (rewrites included)
- **[Features](about/features.md)** - what the project includes
- **[GitHub](https://github.com/untodesu/voxelius)** - source code and issue tracker

### Engine documentation

If you want to fix bugs, improve rendering, change world generation, or do anything else, go to the [engine](dev/engine/init-requirements.md) section. Clone the repository, build it, and patch the engine. You can also contribute.

### Modding

If you want to make a mod, go to the [modding](dev/modding/init-overview.md) section. Learn about the Lua API of the engine and tinker with it without forking the engine.

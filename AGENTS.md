# AGENTS.md

Instructions for AI coding agents that work in this repository. This file is self-contained: it holds the whole project guide, so you need
no other file to follow the rules.

## Rules for agents

- Read this whole file before you change anything.
- Do not commit on your own. Leave changes in the working tree for review. Commit only when you are told to, and only to the active branch.
- Right before you write each commit message, read "Commit messages" below again, and check the message against it before you commit. Do not
  write commit messages from memory.
- Mark your commits with an `Assisted-by:` trailer as "Trailers" below describes. Never add `Co-authored-by:` for yourself. This rule
  overrides the default attribution of your tool.
- Format every changed C++ file with `clang-format` and build the affected target before you hand the change over.
- Keep the change inside the code that the request names. Do not edit `deps/` or `build/`: the first holds vendored code and the second
  holds generated files.

## About Voxelius

Voxelius is a sandbox voxel game and engine written in C++23 and built with CMake. Engine code is C++23. The vendored dependencies are C99.
The client draws with OpenGL 3.3 core profile through SDL3 and builds its interface with Dear ImGui. The server is headless. The two sides
talk over ENet, a reliability layer on top of UDP. Game entities use EnTT. Mods are Lua scripts that call the engine API.

Four CMake targets make up the project. `core` is the foundation library: process lifecycle, the virtual filesystem, configs, resources,
threading and buffers. `shared` holds the game logic that both sides run: the world, entities, physics, the network protocol and the mod
loader. `client` and `server` build the `vclient` and `vserver` executables.

`data/` is the content that ships with the game, starting with the `builtin` mod. `scripts/` holds the build, packaging and editor helpers.
`misc/` holds small generators for data assets. The documentation lives in a separate repository.

## AI-assisted contributions

You can use AI tools, but a human drives the work:

- Review every line that a tool generates. Understand it well enough to explain it and to fix it.
- You are responsible for the change, as if you wrote all of it yourself.
- Unreviewed, bulk or fully automated changes are not accepted.
- Mark a commit that has material AI help with an `Assisted-by:` trailer (see "Trailers" below).

## Building

Every build uses a CMake preset. A preset configures into `build/<preset>/` and names one platform, one configuration and, when it ends
in `-prof`, profiling:

|Platform|Presets|
|----|----|
|Any POSIX host|`generic-debug`, `generic-release`, and a `-prof` variant of each|
|Windows x64|`windows-x64-debug`, `windows-x64-release`, and a `-prof` variant of each|
|Windows x86|`windows-x32-debug`, `windows-x32-release`, and a `-prof` variant of each|

```sh
cmake --preset windows-x64-debug
cmake --build --preset windows-x64-debug
```

- `scripts/build-*.bat` and `scripts/build-*.sh` wrap one preset each, from configure to build.
- The `-prof` presets define `TRACY_ENABLE`, `TRACY_ON_DEMAND` and `TRACY_NO_FRAME_IMAGE`. Use them with the Tracy profiler. A normal
  build must not define them.
- On Windows with MSVC, `deps/SDL3` downloads the SDL3 3.4.12 development package into `build/<preset>/_deps/` at configure time. Every
  other dependency is in the tree, so a build after the first needs no network access.
- A C++23 compiler, a C99 compiler, CMake 3.25 or newer and Python 3 are enough. The presets use schema version 6, so an older CMake
  cannot read them. Python runs the third-party license collector and the asset generators.
- The executables land in `build/<preset>/`. A multi-configuration generator such as Visual Studio adds the configuration directory:
  `build/<preset>/Debug/vclient.exe`.
- No workflow compiles the game, so a compile error is yours to find.

## Running

Run the executables from the repository root. The default game content path is `data`, relative to the working directory:

```sh
build/windows-x64-debug/vclient.exe
build/windows-x64-debug/vserver.exe
```

- `-gamepath <path>` points the engine at another content directory.
- `-user <path>` points it at another user directory. The default is a per-user directory taken from `%APPDATA%`, `$XDG_DATA_HOME` or
  `$HOME`, in that order. The engine keeps the configs, the world and the server whitelist there.
- Both sides mount the content, the user directory and the mods directory into one virtual filesystem. A test run therefore writes nothing
  into the repository.

There is no test suite. Verify a change by building it and by running the client, the server, or both. A change to the network protocol
needs both sides.

## Code style

The [Code Style](https://voxelius.github.io/docs/engine/code-style.html) page is the reference. The rules below are the ones that a change
most often breaks.

### Formatting

Format C++ with `clang-format` and the `.clang-format` in the repository root: 4 spaces, 140 columns, CRLF line endings, `T* p`, and the
opening brace of a function on its own line. The tree is formatted with clang-format 21. An older version can format some constructs
differently.

- `deps/**` is excluded by `.clang-format-ignore`.
- Check changed files with `clang-format --dry-run --Werror <files>`.
- If the result looks wrong, split the code into smaller chunks. Do not edit `.clang-format` first.

### Language

- Use C++23. Prefer the standard facility over a hand-written one: `std::bit_cast`, `std::span`, `std::format`, concepts,
  `std::source_location`.
- Report a failure through `vx::throw_if` or `vx::throw_if_not` from `core/exception.hh`. Both throw a `vx::runtime_error` with the source
  location of the call. Use `assert` only for a condition that must hold in a debug build.
- `main` catches every exception, logs it and returns a failure code. Do not add a second top-level handler.
- Mark a class or structure `final` unless it is a base.
- A function with no parameters writes `(void)`.
- Do not silence an unused value with a `void` cast (`(void)x;`, `static_cast<void>(x);`).
- Name every parameter, including an unused one: `void f(int x, int y)`, not `void f(int x, int)`. An unnamed parameter of a
  fundamental type such as `char` makes `function(char)` read like `function(void)`. Disable the unused-parameter warning for
  that function or translation unit instead.
- Put the body of a `constexpr` function in the header, right after its declaration.
- Do not repeat a `const` that the initializer already carries. When a function returns a const reference, `auto&` is enough:
  `auto& palette = style::palette();`, not `const auto&`. Keep `const auto&` when the reference binds a temporary.
- Write `auto` even when the deduction yields a pointer (`auto world = WorldScope::active();`), not `auto*`.
- When a block of locals comes from deduction, keep them all on `auto`. Do not spell the type between them; put it in the
  initializer instead: `auto elapsed = std::chrono::system_clock::duration(now.time_since_epoch());`.
- Test a pointer or a handle by itself when it must be present, and compare it with the null value when it must be absent: `if(ptr)` and
  `if(ptr == nullptr)`, `if(handle)` and `if(handle == BIOME_ID_NULL)`. A count follows the same split: `if(count)` and `if(count == 0)`.
  Never write the absent case as a negation or an inequality: no `if(!ptr)`, `if(!handle)`, `!= 0`, `!= nullptr`, `0 !=`, `nullptr !=`
  or `!= HANDLE_NULL`. Negating a call that only reports a condition stays: `if(!load(path))`, `if(!value.has_value())`.
- Test a bitmask against zero explicitly: `if(0 == (flags & FLAG))`, not `if(!(flags & FLAG))`.
- Use a ternary only for a short pick between constants, written `cond ? a : b` with spaces around both operators. When either side
  holds a call, a dereference or another non-trivial operand, write `if`/`else`.
- Never write a standalone `++x` or `x--` statement. Advance a counter whose name is longer than one character with `x += 1` or
  `x -= 1` (`dz`, `index`); a one-letter counter (`i`, `z`) may use `++` or `--` in a `for` header. An iterator advances with
  `std::next(it)`.
- A `for` header has all three parts or none (`for(;;)`). A loop that fills two of them is a `while`, with the step at the end of the
  body.
- A `switch` case does not open a scope. Move a body that needs locals into a function.
- Do not chain on the result of a call through a pointer (`registry().find(id)->name`, `map.find(key)->second`); put the result in a
  named local first. A short dot chain reads as one operation and may stay in place: `dispatcher.sink<Event>().connect<&handler>()`. A
  chain of three or more calls goes into named locals. Do not put a call in the middle of another call's arguments or nest calls
  deeper than one level; a call as the only or the last argument is fine.
- Keep the registration calls together. A run of `dispatcher.sink<Event>().connect<&handler>()` lines, or of the
  `on_construct`/`on_update` ones, has no blank lines inside it; one blank line separates a different group of events.
- Keep a condition to three operands joined by `&&` or `||`, on one line. Name the parts in `bool` locals, or split the test into
  several `if` blocks that each return or continue.
- Do not initialize a structure with its fields on one line. Either list the designated fields one per line, with a comma after the
  last one so clang-format keeps them apart, or start from `{}` and assign the fields after it. File-scope tables keep their
  initializers.
- Give a non-trivial size or index expression a name instead of writing it inline.
- Do not qualify a name with the namespace that the enclosing function already lives in. A `gui::Button` method writes
  `detail::draw_icon()`, not `gui::detail::draw_icon()`. An out-of-line definition keeps its qualified name in the declarator.
- Close a namespace with `} // namespace name`. clang-format enforces the comment.
- An intentionally empty body holds `// empty`.

### Names

|Kind|Style|Example|
|----|----|----|
|Files|`snake_case`|`shared/block_storage.cc`|
|Namespaces|`snake_case`|`namespace block_storage`|
|Classes and structures|`PascalCase`|`class ReadBuffer`|
|Enumerations|`snake_case`|`input_device`, `packet_type`|
|Functions and methods|`snake_case`|`block_registry::name_of()`|
|Public fields and members|`snake_case`|`BlockDefinition::model_offset`|
|Private fields and members|`m_snake_case`|`Chunk::m_blocks`|
|Constants and enum values|`UPPER_SNAKE_CASE`|`BASE_WIDTH`, `LOADING_IDLE`|

- An enumeration value is a constant: the enum type is `snake_case` (`loading_stage`), and every value is
  `UPPER_SNAKE_CASE` (`LOADING_IDLE`). This holds for `enum` and `enum class`.
- Two names must differ by more than capitalization: `world_scope` and `WorldScope` read as the same name, so keep one of them.
- A cvar binding takes the cvar name and the `_binding` suffix:
  `auto& gr_weather_cycle_binding = cvar::bind_var(weather, "gr_weather_cycle");`, not `weather_binding`.

### Headers and includes

- Headers use `.hh`, sources use `.cc`. A C source keeps `.c`.
- Every header has a GUID-style include guard. Dashes become underscores. `#pragma once` is not used. End the block with the same token in
  a block comment:

  ```cpp
  #ifndef B843EAA9_60C3_4C2B_8036_DF1026035AA8
  #define B843EAA9_60C3_4C2B_8036_DF1026035AA8

  // ...

  #endif /* B843EAA9_60C3_4C2B_8036_DF1026035AA8 */
  ```

- Every compiled source includes the precompiled header of its module first: `#include "client/pch.hh"`, `#include "shared/pch.hh"` or
  `#include "core/pch.hh"`. The build force-includes the same file, so the include is redundant to the compiler. Keep it anyway: it is
  what an editor reads first and what the module owns.
- Sort the other includes by dependency: `core`, then `shared`, then `client` or `server`. clang-format sorts each group.
- Do not edit `deps/`. A new dependency follows the shape of the current ones: a `deps/<name>/` directory, a `CMakeLists.txt` there that
  defines `deps::<name>`, and an entry in `deps/CMakeLists.txt`.

### Order in a file

Every translation unit keeps the same top-level order:

1. includes;
2. forward declarations;
3. compile-time constants: `constexpr`, `static constexpr` and object-like `#define`s;
4. type definitions: `using`, `enum`, `struct` and `class`;
5. read-only data such as `static const char* const names[] = { ... }`;
6. static and global variables, in any order among themselves;
7. static functions and other functions with internal linkage;
8. public functions, including out-of-line member definitions.

- A constant goes into the constant block and a helper into the static function block; do not put either next to its first use between
  functions of another kind.
- Static functions go in dependency order, callees first, so a static prototype is only necessary for mutual recursion.
- A table of callbacks or a registration list goes right after the last function it points to.
- Function-like macros and `static_assert`s stay next to what they belong to.
- The lifecycle functions form a group of their own, always: `init`, `init_late`, `shutdown`, `fixed_update`, `fixed_update_late`, `update`,
  `update_late`, `render` and `layout`, in this order. A module that has only some of them keeps the same relative order. Keep setters,
  getters and helpers out of this group, and keep the header and the translation unit in the same order.
- Headers follow the same order. A header is a translation unit too: includes, forward declarations, constants, types, read-only data,
  globals, then function declarations and out-of-line definitions. A header that only declares functions still keeps its constants above
  them. A `constexpr` body stays right after its declaration as "Language" says.

#### Class bodies

A class body follows the same idea. A structure that gains a method becomes a class; a structure holds fields and no behavior.
A tag structure marks a template argument: it holds no fields and only declares static members, so it stays a structure.

- one access block per class: `public`, then `protected`, then `private`; do not repeat a specifier;
- statics come first: the constants and the static methods;
- after them the constructor and destructor (plus `operator=`), the non-static methods, then the fields;
- a static member comes before a non-static one, and a field comes after every method.

### Comments

Code documents itself through names. A comment is the exception. Leave one only for the information that the code cannot carry: a
measurement, a tuning value, a batch size, a protocol quirk, an outside requirement, or the reason behind a construct that looks wrong
until the reader knows why it is there. Never restate what the code next to it does.

- Before you write a comment, try to make it unnecessary: rename (`free_slots` instead of `slots // A set bit means free`), move the code
  into a function whose name says what it does, or turn the claim into a `static_assert`.
- Do not describe the algorithm of a function above it. A precondition or an ownership rule that every caller must respect stays at the
  declaration: in the header for a public function, at the definition for a static one. A guarantee that only some callers rely on goes at
  those call sites, as the reason the caller is correct.
- A field, a constant or a local gets no comment that repeats its name, frame, units, axis or range. If the value needs that text, write
  it once in the type or the schema. A comment that explains a field is a missing rename: `local_box`, `axis_contacts`, `volume_fraction`.
- Keep the measurement or the outside rule with the value it chose: `// 64 keeps the upload under the 1 MiB limit`, not `// batch size`.
  Keep the reason with the strange construct: `// The server applies the client state on purpose; the client cannot rebuild it`.
- A comment that fails this test is removed in review; do not add one to look helpful.
- Write comments approximately in ASD-STE100 Simplified Technical English: short sentences, one statement per sentence, active voice,
  present tense, common words with one meaning each.
- Use `//` in C++ and GLSL, `--` in Lua and `#` in CMake and shell. Start with a capital letter and leave off the trailing period.
- A short note about a single line goes after it, if the line still fits in 140 columns.
- Wrap a longer comment at clause boundaries. Balance the lines: match the width of the code line the comment belongs to, or split the
  text into equal parts when there is no such line or it is short. The last line must not be shorter than the line before it.
- Tagged comments:
  - `TODO:` is work that nobody has planned yet.
  - `UNDONE:` is work whose approach is known but not done.
  - `FIXME:` is either of those where the current behavior is a bug.
  - `NOTE:` is a remark worth the attention of the reader. Repeat it to raise the weight: `NOTENOTE:` marks something that the surrounding
    code cannot be understood without.
- An intentionally empty body holds `// empty`.
- Do not leave commented-out code. The jokes and rants already in comments are part of the history of the project; leave them alone.

## Repository map

|Directory|Contents|
|----|----|
|`core/`|Foundation library. `config/` is the generic key-value store, `cvar/` the console variables and commands, `msg/` the logging layer, `res/` the resource loader, `utils/` the small helpers|
|`shared/`|Game logic for both sides. `api/` the Lua API, `component/` and `entity/` the EnTT pieces, `cvar/` the shared variables, `net/` the packets, `physics/`, `system/`, `world/`|
|`client/`|`vclient`. `gui/` the menus, `net/` the connection, `res/` the render resources, `system/` the frame systems, `world/` the renderer|
|`server/`|`vserver`. `entity/`, `net/`, `system/`, `utils/`, `world/`|
|`data/`|Content that ships with the game: `builtin` mod, shaders, textures, language files|
|`defaults/`|Defaults copied into the user directory on the first run|
|`deps/`|Vendored third-party code and its CMake glue|
|`scripts/`|Build, package and editor helpers|
|`misc/`|Small image and model generators|

### Architecture

- **Startup.** `core` resolves three directories: game content, user data and mods. It mounts all three into one PhysFS virtual filesystem.
  The user directory is writable, and every subsystem reads and writes through that filesystem instead of the disk.
- **Identifiers.** A moddable thing is named `mod:some/thing`, not by a path. Blocks, biomes, sounds, resources and textures use the same
  convention.
- **Resources.** One loader is registered per resource type. A request by identifier returns the cached instance or loads it. A soft purge
  drops what nothing references, a hard purge drops everything and warns about what was still in use.
- **Threading.** Heavy work goes to a background pool. Each task has a compute step on a worker thread and a finish step on the main
  thread, so most code needs no lock. `-threads` sets the pool size, with a default of four.
- **Buffers.** One binary format describes both network packets and saved data. A format is a sequence of the same base types.
- **Rendering.** The client renders at an internal pixel scale into three layers (diffuse, alpha, fluid), composites them, and draws ImGui
  last. `video` owns the window; `head` owns the OpenGL context, so the context can be rebuilt without the window.
- **Networking.** Every packet starts with a 16-bit ID. A client does either a status ping or a full login. The login uses an Ed25519
  challenge and compares the registry hashes of both sides.

The [documentation](https://voxelius.github.io/docs/) describes each of these in full.

## Editor and code intelligence

The repository ships a setup script for both C/C++ engines in VS Code. Run it after the first configure:

|Shell|clangd|IntelliSense|
|----|----|----|
|Windows PowerShell|`.\setup_clangd.ps1`|`.\setup_intellisense.ps1`|
|Windows cmd.exe|`setup_clangd.bat`|`setup_intellisense.bat`|
|POSIX shell|`./setup_clangd.sh`|`./setup_intellisense.sh`|

If the Windows execution policy blocks a PowerShell script, use the `.bat` wrapper or run the Python file directly:
`py -3 scripts/setup_environment.py clangd`.

- `clangd` is the default. The script turns off the analysis of the Microsoft extension, keeps its debugger, and checks for
  `compile_commands.json` in the repository root. The CMake build copies it there from the preset binary dir.
- Add `--preset <name>` to name the preset when the root database is missing.
- The `intellisense` mode writes `.vscode/c_cpp_properties.json` from the presets and the dependency tree.
- `.vscode/settings.json` and `build/` are ignored by git. `.vscode/c_cpp_properties.json` is tracked and holds only workspace-relative
  paths.
- `.clangd` in the repository root holds the project-wide clangd switches. It stops the include cleaner from reporting the module
  precompiled header as unused, and it parses headers as C++23.
- CMake writes a compilation database only for the Ninja and Makefile generators. The Visual Studio generator writes none, so clangd
  needs one Ninja configure:

  ```sh
  cmake -S . -B build/clangd -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
  ```

## Commit messages

- Make one logical change per commit.
- Write the subject as one sentence that says what the change does. Start it with a capital letter and do not end it with a period.
- Do not add a type or area prefix. A `core:` or a `feat:` prefix is not the style of this repository.
- Use the imperative mood when the sentence allows it: `Move worldgen code to server`, `Fix crashing`. A noun phrase is also correct:
  `Client connections`, `Checkpoint`.
- Keep the subject on one line. Put the details in the body.
- Most commits need no body. Write one when the change has several parts, or when the subject does not show why the change is done. A body
  is a list of `- ` bullets, one per part. Do not list the changed files and do not repeat the subject.
- The voice is direct and informal. Read `git log` before you write and keep the tone of the history. Do not write in the tone of a report.

### Trailers

Trailers go in the last paragraph of the message, after a blank line.

- A commit that has material AI help gets an `Assisted-by:` trailer in the form `AGENT_NAME:MODEL_VERSION`. Use the full lowercase model
  identifier, not only the model family:

  ```
  Keep the config file open across a reload

  Assisted-by: Claude:claude-opus-5-5
  ```

- Do not add `Co-authored-by:` for AI tools.

## Documentation

- The documentation is a separate repository: https://github.com/voxelius/docs. It builds with mdBook and publishes at
  https://voxelius.github.io/docs/.
- `engine/` covers the requirements, the source hierarchy, the build, the editor setup, the architecture, the code style and the
  contribution process. `modding/` covers the mod API and the file formats.
- A change to a documented behaviour needs a page change in that repository. The page and the code land in separate pull requests, so the
  page can follow the change in the next one.
- Build the book with `mdbook build`, or preview it with `mdbook serve`, in that repository. This repository holds no pages.

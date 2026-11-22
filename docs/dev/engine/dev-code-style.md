# Code Style

## Clang-format

Format all C++ code with `clang-format`. The config file is in the repository root. If your editor supports format-on-save, enable it. Otherwise format before you commit.

Dependencies stay out of automatic formatting. They rarely match Voxelius style.

Notable settings:

|Settings|Value|
|----|----|
|Column limit|140|
|Indentation|4 spaces|
|Line endings|CRLF|
|Braces|Custom, see below|
|Pointer alignment|Left|
|Reference alignment|Left|

> **NOTE:** If formatting looks wrong, do not edit `.clang-format` first. Split the code into smaller chunks so clang-format can handle it better.

## Naming

|Kind|Style|Example|
|----|----|----|
|Files|`snake_case`|`shared/block_storage.cc`|
|Namespaces|`snake_case`|`namespace block_storage`|
|Classes and structures|`PascalCase`|`class ReadBuffer`|
|Functions and methods|`snake_case`|`block_registry::name_of(...)`|
|Public fields and members|`snake_case`|`BlockDefinition::model_offset`|
|Private fields and members|`m_snake_case`|`Chunk::m_blocks`|
|Constants|`UPPER_SNAKE_CASE`|`BASE_WIDTH`|
|Type aliases|Inherited|...|

- Two names must differ by more than capitalization: `world_scope` and `WorldScope` read as the same name, so keep one of them.
- A cvar binding takes the cvar name and the `_binding` suffix:
  `auto& gr_weather_cycle_binding = cvar::bind_var(weather, "gr_weather_cycle");`, not `weather_binding`.

## Brace newlines

- Functions, methods, and namespaces put the opening brace on a new line.
- Classes, structs, enums, unions, control statements, and lambdas keep the brace on the same line.
- Put `else` and `catch` on a new line after the closing brace. Two braces and a keyword on one line are hard to read.

Example:

```cpp
namespace vx
{
void setup(void);
void teardown(void);
} // namespace vx

void vx::setup(void)
{
    // empty
}

void vx::teardown(void)
{
    Type name {};
    name.do_something();

    if(name.valid()) {
        // ...
    }
    else {
        // ...
    }
}
```

## Styling considerations

- Mark classes and structures `final` unless they are meant as a base.
- Functions and methods with zero arguments use C-style `void` inside the parentheses.
- Do not silence an unused value with a `void` cast (`(void)x;`, `static_cast<void>(x);`). Leave an unused parameter unnamed in the
  definition (`void f(int x, int)`).
- Place `constexpr` function bodies in the header, just after the declarations.
- Do not repeat a `const` the initializer already carries. When a function returns a const reference, `auto&` is enough and
  `const auto&` is noise.
- Write `auto` even when the deduction yields a pointer (`auto world = WorldScope::active();`), not `auto*`.
- When a block of locals comes from deduction, keep them all on `auto`. Do not spell the type between them. Put it in the
  initializer instead: `auto elapsed = std::chrono::system_clock::duration(now.time_since_epoch());`.
- C++ headers use `.hh`. C++ sources use `.cc`.
- You may use profanity in source code. I own the code and can comment as I choose.
- Use standard `assert` for debug-time checks. Use `vx::throw_if_xxxx` when the game should crash on failure.
- Test a pointer or a handle by itself when it must be present. Compare it with the null value when it must be absent: `if(ptr)` and
  `if(ptr == nullptr)`, `if(handle)` and `if(handle == BIOME_ID_NULL)`. A count follows the same split. Never write the absent case as a
  negation or an inequality (`if(!ptr)`, `if(!handle)`, `!= 0`, `!= nullptr`), and keep negating a call that only reports a condition:
  `if(!load(path))`, `if(!value.has_value())`.
- Test a bitmask against zero explicitly: `if(0 == (flags & FLAG))`, not `if(!(flags & FLAG))`.
- Use a ternary only for a short pick between constants, written `cond ? a : b` with spaces around both operators. A call, a
  dereference or another non-trivial operand on either side means `if`/`else`.
- No standalone `++x` or `x--` statement: write `x += 1` or `x -= 1`. An iterator advances with `std::next(it)`.
- A `for` header has all three parts or none. A loop that fills two of them is a `while` with the step at the end of its body.
- A `switch` case does not open a scope. Move a body that needs locals into a function.
- Do not chain on a call result through a pointer (`registry().find(id)->name`, `map.find(key)->second`). Put it in a named local
  first. A short dot chain reads as one operation and may stay in place: `dispatcher.sink<Event>().connect<&handler>()`. A chain of three
  or more calls goes into named locals. Do not nest calls deeper than one level. Do not put a call in the middle of another call's
  arguments. A call as the only or the last argument is fine.
- Keep the registration calls together: a run of `dispatcher.sink<Event>().connect<&handler>()` lines has no blank lines inside it,
  and one blank line separates a different group of events.
- Keep a condition to three operands joined by `&&` or `||`, on one line. Name the parts in `bool` locals or split the test.
- Do not initialize a structure with its fields on one line: designated fields one per line with a trailing comma, or `{}` and assign
  after. File-scope tables keep their initializers.
- Give a non-trivial size or index expression a name instead of writing it inline.

## Include guards

- Every header uses an ifndef block with a random GUID-style token. Dashes become underscores.
- Do not use `#pragma once`.
- End the ifndef block with a block comment that repeats the same GUID.

Example:

```cpp
#ifndef B843EAA9_60C3_4C2B_8036_DF1026035AA8
#define B843EAA9_60C3_4C2B_8036_DF1026035AA8

// ...

#endif /* B843EAA9_60C3_4C2B_8036_DF1026035AA8 */
```

> **NOTE:** This matches the default behaviour of the [C/C++ Include Guard](https://marketplace.visualstudio.com/items?itemName=akiramiyakoda.cppincludeguard) VSCode extension.

## Namespaces

- Namespaces follow modules. The same header often reopens a namespace per logical group. Put internal or private types in a `detail` sub-namespace.
- Every closing brace gets a `// namespace name` comment. Clang-format enforces this.

Example:

```cpp
namespace vx::detail
{
class Exception {
  // ...
};
} // namespace vx::detail

namespace vx
{
using runtime_error = detail::TaggedException<struct runtime_error_tag>;
} // namespace vx
```

## Include grouping

- Each engine module has its own precompiled header. Include it first in every compiled source.
- Sort includes by dependency order (for example `core`, then `shared`, then `client`).
- Within a group, clang-format sorts includes alphabetically.

Example:

```cpp
#include "client/pch.hh"

#include "client/game.hh"

#include "core/buffer.hh"
#include "core/exception.hh"
#include "core/identifier.hh"
#include "core/res/resource.hh"

#include "shared/block_registry.hh"
#include "shared/block_storage.hh"
#include "shared/coord.hh"
#include "shared/mod_loader.hh"
#include "shared/utils/coord.hh"
#include "shared/utils/lua.hh"
#include "shared/world.hh"

#include "client/globals.hh"
#include "client/res/texture_2d.hh"
```

## Order in a file

Every translation unit keeps the same top-level order:

1. includes.
2. forward declarations.
3. compile-time constants: `constexpr`, `static constexpr` and object-like `#define`s.
4. type definitions: `using`, `enum`, `struct` and `class`.
5. read-only data such as `static const char* const names[] = { ... }`.
6. static and global variables, in any order among themselves.
7. static functions and other functions with internal linkage.
8. public functions, including out-of-line member definitions.

- A constant belongs in the constant block and a helper belongs in the static function block. Do not put either next to its first use
  between functions of another kind.
- Static functions go in dependency order, callees first. A static prototype is only necessary for mutual recursion.
- A table of callbacks or a registration list goes right after the last function it points to.
- Function-like macros and `static_assert`s stay next to what they belong to.
- Headers follow the same order. A `constexpr` body stays right after its declaration.

### Class bodies

- Use one access block per class: `public`, then `protected`, then `private`. Do not repeat a specifier.
- Statics come first: the constants and the static methods.
- After them the constructor and destructor (plus `operator=`), the non-static methods, then the fields.
- A static member comes before a non-static one. A field comes after every method.

## Comments

- Use plain C++ comments (except for include guards).
- Comment sparingly. Explain non-obvious logic, or vent. Read existing comments for the tone.
- Mark empty function or method bodies with `// empty`.

Example 1:

```cpp
explicit Exception(std::string_view what, std::source_location location = std::source_location::current())
{
  // empty
}
```

Example 2:

```cpp
// NOTE: SDL seems to defer handle release until the GPU
// is truly done with the object, so it's probably safe
// to even release the old handle mid-frame
reset();
```

## See also

- [Clang-format is good enough](https://untode.su/posts/2026-02-16-clang-format/)

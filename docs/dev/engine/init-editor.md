# Editor Setup

The repository works with any editor. Visual Studio Code gets a little extra: a script that configures one of the two C/C++ engines for the current machine.

## Which engine

Two extensions can give you completion, navigation, diagnostics and formatting:

|Engine|Extension|Notes|
|----|----|----|
|clangd|`llvm-vs-code-extensions.vscode-clangd`|Recommended. Reads the CMake compilation database, so it agrees with the build|
|IntelliSense|`ms-vscode.cpptools`|Uses a hand-written include list instead of the build flags|

Both engines cannot run at the same time on one file. The script turns one of them off and the other on.

## Running the script

|Shell|clangd|IntelliSense|
|----|----|----|
|Windows PowerShell|`.\setup_clangd.ps1`|`.\setup_intellisense.ps1`|
|Windows cmd.exe|`setup_clangd.bat`|`setup_intellisense.bat`|
|POSIX shell|`./setup_clangd.sh`|`./setup_intellisense.sh`|

Configure a preset once before you run the script in the clangd mode.

If the Windows execution policy blocks a PowerShell script, use the `.bat` wrapper or call the Python file directly: `py -3 scripts/setup_environment.py clangd`.

The script edits `.vscode/settings.json` in place. It owns the engine settings, so it overwrites them on every run. It also removes the settings that belong only to the other engine. It writes the common settings (format on save, rulers, the include guard extension) only when the key is missing. A value you change by hand then survives. Remove the key and run the script again to get the default back.

## The compilation database

clangd asks the build system what flags a file gets. CMake answers with `compile_commands.json`, but only for the Ninja and Makefile generators. The Visual Studio generator writes no database.

Each preset configures into its own directory, and clangd only looks in the source directories and their `build` subdirectory. The script therefore copies the database of one preset to `build/compile_commands.json`:

```sh
./setup_clangd.ps1 --preset windows-x64-debug
```

Without `--preset`, the script takes the most recently configured preset. The copy goes stale when you reconfigure a preset, so run the script again after a reconfigure.

Only the Ninja and Makefile generators write the database. If no preset has a Ninja configuration, configure a separate directory:

```sh
cmake -S . -B build/clangd -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
```

## Tracked files

|File|Tracked|Contents|
|----|----|----|
|`.clangd`|Yes|Project-wide clangd switches: the module precompiled header is not reported as an unused include, and headers parse as C++23|
|`.vscode/settings.json`|No|Engine choice and editor defaults for this machine|
|`.vscode/c_cpp_properties.json`|Yes|Include paths for the Microsoft engine, all relative to the workspace|
|`.vscode/launch.json`|Yes|Debug configurations for both platforms|
|`build/compile_commands.json`|No|Copy of the database of one preset|

`.clangd` is the only editor file that changes behaviour for everybody. Edit it there, not in the generated files.

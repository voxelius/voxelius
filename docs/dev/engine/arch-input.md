# Input

The client reads input from SDL events. Every event goes through `globals::dispatcher`. The input subsystems connect to the event types that they need.

## Sources and bindings

`client/input/source` defines the input vocabulary. A `ButtonSource` is a device, a code and an edge: down or up. An `AxisSource` is a device and a code. The source also parses and formats names for keys, mouse buttons, the mouse wheel and gamepad inputs.

`client/input/buttons` keeps a map from `ButtonSource` to a command string. A keyboard, mouse or mouse wheel event looks up the source and runs `cvar::exec`. The console commands change this map:

- `kbutton <button> <down|up> <command>` binds a keyboard key.
- `mbutton <button> <down|up> <command>` binds a mouse button.
- `mwheel <direction> <down|up> <command>` binds a mouse wheel direction.
- `gbutton <button> <down|up> <command>` binds a gamepad button.
- `list_buttons` prints every binding.
- `unbind_buttons` removes every binding.

The `buttons` subsystem ignores events while a settings control captures a key, while a GUI screen is open, or while ImGui wants the keyboard.

The `screenshot` command is the one binding that does not follow this rule. The client checks the keyboard binds of that command before the rest of the input path, so the key still works while a screen is open and during the startup screen. A settings control that captures a key keeps priority. The [rendering](arch-render.md) page describes the capture itself.

`client/input/axes` keeps a map from `AxisSource` to a named slot. Mouse motion adds its delta to the slot. Game code reads and removes the value with `axes::consume`. The commands `maxis`, `gaxis`, `list_axes` and `unbind_axes` manage the map.

## Defaults and persistence

The client sets up its default bindings during the input subsystem initialization. These defaults are applied before the configuration files are read. The files can replace or remove them.

The client reads `client.conf` and then `client.user.conf`. Each file is a cvar command script. The binding and axis maps register write hooks, so `cvar::save_values` writes the current bindings as `kbutton`, `mbutton`, `mwheel`, `gbutton` and `maxis` lines. The main settings are written as `name value` lines.

The server uses the same cvar host for `server.conf`. The server does not create input bindings.

## Command line and console

The console is a GUI screen with a log view and a command line. The grave key toggles it. Escape closes it. The console command line sends each submitted line to `cvar::exec`, so it accepts the same variables and commands as `client.conf`.

The console adds a message sink to the `msg` logging layer. The sink copies records into the log view. The `con_history` cvar sets the log capacity. The `con_filter_*` cvars hide levels. The sink reads atomic copies of the filter values, because logging can come from worker threads.

## Settings controls

`gui::ButtonBind` captures a key, mouse button or wheel direction and writes a `+action`/`-action` pair into the binding map. The Controls tab uses one `ButtonBind` for each action. The movement actions are `forward`, `back`, `moveleft`, `moveright`, `jump` and `sneak`. The block actions are `break` and `place`. `bind_command` puts the control into a command mode instead: it binds one edge of the captured button to one command, so the screenshot row binds `screenshot` and nothing else.

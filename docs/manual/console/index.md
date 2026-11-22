# Console

The console is a screen with a log view and a command line on top of the game. The grave key toggles it, Escape closes it. Every line you submit goes to `cvar::exec`, so the console accepts the same variables and commands as `client.conf` and `server.conf`.

A variable holds a setting. Type its name alone to print the current value, or its name and a new value to change it:

```
r_vsync 0
```

A command runs an action. Some commands take arguments:

```
kick player
```

The client and the server each keep their own set. A client variable changes what you see and touch, and the client saves it to `client.conf`. A server variable changes the session for everyone, and the server reads it from `server.conf`. A command that carries the `recall` flag can be typed in the client console. The client forwards it to the server, and the server decides whether to run it.

## Pages

- [Variables](variables.md) - every setting that the client and the server expose
- [Commands](commands.md) - every action, from console housekeeping to operator commands

The build generates both pages from the sources, so they leave out hidden and debug entries. Run `list_vars` or `list_cmds` in the console to see everything the running side has.

## Flags

A flag narrows down who may use an entry and when.

| Flag | Meaning |
|---|---|
| `archive` | the engine saves the value back to the config file |
| `replicated` | the server sends its value to clients, and a connected client cannot change it |
| `admin` | only an operator may change it from a remote client. The engine trusts a context without a client, such as `server.conf` or the local console |
| `recall` | a client may forward the command to the server |
| `readonly` | the value cannot be changed |

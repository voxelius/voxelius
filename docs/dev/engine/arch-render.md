# Rendering

The client renders through OpenGL 3.3 core profile, with no optional extensions required. SDL3 owns the window and the input events. The `video` subsystem sets up both. Only the client links against this code. The server has no renderer.

## Window and video mode

The `video` subsystem creates the window and resolves the video mode. At startup, it picks a display: the primary display, or the one the `-monitor` command-line option names. It also lists the fullscreen modes that display supports. The engine stores the chosen mode in the `r_mode` cvar, so it persists between runs. The mode is either windowed, or a specific fullscreen resolution and rate. Vertical sync is `r_vsync`, applied through the window's swap interval.

The `head` subsystem creates the actual OpenGL context on top of the window `video` made. This split exists for a reason: the context, and everything that depends on it (shaders, framebuffers, ImGui), can be destroyed and rebuilt without recreating the window itself.

## Startup screen

The client draws a boot screen from its first frame, while the rest of the startup runs. `client/boot.cc` owns the screen, and `client/main.cc` registers the startup steps in order as small lambdas. The main loop runs at least one stage per frame and stops after a six millisecond budget, so the window still handles input, resize and close. Every stage reports progress through its weight and may report a part of itself with `boot::set_progress`.

The screen is a black background, a centered texture with its alpha channel, the current stage text, a progress bar, and the last five `msg` records in the bottom left corner. A texture loads with nearest filtering and an integer scale, so its pixels stay sharp. The textures are a fixed table in the client code, with an optional month and day range per entry; `-bootsplash <name>` forces one entry and `-noboot` hides the screen. `-bootdelay <ms>` waits that long after each stage, and `-bootstep` waits for a key press instead. Use them to read the stage log. The video stage does not finish until SDL reports that the window mode change took effect, so the menu does not appear during the switch. It waits five seconds after the report to let the monitor resynchronize. A five second timeout covers a platform that reports nothing.

When the last stage finishes, the screen stays for 1.2 seconds and then fades out. A key, a mouse button or the wheel skips this wait, and only then: the startup itself cannot be skipped. The main menu is built by the last stage, so it is ready when the screen goes away. Input does not reach the interface during the boot, except for the window events.

## Shader programs and variants

A `ShaderProgram` loads a vertex and a fragment source file, named by an [identifier](arch-core.md), and compiles them into one OpenGL program. Each source file can declare numbered variant macros with a `#pragma variant N NAME` line. The shader compiler turns each declared variant into a `#define`, with the value set at runtime.

A `ShaderProgram` only recompiles when a variant value actually changes. This lazy update avoids recompiling shaders every frame for state that changes rarely, such as the active fog model.

## The frame pipeline

The client renders each frame at an internal, pixel-scaled resolution, then upscales the result to the window size. The pixel scale is the `r_pixel_size` cvar. It gives the game its blocky, low-resolution look, regardless of the window's real size.

A frame renders into four separate layers, each with its own color and depth target:

- **Diffuse** - opaque world geometry and the game's own UI elements (crosshair, block outlines). This layer writes both color and depth.
- **Clouds** - the cloud decks, meshed as voxel volumes that borrow the diffuse depth so the ground hides them.
- **Alpha** - translucent world geometry, such as leaves and glass.
- **Fluid** - fluid surfaces, such as water and lava.

The cloud, alpha and fluid layers each reuse the diffuse layer's depth buffer for depth testing. The client copies that depth buffer in before each layer renders. This way, translucent and fluid geometry test correctly against opaque geometry. Each layer still writes into its own separate color and depth targets, instead of overwriting the diffuse layer.

A composite shader then draws one full-screen triangle. It reads every layer's color texture, sorts the translucent and fluid layers that touched a pixel by depth and blends them from the farthest to the nearest, then lays the cloud layer over the result. The client blits that image onto the real window-sized backbuffer. It uses nearest-neighbor filtering to keep the pixelated look. The compose target is 8-bit, so the shader adds half a level of noise to its output: without it a slow fade, such as the cloud deck thinning into the fog, lands on the 255 available levels and breaks into bands, and because each channel rounds at its own step those bands pick up a green or magenta tint instead of reading as a brightness ramp.

ImGui renders last, straight onto the default framebuffer, so its UI always draws on top of the game world. The client then presents the frame with a window buffer swap.

## Sky, fog and the day cycle

The sky is a full-screen pass drawn into the diffuse layer before the opaque world, so terrain simply overwrites it. It draws a horizon-to-zenith gradient from the camera's basis vectors and darkens towards a ground colour below the horizon. A cheap Rayleigh model reddens the sunlight as the sun sinks: the air mass grows towards the horizon, blue is scattered away first, and the horizon warms with it while the zenith keeps its blue. A brightened band around the sun adds the forward-scattering glow. After the gradient the client draws the star field, the moon and the sun, all as camera-relative geometry on the same sky sphere, so the ground and the clouds still hide them.

The sun and the moon are square textured quads, like the sprites the game is inspired by. The textures live in `data/builtin/textures/environment`, and `misc/generate-celestial.py` regenerates the temporary sprites. The sun is one square image: a bright core on an opaque black square with a radial glow, drawn with additive blending so only the glow reaches the sky. The moon is a four-by-two atlas of eight square phase cells, selected by `day % 8` from the timings packet; its unlit side is opaque black and its halo is transparent, and it is drawn alpha blended. They write no depth, so the cloud layer still composites over the sky and the ground still hides it.

The sun's path is a circle in the east-west plane by default: it rises due east and sets due west. `r_sun_angle` tilts that whole circle, and the same rotation is applied to the stars, so a non-zero angle moves the sky as one piece. The cvar is client-side and defaults to `0`.

The colours come from the world clock. The server owns each world's `daylight_tick`, derives the whole-day counter from it and sends both with the timings, while the day length reaches the client as the replicated `sv_day_length` variable; the client advances the phase on its own clock and only corrects towards the server, so a 20 Hz packet stream does not step the sun. A packet that moves the clock by a second or more, such as after `time_set` or a world switch, snaps instead of easing. Weather contributes `sky_visibility`, a blended weight from `0` to `1`: at `0` the sky is just the type's `sky_tint`, at `1` the clear Rayleigh gradient stands, and the sun, moon and stars fade with the same weight. The weather field itself is sampled from the world's seed and weather tick, so the client needs no weather stream to tint the sky.

The warm horizon is directional: the shader mixes a neutral horizon into a sun-coloured one by the horizontal angle to the sun, so the orange stays on the sun's side and the opposite horizon keeps its cooler colour. The fog takes the same mix in the direction the camera faces. The moon is tinted with the same reddened sunlight while the twilight factor is high, and the cloud shader receives the tint as a uniform, so a sunset colours the deck without rebuilding its mesh.

The diffuse layer is cleared with the fog colour, and the fog takes the horizon colour of the same sky, so the skyline and the haze always match. Under a fluid the fog switches to the fluid's own colour. Block lighting uses the same day factor, so dusk dims the world and the sky together. See the [weather API](../modding/api-weather.md) for the tint each type carries.

## Clouds and precipitation

The cloud code is split by role: `cloud_field` samples the weather field and hands back columns, `cloud_mesher` turns one field snapshot into a vertex and index buffer, and `cloud_renderer` owns the GL objects and decides when the deck is stale.

The cloud deck comes from the weather field of the world. The client walks the noise field of every active cloud body on the grid of the ring it is meshing and turns each cell that survives the density check into a column of whole voxels, one or more of them. The deck is meshed in rings that tile the range: an 8-block cell out to 32 chunks, a 32-block cell out to 96, and a 128-block cell out to 256. Each ring uses four times the cell of the one inside it, so the column count grows with the range instead of its square, and a ring overlaps the finer one by a cell so the seam never opens. Each ring also fades out over its last cells into the coarser ring, which is meshed first and covers the same band, so the change of voxel size never opens a strip of sky. A column stays inside its own body's slab and only steps up into it, so two bodies never fight over the same plane. The client meshes only the faces the neighbours leave exposed, into one vertex and index buffer, and a deck goes out as one `glDrawElements`. A build runs on the task pool from a copy of the field state and the drift anchor it belongs to; the previous deck keeps drawing until the new one lands, and only the main thread touches the buffers. A ring keeps its own cell size whatever `r_cloud_range` says, so a longer range extends the deck outwards instead of growing the blocks under the player. The field is sampled in the frame of the anchor, so the cubes keep their shape while the front travels and a rebuild only changes the cells that slid in at the window's edge; the drawn wind offset is the difference between the current field drift and that anchor.

The mesher bakes the face albedo and a flat normal into the vertices: bright on top, dark underneath and shaded per direction on the sides, and each column carries its own few percent of brightness variation, so the deck reads as blocks rather than one flat sheet. The vertex shader lights each face from the sun and the sky, so a moving sun needs no rebuild, and mixes the face towards the fog colour with distance, so a deck seen through a storm reads lighter than the same deck overhead. The deck dissolves into the sky at its own edge; the haze range follows the deck's radius rather than the terrain fog, which would wash a deck that hangs a hundred blocks up out before it is ever seen. The deck is opaque where the field is dense and writes depth, so there is no overdraw to pay for and no blending order to get wrong; only its dissolving edge is translucent. The deck is drawn with near and far planes of its own, so the world keeps the depth range of its view distance, and the composite compares the two distances to hide the deck behind the ground. The cloud layer reaches the composite as one more depth-sorted layer, so water and translucent blocks in front of the deck hide it. See the [weather API](../modding/api-weather.md) for the data side.

There is no precipitation pass yet: a weather type can declare rain or snow, and the fields travel with the mod data, but nothing draws them. The next pass over weather adds the local precipitation buffer the API describes. Precipitation already affects the climate queries, and the storm tint and the local temperature and humidity are driven by the same field.

## Outlines

> **TODO:** add screenshots once the outline shapes are finalized.

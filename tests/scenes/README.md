# Scene harness

`ragexe_scenes` is the client with a few files swapped, so a Lua script can put it into any state
without a server or a login and capture the frame as a PNG. It builds on Linux, macOS and Windows:

| replaced                    | by                   | what changes                                                     |
|-----------------------------|----------------------|------------------------------------------------------------------|
| `src/platform/clock_*.c`    | `scene_clock.c`      | time moves one frame (16 ms) per shown frame; the date is fixed; only the stopwatch that times the [counters](#counters) runs in real time |
| `src/platform/net_*.c`      | `scene_net.c`        | sockets stay in the process; the driver plays the map server     |
| `src/platform/thread_*.c`   | `scene_thread.c`     | no client threads: models load in order on the main thread       |
| `src/platform/settings_*.c` | `scene_settings.c`   | an empty settings store in memory: the registry and `client_settings.ini` are neither read nor written |
| `src/core/drive_state.c`    | `scene_driver.c`     | runs the scene script, answers the map server packets, captures  |
| (C library `rand`, POSIX)   | `scene_rand.c`       | the Microsoft runtime's generator, the one the Windows client uses |
| `src/core/app_sdl.c`        | `scene_app_hidden.c` | the window is never shown, so the GPU device makes no swapchain for it and a run never takes the focus or the mouse or covers the screen |
| `src/platform/window_sdl.c` (Windows) | `scene_window_win.c` | a message box is written to stdout instead of waiting for a click |
| `src/ui/misc/crash_dialog.c` (Windows) | `scene_crash_win.c` | a crash report goes to stderr and the run ends, instead of a dialog waiting for a click |

Everything else is the shipped client, renderer included. With the same game data a scene
produces the same pixels every run, and the same random effects on every platform.

## Running

Linux and macOS: build with `-DRAGE_CLIENT=ON` (the target is built with `ragexe`). Windows: run
`build.bat`, then `tests\scenes\build.bat`, which links `build\ragexe_scenes.exe` from the same
objects. Then run every script in `scripts/` from the game directory. The Granny models of
`granny_models`, `granny_actions`, `granny_dead` and `guild_flags` come from the glTF files that
[`tools/granny_export`](../../tools/granny_export/README.md) writes into the game directory, on
every platform; without them those scenes show message boxes and no models.

```sh
python3 tests/scenes/run.py --exe build/ragexe_scenes --game ~/ro --out /tmp/scenes
python3 tests/scenes/run.py ... --golden ~/scene-golden --update   # accept the captures
python3 tests/scenes/run.py ... --golden ~/scene-golden            # compare with them
python3 tests/scenes/run.py ... prontera_crowd                     # one scene
```

`run.py` writes the captures with their [counters](#counters), one log per scene, a difference
image per changed capture and `report.html` (golden, capture and difference side by side) into
`--out`. It exits with 1 when a scene fails or a capture differs; `--tolerance` and
`--max-pixels` loosen the comparison.
Golden images belong to one data set and one renderer, so they live outside the repository.
With `-DRAGE_SCENE_GAME_DIR=<game dir>` (and optionally `-DRAGE_SCENE_GOLDEN=<dir>`) the same run
is the `scenes` ctest.

One scene by hand:

```sh
cd <game dir>
RAGEXE_SCENE=<repo>/tests/scenes/scripts/prontera_crowd.lua RAGEXE_SCENE_OUT=/tmp/out <build>/ragexe_scenes
```

The client's window is never shown, on any platform, and the GPU device claims a window, and makes
a swapchain for it, only once it is shown, so a scene run has no swapchain: its frames are drawn
and each capture downloads the frame just shown (`gfx_read_frame`). SDL's GPU drivers need no
window, only a video driver that can make what a window would be presented through: Vulkan
surfaces for its Vulkan driver (the offscreen driver's too), Metal views for its Metal driver
(Cocoa's and UIKit's). On Linux the harness defaults to `SDL_VIDEODRIVER=offscreen`, so that nothing
reaches the desktop and a build without X11 and Wayland runs it; on Windows and macOS it keeps the
platform's driver (on macOS the one SDL's Metal driver runs with), with the window hidden. A Mac
has not run it yet. Captures are silent on every platform: `run.py` gives the client
`SDL_AUDIO_DRIVER=dummy` unless `SDL_AUDIO_DRIVER` or `SDL_AUDIODRIVER` is set, and the harness
itself sets `SDL_AUDIODRIVER=dummy` when it is not. Every platform gets `RAGEXE_VSYNC=0`.
`RAGEXE_SCENE_TRACE=1` logs the packets the client sends and the mode changes.
`RAGEXE_SCENE_SHADOW_FIT=1` (`run.py --shadow-fit`) checks every capture's shadow fit, which the
GPU works out, against `src/render/shadow_fit.c` given the same input: it writes
`<out>/<name>.shadow_fit.json` (the receivers, the cascades, the range of depths the slices were cut
from, each square's half size, the error) and fails the scene where the two differ by more than the
rounding the C allows for.

The game directory's `SaveData` (resolution, options, chat windows) and data files are read as
usual; settings and account settings start empty and are never written.

## Same frames in every run

Two runs of a scene with the same executable and game data must give the same frames: the same
captures to the pixel and the same [counts](#counters) in every frame. Where they do not, something
depends on where memory lies or on what memory nothing wrote holds: an order kept by addresses, or a
read of a variable, a field or a stack slot never set. Such a difference shows only now and then,
since the heap's layout and the leftovers on the stack change little from run to run.

`RAGEXE_SCENE_LAYOUT=<n>` (n above 0) makes the harness lay memory out otherwise, the same way in
every run with that n: before the client starts it takes 1,024 heap blocks of sizes drawn from n,
fills them with a byte made from n and gives every other one back, so that the client's blocks lie
elsewhere and the memory it is given again holds that byte; and at every message pump it fills the
64 KB of stack below the driver with the byte. `layouts.py` runs each scene with n = 1 and with
n = 2 and fails where any capture, a capture's counts or a measured frame's counts differ (the times
left out):

```sh
python3 tests/scenes/layouts.py --exe build/ragexe_scenes --game ~/ro --out /tmp/layouts      # arrive_prontera, town_prontera
python3 tests/scenes/layouts.py ... woe_emperium                                              # other load scenes
python3 tests/scenes/layouts.py ... --scripts tests/scenes/scripts prontera_crowd              # regular scenes
```

By default it runs the load scenes `arrive_prontera` and `town_prontera` at `RAGEXE_MSAA=1`, half a
minute in all, which draw hundreds of players in every frame. What a stray read finds can be left
over from the client's own work rather than the harness's byte, so such a bug need not show in every
pair of runs: one pass is no proof, while a failure always is one. With
`-DRAGE_SCENE_GAME_DIR=<game dir>` the default run is the `scene_layouts` ctest.

## Comparing platforms

Capture on one platform with `--golden <dir> --update`, copy the directory, and run the other
with `--golden <dir> --tolerance 3`. The renderer multisamples by default (4x MSAA with alpha to
coverage), which smooths edges a GPU of another make places a little otherwise, so set
`RAGEXE_MSAA=1` on both for captures to compare. What is left between the Windows and the Linux
captures of these scenes:

- text: FreeType draws the same text on both when the same font files are installed on both;
  otherwise each draws what it finds (a Windows without Gulim takes Malgun Gothic, as GDI did;
  Linux substitutes Noto Sans CJK KR or NanumGothic at Gulim's size). With `RAGE_FONT_DIRS` set to
  a copy of the Linux machine's font folders, the Windows client draws the same text but for a few
  pixels. `hangul_text` draws Hangul, symbols and kana, which the interface's face lacks, in the
  scripts' fallback fonts;
- single pixels along triangle edges, where two GPUs' subpixel precision decide a pixel centre
  differently, and colour rounding of one or two steps in blending.

## Writing a scene

A scene script runs as a coroutine, one step per client frame. It starts on the login screen.

```lua
scene.enter_map("prontera", 156, 186, {name = "Watcher", job = 4008, hair = 5})
scene.wait_ready()                       -- the map and its models are loaded and stable
scene.spawn{aid = 110000001, kind = "monster", job = 1002, x = 150, y = 192}
scene.say(110000001, "Poring : hello")
scene.wait(30)
scene.capture("poring_hello")            -- <out>/poring_hello.png
```

| call | does |
|------|------|
| `enter_map(map, x, y, opts)` | from the login screen straight into a map. `opts`: `name, job, sex, hair, hair_color, body_palette, level, job_level, hp, sp, weapon, shield, head_top, head_mid, head_bottom, robe, dir`, and `distance, latitude`, the free camera's in place of the saved options' |
| `spawn{...}` | an actor appears (`ZC_NOTIFY_STANDENTRY`): `aid, kind` (`"monster"`, `"npc"`, `"player"`, `"pet"`, `"homunculus"`, `"mercenary"`), `job, x, y, dir, name, hair, hair_color, body_palette, head_dir, weapon, shield, head_top, head_mid, head_bottom, robe, body, sex, level, hp, max_hp, speed, state, option, boss, guild, emblem` (`option` is the option bits: carts, Peco Pecos, falcons, dragons, mado gears; `boss` is the entry's boss flag, 1 for MVPs and the Emperium; guild id and emblem version: when the client asks for the emblem, the fake map server sends a generated 24×24 one in the guild's colour) |
| `vanish(aid[, how])` | the actor leaves |
| `say(aid, "Name : text")` | chat over an actor |
| `chat(text)` | a line in the player's own chat |
| `send(bytes)` | any packet, as the map server would send it |
| `wait([frames])`, `wait_ready([timeout])` | let frames pass; wait until the map is loaded |
| `mouse(x, y)`, `button(x, y, down[, "right"])`, `click(x, y[, "right"])`, `wheel(delta)` | the mouse, through `core/user_input.h` as every platform's input goes; the wheel turns by `delta` (120 a notch, positive away from the player) |
| `key(vk[, scan])`, `key_event(vk, scan, down)`, `alt_key(vk[, scan])` | keys, through `core/user_input.h`; a key pressed while the script holds Alt (`0x12`) down comes with Alt, as `alt_key` presses it; `scene.VK` names a few virtual keys |
| `text(s)` | characters, typed as a keyboard types them: each letter, digit or space with its key down before it and up after, since the window manager decides on the key whether characters go to the chat line |
| `open(id)` | opens a UI window by its window id |
| `resize(w, h)` | the windowed client area becomes `w`×`h` pixels, as when the player drags the window's frame, no smaller than 1024×664; the game follows on the next frame |
| `capture(name[, {w, h, x, y, jpeg}])` | writes `<out>/<name>.png`: the frame just shown, downloaded from the GPU, or a `w`×`h` part of it at `x, y` (default centred); with `jpeg = quality`, `<out>/<name>.jpg`. Beside it, `<out>/<name>.counters.json` (see [Counters](#counters)). An error while a measure runs |
| `measure(frames, name)`, `measuring()` | the [counters](#counters) of each of the next `frames` frames drawn and shown, a line of JSON each, in `<out>/<name>.frames.jsonl`; the script goes on at once. Until the last of them is shown, a capture or another measure is an error, since reading the frame back stalls the GPU. `measuring()` answers the frames still to come. A run that ends before the last one fails |
| `effects()` | the IDs of the effects attached to the player, and the number of objects in the world's object list |
| `frame()`, `mode()`, `ticks()` | the frame number; `"login"` or `"game"`; the client's clock in milliseconds, which the times in the map server's packets (a walk's start) are on |
| `dir` | the script's directory, with its separator, for the files it loads (`dofile(scene.dir .. "lib/load.lua")`) |
| `shadow_fit()` | the shadows of the frame just shown as the GPU fitted them, or nil without shadows: `cascades` (4, or 0 when nothing in view receives one), `nearest` and `farthest` (the range of view depths the slices were cut from, kept from frame to frame), and by cascade `radius` (its square's half size), `kept` (the half size it remembers for the next frame), `far` (its far view depth) and `x`, `y` (its centre across the light) |
| `shadow_fit_check(name)` | checks the shadow fit of the frame just shown against `src/render/shadow_fit.c`, as a capture does with `RAGEXE_SCENE_SHADOW_FIT=1`: writes `<out>/<name>.shadow_fit.json`, fails the scene where they differ, and answers the mismatches, or nil without shadows |
| `quake([ms])` | the camera shakes as a skill's earthquake shakes it: for 650 ms, or for `ms` where that is more (1: 200 ms) |
| `fail(message)` | marks the scene failed and goes on |

A script error, a map that does not finish loading, or `fail` ends the run with exit status 1.
On Windows a message box the client would show (`error_msg`, a Lua error) is written to stdout
as `scene: message box "<caption>": <text>` and the run goes on.

The fake map server answers what a scene needs: the map entry (`ZC_AID`, `ZC_ACCEPT_ENTER2`), the
client's notice that the map is loaded (`ZC_NOTIFY_ACTORINIT`, without which the client keeps the
hotkeys off), guild emblems, and the player's own chat line, which it says back as rAthena does.
`keyboard.lua` opens the inventory with Alt+E and sends a line from the chat line.

A script whose name starts with `_` is a helper that other scripts load (`dofile`, with the
directory of `debug.getinfo(1, "S").source`), not a scene: `run.py` skips it.

## Motion

The scripts in `motion/` move the camera for hundreds of frames each and log every frame's shadow
fit (`_motion.lua`: a line `fit <motion> <frame> <cascades> <nearest> <farthest>` and per cascade
its square's half size, centre and far depth): walking on Prontera's main street and around its
fountain past the flower beds and banners, zooming in to the camera's closest and tilting it to
its flattest and walking so, turning, zooming, earthquakes (`scene.quake`) on Prontera's street, at
its south gate and on the hills of `manuk` and `hu_fild01`, and turning, zooming and walking on
those hills, each from the camera given in the script. They take minutes, so they are not among
`scripts/`:

```sh
python3 tests/scenes/run.py --exe build/ragexe_scenes --game ~/ro --out /tmp/motion --scripts tests/scenes/motion --shadow-fit
python3 tests/scenes/motion_stats.py /tmp/motion                 # how steady the shadows stay
python3 tests/scenes/motion_stats.py /tmp/motion /tmp/motion-old  # against another build's run
```

`motion_stats.py` counts per motion how often a cascade's square changes size per 100 frames, how
far shadow edges can jump at those changes (a texel of the larger square), each cascade's mean
texel, and how far off whole texels a square moved while it kept its size; given two runs it fails
where the first changes size more often on any motion. With `--shadow-fit` every 25th frame's fit
and each script's closing capture are checked against `src/render/shadow_fit.c`.

## Counters

Every capture writes `<out>/<name>.counters.json` beside the image: the renderer's counters
(`src/diag/perf.h`, which says exactly what each counts) for the frame the capture shows, from the
moment the frame before it was shown to the moment it was. The file is one JSON object of numbers,
and each line of a measure's `<name>.frames.jsonl` is one too, of a frame of its own:

| key | counts |
|-----|--------|
| `frame` | the frame number the capture's log line gives |
| `frame_ms` | the client's whole frame: real time from the moment the frame before it was shown to the moment it was, less the harness's own work: the script's steps (`script_ms`) and writing the counters of the frame before |
| `script_ms` | the scene script's steps in the frame: its Lua, the packets it builds and hands to the client (`scene.send`, `scene.spawn`, ...) and what it reads back from the GPU: a capture's download and the shadow fit's (`shadow_fit`, `shadow_fit_check`, a capture with `RAGEXE_SCENE_SHADOW_FIT=1`); a real client has none of it, so `frame_ms` leaves it out |
| `objects` | the objects in the world's object list: actors, the effects and damage numbers attached to them, ground skills |
| `draws` | draws the device made, after batching, the depth pass and the shadow maps included |
| `indirect_draws` | those of them whose primitives the GPU's cull chooses: a run of the static world's records in the main pass, the depth pass or a shadow cascade |
| `primitives` | the primitives of the other draws, which the CPU knows |
| `draw_calls` | `gfx_draw_quad`, `gfx_draw_vertices` and `gfx_draw_skin` calls and the static world's draw calls (`gfx_world_draw`, `gfx_world_draw_record`, `gfx_world_draw_water`) |
| `modes_sent`, `modes_skipped` | `gfx_set_mode`, `gfx_set_effect` and `gfx_set_fog` calls that changed what the device draws with, and calls that left it as it was |
| `texture_binds` | `gfx_set_texture` calls that changed a stage's texture |
| `vertex_bytes` | a static world mesh's vertices and indices when it is uploaded, the water's cells drawn, and a Granny model's vertices and indices when a draw first takes it up |
| `instance_bytes` | the records of the quads and triangles drawn, their offsets, the matrices they are projected with and the corner sets and client's vertices they name (render/gfx.h, gfx_draw_quad and gfx_draw_vertices), and Granny's actors' poses with the headers and offsets of their draws' instances (gfx_skin_pose, gfx_draw_skin) |
| `texture_bytes` | texels written into textures and surfaces (texture builds, surface uploads, the sprite atlas) |
| `draw_scene_ms`, `view_render_ms` | time spent in `renderer_draw_scene` and in `view_on_render` |
| `present_ms` | time spent presenting the frame (`c3d_device_show_frame`), where the device waits for its render thread to finish the frame before (which waits while the GPU is two frames behind that one), sends the static world's new meshes and textures up, shows the frame before and hands the frame's recorded draws to the render thread |
| `packets_ms` | handling the map server's packets (`game_mode_poll_network_msg`): actors appearing and moving, skills launching their effects |
| `effects_ms`, `actors_ms` | stepping the effect manager's effects (`effect_mgr_process`) and the world's objects (`world_process_actors`: actors walking and animating, the effects and damage numbers attached to them) |
| `objects_render_ms`, `effects_render_ms` | the parts of `view_render_ms` that build the world's objects' quads (`game_object_render`: sprites, shadows, names, damage numbers, Granny models' bones) and the effect manager's (`effect_mgr_render`: STR and RagEffect primitives, particles) |
| `ui_ms` | drawing the windows (`ui_window_mgr_render`, `ui_window_mgr_render_windows`): shop signs, chat rooms and chat balloons among them |
| `atlas_ms` | decoding sprite frames from their palette runs into the sprite atlas (`texture_draw_sprite_frame`) |
| `texels_ms` | widening the client's texels to the GPU's B8G8R8A8 for an upload (`render/gfx_gpu.c`) |

Some times lie within others, so they do not add up to the frame:

- `frame_ms` holds every other time but `script_ms`. `packets_ms`, `effects_ms`, `actors_ms`,
  `view_render_ms`, `ui_ms`, `draw_scene_ms` and `present_ms` do not overlap; what the frame does
  besides them (the input, the camera's step, the weather's particles, the minimap, the mouse
  cursor) has no time of its own.
- `objects_render_ms` and `effects_render_ms` lie within `view_render_ms`.
- `atlas_ms` lies within the time of whatever draws the sprite: `objects_render_ms` for the world's
  objects, `effects_render_ms` for sprite particles, `ui_ms` for windows, none of the others for the
  mouse cursor and what it drags.
- `texels_ms` lies within the time of whatever sends a texture's new texels to the device:
  `draw_scene_ms` for the renderer's lists, `ui_ms` for a window drawn into a texture of its own,
  `present_ms` for the static world's textures, which go up with the frame.
- The fake map server's answers to the client's requests (a guild emblem) are made while the client
  sends them and count as the client's; they take microseconds.

The counts are the same in every run. The times are real time (`platform_stopwatch_ns`, the one
clock the harness leaves running), taken from that one frame and not averaged, and differ by about a
fifth from run to run, so compare several runs; `present_ms` also grows when another program keeps
the GPU busy. Scene runs load models on the main thread, so the textures a frame loads count
in that frame. The harness starts the counters again with every frame it shows, so the overlay
(`RAGEXE_OVERLAY`) shows nothing useful in a scene run.

## Load scenes

The scenes in `load/` stage what players see on busy servers, to measure the client under real load
rather than to compare pictures: dozens to two hundred players with real jobs, equipment, mounts,
carts, guild emblems and chat, and the skills their fights are made of, sent as rAthena sends them
for this client's packet version. Every scene moves: actors walk (`ZC_NOTIFY_MOVE`), the camera
turns (the right button dragged) and zooms (the wheel) as a player's does, and skills keep firing
for the whole measured stretch. A run of one takes up to half a minute, and they are measured
several runs each, so they live outside `scripts/` and neither `run.py` nor the `scenes` ctest
runs them.

| scene | map | what it stages |
|-------|-----|----------------|
| `woe_emperium` | `prtg_cas01` | the War of Emperium in the Emperium room: 113 players of three guilds (36 defending, 77 attacking) (`lib/woe.lua`), the Emperium and its guardians (Granny models) hit all the time and breaking every 400 frames, Storm Gust, Lord of Vermilion, Meteor Storm, Ice Wall, Sanctuary, Magnus Exorcismus, Pneuma, Safety Wall, Land Protector, songs' and dances' cells, Asura Strike, players dying and standing up again |
| `woe_choke` | `prtg_cas01` | the same war at the last choke: the defenders hold the portal to the Emperium room |
| `mvp_baphomet`, `mvp_randgris` | `prt_maze03`, `odin_tem03` | an MVP and its slaves against a party of 19 or 23 (`lib/mvp.lua`): the boss's skills, Earthquake every 150 frames among them, heals and buffs going up, players dying and resurrected, the fight going on through the whole measured stretch; right after it the boss dies and the MVP is announced, which the last capture shows; a first capture `<scene>_quake` is taken while an Earthquake shakes the camera |
| `bombers_ifrit` | `thor_v03` | 30 creators and geneticists casting Acid Demonstration and Cart Cannon on Ifrit, a cast landing nearly every frame, Ifrit's Earthquakes on all of them; Ifrit dies after the measured stretch as the MVPs do |
| `aoe_orc_dungeon` | `orcsdun01` | the player, a high wizard, with a pack of 50 monsters: two Storm Gusts for every Lord of Vermilion, monsters freezing, dying, vanishing and walking in again |
| `town_prontera` | `prontera` | 180 players and 20 pets on the main street (`lib/town.lua`): 60 vending shops with their signs, 15 chat rooms, people walking past, talking and pulling faces |
| `arrive_prontera`, `arrive_castle` | `prontera`, `prtg_cas01` | the player warps (`ZC_NPCACK_MAPMOVE`) into the town of `town_prontera` or the war of `woe_emperium`, which appears at once once the map has loaded, every sprite and emblem new; the measured stretch, 300 frames, starts with that frame, for the hitch of the first frames |
| `snow_lutie` | `xmas` | a Christmas crowd of 100 under Lutie's snow, the weather's particles over the crowd |

`load/lib/load.lua` has what the scenes share: the packets, players dressed by job (`load.CLASS`),
ground skills' cells in rAthena's shapes, a deterministic random source that leaves the client's
`rand()` alone, the fight that keeps who stands where and what follows later (`load.fight`), the
frame's size, the camera and `load.measured`, which runs a scene's warm-up, its measured stretch
(`scene.measure`) and its capture after it. Every scene makes the frame 1600×900 (`scene.resize`)
once its first map has loaded, whatever the game folder's options say, so that two machines measure
the same frame. `RAGEXE_LOAD_FRAMES` sets the measured frames of every scene (by default 600, the
arrive scenes 300) for a quick look. The scenes need this harness's calls (`scene.measure`,
`scene.measuring`, `scene.resize`, `scene.wheel`, `scene.ticks`, `scene.dir`, spawn's `option` and
`boss`).

`load.py` runs them several times, since the times differ by about a fifth from run to run, and
reports per scene the counts (objects, draws, instance, vertex and texture bytes) and, pooled over
the runs' measured frames, the median and the worst frame of every time of the
[counters](#counters) (with the number of the run and the frame of the stretch it was), the spread
of the runs' medians and the first measured frame. It prints the report and writes `summary.md` and
`summary.json` into `--out`, beside each run's captures, log, frames and `run.json`. A scene run
again replaces its earlier runs when every new run ends well; otherwise the earlier runs stay and
the new ones are kept in `<scene>.failed`. The report covers every scene `--out` has runs of
(`--summary-only` writes it without running anything):

```sh
python3 tests/scenes/load.py --exe build/ragexe_scenes --game ~/ro --out /tmp/load            # 3 runs, RAGEXE_MSAA=4
python3 tests/scenes/load.py ... --runs 5 woe_emperium town_prontera                          # some scenes
python3 tests/scenes/load.py ... --msaa 1 --runs 1 --out /tmp/load-pictures                   # pictures to compare
python3 tests/scenes/load.py --compare /tmp/load-old /tmp/load-new                            # two summaries
```

### Against the Direct3D 9 client

Phase 7 of `plans/gpu-renderer.md` compares these numbers with those of the Direct3D 9 client the
GPU renderer replaced: commit `02fc9e5e`, built with FreeType's text, which the GPU renderer's
client draws too. Its harness has none of the calls the load scenes need, so
`load/d3d9_baseline.patch` adds them to it as they are here, with the counters' times, and the
two crash fixes the scenes need on a 64-bit client (a broadcast's speaker narrowed to an int on its
way to the chat window, a bottle thrown at its own cell). Its `scene.resize` cannot resize the
Direct3D 9 device: it only checks that the frame is the size asked for, so run that client from a
game folder whose options (`SaveData/OptionInfo.lua`: `WIDTH`, `HEIGHT`) give 1600×900. On
Windows, beside this checkout:

```bat
git worktree add ..\d3d9-baseline 02fc9e5e
cd ..\d3d9-baseline
git apply <this checkout>\tests\scenes\load\d3d9_baseline.patch
build.bat freetype
tests\scenes\build.bat freetype
python <this checkout>\tests\scenes\load.py --exe build\ragexe_scenes_ft.exe --game <game dir> --out <dir>
```

Its counters are its device's: `states_sent` and `states_skipped` where the GPU renderer has
`modes_sent` and `modes_skipped`, `vertex_bytes` every vertex it is given, and no `instance_bytes`
or `texels_ms` (always 0): it draws from vertex buffers and keeps textures in their own 16- and
32-bit formats.

## Effect gallery

`tools/effect_gallery.py` renders every RagEffect ID and writes a static page, `index.html`, with
a card per ID: a filmstrip of frames 1, 8, 16, 32 and 64 after the effect starts (hover to play
it), its `data/effects/effects.json` row when the effect is data (`Str`, `Play`), a `code` tag
with the first statement of its case when `src/effects/rag/rag_effect_process*.c` has one, its
duration, and what the run saw: effects still on the player at the last frame, objects left over
after clearing, a message box (`Invalid Effect Id`, a missing file) or a crash. Search by ID,
range (`100-200`), hex (`0x1f3`) or text (a file or function name); filter data, Play, Str, code,
no case, or IDs with a crash, a message box or leftovers.

```sh
python tools/effect_gallery.py --exe build/ragexe_scenes --game <game dir> --out <dir>
python tools/effect_gallery.py ... --data           # only the IDs effects.json has
python tools/effect_gallery.py ... --ids 300-373    # some IDs
python tools/effect_gallery.py ... --resume         # only the IDs <dir> does not have yet
python tools/effect_gallery.py --out <dir> --page-only
```

By default it runs every ID `tests/test_effect_trace.c` runs (0 to 2384, 2385, 10000 and 60000),
which takes a few minutes and about 500 MB of JPEG captures. Captures and the page belong to one
data set, so `<dir>` stays outside the repository, like the golden images. Open
`<dir>/index.html` in a browser; it needs no server.

The scene is `gallery/effect_gallery.lua`, outside `scripts/`, so `run.py` and the `scenes` ctest
do not run it. On a Novice on `prt_fild08` it sends `ZC_NOTIFY_EFFECT2` for each ID, as the map
server does (`game_mode_on_notify_effect2`), captures a 448×384 crop around the player, then
clears every effect on the player with `ZC_CLEAR_EFFECT` and waits until the objects the effect
made are gone. The tool runs the client on 400 IDs at a time; when it crashes or stops writing
output for a minute, the ID is marked and the next run starts after it. Options for the frames,
crop, JPEG quality, map, job and settling time are in `--help`; the script reads them from
`RAGEXE_GALLERY_*` environment variables.

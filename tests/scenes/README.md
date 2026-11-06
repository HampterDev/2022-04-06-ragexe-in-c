# Scene harness

`ragexe_scenes` is the client with a few files swapped, so a Lua script can put it into any state
without a server or a login and capture the frame as a PNG. It builds on Linux, macOS and Windows:

| replaced                    | by                   | what changes                                                     |
|-----------------------------|----------------------|------------------------------------------------------------------|
| `src/platform/clock_*.c`    | `scene_clock.c`      | time moves one frame (16 ms) per shown frame; the date is fixed  |
| `src/platform/net_*.c`      | `scene_net.c`        | sockets stay in the process; the driver plays the map server     |
| `src/platform/thread_*.c`   | `scene_thread.c`     | no client threads: models load in order on the main thread       |
| `src/platform/settings_*.c` | `scene_settings.c`   | an empty settings store in memory: the registry and `client_settings.ini` are neither read nor written |
| `src/core/drive_state.c`    | `scene_driver.c`     | runs the scene script, answers the map server packets, captures  |
| (C library `rand`, POSIX)   | `scene_rand.c`       | the Microsoft runtime's generator, the one the Windows client uses |
| `src/platform/window_win32.c`     | `scene_input_win.c` | cursor and key state come from the script, never from the desktop |
| `granny2.dll` clock imports (Windows) | `scene_granny_win.c` | Granny models animate on the scene clock, as the POSIX runtime does |
| `src/ui/misc/crash_dialog.c` (Windows) | `scene_crash_win.c` | a crash report goes to stderr and the run ends, instead of a dialog waiting for a click |

Everything else is the shipped client, renderer included. With the same game data a scene
produces the same pixels every run, and the same random effects on every platform.

## Running

Linux and macOS: build with `-DRAGE_CLIENT=ON` (the target is built with `ragexe`). Windows: run
`build.bat`, then `tests\scenes\build.bat`, which links `build\ragexe_scenes.exe` from the same
objects (`build.bat freetype` and `tests\scenes\build.bat freetype`: `ragexe_scenes_ft.exe`, with
FreeType text as on Linux). Then run every script in `scripts/` from the game directory:

```sh
python3 tests/scenes/run.py --exe build/ragexe_scenes --game ~/ro --out /tmp/scenes
python3 tests/scenes/run.py ... --golden ~/scene-golden --update   # accept the captures
python3 tests/scenes/run.py ... --golden ~/scene-golden            # compare with them
python3 tests/scenes/run.py ... prontera_crowd                     # one scene
```

`run.py` writes the captures, one log per scene, a difference image per changed capture and
`report.html` (golden, capture and difference side by side) into `--out`. It exits with 1 when a
scene fails or a capture differs; `--tolerance` and `--max-pixels` loosen the comparison.
Golden images belong to one data set and one renderer, so they live outside the repository.
With `-DRAGE_SCENE_GAME_DIR=<game dir>` (and optionally `-DRAGE_SCENE_GOLDEN=<dir>`) the same run
is the `scenes` ctest.

One scene by hand:

```sh
cd <game dir>
RAGEXE_SCENE=<repo>/tests/scenes/scripts/prontera_crowd.lua RAGEXE_SCENE_OUT=/tmp/out <build>/ragexe_scenes
```

On Linux and macOS the harness defaults to `SDL_VIDEODRIVER=offscreen` (an EGL context, no
window) and `SDL_AUDIODRIVER=dummy`; set either to watch or hear a scene. SDL's offscreen driver
needs EGL, which macOS does not have, so on a Mac set `SDL_VIDEODRIVER=cocoa`. On Windows the
client window shows while the client starts and is hidden on the first frame. Every platform
gets `RAGEXE_VSYNC=0`. `RAGEXE_SCENE_TRACE=1` logs the packets the client sends and the mode
changes.

The game directory's `SaveData` (resolution, options, chat windows) and data files are read as
usual; settings and account settings start empty and are never written.

## Comparing platforms

Capture on one platform with `--golden <dir> --update`, copy the directory, and run the other
with `--golden <dir> --tolerance 3`. The Direct3D 9 backend multisamples by default (4x MSAA with
transparency supersampling), and on the GPU that result varies a few pixels from run to run, so
set `RAGEXE_MSAA=1` on Windows for a repeatable capture. What is left between the Windows and the
Linux captures of these scenes:

- text: GDI and FreeType draw different fonts unless the same font file is installed on both
  (a Windows without Gulim falls back to Ebrima for Korean-charset text; Linux substitutes
  Noto Sans CJK KR or NanumGothic at Gulim's size). The FreeType build with `RAGE_FONT_DIRS` set to
  a copy of the Linux machine's font folders draws the same text but for a few pixels;
- single pixels along triangle edges, where the GPU's and llvmpipe's subpixel precision decide a
  pixel centre differently, and colour rounding of one or two steps in blending.

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
| `enter_map(map, x, y, opts)` | from the login screen straight into a map. `opts`: `name, job, sex, hair, hair_color, body_palette, level, job_level, hp, sp, weapon, shield, head_top, head_mid, head_bottom, robe, dir` |
| `spawn{...}` | an actor appears (`ZC_NOTIFY_STANDENTRY`): `aid, kind` (`"monster"`, `"npc"`, `"player"`, `"pet"`, `"homunculus"`, `"mercenary"`), `job, x, y, dir, name, hair, hair_color, body_palette, head_dir, weapon, shield, head_top, head_mid, head_bottom, robe, body, sex, level, hp, max_hp, speed, state, guild, emblem` (guild id and emblem version; when the client asks for the emblem, the fake map server sends a generated 24×24 one in the guild's colour) |
| `vanish(aid[, how])` | the actor leaves |
| `say(aid, "Name : text")` | chat over an actor |
| `chat(text)` | a line in the player's own chat |
| `send(bytes)` | any packet, as the map server would send it |
| `wait([frames])`, `wait_ready([timeout])` | let frames pass; wait until the map is loaded |
| `mouse(x, y)`, `button(x, y, down[, "right"])`, `click(x, y[, "right"])` | the mouse, through `core/user_input.h` as every platform's input goes |
| `key(vk[, scan])`, `key_event(vk, scan, down)`, `alt_key(vk[, scan])` | keys, through `core/user_input.h`; a key pressed while the script holds Alt (`0x12`) down comes with Alt, as `alt_key` presses it; `scene.VK` names a few virtual keys |
| `text(s)` | characters, typed as a keyboard types them: each letter, digit or space with its key down before it and up after, since the window manager decides on the key whether characters go to the chat line |
| `open(id)` | opens a UI window by its window id |
| `capture(name[, {w, h, x, y, jpeg}])` | writes `<out>/<name>.png`: the frame, or a `w`×`h` part of it at `x, y` (default centred); with `jpeg = quality`, `<out>/<name>.jpg` |
| `effects()` | the IDs of the effects attached to the player, and the number of objects in the world's object list |
| `frame()`, `mode()` | the frame number; `"login"` or `"game"` |
| `fail(message)` | marks the scene failed and goes on |

A script error, a map that does not finish loading, or `fail` ends the run with exit status 1.
On Windows a message box the client would show (`error_msg`, a Lua error) is written to stdout
as `scene: message box "<caption>": <text>` and the run goes on.

The fake map server answers what a scene needs: the map entry (`ZC_AID`, `ZC_ACCEPT_ENTER2`), the
client's notice that the map is loaded (`ZC_NOTIFY_ACTORINIT`, without which the client keeps the
hotkeys off), guild emblems, and the player's own chat line, which it says back as rAthena does.
`keyboard.lua` opens the inventory with Alt+E and sends a line from the chat line.

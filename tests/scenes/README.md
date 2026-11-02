# Scene harness

`ragexe_scenes` is the client with a few files swapped, so a Lua script can put it into any state
without a server or a login and capture the frame as a PNG. It builds on Linux, macOS and Windows:

| replaced                    | by                   | what changes                                                     |
|-----------------------------|----------------------|------------------------------------------------------------------|
| `src/platform/clock.c`      | `scene_clock.c`      | time moves one frame (16 ms) per shown frame; the date is fixed  |
| `src/platform/net.c`        | `scene_net.c`        | sockets stay in the process; the driver plays the map server     |
| `src/platform/thread.c`     | `scene_thread.c`     | no client threads: models load in order on the main thread       |
| `src/platform/settings.c`   | `scene_settings.c`   | an empty settings store in memory: the registry and `client_settings.ini` are neither read nor written |
| `src/core/drive_state.c`    | `scene_driver.c`     | runs the scene script, answers the map server packets, captures  |
| (C library `rand`, POSIX)   | `scene_rand.c`       | the Microsoft runtime's generator, the one the Windows client uses |
| `src/platform/window.c` (Windows) | `scene_input_win.c` | cursor and key state come from the script, never from the desktop |

Everything else is the shipped client, renderer included. With the same game data a scene
produces the same pixels every run, and the same random effects on every platform.

## Running

Linux and macOS: build with `-DRAGE_CLIENT=ON` (the target is built with `ragexe`). Windows: run
`build.bat`, then `tests\scenes\build.bat`, which links `build\ragexe_scenes.exe` from the same
objects. Then run every script in `scripts/` from the game directory:

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
  Noto Sans CJK KR or NanumGothic at Gulim's size);
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
| `spawn{...}` | an actor appears (`ZC_NOTIFY_STANDENTRY`): `aid, kind` (`"monster"`, `"npc"`, `"player"`, `"pet"`, `"homunculus"`, `"mercenary"`), `job, x, y, dir, name, hair, hair_color, body_palette, head_dir, weapon, shield, head_top, head_mid, head_bottom, robe, body, sex, level, hp, max_hp, speed, state` |
| `vanish(aid[, how])` | the actor leaves |
| `say(aid, "Name : text")` | chat over an actor |
| `chat(text)` | a line in the player's own chat |
| `send(bytes)` | any packet, as the map server would send it |
| `wait([frames])`, `wait_ready([timeout])` | let frames pass; wait until the map is loaded |
| `mouse(x, y)`, `button(x, y, down[, "right"])`, `click(x, y[, "right"])` | the mouse |
| `key(vk[, scan])`, `key_event(vk, scan, down, alt)` | keys, through the entry points `core/app_sdl.c` uses; `scene.VK` names a few virtual keys |
| `text(s)` | characters, as SDL text input delivers them |
| `open(id)` | opens a UI window by its window id |
| `capture(name)` | writes `<out>/<name>.png` |
| `frame()`, `mode()` | the frame number; `"login"` or `"game"` |
| `fail(message)` | marks the scene failed and goes on |

A script error, a map that does not finish loading, or `fail` ends the run with exit status 1.

Open: keyboard input reaches the window manager, but Enter does not open the chat line in a
scene yet, so typed text goes nowhere and the Alt window shortcuts do nothing; the scenes open
windows with the mouse.

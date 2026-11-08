# Portable client foundations

The port is part of `custom` (called `purist` until 2026-11-08): the `purist-portable` branch,
started from `purist` commit `d725b49`, was merged into `purist` on 2026-11-04 (see *Merge of
purist-portable* in the [review ledger](custom-review.md#merge-of-purist-portable-2026-11-04)), and
was deleted on 2026-11-08. `custom` is the one line for Windows and the ports, and the portable CI
workflow runs on its pushes. The original [macOS plan](../plans/mac-port.md)
was written against the older `herc-client/client` tree. Here its paths are relative
to the repository root. Custom has removed Bink, embedded browsing, HTTP
services, several startup checks and the features listed in its README. Those features must not be
reintroduced.

Since the merge the client's text is UTF-8 on every platform (`core/utf8.h`): the platform layer
passes it to SDL as it is, converts it to UTF-16 for the Windows W calls (`platform/win_utf8.c` for
message boxes and the registry), and keeps CP949 only for resource names (`RESOURCE_NAME_CODE_PAGE`).

The complete client builds and plays on 64-bit Linux (see *Linux client*) and macOS (see
*macOS client*). The Windows x86 client still builds with `build.bat` and stays the reference.

## Build and test

Requirements: CMake 3.20+, Python 3.10+ and a C11 compiler. All libraries used by
the default build are already in `vendor/`. No download is needed.

```sh
cmake -S . -B build/portable -DCMAKE_BUILD_TYPE=Debug
cmake --build build/portable --parallel 4
ctest --test-dir build/portable --output-on-failure
```

With Visual Studio, add `-A x64` or `-A Win32` at configure time and use
`--config Debug` for build and `-C Debug` for CTest. Use separate build directories.
Clang and GCC builds can enable `-DRAGE_SANITIZERS=ON` for ASan, UBSan and
float-cast-overflow checking. Set `UBSAN_OPTIONS=halt_on_error=1` when running tests.
Test assertions stay enabled even in Release configurations.

The zlib 1.2.11 `zutil.h` classic-Mac branch excludes modern `__APPLE__` targets:
current SDKs define `TARGET_OS_MAC`, but macOS provides POSIX `fdopen`. Treating it
as classic Mac OS rewrites the SDK's function declaration and prevents compilation.
The matching classic-Mac branch in libpng's `pngpriv.h` also excludes `__APPLE__`,
so current macOS uses `<math.h>` rather than the removed `<fp.h>`.

The initial CTest set covers the real string, container, numeric, 3D math and XML modules,
Lua bytecode, compiled Korean literals, frame timing and filesystem behavior.
The full Windows regression suite still runs through `tests/build.bat`.

With `RAGE_CLIENT=ON`, CTest's `gfx_gl` opens an SDL window. When SDL is built with
`-DSDL_UNIX_CONSOLE_BUILD=ON` (no X11, Wayland or other window backend, as on a WSL without
their development packages), SDL finds no video device unless its offscreen driver is chosen,
and `gfx_gl` reports itself skipped ("No available video device"). Run CTest with that driver
there:

```sh
SDL_VIDEO_DRIVER=offscreen ctest --test-dir build/client --output-on-failure
```

## JPEG without IJL

JPEG loading and screenshots use the pinned stb implementation in `vendor/stb`.
The Windows link no longer includes `ijl15.lib`. The public image representation
remains top-down, four-byte-padded BGR; the codec translates to/from packed RGB.
Failed reads preserve the previous image. Dimension/byte-count checks apply on
both pointer widths.

`jpeg_image_write` accepts an already-open `FILE *`, never opens/closes it, and
reports encoder/write failures. The caller must check `fclose` for buffered I/O
errors. Frame capture unlocks the render target before opening/writing the file,
keeps the existing Windows filename handling and checks close failures. This avoids
mixing filename encoding policy into the codec.

`tests/build_jpeg.bat` covers the real decoder/encoder, bitmap color keys, padded
rows, and framebuffer/file failures. CTest runs the codec test on every platform.
The quadrant JPEG is our own image, generated independently with Pillow by
`tests/fixtures/make_jpeg_quadrants.py`; Pillow is only needed to regenerate it.
UBSan found a signed-shift defect in stb's JPEG bit writer; the documented local
patch uses unsigned shifts instead. No sanitizer suppression is used.

Screenshots now use JPEG quality 90. Compressed bytes are not intended to match
IJL; visual comparison of full in-game screenshots is still pending.

## Platform boundaries

- `platform/clock.h` exposes wrapping **32-bit monotonic milliseconds**, independent
  of wall time. Compute elapsed times with unsigned subtraction. Windows retains
  the existing `timeGetTime` epoch; macOS and Linux use `CLOCK_MONOTONIC`. Absolute
  values must not be persisted or compared across processes/platforms. The frame
  timer is the first migrated caller; other clock calls still need migration.
- `platform/filesystem.h` owns the client's file, directory and path calls. Its paths come
  in two forms. A **game path** is relative, CP949, with `\` or `/` separators.
  A **native path** is absolute and in the OS encoding (the system code page on
  Windows, UTF-8 on POSIX). Native paths only come from `platform_current_directory`,
  `platform_module_path`, `platform_path_full` and `platform_path_join(native
  directory, game path)`, which converts the game part. `client_path(out, size,
  format, ...)` in `core/client_path.h` formats a game path and joins it onto
  `g_clientDir`; never concatenate the client directory and a game name by hand.
  A native path can be passed to the C library (`fopen`) directly.
- On Windows every function that replaced an ANSI call passes the string unchanged
  to the same ANSI API (`CreateFileA`, `FindFirstFileA`, `_access`, ...), so the
  client keeps its system-code-page behaviour. `platform_file_open`,
  `platform_file_remove` and `platform_path_is_directory` decode a game path as CP949
  for the UTF-16 API and pass a native path to the ANSI CRT. Directory creation uses
  `_mkdir` everywhere; the former `CreateDirectoryA` callers get the same result.
- On POSIX a native path (leading `/`) is used verbatim. A game path is converted
  to UTF-8, `\` becomes `/`, and `.`/`..` are resolved lexically as Win32 does.
  When the exact path does not exist, each missing component is matched
  case-insensitively (ASCII folding) against its directory. Directory listings for
  this lookup are cached per directory (32 slots, mutex protected), revalidated by
  device, inode and modification time, and reread while the directory changed
  within the last second. Invalid encodings fail instead of substituting characters.
- `platform_find_*` lists a pattern with `*` and `?` (case-insensitive; `*.*` also
  matches names without a dot, as on Win32) and returns names as game strings: on
  POSIX they are converted back to CP949 and names CP949 cannot represent are
  skipped. `platform_native_list` lists a native folder instead, with names as the system
  gives them, for files outside the game such as the system's fonts. `platform_file_open`
  returns a normal `FILE *`, owned by the caller;
  `struct PlatformFile` is the unbuffered handle used by CFile, the GRF reader and
  the settings writers. `platform_file_create_temporary` with
  `platform_file_replace` or `platform_file_discard` is the atomic replace
  (`GetTempFileNameA` and write-through `MoveFileExA` on Windows; `mkstemp`, `fsync`,
  `rename` and a directory `fsync` on POSIX). GRF archives are mapped through
  `platform_file_map` (file mappings on Windows, `mmap` on POSIX).
- CFile names that are native paths skip the `data\` prefix. GRF keys and loose
  names are not lowercased; only the disk lookup ignores case.
- Public platform headers contain no Win32, AppKit, Metal or SDL types. Keep
  renderer-specific objects inside each backend. Shared POSIX behavior serves
  both macOS and Linux; use Apple-specific code only for Apple-specific services.
- A platform module is one header with one file per implementation: `<module>_win32.c` for
  Windows and `<module>_posix.c` for Linux, macOS and Android, or `_sdl.c` where that side is
  written against SDL (`platform/window_sdl.c`, `core/app_sdl.c`). Code both sides share stays
  in `<module>.c` (`platform/language.c`, `core/app.c`). Each
  implementation file starts with the `#ifdef _WIN32` or `#ifndef _WIN32` that selects it, so
  `build.bat`, which compiles every file under `src/`, and the client's CMake glob each get one
  implementation; `rage_platform` in `CMakeLists.txt` names its files by the same rule. Small
  differences inside a shared file (an include, one call) stay as `#ifdef`s where they are.
- Runtime containers and XML nodes are not serialized layouts. Their old x86
  offset/size checks were removed as those modules were made buildable on x64.
  Packet/file layout checks elsewhere remain intact.
- `core/layout.h` provides the same layout assertions without pulling in Win32.
  The shared 3D math module now builds on every platform, retaining its layout
  checks. Its projection result has an explicit signed 32-bit type so error codes
  stay negative on LP64; transform, quaternion, picking and projection tests run
  in CTest. Float cache keys use `memcpy` rather than aliasing through an integer.

## Korean source literals

MSVC still uses `/execution-charset:.949`. `tools/source_encoding.py` creates
build-only copies whose ordinary string/character literals use fixed-width octal
CP949 bytes. Checked-in sources remain UTF-8. Comments, explicitly Unicode
`L`/`u`/`U`/`u8` literals, existing escapes and physical source lines are preserved.
Unencodable text fails the build. Universal character escapes in ordinary
literals are not translated; use readable UTF-8 Korean text as the tree does.

`rage_legacy_source` is the CMake entry point for individual files. It currently
drives the compiled-literal probe; the complete game's source/header graph is not
yet migrated. When a target starts consuming headers with Korean literals, generate
those headers too and put their generated directory first on its include path.

## Lua 5.1 chunks

The vendored `lundump.c` now accepts a four-byte serialized `size_t` on a 64-bit
host. Only string lengths change width; version, format, endianness, integer,
instruction and number representation still have to match. Native chunks still
load and `lua_dump` still emits the host's native format. This is not a general
cross-endian bytecode converter. The loader also checks string terminators.

`tests/fixtures/lua51_win32.lub` contains only our own test code, built from
`lua51_port.lua` using the vendored x86 Lua. It includes nested closures, debug
names and a CP949 string containing an embedded NUL. Regenerate from the repo root:

```powershell
build/portable-win32/Debug/lua_chunk.exe --dump tests/fixtures/lua51_port.lua tests/fixtures/lua51_win32.lub
```

`lua_chunk FILE` parses a real `.lub` without executing it. A local validation
parsed all 25 files in `client-files/System` on Windows x64. Game assets are not
included in this repository. The tests additionally execute the owned fixture,
round-trip native chunks and reject truncated/header-corrupt chunks.

## Window, IME and Metal probe

Enable `-DRAGE_PLATFORM_PROBE=ON`. CMake uses installed SDL3 3.4+ if available,
otherwise downloads SDL 3.4.18 from an exact commit with a SHA-256 check. On macOS
the probe attaches an SDL Metal view and presents cleared drawables through a small
Objective-C file. Windows and Linux use an SDL window surface for the same probe.

Run `platform_probe` interactively, select a Korean input source, type Hangul,
edit an active composition and commit it. The console prints editing ranges and
committed UTF-8 text. Resize and move between displays to inspect drawable sizes.
Escape exits. This is a diagnostic window; it does not render game text.

`platform_probe --smoke` opens the window, presents frames and closes automatically.
CTest runs it when enabled. This checks initialization/rendering, **not** Hangul
composition or candidate-window behavior. Those require a real interactive Mac.
Linux headless smoke runs can use `SDL_VIDEODRIVER=dummy`; that does not validate
a native desktop window. Linux desktop builds need SDL's X11/Wayland development
dependencies; consult the [SDL build documentation](https://wiki.libsdl.org/SDL3/README-linux).

The CI matrix covers Windows x86/x64, Linux with sanitizers, and macOS arm64 with
the probe. The Windows x86 job additionally builds the complete client and runs
the full existing regression suite. The macOS label follows the [GitHub runner reference](https://docs.github.com/en/actions/reference/runners/github-hosted-runners).
Workflow configuration is not evidence of a successful run; record results when
the workflow actually completes.

## Audio

Windows keeps Miles Sound System. Off Windows, `src/platform/miles_sdl.c`
implements the Miles subset declared in `platform/miles_api.h` on SDL3: one device
stream per digital driver at the requested rate and channels (22050 Hz, 16-bit; mono
in 2D mode, stereo in 3D mode), mixed in the device callback. Samples are converted
to the mix format when their file is set (`SDL_ConvertAudioSamples`); the background
music is decoded from memory with the vendored `dr_mp3` and resampled by an
`SDL_AudioStream`. Volumes are linear 0..127; sample and stream status uses the Miles
done/playing values. The one 3D provider is named "Miles Fast 2D Positional Audio":
inverse-distance attenuation between the minimum and maximum distance, silence beyond
the maximum, and a left/right balance from the listener's orientation. Speaker and
room types report "unsupported"; there is no reverb.

`_AIL_file_type`, `_AIL_WAV_info`, `_AIL_decompress_ADPCM` and `_AIL_decompress_ASI`
give `resources/wave.c` the same data as on Windows: a PCM WAV image with a 44-byte
header, freed by `_AIL_mem_free_lock`. Microsoft and IMA ADPCM both decode; the IMA
decoder is the Intel/DVI reference algorithm (FFmpeg rounds differently). Every ADPCM
file in `data.grf` (five IMA, one Microsoft) decoded identically to the reference
decoders, and all 64 BGM tracks of the local game data played through.
`test_miles_sdl` (built when SDL is part of the build, e.g. with
`RAGE_PLATFORM_PROBE` or `RAGE_CLIENT`) runs headless on SDL's dummy driver;
`tests/fixtures/make_audio_fixtures.py` regenerates its fixtures.

## Text

Text goes through one interface, `render/text_backend.h`, with two backends: GDI
(`render/text_gdi.c`, fonts from `render/fonts.c`), the Windows default, and FreeType
(`render/text_freetype.c`, fonts from `render/fonts_ft.c`) on every other platform and on
Windows when it is built with `build.bat freetype`. `render/fonts.h` says which one a build has
(`TEXT_BACKEND_GDI`). Everything above the backend is the same code for both: the bitmaps'
pixels and their textures (`render/cdc.c`), the client's font choice, the UTF-8 text split into runs
by script (`render/text_runs.c`) with each run's fallback font (`render/textdc.c`) and the extent
cache (`render/ctextdc.c`). A backend gives a bitmap its pixels, opens a text DC on a bitmap or a
surface, sets colours, selects a font, measures and draws one run of UTF-16 text in it, says
whether a font has a script and how high its ascent is, and fills the client's one polygon. When the
selected face lacks a run's script (Hangul, Han, kana, Thai, symbols, emoji), both backends draw the
run in the script's first installed fallback font, in the same order: GDI looks for the Windows
fonts (`render/fonts.c`: Hangul in Gulim, then Malgun Gothic, and so on), FreeType asks for the
same families followed by the free fonts that stand in for them (`render/fonts_ft.c`: then
NanumGothic, Noto Sans CJK KR, ...; emoji in Segoe UI Emoji, then Noto Color Emoji), and the run sits
on the selected face's baseline.

With GDI a `DcBitmap` is a DIB section selected into a memory DC and a `TextDC` draws through
that DC or the surface's. With FreeType a `DcBitmap` is a heap buffer with the same top-down
BGRA layout and no device context, and a `TextDC` draws into that buffer, or into a 32-bit
surface it locks with `gfx_lock` on its first draw (`renderer_text_out`).

`build.bat freetype` (also with `x64` and `dbg`) builds `ragexe_ft.exe`, whose text is
drawn by FreeType as off Windows; `vendor\build_vendor.bat` then downloads FreeType 2.13.3, the
release the Android build uses, and builds it static with CMake, and `tests\scenes\build.bat
freetype` links the scene harness from those objects. Windows has no Fontconfig, so the font
folder scan finds the fonts (see *Android client*): in `%WINDIR%\Fonts`, or in the folders
`RAGE_FONT_DIRS` names, separated by `;`. Given the Linux VM's font folders that way, the text
of the `windows_open` scene came out as in the Linux capture, apart from a few single pixels per
window (this build has FreeType 2.13.3, the VM 2.13.2).

**Choosing the font file** for a face index (the index into
`g_3dDevice.faceNamesB`: 0 Gulim, 1 Arial, 2 MS Gothic, 3 SimSun, 4 MingLiU,
5 Tahoma, 6 Arial, 7 and 8 Verdana, 9 and 10 Arial, 11 Tahoma; 20 to 28 the
embedded EOT faces; 29 SCDream):

1. `RAGEXE_FONT_<n>=<file>` loads face `n` from that file (the first face of a
   collection), for example `RAGEXE_FONT_0=/home/me/fonts/gulim.ttc`.
2. Face 29 loads `System/font/SCDream4.otf` (`SCDream6.otf` for bold) from the
   client directory, as on Windows.
3. `RAGEXE_FONT=<file>` loads every other face from one file.
4. Otherwise Fontconfig matches the Windows family name followed by substitutes
   for the run's charset (Korean: NanumGothic, Noto Sans CJK KR, UnDotum, Baekmuk;
   Japanese and Chinese: Noto Sans CJK JP/SC/TC and others; Latin: Liberation
   Sans, DejaVu Sans). As GDI does, the charset wins over the name: the first
   candidate that covers the charset's language is used. A font installed in
   `~/.local/share/fonts` (for example the player's own `gulim.ttc`) is found by
   its family name without any variable.

Characters missing from the chosen file come from the next Fontconfig candidate
that has them, at the same size; otherwise the file's missing-glyph box is drawn.

**Sizes and rendering** follow GDI's `CreateFont`: a positive height is the cell
height, matched through the font's VDMX table or its OS/2 Windows ascent and
descent; a negative height is the em size. When a CJK face is drawn with a
substitute (NanumGothic for Gulim), the em is made equal to the cell, as it is
for Gulim, MS Gothic, SimSun and MingLiU, so Hangul keeps Gulim's size. Glyphs
are 1-bit like `NONANTIALIASED_QUALITY`, embedded bitmaps are used at sizes the
font has them, missing bold is a one-pixel overstrike with one extra pixel of
advance and missing italic is a shear. On the Windows fonts available for the
comparison (Arial, Tahoma, Lucida Console, MS Gothic, Malgun Gothic, SimSun, at
12 to 16 pixels, regular and bold) the extents, black-box widths and pixels match
GDI; italic text differs by a few pixels. Those fonts are hinted by their own
TrueType instructions. Substitutes and fallback faces are hinted by FreeType's
autohinter instead: the instructions of Liberation Sans and NanumGothic are made for
grayscale and give 1-bit digits such as 0 a stray pixel on each side, and text set
in them comes out a few pixels wider than before.

Not covered: the embedded EOT fonts (MicroType Express compressed; faces 20 to 28
use the charset's substitute at 13 pixels), the serial-number text, which GDI
draws in the device's default font and FreeType in face 0, and the performance
overlay, which asks for the generic `monospace` instead of Lucida Console (the font
folder scan takes Lucida Console, Consolas, DejaVu Sans Mono or another fixed-pitch
family for it). Invalid UTF-8 bytes draw as U+FFFD, one per byte, as `core/utf8.c` steps over
them; Windows draws one per invalid sequence.

## Linux client

The complete client builds and runs on 64-bit Linux. CMake builds it when `RAGE_CLIENT` is on:

```sh
cmake -S . -B build/client -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=gcc -DRAGE_CLIENT=ON
cmake --build build/client
ctest --test-dir build/client --output-on-failure
cd /path/to/game/data && /path/to/build/client/ragexe
```

GCC compiles the narrow string literals as CP949 itself with `-fexec-charset=CP949`, as MSVC's
`/execution-charset:.949` does. Clang has no such option, so with any other compiler CMake builds
a mirror of `src/` in the build directory whose narrow literals are CP949 octal escapes
(`tools/source_encoding.py --tree`, see *Korean source literals*). The mirror is refreshed before
every build and only changed files are rewritten, so incremental builds stay incremental. A clang 18
build played the same as the GCC build in the VM. The client links SDL3 (found or fetched as
above), FreeType, Fontconfig, iconv (part of glibc, a library of its own on macOS) and the C++
runtime (the vendored CRijndael is C++). SDL needs the usual X11/Wayland, ALSA/PulseAudio and IBus
development packages; the CI job in `.github/workflows/portable.yml` lists them for Ubuntu 24.04.
Files that only make sense on Windows are listed in `tools/posix_excluded.txt`; files that only
make sense elsewhere are wrapped in `#ifndef _WIN32` and compile to nothing in `build.bat`.

Run it from the game folder (the one with `DATA.INI`, the GRF files and `System`), like the Windows
client. When the current directory has no `DATA.INI`, the client changes to its executable's folder,
or, inside a macOS app bundle, to the folder that holds `ragexe.app`. Loose files are found
case-insensitively; on a case-insensitive volume, finds still report the stored spelling. Settings that Windows keeps in the registry go to
`client_settings.ini` in that folder. Lua scripts open files themselves (the option scripts write
`SaveData\OptionInfo.lua` with `io.open`), so every client Lua state is opened with
`lua_open_client_libs`: on POSIX it passes the path arguments of `io.open`, `io.lines`,
`os.remove`, `os.rename`, `dofile` and `loadfile` through `platform_path_full`; on Windows it is
`luaL_openlibs`.

**Granny models** (the emperium, the guardians, the guild flag and the treasure box) have no
Granny runtime off Windows. `tools/granny_export` converts the game's `.gr2` files once, with
`granny2.dll` on Windows or under Wine, to glTF 2.0 files in `data\model\gltf`, named by the CRC-32
of the `.gr2` they came from; `resources/granny_gltf.c` implements the Granny calls the client
makes (file info, poses, controls, skinning, textures) on those files, found by the CRC of the
bytes the client loads. Without them the models are not drawn and stderr says so once. The
scenes `granny_models` and `granny_dead`, with the Windows scene harness on the same clock, match
`granny2.dll` except for single pixels along edges ([tools/granny_export](../tools/granny_export/README.md)).

### Window, input and IME

Every platform's keys, characters and pointer go through one module, `core/user_input.h`: the
Windows window procedure (`core/wndproc.c`), the SDL layer and the scene scripts all describe a key
transition the same way (virtual-key code, the Win32 VK numbering in `platform/vk.h`; scan code;
repeat; Alt held) and hand over pointer moves, buttons and wheel notches in scene pixels. The
module packs the `lParam` scan-code layout that the window manager and edit boxes read, sends a
press with Alt held, or F10, to the window manager's system-key handling (`WM_SYSKEYDOWN` on
Windows) and everything else to `core/window_events.c`, and tells `platform_note_key` and
`platform_note_cursor` the key and pointer state, which the SDL platform layer keeps and Windows
keeps itself.

`core/app_sdl.c` replaces the Win32 window procedure. It creates the SDL window and GL context and
turns SDL events into that input: Print Screen only as it goes up and Backspace also as a
character, as Windows delivers them, the window activation, and touch screens through
`core/touch.c` (*Android client*). Composition events go to `language_set_composition` and
committed text to `language_commit`, both UTF-8 as the client's text is; the edit boxes'
`language_place_candidates` reaches SDL's text input area on the desktop, so that the IME opens its
candidate list below the composition's line as on Windows. The window sets
`SDL_HINT_IME_IMPLEMENTED_UI` to `composition`: without it SDL does not report the preedit text
and the IME draws its own window. SDL reports the end of a composition (an empty editing event)
before the committed text, and the key that caused the commit never arrives as a key event, so a
commit right after a composition is taken as IME text whatever the key-as-text state of the
window manager. With IBus and `ibus-hangul`, Hangul chat was typed, corrected with Backspace
through and past the composition, and sent in game (`SDL_IM_MODULE=ibus`, `XMODIFIERS=@im=ibus`).

`platform/window.h` holds the remaining window services: message boxes, key and mouse state,
clipboard text, cursor visibility and clipping, window and monitor rectangles, window placement
for windowed, frameless and fullscreen modes, and IME cancellation. They take the platform's own
window and display (`platform/window_handle.h`: `PlatformWindow` and `PlatformDisplay`, on
Windows a window and a monitor handle, elsewhere SDL's window and display id), so the SDL layer's
window is the client's main window (`g_hMainWnd`) and the device's, with no cast and no copy of it
in the platform code. The display queries follow the window: off Windows the window's display is
the one SDL has it on, so a borderless or full-screen switch and its placement happen on the
display the window is on, not always on the primary one. Message boxes and the clipboard take
the client's UTF-8 as SDL does (a message box shows text that is not UTF-8 as CP949, the
resource-name encoding, as Windows does), and the message box shows the error, warning or
information icon the caller asked for;
`tests/test_platform_window.c` checks the clipboard in Korean, Japanese, Cyrillic, Western and
emoji UTF-8, and U+FFFD for invalid bytes. The GL backend learns what covers the window (*Android client*) from `gfx_gl_set_cover`,
which the SDL layer calls once a frame.

### Renderer

`render/gfx_gl.c` implements `render/gfx.h` with OpenGL 3.3 core (macOS also offers 4.1 core, so the
backend is usable there until a Metal backend exists). All client geometry but the ground arrives
pre-transformed (D3D `XYZRHW`), so one vertex shader maps D3D9 pixel centres to GL clip space with
the vertex's `1/rhw` as `w`; the ground's static meshes (`gfx_draw_mesh`, *render/gfx.h*) take a
matrix to the same pixel space times the view depth, so they land on the same pixels, and work
out their fog from that depth. One fragment shader evaluates the two fixed-function texture stages
(`GFX_TOP_*`, argument modifiers, bump-env offsets), the specular-alpha vertex fog and the alpha
test. Positions move 1/128 pixel up and left before rasterization: GL breaks ties on
horizontal edges the other way from D3D's top-left rule, so a quad on whole-pixel
coordinates covered the rows one lower (the menu icons lost their top border, and the
startup texel-offset probe, which draws on whole pixels, measured against that). The fragment
shader takes that 1/128 pixel back out of the texture coordinates (with their screen derivatives),
so filtered textures are still read where Direct3D reads them; otherwise 1:1 UI art picks up a
little of each neighbouring texel. Blend, depth and cull state map directly; the y flip
keeps the on-screen winding, so `GFX_CULL_CW` culls GL back faces. Samplers carry the address, border and filter state.
The scene is drawn into an offscreen framebuffer that the present call blits to the window, so locks,
captures and render-target textures all work on ordinary textures. Texture creation, locking and
unlocking never touch GL: pixels live in a CPU copy in the client's 1555/4444/565/8888/V8U8 layout
and are uploaded on the main thread when the texture is next bound, because the model loader
thread creates textures. `tests/test_gfx_gl.c` drives the backend through the gfx API and reads back
pixels: clears, D3D pixel-centre edges, all texture formats, alpha test, vertex fog, blending, render
target orientation and system-surface blits. A mesh keeps its arrays and gets its GL buffers on its
first draw, on the main thread; the test does not draw one yet.

### What was verified

On Linux Mint 22.3 (a Hyper-V VM with Mesa llvmpipe) against rAthena built for packet version
20220406 and the kRO data the Windows client uses: service and server selection, login, the PIN
dialog, character creation, entering `iz_int03`, walking, chat, the inventory, equipment and skill
windows, Hangul chat through IBus, and an active audio stream. The frame overlay (`RAGEXE_OVERLAY=1`) showed about 90 fps at
1024×768 on llvmpipe. A `-DRAGE_SANITIZERS=ON` build reached the same points; the misaligned
packet-key loads and the null `memmove` it reported are fixed.

For rendering questions, the Windows build runs under Wine in the same VM (`wine32` plus 32-bit
Mesa, the client DLLs next to it) against the same data and server. It drew `iz_int03` the same
way as the GL backend, including the black outside of the cabin's south wall that hides the lower
half of the room from the default camera.

## macOS client

The same CMake build makes `ragexe.app` with Apple clang (Homebrew `ninja freetype fontconfig`):

```sh
cmake -S . -B build/client -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_OSX_ARCHITECTURES=arm64 -DRAGE_CLIENT=ON
cmake --build build/client
ctest --test-dir build/client --output-on-failure
```

Put `ragexe.app` in the game folder and open it. Everything in *Linux client* applies: the GL backend
asks for a forward-compatible core context, which macOS serves as OpenGL 4.1; SDL provides CoreAudio
and Cocoa text input; Fontconfig finds the system fonts. Arial is Apple's own copy and keeps its
hinting; Gulim is replaced by Apple SD Gothic Neo, whose last glyph can overhang the text box by a
pixel. The `macos-client` CI job builds the bundle on GitHub's macOS 15 arm64 runners and runs the
tests there, the GL backend test included, against a real macOS OpenGL context.

The game was played on macOS 12.7 (x86_64, VMware, Apple's software OpenGL renderer, Apple clang 14
from the command line tools, MacPorts for cmake, ninja, freetype and fontconfig) against rAthena
built in the same machine: `ragexe.app` opened from the game folder, logged in, created a
character, entered `iz_int03`, walked and chatted. The 21 tests pass there as well.

## 64-bit Windows client

`build.bat x64` builds `build\ragexe_x64.exe` (`build.bat x64 dbg`: `ragexe_dbg_x64.exe`) with
the same Win32 window, GDI text and Direct3D 9 renderer as the 32-bit client. The only 32-bit parts
of the 32-bit client are RAD's `granny2.dll` and `mss32.dll`, so the 64-bit build swaps them
(`PLATFORM_RAD_DLLS` in `platform/win_types.h` is set only for 32-bit Windows):

- sound goes through `platform/miles_sdl.c`, the Miles subset the POSIX builds use, on an
  audio-only static SDL3 that `vendor\build_vendor.bat x64` downloads (the commit
  `cmake/SDL3.cmake` pins, hash-checked) and builds with CMake;
- Granny models come from the converted glTF files (`tools/granny_export`), as off Windows;
- `cps.dll`, which the client never calls, is no longer linked.

The crash report prints full-width addresses. `tests\scenes\build.bat x64` builds the scene
harness; every non-Granny scene captures bit for bit what the 32-bit client captures, and the
Granny scenes differ from `granny2.dll` only along edges, as on Linux. The `-A x64` CI job builds
both.

## Android client

`android/` is a Gradle project around the same CMake build: the client is `libmain.so`, SDL3 is
`libSDL3.so`, and SDL's own Java activity (`vendor/sdl3_android`, from the SDL commit
`cmake/SDL3.cmake` pins) loads both and calls the client's `main`. It needs JDK 17 and the Android
SDK with NDK 27.3.13750724 and CMake 3.31.5 (Gradle installs those two when they are missing):

```sh
cd android
./gradlew assembleRelease     # app/build/outputs/apk/release/app-release.apk, arm64-v8a and x86_64
adb install -r app/build/outputs/apk/release/app-release.apk
```

The release APK is signed with the local debug key, for installing on your own devices. On a
Windows host the SDK and NDK must be on a path without spaces (for example a junction
`C:\Android\Sdk`, named in `android/local.properties` as `sdk.dir`): CMake hands Ninja the short
8.3 name of a compiler in a path with spaces, and `CLANG_~1.EXE` no longer knows it is clang++,
so nothing links the C++ runtime.

**The game folder** is the app's own external storage directory,
`/sdcard/Android/data/local.ragexe.client/files`. It holds a game when it has `DATA.INI`, as on
Windows. While it has none the app shows its import screen instead of starting the client: the
player picks the game's folder with the system's folder picker (the device's storage, a USB drive,
on a Chromebook its own files, Linux files or Google Drive) and the app copies everything in it
into the game folder, then starts the game. The copy needs as much free space as the folder; a
folder below `Download` can be picked, `Download` itself cannot (Android refuses it). A copy cut
short leaves `.import-unfinished` behind, so that the screen comes back and asks for the folder
again. The app's shortcut *Import game files* (a long press on its icon in the launcher) shows the
screen once more, to copy a newer game over the old one. `adb push` into the game folder works as
well. The glTF models from `tools/granny_export` belong in `data/model/gltf`. `clientinfo.xml`
names the servers as on any other platform; an emulator reaches servers on its host through
`adb reverse` or the host's own address.

**What is different on Android:**

- The client always runs in its full-screen mode, at the resolution set in its options, and the
  backend scales the scene to the screen with the aspect kept (`render/present_fit.h`), as the
  desktop backends do for a window of another size. The system bars are hidden.
- On a desktop (ChromeOS, Android's own desktop windowing) the app runs in a window the
  player moves, resizes and maximises; the first one is about 4:3 (`<layout>` in the manifest) and
  the system remembers the size after that. The full-screen request only takes effect when the
  window is maximised; the caption bar then stays drawn over the top of the app, so the activity
  reports how far it reaches into SDL's surface and the scene is fitted below it. Resizing, moving
  to a display of another density (`density` in `configChanges`) and maximising keep the running
  game. A mouse is a mouse there: hover, both buttons (the right one turns the camera), the wheel
  (zoom) and SDL's empty cursor over the client's own. A hardware keyboard types and plays as on a
  desktop and the on-screen keyboard stays away.
- Drawing goes through OpenGL ES 3.0 (`RAGE_GLES`, which a desktop build can also turn on for
  testing). ES has no BGRA uploads, read-backs or vertex colours, no reversed 16-bit layouts and no
  border clamping: 8888 pixels are swizzled, 1555/4444/565 textures are widened on the CPU by bit
  replication as desktop drivers widen them, vertex colours are swizzled in the shader and border
  addressing clamps to the edge. Every scene drawn through ES on Mesa stays within 2 of the desktop
  GL capture in every channel.
- Fonts: there is no Fontconfig, so `render/font_match_scan.c` reads the faces in `/system/fonts`
  and `/product/fonts` once and sorts them as Fontconfig does for the client's requests (the asked
  families in order, then the faces that cover the language, then slant and weight); Korean text is
  set in Noto Sans CJK KR, Latin text in Roboto. `RAGE_FONT_SCAN` turns the scanner on elsewhere
  (`RAGE_FONT_DIRS` names the folders); on the Linux VM it chose the same fonts as Fontconfig for
  every scene.
- Text conversion: Android's iconv converts only between Unicode forms, and the client converts
  between UTF-8 and its Windows code page all the time (file names, input, message boxes). The client
  carries the code pages it can select (874, 932, 936, 949, 950 and 1250 to 1258) in
  `platform/code_page_tables.c`, written by `tools/code_pages.py` from Windows' own
  `MultiByteToWideChar` and `WideCharToMultiByte`, and converts with them through the iconv calls
  the POSIX code already makes (`platform/iconv_port.h`, `RAGE_OWN_ICONV` elsewhere).
  `tests/test_code_page_iconv.c` checks every code of every table against Windows on Windows.
- Touch (`core/touch.c`, the same on any SDL platform with a touch screen): one finger is the left
  button. A tap clicks where it lands, a finger held or dragged presses there and moves with the
  finger, and a tap close to the last one lands exactly on it, so that two taps make a double
  click. Two fingers dragged the same way turn the camera, as the right button dragged does;
  spread apart or pinched together they zoom it, a wheel notch per 40 scene pixels; tapped together
  they right-click, and tapped twice they put the camera back as the right double click does. A
  first finger waits 120 ms before it presses, so that a second one landing just after it makes a
  two-finger gesture and no left click. The client finds the window under the pointer once a frame,
  so the recognizer hands over at most one button change and one wheel notch a frame, and a button
  never in the frame the pointer moved before it. SDL's own touch-to-mouse events are off, and only
  direct touch screens reach the recognizer: a trackpad stays the mouse it already is. Zooming over
  a UI window does what the wheel does there, nothing without Ctrl.
- Input: the back button is Escape. Text input is on while
  an edit box has the focus, so a hardware keyboard types as on a desktop; the on-screen keyboard
  comes up when the player taps the focused edit box and goes when they tap anything else, and while
  it is up the scene is raised so that the box stays above it (the activity reports the keyboard's
  height from its window insets, which SDL does not). The in-game chat line opens with the keyboard
  icon at the top of the chat window, or Enter.
- Lua 5.1 keeps a string's bytes after its header, which bionic's fortified `strchr` takes for an
  overread, so the vendored Lua is compiled without `_FORTIFY_SOURCE`. `char` is signed on every
  build (`-fsigned-char`), as the code written for MSVC expects; arm64 Android and Linux make it
  unsigned.

The game was played on the Android 15 x86_64 emulator (Google APIs image, host GPU) against the
Linux VM's rAthena: service and server selection, login through the on-screen keyboard, character
selection, entering `iz_int03`, walking by tapping, and chat. The desktop side was played on the
Android 14 desktop emulator image (`android-desktop`, whose model `sdk_gpc_x86_64` SDL takes for a
Chromebook) at 1920x1080: the import from a folder in `Download` (and its restart after a copy cut
short), the window at its first size, resized, maximised and at another density, a virtual USB
mouse through uinput (hover, clicks, right-drag turning the camera, the wheel zooming it) and chat
typed on the keyboard. The touch gestures were played there through a virtual multi-touch screen
(uinput again): two fingers turning the camera without walking, spreading and pinching to zoom in
and out, a two-finger double tap putting the camera back; and on the phone emulator the login and
walking by tapping, with the on-screen keyboard coming up for the tapped edit box. No real
Chromebook or touch screen was tried. The `android-client` CI job builds the APK for both ABIs.

Driving the login by hand: the client gives up on the server list after 15 s without a choice and
quits with "Failed to Connect to Server" (`login_mode_check_timeouts`, sub-mode 6). That is the
client's own timeout, not a network fault.

## Scene captures

`ragexe_scenes` is the client with the clock, the sockets, the threads, the settings store and
the per-frame driver hook replaced: a Lua script enters a map from the login screen, spawns
monsters, NPCs and players, makes them talk, clicks windows open and captures PNGs, headless
through SDL's offscreen driver on Linux and with a hidden window on Windows
(`tests\scenes\build.bat`). `tests/scenes/run.py` runs every script and compares the captures
with golden images (`tests/scenes/README.md`). On the Mint VM each of the three scenes takes 3–4 s,
on Windows 2 s, and both repeat bit for bit (Windows with `RAGEXE_MSAA=1`).

Comparing the Windows (Direct3D 9) and Linux (GL on llvmpipe) captures of the same scenes and data:

- **Text** is the largest difference. GDI and FreeType draw different fonts unless the same file
  is on both machines; a Windows without the Korean supplemental fonts has no Gulim, and GDI then
  draws Korean-charset text in Ebrima, which is much narrower than Gulim, while Linux uses
  Noto Sans CJK KR or NanumGothic at Gulim's size.
- **Edges and foliage**: Direct3D 9 renders the world with 4x MSAA and transparency
  supersampling by default; the GL backend does not multisample, so model edges and alpha-tested
  foliage (Prontera's flower beds) are aliased on Linux and macOS. Sprites and UI are drawn
  without MSAA on both.
- **Random effects** (level 99 auras, clouds, particles) took other paths because glibc's and
  Microsoft's `rand()` are different generators; the scene harness uses Microsoft's on POSIX,
  after which they match. Two places assumed Microsoft's 15-bit range and misbehaved on Linux
  and macOS, where `rand()` returns 31 bits: a random point in a disc divided by 32767 (particles
  landed up to 256 radii away) and the shuffle copied from Microsoft's STL rejected all but one
  draw in 65536. Both take the low 15 bits now.
- What remains is single pixels along triangle edges, where the GPU's and llvmpipe's subpixel
  precision decide a pixel centre differently, and blending rounded one or two steps apart.

Making the captures repeat showed three draw orders that depended on heap addresses, which ASLR
and allocation timing change from run to run: the render queues sorted by texture pointer
(now by a texture serial assigned at creation), tied queue keys broken by primitive pointer (now
by submission order), and emitter particles reused in address order from a pointer-keyed map
(now in the order they were added). The multitexture sort also compared the first texture with
the other entry's second one, which is not an ordering; it compares like with like now.

## Remaining work

1. macOS: check Hangul composition through the macOS input method and Retina scaling on
   Apple hardware (the window does not ask for a high-density drawable,
   so macOS scales a 1x picture). A Metal backend behind `render/gfx.h` can replace the
   deprecated OpenGL later.
2. Granny models need a one-time conversion with `tools/granny_export` (Windows or Wine).
3. Listen to the POSIX audio in game and compare JPEG screenshots with Windows.
4. Keep auditing pointer-width message arguments, raw strides and packed packet
   fields; the sanitizer run covered login to the first map, not every window.
5. Expand CMake to the remaining test suites.
6. The GL backend does not multisample; Direct3D 9 uses 4x MSAA with transparency
   supersampling by default (*Scene captures*).
7. Android: only the x86_64 emulators (phone and desktop) were played; try arm64 hardware, a real
   Chromebook and a real touch screen (the gestures were driven through a virtual uinput one), and
   the default resolution is 4:3 whatever the screen's aspect (*Android client*).

`g_clientDir` is filled by `platform_current_directory` with at most 0x7f bytes, as
before; a longer client directory leaves it empty on every platform.

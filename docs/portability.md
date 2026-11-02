# Portable client foundations

This is the `purist-portable` branch, started from `purist` commit `d725b49` and kept current by
merging `purist` into it. The original [macOS plan](../plans/mac-port.md)
was written against the older `herc-client/client` tree. Here its paths are relative
to the repository root. Purist has already removed Bink, embedded browsing, HTTP
services and several startup checks. Those features must not be reintroduced.

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
  skipped. `platform_file_open` returns a normal `FILE *`, owned by the caller;
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

Windows keeps Miles Sound System. Off Windows, `src/platform/miles_posix.c`
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
`test_miles_posix` (built when SDL is part of the build, e.g. with
`RAGE_PLATFORM_PROBE` or `RAGE_CLIENT`) runs headless on SDL's dummy driver;
`tests/fixtures/make_audio_fixtures.py` regenerates its fixtures.

## Text on Linux and macOS

Windows keeps GDI. Elsewhere a `DcBitmap` is a heap buffer with the same top-down
BGRA layout and no device context, and `TextDC` draws with FreeType into that
buffer, or into a 32-bit surface it locks with `gfx_lock` on its first draw
(`renderer_text_out`). `render/fonts_ft.c` replaces `render/fonts.c`; the client
calls the same `text_dc_*` and `font_cache_get_font` functions on every platform.

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
overlay, which uses Fontconfig's `monospace` instead of Lucida Console. Invalid
code-page bytes make a run disappear instead of drawing replacement characters.

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

`core/app_sdl.c` replaces the Win32 window procedure. It creates the SDL window and GL context,
turns SDL events into the client's own key codes (`platform/vk.h`, the Win32 VK numbering), the
`lParam` scan-code layout that the hotkey code compares against, mouse state and window
activation, and feeds them to the same handlers the Windows window procedure calls
(`core/window_events.c`). Composition events go to `language_set_composition` and committed text
to `language_commit`, converted from UTF-8 to the client code page. The window sets
`SDL_HINT_IME_IMPLEMENTED_UI` to `composition`: without it SDL does not report the preedit text
and the IME draws its own window. SDL reports the end of a composition (an empty editing event)
before the committed text, and the key that caused the commit never arrives as a key event, so a
commit right after a composition is taken as IME text whatever the key-as-text state of the
window manager. With IBus and `ibus-hangul`, Hangul chat was typed, corrected with Backspace
through and past the composition, and sent in game (`SDL_IM_MODULE=ibus`, `XMODIFIERS=@im=ibus`).

`platform/window.h` holds the remaining window services: message boxes, key and mouse state,
clipboard text, cursor visibility and clipping, window and monitor rectangles, window placement
for windowed, frameless and fullscreen modes, and IME cancellation.

### Renderer

`render/gfx_gl.c` implements `render/gfx.h` with OpenGL 3.3 core (macOS also offers 4.1 core, so the
backend is usable there until a Metal backend exists). All client geometry arrives pre-transformed
(D3D `XYZRHW`), so one vertex shader maps D3D9 pixel centres to GL clip space with the vertex's
`1/rhw` as `w`, and one fragment shader evaluates the two fixed-function texture stages
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
target orientation and system-surface blits. The client has no vertex-buffer mesh producer, so
`gfx_draw_vbuffer` is empty in this backend.

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
6. Scenes: keyboard input does not open the chat line yet (`tests/scenes/README.md`).
7. The GL backend does not multisample; Direct3D 9 uses 4x MSAA with transparency
   supersampling by default (*Scene captures*).

`g_clientDir` is filled by `platform_current_directory` with at most 0x7f bytes, as
before; a longer client directory leaves it empty on every platform.

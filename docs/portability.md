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

The complete client builds and plays on 64-bit Windows (`build.bat`, see *Windows client*), Linux
(see *Linux client*), macOS (see *macOS client*) and Android (see *Android client*). There has been
no 32-bit client since 2026-11-11. Three things are still built 32-bit, because they load or write
32-bit code or data of the original: the oracle tests (`tests/oracle`, the original executable),
`tools/granny_export` (the game's `granny2.dll`) and, when the Lua fixture is regenerated,
`lua_chunk` from a CMake `-A Win32` configuration (the `.lub` files' 4-byte sizes, see *Lua 5.1
chunks*).

## Build and test

Requirements: CMake 3.22+, Python 3.10+ and a C11 compiler. All libraries used by
the default build are already in `vendor/`, and the C the client shares with the server is in the
submodule `game` (the set of the game's pieces): clone with `--recurse-submodules`, or run
`git submodule update --init --recursive` in a clone. No other download is needed.

```sh
cmake -S . -B build/portable -DCMAKE_BUILD_TYPE=Debug
cmake --build build/portable --parallel 4
ctest --test-dir build/portable --output-on-failure
```

With Visual Studio, add `-A x64` at configure time and use `--config Debug` for build and
`-C Debug` for CTest; `-A Win32` configures the tests that do not need 64-bit Windows, and makes the
32-bit `lua_chunk` that writes the Lua fixture below. Use separate build directories.
Clang and GCC builds can enable `-DRAGE_SANITIZERS=ON` for ASan, UBSan and
float-cast-overflow checking. Set `UBSAN_OPTIONS=halt_on_error=1` when running tests.
GCC and Clang compile everything with `-ffp-contract=off`. MSVC rounds each multiply and add of the
client's float math on its own; GCC and Clang fuse `a * b + c` into one multiply-add where the CPU
has one (arm64: Android phones, Apple Silicon, arm64 Linux), which rounds once and gives other
floats than the Windows client in the projection, the sprites' depth and the effects. With the flag
the NDK's clang emits no fused multiply-add for arm64 in `render/math3d.c`, `render/rp_project.c`
and `actors/sprite_quad.c` (191, 217 and 18 without it); x86-64 builds have none either way.
Test assertions stay enabled even in Release configurations: the tests' directory drops `NDEBUG`
for the tests and for the client files they link.

The zlib 1.2.11 `zutil.h` classic-Mac branch excludes modern `__APPLE__` targets:
current SDKs define `TARGET_OS_MAC`, but macOS provides POSIX `fdopen`. Treating it
as classic Mac OS rewrites the SDK's function declaration and prevents compilation.
The matching classic-Mac branch in libpng's `pngpriv.h` also excludes `__APPLE__`,
so current macOS uses `<math.h>` rather than the removed `<fp.h>`.

`tests/CMakeLists.txt` declares the whole test suite, once for every platform (`rage_test`; its
helpers say what each keyword does). Windows, Linux and macOS build the portable tests (Android
configures with `-DBUILD_TESTING=OFF`, and the tests' platform library expects an iconv there):
strings,
containers, numbers, 3D math, XML, Lua bytecode, compiled Korean literals, frame timing, the
platform layer (clock, files, threads, sockets, code pages, SHA-256), JPEG, two extracted
production routines and the Python checks (source encoding, the library table's include check,
the map-name migration). 64-bit Windows adds the rest of the suite, about 290 tests that only
build there, and the text against GDI (`font_system`); Linux and macOS add the settings store and,
with SDL, the clipboard. Where FreeType is part of the build, the font finder and the font cache
are tested over fonts the tests make, and the text over the system's fonts (`text_freetype`, which
on Linux and macOS needs Fontconfig). Where SDL is part of the build (always on 64-bit Windows, which gets it from
`cmake/SDL3.cmake` as the Linux client does), `audio` checks the sound module.

Client files are compiled once, into one static library per library of `tools/libraries.txt`
(`rage_client_<library>`), and each test links the libraries of the files it names; a test that
compiles a client file with defines or a fixture of its own keeps its own copy. Each test is
labelled with the libraries whose `.c` files it compiles, links or includes, so
`ctest -L render` runs the tests a change in the render library reaches.

The pieces `rules` and `delivery` of `game` register their own tests in the same CTest (`rules_*`
and `delivery`, with the digests their vectors record), so they run with every compiler the client
is built with; `ctest -R "^(rules_|delivery)"` runs them alone. Their WebAssembly links are tests
only where the compiler is Clang. `script-vm`, which `rules` links, comes without its tests, which
need Node.

Every test runs headless and silent: CTest gives it `SDL_VIDEO_DRIVER=dummy` and
`SDL_AUDIO_DRIVER=dummy` (`RAGE_TEST_ENVIRONMENT`), `audio` SDL's `disk` driver, which writes
into files in its directory, `platform_window` the offscreen video driver, `gfx_gpu` and
`gfx_gpu_msaa` keep the platform's video driver, in a hidden window, for a GPU device (the dummy
driver has no Vulkan), and `gfx_gpu_present` the offscreen one, which shows its window nowhere, so
that the window is claimed and presented into where that driver gives windows a swapchain (a Vulkan
with `VK_EXT_headless_surface`, as Mesa's are; NVIDIA's on Windows has none, and the test says it
did not present); they report themselves skipped where SDL finds no GPU. SDL shows no message box
with the dummy video driver, and on Windows the client libraries compile `platform/window_sdl.c`
with `tests/headless.h`, which writes the client's message boxes to stderr even when a test
program runs by hand. `platform_probe` shows a real window and is labelled `interactive`; leave
it out with `ctest -LE interactive`.

On Windows `tests\build.bat` configures `build\tests` with Ninja (Release, `/MT`), builds what
changed and runs CTest on every core, then the oracle tests (`tests/oracle`), a 32-bit
configuration of their own (`-DRAGE_ORACLE=ON`) in `build\oracle`. Its arguments go to CTest
(`tests\build.bat -R chat_tabs`, `-L render`).

With `RAGE_CLIENT=ON`, CTest's `gfx_gpu` opens a hidden SDL window. When SDL is built with
`-DSDL_UNIX_CONSOLE_BUILD=ON` (no X11, Wayland or other window backend, as on a WSL without
their development packages), SDL finds no video device unless its offscreen driver is chosen,
and `gfx_gpu` reports itself skipped ("No available video device"). Run CTest with that driver
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
errors. Frame capture downloads the screen (`gfx_read_frame`), copies it into the JPEG image
and frees the download before opening/writing the file, keeps the existing Windows filename
handling and checks close failures. This avoids
mixing filename encoding policy into the codec.

The tests `jpeg_codec`, `jpeg_bitmap` and `jpeg_capture` cover the real decoder/encoder, bitmap
color keys, padded rows, and framebuffer/file failures. The codec test runs on every platform.
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
  Windows and `<module>_posix.c` for Linux, macOS and Android. Code both sides share stays
  in `<module>.c` (`platform/language.c`, `core/app.c`), and a module written against SDL is one
  `_sdl.c` file for every platform (`platform/window_sdl.c`, `core/app_sdl.c`). Each
  implementation file starts with the `#ifdef _WIN32` or `#ifndef _WIN32` that selects it, so
  `build.bat`, which compiles every file under `src/`, and the client's CMake glob each get one
  implementation; `platform_files` in `tests/CMakeLists.txt` names its files by the same rule. Small
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
names and a CP949 string containing an embedded NUL. `tools/utf8-data`'s tests read it
too, for the `.lub` files' 4-byte sizes, which their 64-bit `luadump` does not write.
Regenerate it from the repo root, with `lua_chunk` from a `-A Win32` configuration in
`build/portable-win32`:

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
CTest runs it when enabled, labelled `interactive` since its window shows. This checks
initialization/rendering, **not** Hangul composition or candidate-window behavior. Those require a
real interactive Mac.
Linux headless smoke runs can use `SDL_VIDEODRIVER=dummy`; that does not validate
a native desktop window. Linux desktop builds need SDL's X11/Wayland development
dependencies; consult the [SDL build documentation](https://wiki.libsdl.org/SDL3/README-linux).

The CI matrix covers Windows x64 (the CMake `-A x64` Debug build, which declares the whole suite
there), Linux with sanitizers and macOS arm64 with the probe. The Windows job also builds the
complete client with the library table's check (`build.bat check`) and the scene harness, and
runs `tests\build.bat`, the whole suite in Release; its oracle tests are skipped there, since the
original executable is not in the repository.
The macOS label follows the [GitHub runner reference](https://docs.github.com/en/actions/reference/runners/github-hosted-runners).
Workflow configuration is not evidence of a successful run; record results when
the workflow actually completes.

## Audio

Sound goes through the client's own module, `platform/audio.h`, implemented once for every platform
on SDL 3 in `platform/audio_sdl.c`. Until 2026-11-11 `platform/miles_sdl.c` implemented the subset
of the Miles Sound System API (`platform/miles_api.h`) that the original calls, with a mixer of its
own and a 3D provider; both files are gone (`docs/custom-review.md`, "Sound without a position").

- **Device.** `audio_open` opens SDL's default playback device with SDL's default format, and SDL
  mixes every `SDL_AudioStream` bound to it itself, converting each to the device's format and rate
  and applying each stream's gain. With the `SOUNDMODE` setting 1, the original's 2D mode, for which
  the client opened a mono Miles driver, a post-mix callback averages the first two channels, so
  both carry the same mono mix.
- **Effects.** `audio_voice_play` gives a decoded sound its own stream, bound to the device with a
  copy of the samples, a linear gain and nothing else; `audio_voice_playing` asks SDL whether the
  stream still has output, and `audio_voice_stop` destroys it. `contents/sound.c` keeps the
  original's 48 voices: an effect that finds all of them playing is dropped.
- **Music.** `audio_music_play` reads the MP3 file into memory and decodes it with the vendored
  `dr_mp3` on the device's thread, in the stream's get callback, as the device asks for more; it
  plays the file `loop_count` times, 0 for ever, and `audio_music_done` answers once the last loop
  has played out. The fade sets the music's gain in steps from its thread as before.
- **Decoding.** `audio_sound_decode` reads a RIFF WAVE with SDL's own loader (`SDL_LoadWAV_IO` on
  the bytes in memory: PCM, Microsoft and IMA ADPCM and SDL's other WAV formats) and anything that
  starts with an ID3 tag or an MPEG frame sync with `dr_mp3`, as Miles' file type told the original
  apart. It sets `SDL_HINT_WAVE_TRUNCATION` to `dropframe` once, with override priority, so an ADPCM
  file whose last block is cut short keeps that block's whole frames whatever the environment says,
  and cuts the result to whole frames, since SDL takes a PCM file's block size from its header. It
  refuses a sound outside 1,000 to 384,000 Hz or longer than five minutes, so a damaged header
  cannot hold a voice for hours; the game's WAVs are 8,000 to 48,000 Hz, the longest 40 s. A WAV
  that is MPEG Layer 3 inside RIFF (format 0x55) decodes neither now nor in `miles_sdl.c` before;
  `data.grf` has none, nor any MP3 data named `.wav`. `resources/wave.c` decodes a sound file once,
  when it loads, and keeps the result in the resource cache; a file that reads but does not decode
  loads without a sound and plays nothing.

Sounds have no position: there is no listener, 3D provider, distance, panning or falloff, and
`sound_play_wave` takes the file and a volume only. Every effect plays at the effect volume times
that volume, which is 1 for every caller but a map's sound sources (the RSW's own volume per
source), and the music at the music volume. What the player hears differently from `miles_sdl.c`: no
distance (in the default 3D mode its falloff and balance, in mode 1 the client's own), and the mix
at the output device's rate instead of 22,050 Hz, so the music keeps the frequencies above 11 kHz
that the old mix cut. The one test of distance left is a map's sound sources' own:
`sound_maker_on_process` (`actors/small_actors.c`) starts a source's sound only while the player is
within 130 of the source's range plus the diagonal of its area, measured from its position, and
within 75 of its range from the edge of its area towards the player; within that it now plays at its
full volume. Every one of the game's 2,976 WAV files (2,974 in `data.grf`, 2 loose under `data\wav`)
decodes to the same samples as through `resources/wave.c` and `miles_sdl.c` before; without
`dropframe` four IMA ADPCM files would lose a last block of 31 to 46 ms.

`test_audio` (CTest's `audio`, built when SDL is part of the build: always on 64-bit Windows,
elsewhere e.g. with `RAGE_PLATFORM_PROBE` or `RAGE_CLIENT`) decodes the ADPCM fixtures to their
reference decodes, PCM byte for byte and the MP3 fixture to its tone, refuses rates and lengths
beyond the bounds, feeds every prefix of each file and thousands of damaged copies to the decoder
(on Windows each ending where readable memory ends), and mixes through SDL's disk driver: a voice
plays its sound once, the music its two loops, a gain halves, a stereo sound stays stereo and the
mono mode puts the mix in both channels. Given a game folder (`RAGE_AUDIO_GAME_DIR`, by default
`client-files` beside the checkout; the test is labelled `client-files`) it also decodes PCM, 8-bit,
stereo and both ADPCM sounds from `data.grf` and `BGM\01.mp3`, and mixes two effects with the music;
without one that part is skipped. `tests/fixtures/make_audio_fixtures.py` regenerates the fixtures.

## Text

FreeType draws the text on every platform, into the client's 32-bit bitmaps. A `DcBitmap`
(`render/cdc.c`) is a heap buffer of top-down BGRA pixels with the textures that show it; a text DC
(`render/textdc.c`) draws and measures UTF-8 text into one in the selected face, split into runs by
script (`render/text_runs.c`): a run of a script the face lacks (Hangul, Han, kana, Thai, symbols,
emoji) is drawn in the script's fallback font, on the face's baseline, and `render/ctextdc.c`
caches the extents. The fonts are `render/fonts_ft.c`'s; the font finder (`render/font_match.h`,
the libraries `font-finder` and `font-faces`) lists the installed fonts and sorts them for a
request.

No text is drawn into the frame: the login screen's serial number, which the original drew there
through a DC, is drawn once into a bitmap and shown as a quad over everything else the frame holds
(`ui_window_mgr_draw_serial_number`).

`vendor\build_vendor.bat` downloads FreeType 2.13.3, the release the Android build uses, and builds
it static with CMake for the Windows client; the Windows tests build the same archive through
`cmake/Freetype.cmake`.

**The installed fonts.** On Windows the finder reads the fonts GDI has: those the registry lists
under `SOFTWARE\Microsoft\Windows NT\CurrentVersion\Fonts`, the machine's (`HKEY_LOCAL_MACHINE`,
files in `%WINDIR%\Fonts`) and then the user's (`HKEY_CURRENT_USER`, full paths: where Windows 10
1809 and later install a font for the user alone). On Android, and with `RAGE_FONT_SCAN`, it reads
the faces in the system's font folders; on Linux and macOS Fontconfig finds the fonts, also those in
`~/.local/share/fonts`. `RAGE_FONT_DIRS` (folders separated by `;` on Windows, `:` elsewhere)
replaces the registry or the system's folders, four levels deep. The finder knows each face by its
family name (the name table's name 1, the one GDI matches) in English and in the display language
(on Windows the user's interface language), its weight, slant and pitch, and the languages it
covers (a few sample characters each). A file FreeType cannot open, a face without a family name or
a character map, and the faces of a collection past its 1,024th are left out. On a Windows 10 with
340 faces (57 installed for the user) the finder takes 65 to 90 ms at startup.

**Choosing the font** for a request, as GDI's font cache chose it:

1. `RAGEXE_FONT_<n>=<file>` loads face `n` from that file (the first face of a collection), for
   example `RAGEXE_FONT_0=/home/me/fonts/gulim.ttc`; face 29 loads `System/font/SCDream4.otf`
   (`SCDream6.otf` for bold) from the client directory, as on Windows; `RAGEXE_FONT=<file>` loads
   every other face from one file.
2. A face of the device's tables (`render/c3ddevice.c`: 0 Gulim, 1 Arial, 2 MS Gothic, 3 SimSun,
   4 MingLiU, 5 Tahoma, 6 Arial, 7 and 8 Verdana, 9 and 10 Arial, 11 Tahoma; 20 to 28 the embedded
   EOT faces; 29 SCDream) is asked for by its name in the display language (`faceNamesA`: 굴림,
   ＭＳ Ｐゴシック, 宋体 and 細明體 for faces 0, 2, 3 and 4), then by its English name
   (`faceNamesB`). A face answers to its English family name and to its name in the display
   language, as GDI matched them, so face 2 is MS PGothic on a Japanese Windows and MS Gothic
   elsewhere. After them come the charset's Windows family (Arial for the Latin charsets, Gulim, MS
   Gothic, SimSun, MingLiU, Tahoma), the font GDI's mapper gives text of that charset, and then the
   free fonts that stand in for them on other platforms (Korean: NanumGothic, Noto Sans CJK KR,
   UnDotum, Baekmuk; Japanese and Chinese: Noto Sans CJK JP, SC and TC and others; Latin:
   Liberation Sans, DejaVu Sans). As with GDI the charset wins over the name: the first of these
   that covers the charset's language is used.
3. A script's fallback font is the first of the script's Windows fonts that is installed, as GDI's
   cache took it (Hangul: Gulim, then Malgun Gothic; symbols: Gulim, Malgun Gothic, Segoe UI
   Symbol; kana: MS PGothic, MS Gothic, Meiryo, Yu Gothic, Gulim, Malgun Gothic; Thai: Tahoma,
   Leelawadee UI; emoji: Segoe UI Emoji, Segoe UI Symbol), then the free fonts that stand in for
   them (NanumGothic, Noto Sans CJK KR, ...; emoji in Noto Color Emoji).
4. The frame-time overlay asks for Lucida Console at an em of 12 pixels, of fixed pitch; without
   it, Courier New, which GDI's mapper gives that request, then a fixed-pitch stand-in (DejaVu Sans
   Mono, Liberation Mono, ...), then any fixed-pitch face.

Characters missing from the chosen font come from the next candidate that has them, at the same
size; otherwise the font's missing-glyph box is drawn.

**Sizes and rendering** follow GDI's `CreateFont`. A negative height is the em. A positive height is
the cell: from the face's VDMX table where it has a group for the Windows ANSI subset (the first
size whose glyphs are that tall, or else the size before the first taller one; GDI leaves a group
made for all glyphs, as Tahoma's, unread), otherwise the largest em whose Windows ascent and
descent, each rounded, fit in the cell, with the cell's ascent rounded and its descent the rest.
A Windows family is sized by its own metrics: Malgun Gothic's cell is 1.33 of its em, so Hangul set
in it at a 12-pixel cell has a 9-pixel em. A free font drawn in place of a family whose em is its
cell (Gulim, MS Gothic, SimSun, MingLiU) gets the cell as its em, as that family would. Glyphs are
1-bit like `NONANTIALIASED_QUALITY`, embedded bitmaps are used at sizes the font has them, missing
bold is a one-pixel overstrike with one extra pixel of advance and missing italic is a shear. In
a fixed-pitch face a character twice as wide as the average advances twice the average's pixels,
as GDI keeps a full-width character two half-width ones wide (MS Gothic's Han at odd sizes). A
Windows family is hinted by its own TrueType instructions, as GDI hinted it; a free font that
stands in for one is hinted by FreeType's autohinter, since the instructions of Liberation Sans and
NanumGothic are made for grayscale and give 1-bit digits such as 0 a stray pixel on each side.

**Against GDI.** `test_font_system` makes every request the client makes, at the sizes it uses (10
to 24 pixels, the overlay's, the embedded faces'), of FreeType and of GDI on the same machine's
fonts. On a Windows 10 without Gulim and MingLiU, of its 242 requests every one GDI found by name
draws GDI's face at GDI's ppem, ascent and descent, with GDI's advance widths but for MS PGothic's
kana at odd sizes (11 to 21 pixels and 22), drawn from its embedded bitmaps, which GDI advances
some by the bitmap's width and some by the outline's, by a rule not found: FreeType's advance, the
bitmap's, is a pixel off on some characters. Drawn, a line of
text in Arial, Tahoma, Verdana, MS Gothic, SimSun, Malgun Gothic or Lucida Console differs from
GDI's in 0 to 43 pixels (479 pixels in 85 of 211 lines), single pixels FreeType's rasterizer sets
or leaves where GDI did not, and a line of MS PGothic's kana in up to 274. The `hangul_text` scene, with Hangul, symbols and kana
in the chat window, the names and what the actors say, differs from the GDI client's capture in
483 pixels, all but two in its two runs of kana.

**Not as GDI drew it.** A face GDI did not find by name was its font mapper's choice for the
charset (on that Windows Ebrima, for MingLiU's Chinese charset), which the client does not imitate:
it takes the first installed font that covers the charset's language, as a free font standing in.
The embedded EOT faces are drawn as GDI drew them, in Arial at an em of 13 pixels, but 1-bit, where
GDI smoothed them as the system smooths text (they asked for proof quality). The serial number was
drawn in a new DC's default font, GDI's System, a raster font that is not among the registry's
fonts; FreeType draws it in face 0. Invalid UTF-8 bytes draw as U+FFFD, one per byte, as
`core/utf8.c` steps over them; Windows draws one per invalid sequence.

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
Granny runtime but the game's 32-bit `granny2.dll`, so no build of the client uses one, Windows
included. `tools/granny_export` converts the game's `.gr2` files once, with `granny2.dll` on
Windows or under Wine, to glTF 2.0 files in `data\model\gltf`, named by the CRC-32 of the `.gr2`
they came from; `resources/granny_gltf.c` implements the Granny calls the client makes (file info,
poses, controls, the meshes' vertices with their bones and weights, textures) on those files, found
by the CRC of the bytes the client loads; the device skins the vertices (*Renderer*). Without them
the models are not drawn: each load shows the client's "Unable to load file .gr2
(C3dGrannyModelRes :: Load)" box, and the first also writes a line naming the missing file to the
debug output (`OutputDebugString` on Windows, seen only with a debugger attached; stderr
elsewhere).

Against the last client that used `granny2.dll` (the 32-bit build of `6b176290`), the 64-bit scene
harness on the same clock captures the Granny scenes with converted models like this (pixels whose
largest channel difference is more than 1, and more than 16):

| capture | more than 1 | more than 16 |
|---|---|---|
| `granny_dead` | 0 | 0 |
| `granny_dying` | 292 | 16 |
| `granny_models` | 7,058 | 365 |
| `guild_flags` | 0 | 0 |
| `guild_flags_later` | 2 | 2 |

In `granny_models` the emperium, a guardian and the treasure box differ over their surfaces by a
few steps of shading and by more along edges inside them; in `granny_dying` along the edges of a
guardian and the treasure box ([tools/granny_export](../tools/granny_export/README.md), and *One
64-bit Windows client* in the [review ledger](custom-review.md)).

### Window, input and IME

Every platform's keys, characters and pointer go through one module, `core/user_input.h`: the
SDL layer and the scene scripts both describe a key transition the same way (virtual-key code, the
Win32 VK numbering in `platform/vk.h`; scan code; repeat; Alt held) and hand over pointer moves,
buttons and wheel notches in scene pixels. The module packs the `lParam` scan-code layout that the
window manager and edit boxes read, sends a press with Alt held, or F10, to the window manager's
system-key handling (the original's `WM_SYSKEYDOWN`) and everything else to
`core/window_events.c`, and tells `platform_note_key` and `platform_note_cursor` the key and
pointer state, which the platform layer keeps. A character comes on its own only as Backspace or
printable ASCII (`language_on_char`); other text arrives as UTF-8 through `language_commit`. The
key state is what the window's key events said, so a key held while the window gets the focus
counts once it is pressed again, and the lock keys' toggles start off.

`core/app_sdl.c` is the client's window on every platform, Windows included since 2026-11-11 (the
Win32 window procedure and message loop of `core/wndproc.c` and `core/app_win32.c` are gone). It
creates the SDL window, which the GPU device claims and presents into (*Renderer*), and turns SDL
events into that input: Print Screen
only as it goes up and Backspace also as a character, as Windows delivered them, the keypad with
NumLock off as the navigation keys Windows names it by (Home, End, the arrows, Page Up and Down,
Insert, Delete, Clear), the window activation, one close for SDL's close request and the quit that
follows it, the client's own cursor taken out of the scene when the pointer leaves the client area
(the system cursor is hidden over it, `SDL_HideCursor`), and touch screens through `core/touch.c`
(*Android client*). AltGr types characters as Ctrl+Alt does, and Windows reported it to the Win32
client as a left Ctrl and a right Alt, so that edit boxes took the keys pressed with it as
characters (`altgr_down` in `ui/wndmgr/key_down.c`); SDL reports it as `SDLK_MODE` on X11 and
Wayland and as a right Alt alone on Windows, and `app_sdl.c` turns either into Ctrl and Alt held,
with the keys pressed with it carrying no Alt. The other keys follow SDL's: letters and digits by
the keyboard layout, the rest by their US positions, so a punctuation key of another layout has
the virtual key of the US key in its place (hotkeys bound to it with the Win32 client may now sit
on another key), the AZERTY `,` key, in the US `M` position, gives `M` too, and the ISO `<>` key,
Apps and the Hangul and Hanja keys give none and are not passed on.
Composition events go to `language_set_composition` and committed text to `language_commit`, both
UTF-8 as the client's text is; the edit boxes' `language_place_candidates` reaches SDL's text input
area on the desktop, so that the IME opens its candidate list below the composition's line, as the
Win32 client placed it. On a desktop the window is created hidden, its frame's corner put at
`Window_XPos` and `Window_YPos` as `platform_window_rect` saves them, and then shown; X11 knows the
frame only once the window is shown, so there the window is moved by it after showing (not tried on
X11).

On Windows `app_sdl.c` also does what the Win32 window did beyond SDL's defaults: the click that
activates the window reaches the game (`SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH`), the window class keeps
the name `Ragnarok` and the icon of the resources (`SDL_RegisterApp`, `SDL_HINT_WINDOWS_INTRESOURCE_ICON`),
and a window that is not active shows its frame again at once when it is moved. SDL's Windows
message hook takes a press on the caption, so that the client moves the window itself from its own
loop and the game runs on while it is dragged, where Windows' move loop would stop it; a modal loop
of the window's system menu counts as inactive (`app_activate`). The hook also tells a right Alt
that came with the left Ctrl Windows sends for AltGr, which SDL leaves out, from a plain right Alt,
and it queues the keys the IME takes in Hangul or kana mode, which Windows reports as
`VK_PROCESSKEY` and SDL leaves out too, so that they reach the client in order with their scan
codes and the hotkeys still find their letters (`ui_window_mgr_classify_key`); the IME still gets
them. Fullscreen is SDL's own fullscreen window, as on the other platforms (*Display modes* below). The executable's manifest (`res/app.res`, resource
`RT_MANIFEST` 1) declares per-monitor (v2) DPI awareness, so SDL's window coordinates are the
display's pixels on every display, whatever its scale, and Windows never stretches the window. The
IME's composition and candidates come from SDL's handling of `WM_IME_*`, which
places the candidate list with `CFS_EXCLUDE` around the text input area and reports the cursor in
UTF-16 units (the same as characters for Hangul); the password box still switches the IME to
alphanumeric input through the window's input context (`platform_ime_alphanumeric`). The window sets
`SDL_HINT_IME_IMPLEMENTED_UI` to `composition`: without it SDL does not report the preedit text
and the IME draws its own window. SDL reports the end of a composition (an empty editing event)
before the committed text, and the key that caused the commit never arrives as a key event, so a
commit right after a composition is taken as IME text whatever the key-as-text state of the
window manager. With IBus and `ibus-hangul`, Hangul chat was typed, corrected with Backspace
through and past the composition, and sent in game (`SDL_IM_MODULE=ibus`, `XMODIFIERS=@im=ibus`).

`platform/window.h` holds the remaining window services, `platform/window_sdl.c` on every
platform: message boxes, key and mouse state, clipboard text, cursor clipping, window and monitor
rectangles, window placement in the windowed and the fullscreen mode, and IME cancellation.
They take SDL's window and display id (`platform/window_handle.h`: `PlatformWindow` and
`PlatformDisplay`), so the SDL layer's window is the client's main window (`g_hMainWnd`) and the
device's, with no cast and no copy of it in the platform code. The display
queries follow the window: the window's display is the one SDL has it on, so fullscreen covers,
and a window coming out of it is centred on, the display the window is on, not always the
primary one. Windowed placement takes the corner of the window's frame, as the Win32 window did,
where SDL places the client area inside it; on X11 before the window manager has framed the window
the frame counts as none. Message boxes and the clipboard take
the client's UTF-8 as SDL does (a message box shows text that is not UTF-8 as CP949, the
resource-name encoding, as Windows does), and the message box shows the error, warning or
information icon the caller asked for and ignores the other flags, `MB_TASKMODAL` among them;
`tests/test_platform_window.c` checks the clipboard in Korean, Japanese, Cyrillic, Western and
emoji UTF-8, and U+FFFD for invalid bytes. The device learns what covers the window (*Android
client*) from `gfx_set_cover`, which the SDL layer calls once a frame.

**Display modes.** The client is windowed or fullscreen, on every platform
(`render/display_options.h`, the `display-options` library), and draws at its window's client size
in pixels: the window is created with `SDL_WINDOW_HIGH_PIXEL_DENSITY`, the device takes
`SDL_GetWindowSizeInPixels` (`platform_client_rect`), window coordinates for a size in pixels come
from `platform_window_units`, and the pointer, in SDL's window coordinates (points on macOS and
scaled Wayland), is mapped to the device's pixels through `present_point` (`render/present_fit.h`).
Windowed, the window has a frame, the player can resize it by its border, down to 1024 by 664 pixels
(`platform_window_minimum`, set again, rounded up, when the window goes to a display of another
pixel density), and maximise it (its button, or a double click on the title bar). `WIDTH` and
`HEIGHT` keep the last size the window had when it was neither maximised nor fullscreen,
`Window_XPos` and `Window_YPos` where its frame was then, and `ISMAXIMIZED` whether it was
maximised; a window opens at that size and place, made to fit the room its display has
(`display_fit`), and is restored to them. Fullscreen is SDL's fullscreen desktop
(`platform_window_fullscreen`: `SDL_SetWindowFullscreenMode` with no mode, then
`SDL_SetWindowFullscreen`): a borderless window over the display the window is on, at the display's
own mode, so the display never changes mode. Once a pump (`display_follow_window`,
`render/resize.c`) the device follows the window's size through `gfx_resize`, whoever changed it
(the player, the settings, SDL, the system), and the display options follow what the window is,
fullscreen included; when the device does not take a size, it tries again once the window's size
changes, and until then presents the picture where `present_fit` puts it. The 3D view
keeps its vertical field of view, so a wider window shows more of the world sideways, and the UI
windows keep their distance to the nearer edge of the screen, or stay centred, and are kept on it.
`set_display` puts the window in the mode the graphics settings ask for. While fullscreen, the
window is made to cover its display again when the display's own resolution changes
(`SDL_EVENT_DISPLAY_DESKTOP_MODE_CHANGED`; SDL itself makes a fullscreen window moved to another
display cover that one), except on Android, whose window SDL keeps at the screen's size. A client
started in fullscreen goes fullscreen on the display its saved windowed position is on, or the
primary display when no display holds it, before its window is first shown. The graphics settings'
resolution picker is disabled in fullscreen and shows the display's size; windowed, it shows the
window's size and offers the common sizes from 1024x768 up whose window, frame included, fits the
work area of the display the window is on, and the largest that fits; a size picked there restores a
maximised window and is set when the player presses Apply and confirms. The device draws on the GPU
SDL picks; a window on a display another graphics card drives is copied across by the system. While
the player drags the border on Windows, Windows runs its own sizing loop, in which the
game does not draw; it draws at the new size when the drag ends.

### Renderer

`render/gpu_device.c`, the graphics device (the library `gpu-device`), implements `render/gfx.h`
on SDL 3's GPU API on every platform: Vulkan with SPIR-V shaders on Windows, Linux and Android,
Metal with MSL on macOS, the one shader format of the platform handed to `SDL_CreateGPUDevice`, so
that SDL has one driver to choose. Without a GPU device the client does not start ("Cannot init
d3d OR grf file has problem.", with SDL's reason in the log). It knows nothing of maps or actors:
the static world (`render/gpu_world.c`, the library `gpu-world`) is a layer of it, which records
into the device's frames through the hooks it registers (`render/gpu_layer.h`, `gpu_add_layer`;
`c3d_device_init` attaches it, and `tests/test_gpu_layer.c` checks the order the device calls them
in). `tests/test_gfx_gpu.c` drives the two through the gfx API and reads the screen's pixels back
with `gfx_read_frame` (the tests `gfx_gpu`, `gfx_gpu_msaa` and `gfx_gpu_present`): clears, Direct3D 9's
pixel centres and top-left rule, every texture format, texel centres, culling, textures changed and
released within a frame, the alpha test, the fog (the quads' at the first rhw of every step, and the
one before it, against the original's CPU formula), blending, the effect modes' blending and the 16
effect presets against Direct3D 9's texture stages, two textures, the ground's records with and
without lightmaps, two-pass alpha, effects behind depth, composites into a target's texture and that
texture drawn, clears inside a pass, the overlays drawn as one batch, mipmaps, layered textures read
as the same texels in plain ones (filtered and with mipmaps, several layers one draw, a layer given
up and taken again, arrays of 16 MB), lists of triangles, render targets, a composited window's
target drawn as its quad draws it (every pixel its own texel), system-surface blits, copies between
targets, the screen wobble recorded between other passes of a frame, the screen read whole and in
part, Granny's models skinned by the device (an actor's meshes running on as one draw, one mesh
in two poses as one instanced draw, rigid meshes on their bone or lying still, a bone a vertex
weights by 0 that is not a number, modes and targets that take no meshes, a model given up while its
draw is recorded, the fog by the vertices' depth within a step at every step of both fog tables),
the static world's records (each cull mode, one by one and culled by the GPU, from an odd index),
its transform slots, animated ones too, its texture-coordinate matrices and texture arrays, the water's cells, the GPU's cull of a pass run by
run against a frustum narrower than the screen (an animated record moved into it and out), the
world's buffers and arrays grown while holding what earlier frames put up, an array layer's mipmaps
downloaded level by level against the same texture's own, the frame's shadows as the device fits,
culls and draws them (a caster whose texture's alpha drops half of it, the ground in its shadow and
lit around it, the baked shadows kept by a frame whose receivers all lie beyond the shadows' reach
and by one without shadows, and the fit's history kept from frame to frame and cleared with the
world), and a world cleared after its draws were recorded, without and with multisampling, and the
window not claimed while hidden, and claimed and presented into once shown where the video driver
allows it.

Draws are made in draw modes (`render/draw_modes.c`, the library `draw-modes`, tested by
`test_draw_modes` against the render states the Direct3D 9 client set for each of its lists): each
mode is one combination of blending, depth test and write, alpha test, fog, what the two texture
stages make of the textures and the vertex colour, how the stages sample, and which targets it
draws into. The device makes the pipelines of every mode, for every kind of target it draws into,
once, when it knows the screen's samples and depth (147, 148 with multisampling, in a few
milliseconds where the driver keeps its shader cache): the effect modes, whose quads carry their
effect file's D3DBLEND factors (taken whole) and texture-stage preset, get one pipeline for each
pair of the eight blend factors a target without alpha of its own distinguishes. Quads are batched
until what they are drawn with changes, as Direct3D 9's draws were until a state changed. Fog is
computed in the vertex shaders and interpolated across the triangle: for the static world's
records from the view depth as the Direct3D 9 client's mesh shader did; for the quads the
original's vertex fog, the 1/255 step its CPU computed from a vertex's rhw, which the shader
computes from the rhw within a step of the CPU's (`src/render/shaders/quad.hlsli`, `vertex_fog`).

A frame is recorded, not drawn. Each draw becomes a record of what its mode and the draw give the
shaders (the combination, the alpha test, the fog, the viewport scale; `struct GpuDraw`) and goes
into a list of passes, one per target (the device records the depth pass and the shadow maps' pass
itself, from the static world's records); what it draws, and the texels of the textures it uses
that changed since they last went up, widened to B8G8R8A8 (`render/texel_convert.c`, the
`texel-convert` library), go into the frame packet's transfer buffers (below). Every quad and triangle the client draws, the
static world and Granny's actors aside, is an instance (`render/gfx.h`, `gfx_draw_quad` and
`gfx_draw_vertices`): a record of the frame's quads (`render/quads.c`, the library `quads`) from
which the quad shader's vertex shader makes the vertices the client's C code made before, within
the tolerance below: a sprite's from its quad's rows, their rhw and its turn
(`render_object_submit_spr_quad`'s vertices); a particle's, an effect primitive's triangle's and a ground cell tile's (the cursor's
cell, ground skills' areas, the navigation path and its marker) by projecting its points as
`renderer_project_vertex` did, with the rhw and depth rules the client applied to those vertices
(the rhw added to or set, the depth set or moved, an effect quad stood up on the screen about its
point), the CPU computing their rhw, which the lists sort by and which say whether one lies flat
(and for the particles and ground tiles their z, below); and the rest, which the client builds on the screen, from the client's own vertices, an
axis-aligned rectangle with one rhw and colour into a short record of its own. An effect
primitive's triangles name corners of a set (`struct GfxCorners`) that the triangles of one quad
share, put among the frame's quads by the first of them drawn, so that a triangle's record is a
header of 16 bytes. The frame's quads, one storage buffer, hold the records and what they name:
the matrices projected points go through, the corner sets and the client's vertices. A draw is a
run of instances made with the same mode, effect, fog, textures and target, all flat or none, drawn
as one instanced draw of the triangles (0, 1, 2) and (2, 1, 3) of their corners. The sprite atlas's
pages are layered textures (`gfx_create_layer_texture`), which the device places as layers of
texture arrays when a draw first reads them, so that sprites of several pages are one draw; the
fragment shader reads stage 0 from the array where the draw's record says so, at the layer the
instance gives. An array holds pages of one size and number of mipmap levels, as many as make 16 MB
and no more than 64 (64 of the atlas's 256 by 256 pages; one of 2048 by 2048 or more, as large as
it would be alone), another is made when it is full, and it goes with its last page: on a map
change only the arrays holding the atlas's spare pages (up to 50, kept from map to map) stay. A
texture or array the GPU has no memory for is not tried again: its quads read white. A frame is
recorded into a packet of its own, one of a ring of four (`render/frame_ring.c`, the library
`frame-ring`): its lists of passes, draws and uploads, and its records, quads, offsets and texels,
written once into transfer buffers of the packet's own, mapped while the frame is recorded. When
the frame is presented, or the screen's picture is read (`gfx_read_frame`: screenshots and the
scene harness's captures, after the frame has ended), the main thread waits for the render
thread to finish the frame handed over before, makes the frame's buffers large enough (the static
world's meshes and textures go up then, through a stream of their own, below), and hands the packet
over with every GPU object its passes use as they are then; the render thread
(`render/render_thread.c`, the library `render-thread`, one of SDL's threads) records it into one
command buffer, one copy pass from the packet's transfer buffers, the static world's compute passes
(below) and the passes, and submits it, while the main thread records the next frame. A GPU object
the main thread gives up (a texture released, a buffer grown, the screen resized) goes with the next
frame handed over and is released once the render thread has submitted that one, since a frame
handed over before may still record it; a surface or Granny model given up while a frame draws with
it goes when that frame is handed over, since the render thread reads none, only the GPU objects
the packet holds. The main thread presents, in a command buffer of its own, which SDL wants on the
window's thread: it takes the window's next picture (waiting for one with vsync) while the render
thread finishes the frame handed over before, then puts the screen, as that frame left it, into the
picture, fitted by `present_fit`, and only then hands the new frame over, so a frame is shown at the
next present, as Direct3D 9's driver queued a frame; nothing is shown until a frame drawn into the
screen as it is (after a resize) has been submitted. SDL's Vulkan driver moves textures and buffers
to other memory when a command buffer is submitted (every submit while no window is claimed, the
presenting ones while one is) and a pass takes an object's memory as it is recorded, so command
buffers are recorded one at a time under a mutex of the device's: the render thread's frame from
acquire to submit, the main thread's own (the present's blit, the static world's stream, the
reads) and the one SDL submits for each texture it makes. A device is made whole or not at all, and
a frame the render thread could not submit puts back its uploads and takes the next frame with it,
which was recorded on them having gone up. A read of the screen hands the
frame over with a download of it (`SDL_DownloadFromGPUTexture`), which the render thread waits for
and copies out while the main thread waits, so it reads the frame just drawn. Nothing else is read
back: a render target's pixels never reach the CPU (`gfx_lock` takes only textures and system surfaces, and `gfx_blit` copies from
a target only into a target). The shaders read the records from storage buffers, at most four a
shader stage, the least every Vulkan device takes (`STORAGE_BUFFERS_MAX`; SDL tells nothing of a
device's limits, so the device refuses a shader that reads more, a compute pass's written buffers
counted with the rest). What a shader needs is packed into four: the static world's cull reads one
buffer of words holding the records' bounds, the runs of the view and of the casters, the casters'
order and the chunks (`render/world_layout.c`, `wl_cull_layout`), and writes one holding the
indirect commands and the kept records' places (`wl_draw_layout`); the water's cells and its table
of sines share one; the quad and skin shaders read three. A quad finds its draw's record through
the header of its own record, a Granny actor's draw through the header of its pose's instance, a
draw of the static world takes its record as a uniform, the wobble finds its record through an
instance-rate vertex attribute that holds its index (a draw's first instance), never through the
instance ID. Textures keep their pixels on the CPU in the client's own format, so the model
loader's thread creates, locks and fills them without calling SDL; the main thread stages them
into the frame's texels when a draw first uses them after a change, and the render thread records
their upload, as the rectangles that changed (`render/dirty_rects.c`,
the library `dirty-rects`: a texture keeps up to eight apart, merging those that touch, and where
there is no room the two whose union wastes least, so that the cells written on a sprite atlas's
page go up each alone and not the page), and a texture changed after a draw used it in the same
frame takes its new texels in a copy pass of its own between the render passes, so that the earlier
draw keeps the old ones. Mipmaps are made on the GPU (`SDL_GenerateMipmapsForGPUTexture`) whenever
the texels of a texture that has them go up; a layered texture's top level is copied on the GPU
from its layer into a scratch texture of its own, the rectangles that changed go up over it, SDL
makes its levels, and they are copied into the layer, so that a layer's levels are a texture's on
every backend while only what changed goes up. A texture not yet sent goes up as what was written
on it, its other texels cleared on the GPU to what its zero pixels widen to (`clear_texels`). The
windows that composite (the menu icons, the party plates) draw into a render target of their own, which their quad reads as a
texture (`texture_create_target`, `render/render_clients.c`). The screen wobble (the player's
status 0x108) is a step of the frame: a copy of the screen's picture so far, then a pass over the
whole screen that draws it again with each row taken from the row and the shift a table gives
(`gfx_wobble`, `src/render/shaders/rows.hlsl`), every sample of a pixel alike, so that a
multisampled screen resolves to the texels exactly; the CPU computes the table from the same random
numbers, drawn in the same order, as the original's loop over the locked frame
(`render/screen_wobble.c`, the `screen-wobble` library); where the device cannot draw the wobble,
the table is not computed, so no random number is drawn, as the original drew none when it could
not lock the frame. The device claims its window, and makes a swapchain for it, at the first
present while the window is shown, not minimized and of a size in pixels, and a claim counts once
SDL keeps the window (SDL 3.4's Vulkan driver answers a claim of a window whose surface has no
size, a minimized one on Windows, with success but keeps nothing of it); a claim that fails or is
not kept is tried again at the next present (on Android the window has no surface while the app is
in the background). A window never shown (the scene harness's) has no swapchain, its frames are
drawn, end at each present and can be read all the same, and a fence on each frame keeps the render
thread at most two frames ahead of the GPU, and the main thread one ahead of the render thread,
with or without one. The render thread waits on the fence of the frame two before before it takes
a command buffer: SDL's Vulkan driver gives each command buffer the descriptor sets of one it has
found finished, and a command buffer that finds none, as after textures were made (SDL sets a new
texture's layout in a command buffer of its own), makes a descriptor pool for each kind of binding
the frame binds, which took NVIDIA's driver up to 19 ms with the shadows' passes. A packet's
transfer buffer of more than 4 MB that its frames have filled a quarter of at most, 120 times in a
row, is made again as large as they need. The main thread makes the GPU objects while the render
thread records; SDL's Vulkan and Metal drivers lock what that shares (their command buffer pools,
allocators and lists of objects to release), which SDL's API does not promise. The
present mode is vsync unless `RAGEXE_VSYNC=0`. A frame the client shows without waiting
(`renderer_flip` without vsync, as the map loading screen's are) never waits for the window: SDL's
acquire of a picture waits for the presentation engine even when told not to block, so such a
frame asks for one only once a refresh of the display (60 Hz where its mode does not say) has
passed since the last picture, and otherwise is drawn but not shown, as Direct3D 9's flip without
waiting left a frame the screen had no room for. The device asks Vulkan for none of the features
it does not use (depth clamping, clip distances, indirect draws from a first instance), so GPUs
without them run it. A GPU device is never lost: the client has no lost-device handling, and a
window size the device has no memory for is tried again when the size changes.

Pixels: every vertex lands where Direct3D 9 put it, within the shaders' tolerance (below). Every
pass's viewport is its target moved by half a pixel, (0.5, 0.5, width, height), so that Direct3D 9's pixel centres fall on the target's
(the wobble's pass, which draws whole pixels of a copy of the screen, covers the whole target from
0, 0);
since a viewport must lie within its texture (SDL's GPU API, Metal), a target's textures are a pixel
wider and taller than the target, and a stage that samples a target scales its texture coordinates
to match and, when it clamps, keeps them off the margin. The static world's records take the
matrices the Direct3D 9 client drew with, which the GPU computes as its CPU did, and the GPU
divides and moves their points as Direct3D 9's did; the casters take the cascades' matrices, which
the GPU fits (below).
Pre-transformed vertices are snapped in the vertex shader to the 1/256 of a pixel Direct3D 9's
rasterizer snapped them to, rounding to even, and handed over a 1/1024 of a pixel further on, so
that the GPU's own viewport transform, which lands within a few ten-thousandths of a pixel of that
point, snaps to the same place whether it rounds or truncates
(`src/render/shaders/records.hlsli`, `snapped_ndc`); they cover the pixels Direct3D 9 covered, with
Direct3D's top-left rule. Their `w` stays 1 so that nothing divides the position, and the shader
interpolates colours and texture coordinates perspective-correct itself, with the rhw. The shadow
lookups read the maps by texel, the one point sampling picked. Like Direct3D 9's own viewport, the
passes' viewport leaves out the left half of the first column and the top half of the first row,
which a GPU that clips to it (Mesa's lavapipe) leaves uncovered when multisampling. The static
world's index buffer holds 32-bit indices, so that a draw may start at any index where Metal takes
an index buffer's offset in multiples of 4 bytes. The shadow maps are four layers of one R32F array
texture. Multisampling (`RAGEXE_MSAA`: 2, 4 or 8 samples, 4 when unset, 1 for none; a count the GPU
does not take falls to the next lower one it does) resolves the screen at the end of each pass on
it, and alpha-tested draws without blending use alpha to coverage. Direct3D 9 drew flat draws (every
vertex the same rhw: the UI, sprites) without multisampling, which SDL's GPU API cannot switch off
for a draw: in a multisampled pass the vertex shader moves a flat draw's corners onto the pixel
boundaries that keep the pixels Direct3D 9 covered, with the values their triangle has there, as the
CPU moved them before (`render/shaders/instances.hlsli`, `snapped`), so edges along the pixel rows
and columns come out as Direct3D 9 drew them and only slanted ones are smoothed. Direct3D 9's
transparency anti-aliasing was the driver's (`SSAA` or `ATOC`); on NVIDIA its supersampling shaded
every sample of each alpha-tested draw without blending, which the ground's and the models' are, so
with multisampling on their textures were read at each sample, where alpha to coverage reads them
once a pixel.

The static world (`render/gfx_world.h`, `gfx_world_*`, and `render/gpu_world.c`, the library
`gpu-world`; `render/static_world.c`, the library
`static-world`; `render/world_layout.c`, the library `world-layout`, which packs it as the shaders
read it; and `world/world_gpu.c`) stays on the GPU while a map is shown: the ground's blocks and the
models' nodes in one vertex and one index buffer, each mesh placed behind the others as the map
loads and its models stream in, with its own 16-bit indices widened to 32 bits and a base vertex;
their textures and lightmap pages in texture arrays, one for each size and number of mipmap levels
(Vulkan promises 256 layers an array; a size with more takes another array), each made with as many
layers as the map's first frame places in it and made again with the power of two at or above the
layers placed where more come;
a draw record for each run of one texture of a block or node; and a transform slot for each model
and node, with its matrix and its parent's slot. The meshes and the textures go up through a
transfer buffer of their own, 32 MB (or the largest texture), a batch at a time in a command buffer
of its own that is waited for before the next fills it, the textures' pixels widened straight from
their surfaces, and each layer's mipmaps made as a texture's are: its pixels go up into a scratch
texture of their size, SDL makes its levels (`SDL_GenerateMipmapsForGPUTexture`) and each level is
copied into the layer, as the sprite atlas's layers are, so that a layer's levels are a texture's
on every backend. A texture reload (`texcache_reload_all`) makes the device's world again from the
new surfaces. Each frame three compute passes run before the render passes: one computes every
slot's view and its place in the world from the camera's view, the model's matrix, the nodes' own
matrices and the frame's matrices of the animated nodes, as the CPU computed a node's matrix
(`world_transform.hlsl`); the next culls the records against the frustum of the view, a group of
threads to each run of records that share their texture array, lightmap array and cull mode,
giving each kept record its place in the frame's index buffer of them, one after the other, and
writing the run's one indexed indirect draw (`world_cull.hlsl`), since SDL takes the count of
indirect draws from the CPU; the last, a group of threads to each 64 indices of a record of those
runs (`WlChunk`, fixed with the records), copies them there where the record is kept, its base
vertex added, and leaves at once where it is not (`world_compact.hlsl`), so that no group copies a
long run alone and no thread searches for its index. The ground and the opaque models are then one
indirect call of one command for each run. A vertex carries its record's number in a buffer beside
it, the indirect draws having no first instance to tell it (the device asks for no
`drawIndirectFirstInstance`). A draw of the world is recorded with the world's generation and left
out where the world was made again before it went up; uploads that fail make the world again, and
after two in a row the device takes no more of it until the next map. The water's cells are kept
likewise, and the vertex shader raises each by the frame's waves (`water.hlsli`).
`test_static_world_maps` holds the passes' results to the tolerance (below) of the C on real maps:
every vertex through its slot, the records each cull keeps but those whose box lies within the
rounding of a plane, and the water's quads.

The shadows are the static world's layer's (`render/gfx_world.h`, `GfxShadow`): the client gives each frame the
light, its view and the box the static world's draw records lie in, the animated ones in the
frame's pose (`render/shadow_fit.h`, `ShadowFitFrame`), and the CPU never sees the cascades. After
the view's cull a single-sample pass draws the runs it kept (`world_depth.hlsl`) into two R32F
targets of view depth at the screen's size: the nearest receiver of every pass, and the nearest of
the opaque passes and the ground, so that a pixel's receivers lie between the two (the ground shows
through an alpha-blended face in front of it). Three compute passes fit the cascades as
`render/shadow_fit.c`, their reference, does, in groups of at most 64 threads:
`shadow_range.hlsl` takes the nearest and farthest depth on screen within the shadows' reach,
`shadow_extents.hlsl` cuts a range kept from frame to frame into four slices, each slice's far depth
the same multiple of its near one, and takes each slice's extent across the light, and
`shadow_cascades.hlsl` makes each cascade's square, its centre on whole texels, with its matrices,
lookups and the planes of its box. The range and the squares are
kept in a buffer of the device's until the world is made again, so that the camera's moves seldom
move a slice or resize a square (`render/shadow_fit.h` gives the rules). The cull pass then culls
the casters' runs against each cascade's box as `sw_cull_cascade` does (the records ordered by
texture array and in draw order within each, a run each stretch of that order that one texture's
alpha test decides or none does, `wl_caster_runs`), the compaction gives each cascade's kept records
their places in a section of their own, and the shadow pass draws them into the four layers of one
R32F array, a draw a run a layer: the models' textured records with a pipeline that drops their
texels by the alpha test, the ground and the records without a texture with one that cannot drop
any, which keeps the GPU's early depth test. Every draw of the world reads the fit's result, which also says how many
cascades receive: where nothing within reach receives, the ground keeps its baked shadows, as it
does on a frame whose shadow passes could not be recorded. Shadows are optional: a device that
cannot make the fit's pipelines, buffers or shadow maps draws without them, and one that cannot
make the fit's targets at the screen's size draws without them until the size changes. The range,
the range kept and, given the same split, the extents, the squares' sizes and the texels their
centres lie on are the C's to the bit on every GPU, as they rest on additions, subtractions,
multiplications, comparisons and divisions by powers of two, which Vulkan rounds exactly, and on a
snap to whole texels that an exact correction makes independent of how the GPU divides; the split
and the matrices are within the rounding Vulkan allows a division or a square root.
`test_shadow_fit_gpu` and the scene harness (`RAGEXE_SCENE_SHADOW_FIT`) hold them to that.

Granny's actors (the emperium, castle guardians, treasure boxes and guild flags of
`actors/granny.c`; `render/gfx.h`'s `gfx_create_skin`, `gfx_skin_pose` and `gfx_draw_skin`;
`render/skin_layout.c`, the library `skin-layout`, which packs what the skinning shader reads) are
skinned, lit and projected by the device. A model's meshes go up once, when a draw first takes the
model, as one vertex buffer (each vertex its position, normal, texture coordinates and up to four
bones of the skeleton with their weights, 56 bytes; a rigid mesh's on its one bone) and one index
buffer of 32-bit indices, one mesh behind the other with its first vertex added, so that an actor's
meshes that follow one another with one texture are one draw; a copy stays on the CPU, as a
texture's pixels do, for a device made again. Each frame the CPU poses each actor's bones as before and hands the device one pose among the
frame's quads (`skin_pose_rows`: its bones' matrices, its meshes' matrix into view space, the light
it is lit by, the projection), and each mesh is queued where its triangles were, in the opaque list
by texture. The vertex shader (`skin.hlsl`, `skin.hlsli`) sums each vertex's weighted bones, lights
it by its normal as the CPU did (rigid meshes by the normal they lie with) and projects it as
`renderer_project_vertex` projected the CPU's corners, to the pixel and depth the CPU gave them; the
fragment shader is the quads'. Consecutive draws of one model, mode and textures are one draw: an
actor's meshes that run on in the model's indices, or one mesh of several actors, instanced, each
instance a header among the frame's quads naming its pose and the draw's record.
`tests/test_skin_instances.c` compares every corner of every model in the game folder's converted
files, in their animations and fuzzed poses, with the CPU code it replaced
(`tests/fixtures/granny_reference.c`) within the tolerance (below), and draws actors both ways
through `render/gpu_device.c` within the captures' budget. A model whose skeleton has more than 256
bones, which the shim samples no animation for, is not drawn.

The shaders are HLSL in `src/render/shaders` (`quad.hlsl` for the quads, which builds their vertices
from their records, `instances.hlsli`; `skin.hlsl` for Granny's actors, `skin.hlsli`; `world.hlsl`
for the static world's records with their shadows, `world_depth.hlsl` for their depth on screen that
the shadow fit takes, `world_caster.hlsl` for its casters in the shadow maps and
`world_caster_solid.hlsl` for the casters without an alpha test, `water.hlsl` for the
water, which draws its pixels as the quads draw theirs (`quad.hlsli`: the texture stages'
combinations, an atlas layer, the vertex fog and the alpha test); `rows.hlsl` for the screen wobble,
and the compute passes `world_transform.hlsl`, `world_cull.hlsl` and `world_compact.hlsl`;
`shadow_range.hlsl`, `shadow_extents.hlsl` and `shadow_cascades.hlsl`, the shadow fit; and what they
share in `records.hlsli`, `quad.hlsli`, `instances.hlsli`, `skin.hlsli`, `world.hlsli`,
`water.hlsli`, `projection.hlsli` (the client's 4x3 matrices and its projection of a point),
`shadow_fit.hlsli`, `cascades.hlsli` (the fit's result, as the world's shaders read it) and
`world_caster.hlsli`; test
programs' own shaders in `tests/shaders`).
The shaders are plain arithmetic: they give the floats of the C code they stand for within a
tolerance, not to the bit (`plans/gpu-renderer.md`, Decisions). A vertex lies within 1/16 of a
pixel of the C's, its rhw within 1e-5 of the C's relative, its depth within 1e-5 relative from
twice the near plane on and within 1e-5 of the depth range nearer than that (where a bound relative
to the depth would measure the C's own cancellation, and a depth buffer compares absolute depths),
its texture coordinates within 1/4096 and its colour within a step; the C's floats are themselves
rounded, so where a test computes the exact value of the same arithmetic (in double), a GPU's value
that is not within the tolerance of the C's may be within it, and within the rounding any float
computation of it may carry, of the exact one (a point near the eye seen from a camera thousands of
units away has a view depth that is the small sum of large terms). `tests/test_quad_instances.c`
runs the quad shader's `quad_vertex` on the GPU against the CPU code it replaced
(`tests/fixtures/quad_reference.c`) for millions of records, made by the client's own functions,
and draws scenes through `render/gpu_device.c` both as instances and as the reference's vertices;
`tests/test_static_world_maps.c` and `tests/test_skin_instances.c` hold the static world's passes
and Granny's skinning likewise. Pictures are held to a budget: the pixels more than a colour step
apart at most 0.05 % of the screen, in groups of at most 16 that touch, every differing capture
looked at. A decision the GPU takes from its own floats can fall the other way from the C's where
the input lies within the rounding of a boundary: a flat draw's corner may land on the pixel
boundary next to the C's in a multisampled pass, a record whose box touches a plane of the view be
culled otherwise than `sw_record_in_view` decides, a wave's corner take the next degree, a vertex's
fog the next step; the tests count those. What decides the lists' order and which draws are flat
stays on the CPU, from its own floats, and so does the depth of what the depth test must decide by
its bits: a figure's sprite layers lie at depths that tie, and so do the particles and ground tiles
drawn where it stands, so the client computes the sprites' rows' z and the particles' and tiles'
corners' z from the rhw it has for their keys (`renderer_project_depth`) and hands them over
(`GFX_DEPTH_CORNERS`); the shadow fit's squares and texels are its C reference's to the bit
(above). The C code's floats are the same on every platform where each
operation rounds once, in the order written: MSVC's `/fp:precise` contracts nothing on x64; GCC and
Clang contract unless compiled with `-ffp-contract=off`, which the CMake builds pass (above). The
quad vertex shader is 34 KB of SPIR-V and the client's shaders 218 KB: a run of the scene harness
that starts the client and captures the login screen takes 2.0 s where NVIDIA's driver has none of
the pipelines cached and 1.6 s where it has (a GTX 970). `tools\shaders\build.bat` compiles each
with the Vulkan SDK's `dxc` to SPIR-V for Vulkan 1.0, checks it with `spirv-val`, turns it into MSL
through SPIRV-Cross's C API with the resource indices SDL's Metal driver binds, and writes
`<name>.h` beside the source: both as byte arrays, the resource counts `SDL_GPUShaderCreateInfo`
needs, the compilers' versions, and
hashes of the source with its includes, of the tool's own source (which holds the flags and the
binding rules) and of the arrays. The tool fails when a resource is not where SDL's binding rules
put it. The headers are committed, so builds need no shader compiler; `tools/shaders/check.py` (in
`build.bat check`, and the test `shader_sources`) fails when a source or the tool changed and the
headers were not made again, or when a header was edited. Run the tool after changing a shader; it
needs the Vulkan SDK (`VULKAN_SDK`) and Visual Studio's C compiler. The MSL has not been compiled by
a Metal compiler yet.

### What was verified

On Linux Mint 22.3 (a Hyper-V VM with Mesa llvmpipe) against rAthena built for packet version
20220406 and the kRO data the Windows client uses: service and server selection, login, the PIN
dialog, character creation, entering `iz_int03`, walking, chat, the inventory, equipment and skill
windows, Hangul chat through IBus, and an active audio stream. The frame overlay (`RAGEXE_OVERLAY=1`) showed about 90 fps at
1024×768 on llvmpipe. A `-DRAGE_SANITIZERS=ON` build reached the same points; the misaligned
packet-key loads and the null `memmove` it reported are fixed. The client drew through OpenGL
then. The GPU renderer (*Renderer*) has run on Linux in WSL (Ubuntu 24.04) with Mesa's lavapipe,
before Granny's actors were skinned on the GPU: `test_quad_instances` passed with and without
multisampling, `test_shadow_fit_gpu` gave the C's values to the bit, and `test_gfx_gpu` passed with
and without multisampling, the frame's shadows and the texture arrays' mipmaps, downloaded level by
level, among its checks; the scene harness, built with SDL for the console
(`-DSDL_UNIX_CONSOLE_BUILD=ON`, as it draws with the offscreen driver), drew the scene set as on
Windows before the static world moved to the GPU; the client has not been played on it.

## macOS client

The same CMake build makes `ragexe.app` with Apple clang (Homebrew `ninja freetype fontconfig`):

```sh
cmake -S . -B build/client -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_OSX_ARCHITECTURES=arm64 -DRAGE_CLIENT=ON
cmake --build build/client
ctest --test-dir build/client --output-on-failure
```

Put `ragexe.app` in the game folder and open it. Everything in *Linux client* applies: the renderer
draws through Metal; SDL provides CoreAudio and Cocoa text input; Fontconfig finds the system fonts. Arial is Apple's own copy and keeps its
hinting; Gulim is replaced by Apple SD Gothic Neo, whose last glyph can overhang the text box by a
pixel. The `macos-client` CI job builds the bundle on GitHub's macOS 15 arm64 runners and runs the
tests there; the GPU backend's tests skip themselves where SDL finds no Metal device.

The game was played on macOS 12.7 (x86_64, VMware, Apple's software OpenGL renderer, Apple clang 14
from the command line tools, MacPorts for cmake, ninja, freetype and fontconfig) against rAthena
built in the same machine: `ragexe.app` opened from the game folder, logged in, created a
character, entered `iz_int03`, walked and chatted. The 21 tests pass there as well. That was with
the OpenGL renderer the client had then; its Metal path has not been run yet.

## Windows client

`build.bat` builds `build\ragexe.exe` (`build.bat dbg`: `ragexe_dbg.exe`), a 64-bit program with
SDL's window (*Window, input and IME*), FreeType text (*Text*) and the renderer on SDL's GPU API
(*Renderer*). Until 2026-11-11 `build.bat` built a 32-bit client by default and this one with
`x64`. The 32-bit client linked three DLLs of the original that exist only for 32-bit Windows; the
64-bit client does without them, as every other platform does:

- sound goes through `platform/audio_sdl.c` in place of RAD's `mss32.dll` (see *Audio*), on a
  static SDL3 that `vendor\build_vendor.bat` downloads (the commit `cmake/SDL3.cmake` pins,
  hash-checked) and builds with CMake into `build\vendor`;
- Granny models come from the converted glTF files (see *Linux client*) in place of RAD's
  `granny2.dll`;
- GRF tables and files and guild emblems inflate, and the emblem cache deflates, through the
  bundled zlib in place of `cps.dll`'s `uncompress` and `compress`. A `cps.dll` that does more
  than zlib (GRF Editor's, for the GRFs it encrypts) therefore has no effect: a GRF whose file
  table needs it does not open (`fileio/grf_pak.c`, `load_table_v2`), so the client goes without
  its files, and a file in a GRF that needs it inflates partly or not at all and reads as truncated
  data padded with zeros (`grf_archive_read`, which does not check what `uncompress` returns).

Everything `build.bat` links, the vendor libraries included, is compiled with `/MT`: the client
carries the C runtime and imports no `vcruntime*.dll`, `msvcp*.dll` or `api-ms-win-crt-*` set, so it
needs no Visual C++ redistributable. (The 32-bit client used the 32-bit runtime DLLs the game folder
ships; a 64-bit program skips those and would need the x64 redistributable installed.)
`vendor\build_vendor.bat` writes what it built the libraries as into `build\vendor\kind.txt`, and a
`build\vendor` without that file, or with another kind, such as a checkout's 32-bit libraries from
before 2026-11-11, is removed and built again. The tests do not use `build\vendor`'s libraries:
CMake builds the ones they link from `vendor/`, and SDL and FreeType through `cmake/SDL3.cmake` and
`cmake/Freetype.cmake`, unpacked into the tests' build from `build\vendor\sdl3.tar.gz` and
`build\vendor\freetype.tar.gz`, the same pinned and hash-checked archives, when they are there.

SDL is built with its video and GPU subsystems as well as audio, with Vulkan and without OpenGL and
OpenGL ES. SDL has no switch for its Direct3D 12 GPU driver, which it builds whenever `dxgi1_6.h` is
found, so that driver is in too, but the client asks only for SPIR-V and so always gets Vulkan. The
client plays sound, runs its window and draws through SDL, which imports `setupapi.dll` and
`version.dll` for them. `build.bat` already
linked every library they need.

The crash report prints full-width addresses. `tests\scenes\build.bat` builds the scene harness,
`build\ragexe_scenes.exe`. Against the last 32-bit build its captures of the scene set are the same
bit for bit except in the Granny scenes, which with converted models differ as *Linux client* lists.
`tests\build.bat` builds and runs the tests 64-bit too, with `/MT` like the client, except the
oracle tests (`tests/oracle`), which map the original executable into their own process and stay
32-bit: `tools\find_vcvars.bat x86` gives them, and `tools/granny_export`, the 32-bit compiler,
and they link with `/MACHINE:X86` (see *Build and test*). The Windows CI job builds the client and
the scene harness and runs the suite.

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

- The client always runs in its fullscreen mode (*Display modes*), at the size of its window, the
  screen on a phone and the app's window on a desktop, and follows the window when it is resized
  or turned. SDL gives an Android window's size in the screen's pixels, so the interface is drawn
  small on a dense screen until it is scaled. The system bars are hidden.
- On a desktop (ChromeOS, Android's own desktop windowing) the app runs in a window the
  player moves, resizes and maximises; the first one is about 4:3 (`<layout>` in the manifest) and
  the system remembers the size after that. The full-screen request only takes effect when the
  window is maximised; the caption bar then stays drawn over the top of the app, so the activity
  reports how far it reaches into SDL's surface and the scene is fitted below it. Resizing, moving
  to a display of another density (`density` in `configChanges`) and maximising keep the running
  game. A mouse is a mouse there: hover, both buttons (the right one turns the camera), the wheel
  (zoom) and SDL's empty cursor over the client's own. A hardware keyboard types and plays as on a
  desktop and the on-screen keyboard stays away.
- Drawing goes through Vulkan, as on Linux: the app asks for Vulkan 1.0
  (`android.hardware.vulkan.version` in the manifest). On the Android 15 x86_64 emulator, headless
  (`-no-window`), the renderer draws the login screen through both of the emulator's Vulkan
  drivers: the host's GPU (`-gpu host`, gfxstream) and SwiftShader (`-gpu swiftshader`). The arm64
  libraries, installed there with `adb install --abi arm64-v8a` and run through the image's ARM
  translation, draw the same pixels as the x86_64 ones under SwiftShader, apart from the animated
  cursor. No map has been drawn on Android with this renderer, and no Android GPU (Adreno, Mali)
  has run it.
- Fonts: there is no Fontconfig, so `render/font_match_scan.c` reads the faces in `/system/fonts`
  and `/product/fonts` once and sorts them for the client's requests as `render/font_faces.c` says
  (the asked families in order, then for a fixed-pitch request the fixed-pitch faces, then the
  faces that cover the language, then slant and weight); Korean text is
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
monsters, NPCs and players, makes them talk, clicks windows open and captures PNGs, headless: the
client's window is never shown, so the device makes no swapchain, and a capture downloads the frame
just drawn (SDL's offscreen video driver on Linux, the platform's own, the window hidden, on
Windows and macOS; `tests\scenes\build.bat` builds it on Windows). `tests/scenes/run.py` runs every
script and compares the captures with golden images (`tests/scenes/README.md`). On the Mint VM each
of the three scenes takes 3–4 s, on Windows 2 s, and both repeat bit for bit (Windows with
`RAGEXE_MSAA=1`).

Comparing Windows and Linux captures of the same scenes and data:

- **Text** is the largest difference. FreeType draws the same text on both only where the same
  font files are on both machines; a Windows without the Korean supplemental fonts has no Gulim,
  and draws Hangul in Malgun Gothic, as GDI did, while Linux uses Noto Sans CJK KR or NanumGothic at
  Gulim's size.
- **Edges and foliage**: every platform multisamples alike (*Renderer*); set `RAGEXE_MSAA=1` on
  both for captures to compare.
- **Random effects** (level 99 auras, clouds, particles) took other paths because glibc's and
  Microsoft's `rand()` are different generators; the scene harness uses Microsoft's on POSIX,
  after which they match. Two places assumed Microsoft's 15-bit range and misbehaved on Linux
  and macOS, where `rand()` returns 31 bits: a random point in a disc divided by 32767 (particles
  landed up to 256 radii away) and the shuffle copied from Microsoft's STL rejected all but one
  draw in 65536. Both take the low 15 bits now.
- What remains is single pixels along triangle edges, where two GPUs' subpixel precision decide a
  pixel centre differently, and blending rounded one or two steps apart.

Making the captures repeat showed three draw orders that depended on heap addresses, which ASLR
and allocation timing change from run to run: the render queues sorted by texture pointer
(now by a texture serial assigned at creation), tied queue keys broken by primitive pointer (now
by submission order), and emitter particles reused in address order from a pointer-keyed map
(now in the order they were added). The multitexture sort also compared the first texture with
the other entry's second one, which is not an ordering; it compares like with like now.

## Remaining work

1. Windows: check by hand Hangul composition and the Hanja candidate list through the Microsoft
   IME on SDL's text input, the letter hotkeys in Hangul mode, AltGr on a German or French layout,
   and the display modes: fullscreen on and off, starting in it, on a second display, Alt+Tab, a
   display of another scale, and resizing, maximising and restoring the window.
2. macOS: check Hangul composition through the macOS input method and the game on a Retina
   display, where the window asks for a high-density drawable and the game draws at the display's
   pixels, so the interface is small until it is scaled.
3. Granny models need a one-time conversion with `tools/granny_export` (Windows or Wine).
4. Listen to the POSIX audio in game and compare JPEG screenshots with Windows.
5. Keep auditing pointer-width message arguments, raw strides and packed packet
   fields; the sanitizer run covered login to the first map, not every window.
6. Run the renderer on Linux (Vulkan), macOS (Metal, whose MSL no Metal compiler has seen yet) and
   Android (so far only its login screen, on the emulator), and compare their captures with
   Windows'.
7. Android: only the x86_64 emulators (phone and desktop) were played; try arm64 hardware, a real
   Chromebook and a real touch screen (the gestures were driven through a virtual uinput one), and
   play at the screen's own size, which the client draws at (*Android client*).

`g_clientDir` is filled by `platform_current_directory` with at most 0x7f bytes, as
before; a longer client directory leaves it empty on every platform.

# Portable client foundations

Work starts at `purist` commit `d725b49`. The original [macOS plan](../plans/mac-port.md)
was written against the older `herc-client/client` tree. Here its paths are relative
to the repository root. Purist has already removed Bink, embedded browsing, HTTP
services and several startup checks. Those features must not be reintroduced.

This is a subsystem build, not a playable macOS or Linux client. The complete
application remains Windows x86 and builds with `build.bat`.

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
GDI; italic text differs by a few pixels.

Not covered: the embedded EOT fonts (MicroType Express compressed; faces 20 to 28
use the charset's substitute at 13 pixels), the serial-number text, which GDI
draws in the device's default font and FreeType in face 0, and the performance
overlay, which uses Fontconfig's `monospace` instead of Lucida Console. Invalid
code-page bytes make a run disappear instead of drawing replacement characters.

## Remaining work

1. Validate the probe on macOS, especially real Hangul composition and Retina.
2. Compare JPEG screenshots in-game, listen to the POSIX audio in game, and
   stub/convert Granny as the plan describes. The IJL code and import library
   have been removed.
3. Migrate all message arguments and pointer-bearing runtime fields to pointer
   width; audit raw strides and keep packet fields fixed width.
4. Expand CMake to the application and remaining test suites; migrate filesystem,
   clock, synchronization, network, settings, text and input consumers by subsystem.
5. Implement the Metal backend behind a platform-neutral graphics seam. Linux
   will need its own graphics backend; it must not inherit Metal-specific types.

`g_clientDir` is filled by `platform_current_directory` with at most 0x7f bytes, as
before; a longer client directory leaves it empty on every platform.

No renderer, font, audio or gameplay parity claim is made by these foundation tests.

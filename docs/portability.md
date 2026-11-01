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
- `platform/filesystem.h` takes **CP949 game paths**, converts at the disk boundary
  to Windows UTF-16 or POSIX UTF-8, and normalizes backslashes. It does not change
  GRF keys or lowercase names. Invalid encodings fail instead of substituting
  characters. `platform_file_open` returns a normal `FILE *`, owned by the caller;
  close it with `fclose`. The mode is the usual ASCII stdio mode. Exclusive `x`
  creation remains atomic. Ping logs and the relative mini-party settings path
  are the first migrated consumers.
- Native paths from an OS file picker are a separate input domain: do not pass
  UTF-8 strings to the CP949 interface. A future native-path API must preserve
  characters outside CP949. Exact case is currently required on case-sensitive
  filesystems; a game-data case lookup has not been implemented.
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

## Remaining work

1. Validate the probe on macOS, especially real Hangul composition and Retina.
2. Compare JPEG screenshots in-game, replace Miles, and stub/convert Granny as
   the plan describes. The IJL code and import library have been removed.
3. Migrate all message arguments and pointer-bearing runtime fields to pointer
   width; audit raw strides and keep packet fields fixed width.
4. Expand CMake to the application and remaining test suites; migrate filesystem,
   clock, synchronization, network, settings, text and input consumers by subsystem.
5. Implement the Metal backend behind a platform-neutral graphics seam. Linux
   will need its own graphics backend; it must not inherit Metal-specific types.

Absolute paths assembled from `g_clientDir` are still Windows system-code-page
paths, sometimes mixed with CP949 filenames. Their existing Windows I/O is
preserved. Migrating them requires separating the native directory from the
game-encoded filename; passing the mixed string to the CP949 API is incorrect.

No renderer, font, audio or gameplay parity claim is made by these foundation tests.

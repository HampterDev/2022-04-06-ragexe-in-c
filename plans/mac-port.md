# Native macOS port

Status: implementation started on `codex/mac-port`, 2026-11-01, from `purist` commit `d725b49`.
See [portable foundations](../docs/portability.md) for implemented boundaries, commands and validation.
The original assessment below predates this branch. Its `client/` prefix is absent in this repository;
work belongs in the root `src/`, `tests/` and `vendor/`. Purist already removed Bink, embedded browsing
and HTTP services. The full client has not yet run on a Mac.

Original assessment (2026-10-31): The
counts come from the working tree of 2026-10-31, including its uncommitted changes, and from the data in
`GIT/client-files`. The time estimates are judgement, not measurements. Line numbers will drift.

The work is all in `client/`. The server does not change. Paths below are relative to `client/src/`
unless they say otherwise.

## Goal

A native arm64 macOS build of the client, with no Wine, Rosetta, CrossOver or other translation layer,
that plays the same as the Windows build on the same game data and server.

macOS cannot run 32-bit code, so the port has to be 64-bit. Keeping pointers in 32 bits is not an option
either: as far as I know, arm64 macOS reserves the low 4 GB of address space, so `(int)&local` always
truncates.

Intel Macs are out of scope unless someone asks. Supporting them later mostly means testing the renderer
on a second GPU family.

## Verdict

Hard, but I found no blocker. About 2 to 3 months for one experienced developer to reach a playable
client, and 4 to 6 months to match Windows. Agents can shrink the mechanical work in milestones 2 and 3.
Renderer, font and IME matching will not shrink much, because someone has to look at the results on a
real Mac.

The biggest schedule risks are hidden pointer bugs in rarely used windows, and the UI not looking right
with different fonts.

## Why it is feasible

- 415,090 lines in 2,014 `.c` and `.h` files, plain C. No inline assembly, SEH or SSE intrinsics.
- Real Direct3D calls exist only in `render/gfx_d3d9.c` (2,039 lines), behind about 45 functions in
  `render/gfx.h`. The other D3D names outside it are a few local `#define`s and message strings.
- The scan found no pointer, `long`, `size_t`, `wchar_t` or `time_t` members in the 589 packed struct
  regions, so packet and file layouts should survive 64-bit. It was a regex scan of simple field
  declarations, so this is strong evidence rather than proof.
- 123 test suites (`tests/build_*.bat`) to catch regressions. Their builds need porting too.

## What has to change

| Area | Measured | Work | Difficulty |
|---|---|---|---|
| 64-bit pointers | 1,145 pointer/int casts | Widen the message arguments, hand-fix the rest | Medium, with subtle bugs |
| Renderer | About 45 seam functions, fixed-function state | Metal backend with one general shader | Hard |
| Text | GDI in 7 files | CoreText or FreeType, same rendering funnel | Hard to match exactly |
| Korean input | `platform/language.c` (654 lines), 9 IME functions | macOS text input through SDL3 | Medium to hard |
| Win32 calls | 243 imported functions, `windows.h` in 231 files | Small platform layer | Medium, mostly mechanical |
| Granny | 44 functions, about 2,500 lines of client code, 21 files | See *Granny* | Medium |
| Miles | 45 functions, about 600 lines | miniaudio plus MP3 and ADPCM WAV decoding | Easy to medium |
| Bink | 12 functions, 5 `.bik` files | Stub first, FFmpeg's decoder if wanted | Easy |
| IJL | 4 functions, `resources/jpeg_codec.c` (96 lines) | stb or libjpeg | Trivial |
| Embedded browser | About 1,400 lines of COM in `browser/` | Default browser first, WKWebView later | Easy or medium |
| libcurl | `LoadLibraryA` in `web/curl.c` | Link the system library or `dlopen` | Trivial |

Imports by DLL, from `build/main_rebuild.exe`: kernel32 98, user32 66, gdi32 22, ws2_32 16, gdiplus 10,
imm32 9, advapi32 5, winmm 4, ole32 4, oleaut32 3, shell32 2, and one each from iphlpapi, dbghelp, shlwapi
and d3d9. A few of the kernel32 entries are only there for the CRT start-up code. Most functions have
few call sites. The ones used widely are `timeGetTime` (453 uses in 165 files), `MessageBoxA` (42 in 23)
and the critical-section calls.

## Findings

### 64-bit pointers

A syntax-only clang sweep as Windows x64 reports these, counted once per file, line and column:

| Warning | Sites | Files |
|---|---:|---:|
| `-Wpointer-to-int-cast` | 785 | 271 |
| `-Wint-to-pointer-cast` | 297 | 133 |
| `-Wint-to-void-pointer-cast` | 49 | 30 |
| `-Wvoid-pointer-to-int-cast` | 14 | 9 |

About 88% of the pointer-to-int casts are arguments to message calls: `ui_window_mgr_send_msg_` (271),
`ui_window_send_msg` (103), `ui_window_send` (80), `mode_send_message` (55), `actor_send` (51) and their
smaller relatives. The other side of those calls is most of the int-to-pointer group, such as
`(const char *)val1` and `(struct WindowInfo *)val1`. Widening the message argument type to `intptr_t`
should clear most of the 1,145 in one change. Return values need the same check.

What is left needs hand fixes:

- `int` fields and slots holding pointers: `*slot = (int)e` in `actor/able_send_msg.c`,
  `self->clan_emblem = (int)par4` in `actor/actor_efst.c`, `self->emotion_mute` in
  `actor/actor_sendmsg.c`, `self->falcon`.
- Strange cases that need reading, such as `long long p = (int)&v` in `actor/actor_efst.c`.
- 146 `-Wshorten-64-to-32` sites, mostly `size_t` going into `int` lengths. Mostly harmless, but each
  needs a look.

**The compiler cannot see everything.** Hardcoded strides such as `NCASH_BUTTON_AT` in
`ui/shops/ncash_wnd.c` (`(i) * 4` over an array of pointers) and pointers copied through `memcpy` raise
no warning. Raw offsets and strides caused real bugs during the container work, so expect more of them
here, and expect them in rarely used windows.

**Layout asserts.** 653 of the 1,417 fail on x64, and none of them is on one of the 839 types defined in
packed regions. They cover about 300 runtime types and most can probably be deleted. Each failure on a type
that is read from a file or passed to a library needs checking first.

**What the sweep does not cover.** Windows x64 keeps `long` at 32 bits, so the sweep says nothing about
`long` becoming 64-bit on macOS. That is 69 lines, read by hand in milestone 3. It also skips `tests/` and
`vendor/`. `browser/web_browser.c` did not parse, because it uses `GWL_WNDPROC`.

### Traps the compiler will not report

- **Compiled Lua.** The compiled `.lub` files I sampled in `System/` have a 4-byte `size_t` in their
  header, and `vendor/lua-5.1.4/src/lundump.c` compares the header byte for byte. 64-bit Lua rejects them
  with "bad header", and they also store string lengths at that width. The fix is a small patch to the
  loader. It has to be made in `vendor/`, which is otherwise kept as published.
- **`wchar_t`** is 32-bit on macOS: 51 uses in 19 files, plus 10 `L"..."` literals. Use a 16-bit type
  where the code means UTF-16.
- **`rand`.** `RAND_MAX` is 32767 with MSVC and much larger on macOS. There are 285 uses in 112 files,
  mostly `%`, which is fine. `effects/ez2str_particle.c` divides by its own `k_ez2strRandMax`, which needs
  checking. Effects that depend on exact random sequences will differ, which should not matter.
- **Float casts.** Casting an out-of-range float to an integer gives different results on ARM than on
  x86, which could produce wrong colours or values. I did not count these. UBSan's
  `float-cast-overflow` finds them at run time.
- **Format strings.** 17 lines use `%I64u` or `%I64d`: 16 in `contents/item_reform.c` and one in
  `contents/repute_wnd.c`.
- **32-bit time.** `_time32` and friends in about 20 files. Keep the values 32-bit where packets carry them.
- **Windows runtime names.** `_mbsstr` (48 uses), `_itoa` (46), `_access` (33), `_snprintf` (25),
  `sprintf_s`, `strcpy_s`, `__int64` (287), `__stdcall` and `WINAPI` (142). Mechanical shims.

### Text encoding and paths

- **Korean literals.** `build.bat` compiles with `/execution-charset:.949`. Clang has no equivalent, and
  1,415 lines in 261 files contain non-ASCII text. A build step can transcode the sources to CP949 before
  compiling: clang should pass the bytes through with a warning, and CP949 trail bytes never equal `\`.
  Test this first. Hex-escaping the literals is the fallback, but it makes them unreadable, and the tree
  has no comments to explain them.
- **File names.** About 121,000 of the 160,696 names in `data.grf` are CP949. Those stay as bytes inside
  the archive, but loose files need converting to UTF-8 where they are opened.
- **Separators.** 5,230 lines contain `\\`, mostly paths. Normalise them where files get opened instead
  of editing the strings. That covers `fileio/`, the 34 direct `fopen` uses, `_access`, `FindFirstFileA`
  and `luaL_loadfile`.
- **Case.** The default macOS volume ignores case. A case-sensitive volume would break lookups, so either
  document it as unsupported or add a lookup table.

### Renderer

`render/gfx.h` is a good seam, with three leaks: it uses `RECT`, `GUID` and `HMONITOR`, `gfx_ddraw_enumerate`
still has a DirectDraw shape, and `gfx_get_dc` hands out an `HDC` (its only caller is `render/textdc.c`).

The backend has to reproduce:

- Fixed-function texture stages: modulate, add, select, blend-by-alpha and the other `GFX_TOP_*` operations
  in one general shader, with pipeline objects cached per blend combination.
- Alpha test, colour key, vertex fog, bump-mapped water, two vertex kinds, pre-transformed lists and the
  vertex buffer mesh path.
- `gfx_lock` and `gfx_unlock` on textures, render-target locks and frame capture.
- The 1555, 4444 and 565 texture formats. Converting to 8888 on upload is simplest. The packed formats
  exist in Metal only on Apple GPUs.
- 4x MSAA on the 3D scene and none on flat 2D draws, as in `README.md`, plus resize and device replacement
  (much simpler, since Metal has no lost-device state).

The main visual trap is D3D9's half-pixel mapping. Metal samples at pixel centres, so UI and sprite
coordinates will be off by half a pixel unless the shader corrects for it, and text will blur.

Decision: Metal, not OpenGL. As far as I know, macOS's legacy OpenGL profile still has fixed-function
state and would be quicker to bring up, but Apple deprecated OpenGL in 2018 and it would need replacing.

Metal and AppKit need a thin Objective-C file, or `objc_msgSend` calls from C. This is one exception to the
"plain C" rule in `client/README.md`. One `.m` file is easier to read than hand-written message sends.

### Text and fonts

GDI text lives in `render/fonts.c`, `textdc.c`, `ctextdc.c`, `cdc.c`, `cframe.c`, `ui/controls/window_draw.c`
and `diag/perf.c`, plus `vendor/font_backend/font_family.cpp` (GDI+). Everything goes through a small
funnel, so replacing it is contained. Matching it is the hard part.

`render/c3ddevice.c` asks for Gulim, Arial, MS Gothic, SimSun, MingLiU, Tahoma and Verdana, and
`render/fonts.c` only looks up fonts already installed on the system. macOS does not include Gulim.

**Licensing.** Gulim is a commercial font that comes with Windows under the Windows licence, which does
not let you pass the file on. This is from memory of Microsoft's font redistribution guidance, not
checked and not legal advice. A font's "embeddable" setting covers documents, not bundling with an app.
Check before shipping any font from Windows, including Arial and Tahoma.

Decision: bundle a free font as the default, Noto Sans KR or Nanum Gothic (both SIL Open Font License), so
the client works out of the box. Add a setting that loads `gulim.ttc` from a path the player chooses. You
ship nothing, and whether the player's licence covers that use is their question. A paid licence is only
worth pursuing if exact looks turn out to matter.

A substitute will not look identical. Gulim has hand-tuned bitmaps at small sizes, which is a large part
of the classic look, and different character widths. Expect text wrapping and alignment to shift, so the
UI needs checking window by window. If players can supply Gulim, the renderer must also draw its embedded
bitmaps. FreeType can; I have not checked CoreText.

### Korean input

`platform/language.c` and `platform/language_tc.c` use the Windows IME directly. SDL3's text editing events
should carry the composition. Test real Hangul composition early, including candidate windows and the
clipboard. Keep the `VK_*` numbering (188 uses in 29 files) and translate at the input edge. `GetAsyncKeyState`
and `GetKeyState` (37 uses in about 20 files) become reads from a key table the event loop maintains.

### Granny

Only 21 `.gr2` files exist, all in `data.grf`: 15 in `model/3dmob_bone` and 6 in `model/3dmob`. All are
version 6 files, with 105 of their 126 sections compressed as Oodle0. `core/winmain.c` loads
`guildflag90_1.gr2` at start-up, so a stub has to handle that.

The client code is about 2,500 lines (`actor/granny*`, `resources/granny*`), and `resources/granny_sdk.h`
reads Granny's own structs at 32-bit offsets. Replacing it means loading meshes, skeletons and animations,
sampling the animations, and deforming the vertices.

Decision: convert the files once on Windows with `granny2.dll` into a simple format of our own, and write
a small runtime for that format. No Oodle0 decoder needed. Stub the feature until then, since almost all of
the game does not use it.

### Other libraries and platform pieces

- **Miles.** `contents/sound.c` and `resources/wave.c`, about 600 lines. `data.grf` has 2,974 WAV files, and
  BGM is MP3. Positional samples map onto miniaudio. Miles' room types can probably be dropped.
- **Bink.** `data.grf` has 5 files: two in `texture/gevent`, two in `video/gevent` and `video/ad/01.bik`.
  `render/tex_cache.c` already falls back to a black texture, so a stub is cheap.
- **Browser.** `ShellExecuteA` has 19 call sites, and 4 of them name `iexplore.exe` (`login/auth.c`,
  `ui/quests/attendance.c`, `ui/shops/cash_shop.c` twice). All become "open the default browser".
- **Registry.** 5 functions in 16 files become a settings file.
- **Sockets.** `contents/connection.c` uses non-blocking sockets and `select`, close to POSIX. The
  `ws2_32` hook-check code goes.
- **Threads.** `world/models.c`, `contents/sound.c`, `web/work_mgr.c`. Critical sections and events become
  pthreads.
- **Crash report.** `diag/crash_report.c` and `platform/crash_dump.c` use dbghelp, `VirtualQuery` and
  32-bit addresses. Replace them with a small signal handler. macOS already writes its own crash reports.
- **Start-up checks.** The process checks in `core/startup.c` have no job on macOS and go.
- **Main thread.** AppKit needs the main thread, so the game loop stays there and polls events.
- **Retina.** Render at the backing scale, and map mouse coordinates through it.

## Decisions

- SDL3 for the window, input, clipboard, IME events, cursors and display modes. Metal is written by hand
  against the seam. This could also make a Linux port easier later.
- miniaudio for audio, and stb or libjpeg for JPEG.
- Do as much as possible on Windows, where the current client is there to compare against.
- Ship without Granny models and embedded browsing at first.

## Milestones

Each one should leave the Windows build working and the tests passing.

### 0. Prove the risky bits (2 to 3 days, on a Mac)

- Compile `vendor/` and a handful of `src/` files with Apple clang using the CP949 build step.
- Load a real `.lub` with the patched `lundump.c`.
- Open an SDL3 window with a Metal layer and type Hangul into it, reading the composition events.

Done when all three work. A failure here changes the plan before any real work is spent.

### 1. Replace the libraries on x86 Windows (2 to 3 weeks)

Swap IJL, Miles and Bink, and stub or convert Granny, while the current exe is there to compare against.
Port the build and test runner off `.bat` files where it helps later milestones.

Done when the client plays with none of `binkw32`, `granny2`, `ijl15` or `mss32` loaded, and sound and JPEG
screenshots match by ear and eye.

### 2. Build for Windows x64 (2 to 4 weeks)

Widen the message arguments, fix the remaining casts, delete or fix the layout asserts, and patch Lua. Run
the tests, ASan and UBSan with `float-cast-overflow`, then play through the windows and effects. The
`@effect N` sweep is already a good smoke test.

Done when the x64 build passes the tests and a long session with ASan reports nothing new. This fixes most
64-bit bugs before a Mac is involved.

### 3. Add the platform layer (3 to 4 weeks, Mac needed from here)

Replace `windows.h` with a small types and shims header, and move the message pump, input, clipboard, files,
threads, sockets and settings onto SDL3 and POSIX. Read the 69 `long` lines and fix `wchar_t`, `%I64` and paths.
Keep Windows building on the same layer, with GDI and D3D9 behind it for now.

Done when the client reaches the login screen logic on macOS with a stub renderer, and the tests pass there.

### 4. Write the Metal backend (3 to 5 weeks)

Implement the `gfx.h` functions, and remove `HDC` and the DirectDraw shape from the seam. Bring up the 2D path
first, since it gives the login screen, then the 3D scene, effects and water. Compare with Windows screenshots
of the same scene.

Done when the first playable Mac build logs in, walks, fights and uses skills on a real server.

### 5. Finish fonts, IME and Mac details (4 to 8 weeks)

- Font backend, bundled font and the Gulim setting, then check the UI window by window.
- Full IME testing.
- Retina, fullscreen, Cmd keys, the app bundle, signing and notarization.
- Crash handling, Granny runtime, and WKWebView if wanted.

Done when a player can do everything they can on Windows, and the differences left are written down in
`client/README.md`.

## Open questions

- **Distributing converted Granny data.** A Mac-only player cannot run a converter that needs `granny2.dll`.
  Either ship the 21 converted files, which raises the same rights question as the game data, or write an Oodle0
  decoder. I did not check whether a public one exists.
- **Custom `.gr2` files.** Servers may add their own, which also favours a real decoder over one-time conversion.
- **Which Windows build to keep.** Once x64 works, the x86 build might only be needed for comparison. Dropping
  it would remove the need to keep both pointer widths working.

## Reproducing the counts

The scripts were throwaway, but they are short:

- **Casts.** Run `clang-cl --target=x86_64-pc-windows-msvc /std:c11 /Zs -ferror-limit=0 -Wno-everything` plus the
  warnings above over every `.c` in `src/`, with the include paths from `build.bat`. Force-include a header
  containing `#define STATIC_ASSERT(name, cond) typedef char static_assert_##name[1]` so the layout asserts do
  not hide everything else.
- **Layout asserts.** Run the same command without that header, and count the "negative size" errors.
- **Imports.** Read the import table of `build/main_rebuild.exe` with `pefile`.
- **Assets.** Read the GRF 0x200 file table, then the Granny section headers of each `.gr2`.

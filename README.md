<p align="center">
  <img src="res/icon.png" width="112" alt="Ragnarok Online icon">
</p>

<h1 align="center">2022-04-06 Ragexe in C</h1>

<p align="center">
  <img src="https://img.shields.io/badge/client-Ragexe_2022--04--06-1f6feb" alt="client: Ragexe 2022-04-06">
  <img src="https://img.shields.io/badge/sha256-656b2f274c09-444" alt="original exe sha256 656b2f274c09">
  <img src="https://img.shields.io/badge/md5-f914d47b44f1-444" alt="original exe md5 f914d47b44f1">
  <img src="https://img.shields.io/badge/language-C11-00599C" alt="language: C11">
  <img src="https://img.shields.io/badge/platform-Windows_x86-0078D6" alt="platform: Windows x86">
  <img src="https://img.shields.io/badge/renderer-Direct3D_9-6e40c9" alt="renderer: Direct3D 9">
  <img src="https://img.shields.io/badge/exe-4.5_MB_(original_11.6_MB)-2ea44f" alt="exe 4.5 MB, original 11.6 MB">
</p>

The Korean Ragnarok Online client of 6 April 2022 (`Ragexe`), reconstructed from its
executable and rebuilt as a plain C source tree: the sources the client is built from, the
third-party libraries it links, and its behavioral tests. It builds with the MSVC C
compiler and linker for 32-bit Windows (Visual Studio 2022 or its Build Tools, x86).

Ragexe 2022-04-06 is the client version the BrowEdit community recommends most, for its
stability.

## Build

From cmd or PowerShell in this folder:

    build.bat          build\main_rebuild.exe
    build.bat dbg      build\main_dbg.exe with build\main_dbg.pdb

`build.bat` builds the libraries in `vendor\` first (into `build\vendor\`), then compiles
every `.c` under `src\` and links. There are no configuration switches: the client is
the Direct3D 9 build with the crash report, the texture cache lock and the Lua
compatibility layer.

## Tests

    tests\build.bat

builds and runs every behavioral test (about 15 minutes); each test prints a line that
ends in "passed". It builds the libraries in `vendor\` first if `build.bat` has not. A single
area has its own `tests\build_<name>.bat`; run `build.bat` or `tests\build.bat` once before
it, for the libraries. The tests need nothing outside this folder: no game data, server,
account or registry settings.

## Layout

| folder | what |
|---|---|
| `src\base.h` | the base header every unit includes |
| `src\core` | start-up, the message pump, configuration, text, files, Lua |
| `src\login`, `src\game`, `src\world`, `src\session` | the login screens, the game mode, the 3D world, the player's session state |
| `src\initializers`, `src\iteminfo`, `src\json`, `src\replay`, `src\browser` | static tables, item data, JSON, replays, the web browser window |
| `src\actor`, `src\skills`, `src\effects` | characters and monsters, skill logic, visual effects |
| `src\ui` | windows and controls |
| `src\render` | the Direct3D 9 renderer, textures, fonts, sprites |
| `src\resources`, `src\fileio` | game resource formats, GRF archives, the resource cache |
| `src\contents`, `src\web`, `src\navi` | game systems, web services, navigation |
| `src\std` | containers and strings |
| `src\platform`, `src\security`, `src\diag` | Windows integration, checks, the crash report and the in-game overlay |
| `vendor` | zlib, libpng, giflib, Lua 5.1.4, yyjson, Rijndael and the font backend (C++), curl headers |
| `lib`, `res` | prebuilt import libraries (Granny, Bink, Miles, IJL, ...), the compiled resources (`app.res`) and the client's icon (`app.ico`, `icon.png`) |
| `tests` | the behavioral tests and their fixtures |

## Conventions

- C11, the MSVC compiler; one declaration per function and object, in its header.
- No comments in the sources; names say what things are.
- Nothing that the client does not use: a function, global, type or macro without a user
  is removed.
- Third-party code in `vendor\` stays as published, with its licence.

## Changes from the original client

This client is Ragexe 2022-04-06, the Korean Ragnarok Online client of 6 April 2022,
reconstructed from its executable and rebuilt as plain C. It is meant to behave like the
original exe. The differences below are deliberate. A change that a patch of the
[NEMO](https://gitlab.com/4144/Nemo) patcher also makes carries that patch's name, as NEMO
lists it, in an orange badge.

### Starting and connecting

- [![NEMO: HKLM To HKCU](https://img.shields.io/badge/NEMO-HKLM_To_HKCU-e67e22)](https://gitlab.com/4144/Nemo/-/blob/master/Patches/HKLMtoHKCU.qs)
  Settings are read from and written to `HKEY_CURRENT_USER` instead of `HKEY_LOCAL_MACHINE`,
  so the client needs no administrator rights.
- [![NEMO: Always read msgstringtable.txt](https://img.shields.io/badge/NEMO-Always_read_msgstringtable.txt-e67e22)](https://gitlab.com/4144/Nemo/-/blob/master/Patches/ReadMsgstringtabledottxt.qs)
  The message strings come from `msgStringTable.txt` for every service type. The file is
  required: the client has no built-in strings, and the kRO full client does not ship one, so
  it comes from a translation or the server's data (in `data\` or a GRF). Without it every
  label reads "NO MSG".
- [![NEMO: Force Send Client Hash Packet](https://img.shields.io/badge/NEMO-Force_Send_Client_Hash_Packet-e67e22)](https://gitlab.com/4144/Nemo/-/blob/master/Patches/ForceSendClientHash.qs)
  Packet 0x204 always carries the MD5 of the running exe. The original sent it only for some
  service and server types, with a fixed key or zeros in place of the digest.
- [![NEMO: Disable Game Guard (NProtect)](https://img.shields.io/badge/NEMO-Disable_Game_Guard_%28NProtect%29-e67e22)](https://gitlab.com/4144/Nemo/-/blob/master/Patches/DisableNProtect.qs)
  nProtect GameGuard is not started, its FindHack scan does not run, and the login and game
  loops ignore its error state. On an error the original disconnected, quit and showed an
  "nProtect GameGuard" message box. The GameGuard code itself is gone; a stub keeps its
  interface.
- [![NEMO: Disable 1rag1 type parameters](https://img.shields.io/badge/NEMO-Disable_1rag1_type_parameters-e67e22)](https://gitlab.com/4144/Nemo/-/blob/master/Patches/Disable1rag1Params.qs)
  The client starts without a launcher argument. The original refused to run unless the
  command line contained `1rag1`, `1sak1`, `1gld1`, `2rag2`, `1eve1` or `1pk1`.
- [![NEMO: Disable Ragexe Filename Check](https://img.shields.io/badge/NEMO-Disable_Ragexe_Filename_Check-e67e22)](https://gitlab.com/4144/Nemo/-/blob/master/Patches/DisableFilenameCheck.qs)
  The start-up process check never stops the client. The original quit with "Please run a
  genuine Ragnarok client".
- [![NEMO: Remove hardcoded address/port](https://img.shields.io/badge/NEMO-Remove_hardcoded_address%2Fport-e67e22)](https://gitlab.com/4144/Nemo/-/blob/master/Patches/RemoveHardcodedAddress.qs)
  The login server is `<address>:<port>` from `clientinfo.xml`. The original picked a
  built-in `*.ragnarok.co.kr` server by server type, and set the client version and the
  built-in admin lists with it.
- The GDI check is retired. The original compared gdi32's `TextOutW`
  import with `GetProcAddress` and checked for a debugger. On a mismatch it disconnected and
  quit three times in five, and once due it ran every frame.

### Input and text

- [![NEMO: @ Bug Fix](https://img.shields.io/badge/NEMO-%40_Bug_Fix-e67e22)](https://gitlab.com/4144/Nemo/-/blob/master/Patches/FixChatAt.qs)
  While a text field has the keyboard, Ctrl+Alt (AltGr) keys type text instead of triggering
  hotkeys. AltGr+Q types `@` on a German layout; the original took it for Alt+Q (the
  equipment window) and lost the character. The same goes for `€` and `{ [ ] }`.

### Bugs of the original fixed

Dangling pointers and races:

- A window that is destroyed is removed from everything that
  points at it: the window manager's slots, the modal, capture, focus and hover pointers, and
  the windows actors float above their heads. The original cleared these by hand at each
  close site, so a window destroyed any other way (a mode change, a parent closing its
  children) left dangling pointers.
- The blur and hallucination clones check that the actor they copy is
  still in the world before they read it. The original read freed memory when that actor was
  deleted before its clone's tick.
- Hiding the UI (F11) no longer leaves the player's status-icon
  effect pointer dangling. With the UI hidden, using an item from a hotkey read freed memory.

Uninitialized values:

- The damage numbers of one hit packet came out blue at random. Its
  "SP damage" byte was left as the stack had it.
- The key-stroke window read its colour chip's green from the stack.
  Green and blue went into one variable.
- Achievement default records, the tetra-ball and flare effect
  positions, world-map party marks and radio-button commands now start with defined values
  instead of leftover memory.
- A button in an unknown state draws no
  bitmap. An unknown on-screen timer kind is hidden; the original showed a pointer as the
  timer's value.

Overruns and bad table indexes:

- The cash shop window's button loop ran a twentieth time, past its
  19-entry tables. It took stack data as bitmap names, a likely crash.
- The length of NPC dialog text from the server is clamped to the
  buffer (0..0x7fd), and incomplete packets are rejected.
- A long item name can no longer overflow the file path of the
  package item test report.
- Negative angles are normalised (-90 is 270). The original indexed
  before its sine and cosine tables.
- Bytes above 0x7f no longer index before the hex digit table, and
  non-hex characters decode as 0.
- The font lookup releases the screen DCs it takes; the original
  leaked two per lookup. An unknown font id, SCDream's fallback included, uses GDI's default
  font instead of reading past the twelve font slots.

Format strings:

- The collection window copies the pet's name into its field. The
  original passed the name to `sprintf` as the format.
- The renamed-pet prefix from the message table is printed as text, not
  used as a format; only `%%` still becomes `%`.
- The OTP text is sent as it is, not used as a `printf` format.
- The attendance error message prints the item id with `%d`. The
  original passed the id to a `%s`, which crashed on a duplicate reward.
- The equipment-swap tooltip for an item with random options gets
  the name, the option count and the amount. The original passed the name's `std::string`
  object itself.

Markup, links and wrapping:

- Colour codes (`^rrggbb`), markup tags
  and message tags are parsed within the text's length. An incomplete code or an unmatched tag
  stays as text. It no longer causes a loop, a read past the end, or a read before the font
  stack.
- Item and navigation links in chat and edit fields stay inside their text. At most 4 cards and 5
  random options are decoded. A malformed link cannot underflow an offset, rescan forever or
  delete the wrong text.
- When a link's button cannot be
  created, the text stays readable instead of a null pointer being used.
- Wrapping text into
  a box narrower than one character no longer loops forever. Every line takes at least one
  character.

Data, time and leaks:

- A reputation or EZ2STR effect table that is not a JSON object fails the
  read instead of throwing an exception out of the loader.
- GRF, SPR, IMF, GND and EZ effect files and the
  water settings are checked (counts, sizes, names) before use. A short or malformed file fails
  to load instead of being read past its end. A GRF record that points outside the archive
  (the kRO `data.grf` has such a deleted-file record) is left out, and the other files stay
  readable.
- An empty XML element written without a space before the slash, such as `<readfolder/>`
  in `clientinfo.xml`, is read as empty. The original cut the tag name at the slash before
  checking for it, so the element swallowed the rest of the file and the connections in it.
- Player-name requests and the name cache still time out correctly
  when the millisecond clock wraps, after 49.7 days of Windows uptime.

### Graphics and display

- The renderer is Direct3D 9 (fixed function) instead of DirectX 7 (DirectDraw 7 and
  Direct3D 7); DirectDraw is no longer used. Vertex processing is in hardware, with software as
  the fallback. A lost device is reset and every state the client set is replayed.
- The Graphic Device list has one entry per display adapter, named after it ("... (display
  2)" for a second monitor), instead of DirectX 7's "Direct3D HAL" and "T&L HAL" pair.
- The resolution list takes modes up to 8192x4096 instead of 2048x1440, so
  wide desktops such as 3840x1080 appear. Each size is listed once (not once per refresh rate),
  and none is larger than the desktop.
- Exclusive fullscreen always uses a 32-bit back buffer; the 16-bit setting is not used.
- A new resolution and the Fullscreen checkbox in the graphics settings apply
  at once. The original asked for a restart and relaunched the exe. UI windows near the right
  or bottom edge keep their distance to that edge, centred ones stay centred, and the size is
  saved as before.
- Changing the Graphic Device applies at once when the new adapter has the same
  GPU model (same vendor and device id); textures are copied to the new device. Other device
  changes restart as before.
- A new "Borderless" checkbox, on by default and saved as `ISBORDERLESS` in `OptionInfo.lua`.
  With it on, Fullscreen is a frameless window covering the monitor at the monitor's own
  resolution: no mode switch, and no device loss on Alt+Tab. With it off, Fullscreen is the
  exclusive mode as in the original.
- The 3D scene is drawn with 4x multisampling; the original never anti-aliases.
  Flat 2D draws (the UI, sprites, the login screen) are drawn as before, without
  multisampling. Alpha-tested edges (fences, leaves) get the driver's transparency
  anti-aliasing where it is offered. `RAGEXE_MSAA=<n>` sets the sample count, and `0` turns it
  off. `RAGEXE_MSAA_ALPHA=0|ssaa|atoc` controls the transparency anti-aliasing.
- The wait for the vertical blank is the swap chain's, so `Present`
  blocks without spinning and the frame rate is the display's refresh rate.
  `RAGEXE_VSYNC=0` turns vsync off.
- A device the driver reports as hung or removed, or one whose reset fails 8
  times in a row, is replaced by a new device on the same adapter, with the textures copied
  over. If the new device refuses the exclusive mode, the client falls back to windowed.
- A saved display device that is no longer attached (an unplugged
  monitor, a swapped GPU) is reset to the primary display in the registry, and a message box
  says so once.

### Performance

- Consecutive pre-transformed draws are merged into one indexed triangle list,
  in the original order. The `Sleep(0)` the original made after each such draw is gone.
- A render state, stage state or texture the
  device already holds is not sent again. The original re-sent most of them at every list
  flush.
- A blended draw with the same texture-stage mode as the previous one
  skips the mode switch.
- A sprite or UI texture rebuild that would upload the same pixels
  into an unchanged texture is skipped. In a loaded town that is nearly all of them.
- The frame no longer locks the render target for values nothing reads.
  The lock made the CPU wait for the GPU every frame.
- The game loop yields (`Sleep(0)`) after each frame instead of sleeping a
  millisecond.
- Text measurements are cached, keyed on the font, the charset and the
  string.
- The lookup key prepared from a resource name is remembered, so per-frame
  fetches such as `shadow.spr` skip lower-casing and alias resolution. `RAGEXE_RES_KEYS=0`
  turns it off.
- Finding an actor by its id goes through an index instead of walking
  the world's actor list. Ids that are absent are remembered too.
- An actor's displayed job reads the effect-state mask once instead of
  once per rule (up to forty virtual calls).
- Effect particles reuse the camera angle's sine,
  cosine and rotation matrix while the camera does not turn. The results are the same as
  before, to the bit.
- UI windows the original redraws every frame (character selection, equipment, skill tree,
  dress room, cash shop, shortcut bar, quick slots, minimap buttons, vendor shop titles and
  the window of the focused edit line) redraw only in the frame where something they show
  changes: a drawn value, a hover, an animation frame, the caret blink. Cooldown pies and the
  characters on the selection screen are still drawn every frame over the unchanged window.
- The exe is large-address-aware: under 64-bit Windows it can use 4 GB of address space
  instead of 2 GB.

### Diagnostics built in

- An unhandled exception writes `crash_report.txt` (appended, at most 2 MB)
  and a minidump `crash-<date>-<time>-<pid>-<tid>-<n>.dmp` beside the exe. The report
  includes the exception, the address as an exe offset, memory and module lists, job, map and
  cell, the last effect ids and the render checkpoint. The original's error dialog then shows
  the report, and it can be copied from there.
- With `RAGEXE_OVERLAY=1` the client draws an FPS and frame-phase overlay
  (scene, present, logic, text, busiest render lists, draw and state counts). Scroll Lock
  toggles it in any session.
- With `RAGEXE_DRIVE_STATE=<file>` the client rewrites that file about four
  times a second with `key=value` lines. They hold the mode and screen, the map, the
  character's cell, HP, SP and level, the pickable actors with their screen boxes, the open
  and modal windows, the chat focus and the last chat lines. The file is for scripts that
  drive the client in tests.
- `RAGEXE_SWITCH_DEVICE=1` takes the device-switch path for every display change, to test it
  on a machine with one monitor.

### The C rewrite itself

Most of the rewrite is structural, with no intended change in behaviour. Objects have
ordinary C ownership instead of the MSVC C++ runtime's layouts: `std::string`, `vector`,
`list`, `map` and `deque` became `struct String`, `Vec`, `List` and `Tree`. Virtual tables
became operation tables, kept only where the implementation varies. The standard MSVC CRT
replaces the image's private runtime. `client_init()` builds the globals in the original
order instead of static constructors. yyjson replaces jsoncons. An unrecoverable error (out
of memory) prints a message to stderr and aborts instead of throwing a C++ exception.
Internal memory layouts are no longer the original's; packet, file and library layouts are.

Faults of the original found during the rewrite, now fixed:

- Player-only code wrote player fields into NPC actors (the hat effect, statuses 0x57, 0xeb
  and 0x3d6, the hallucination doubles). After-images of a caster that is not a player read
  past the object; they are now made only for players.
- The footprint effect launcher passed a float where a 3D vector is read.
- `CZ_ENTER`'s client tick field was sent with whatever was on the stack; it is now sent as 0.

## Contributing

The aim is a client that behaves exactly like the original exe, apart from the changes
listed above. If you find a reconstruction bug, anything this build does differently from
the original 2022-04-06 client, feel free to open a pull request (or an issue, if you have
not tracked it down yet). Please keep to the conventions above and run `tests\build.bat`
before you open it.

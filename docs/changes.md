# Changes from the original client

The client is meant to behave like the original exe. The differences below are deliberate;
any other difference is a reconstruction bug. A change that a patch of the
[NEMO](https://gitlab.com/4144/Nemo) patcher also makes starts with that patch's name, as
NEMO lists it.

## Starting and connecting

- [NEMO: HKLM To HKCU](https://gitlab.com/4144/Nemo/-/blob/master/Patches/HKLMtoHKCU.qs).
  Settings are read from and written to `HKEY_CURRENT_USER` instead of `HKEY_LOCAL_MACHINE`,
  so the client needs no administrator rights. `Setup.exe` and most servers' setup tools still
  write `HKEY_LOCAL_MACHINE`, so the original's wait at start-up (run `Setup.exe` until the setup
  keys exist) could never end. Without the keys the client starts with the setup defaults
  instead: the primary display and Miles 2D positional sound.
- [NEMO: Always read msgstringtable.txt](https://gitlab.com/4144/Nemo/-/blob/master/Patches/ReadMsgstringtabledottxt.qs).
  The message strings come from `msgStringTable.txt` for every service type. The file is
  required: the client has no built-in strings, and the kRO full client does not ship one, so
  it comes from a translation or the server's data (in `data\` or a GRF). Without it every
  label reads "NO MSG".
- [NEMO: Force Send Client Hash Packet](https://gitlab.com/4144/Nemo/-/blob/master/Patches/ForceSendClientHash.qs).
  Packet 0x204 always carries the MD5 of the running exe. The original sent it only for some
  service and server types, with a fixed key or zeros in place of the digest.
- [NEMO: Disable Game Guard (NProtect)](https://gitlab.com/4144/Nemo/-/blob/master/Patches/DisableNProtect.qs).
  nProtect GameGuard is not started, its FindHack scan does not run, and the login and game
  loops ignore its error state. On an error the original disconnected, quit and showed an
  "nProtect GameGuard" message box. The GameGuard code itself is gone; a stub keeps its
  interface.
- [NEMO: Disable 1rag1 type parameters](https://gitlab.com/4144/Nemo/-/blob/master/Patches/Disable1rag1Params.qs).
  The client starts without a launcher argument. The original refused to run unless the
  command line contained `1rag1`, `1sak1`, `1gld1`, `2rag2`, `1eve1` or `1pk1`.
- [NEMO: Disable Ragexe Filename Check](https://gitlab.com/4144/Nemo/-/blob/master/Patches/DisableFilenameCheck.qs).
  The start-up process check (the IProcess module scan) never stops the client. The original
  quit with "Please run a genuine Ragnarok client".
- [NEMO: Remove hardcoded address/port](https://gitlab.com/4144/Nemo/-/blob/master/Patches/RemoveHardcodedAddress.qs).
  The login server is `<address>:<port>` from `clientinfo.xml`. The original picked a
  built-in `*.ragnarok.co.kr` server by server type, and set the client version and the
  built-in admin lists with it.
- [NEMO: Restore old login packet](https://gitlab.com/4144/Nemo/-/blob/master/Patches/RestoreOldLoginPacket.qs).
  Every service type logs in with packet 0x64 (account and password). For korea (langtype
  0), brazil (12) and france (18) the original sent the token login 0x825, with the token
  from the login server's reply in place of the password, which an emulator cannot check
  against the account.
- The client does not look for a newer patcher at start. When `Patchup.exe` (`Sakup.exe`,
  `Indoorup.exe`, ... by server type) was next to it, the original asked "Ragnarok.exe is
  changed. Press ok to restart" and on OK replaced `Ragnarok.exe` with that file and ran it,
  which in a kRO full client folder starts the kRO patcher.
- The GDI check is retired. The original compared gdi32's `TextOutW`
  import with `GetProcAddress` and checked for a debugger. On a mismatch it disconnected and
  quit three times in five, and once due it ran every frame.
- The integrity list `RagHash.dat`, which Gravity's patcher supplies, is never read; the list
  stays empty. The original stopped at start-up when the file was missing, held fewer than two
  entries or failed its checksum.
- The start-up patch applier is gone. The original's applier opened `data.grf ` (with a
  trailing space) and returned before writing anything. The open is kept, so a missing `data.grf` is still
  created empty, as in the original.

`tests/parity/original_patches.txt` patches the original exe the same way for the GameGuard
blocks, the anti-cheat scans, the IProcess module scan and the integrity list, so that it runs
outside Gravity's service in the parity harness.

## Input and text

- [NEMO: @ Bug Fix](https://gitlab.com/4144/Nemo/-/blob/master/Patches/FixChatAt.qs).
  While a text field has the keyboard, Ctrl+Alt (AltGr) keys type text instead of triggering
  hotkeys. AltGr+Q types `@` on a German layout; the original took it for Alt+Q (the
  equipment window) and lost the character. The same goes for `€` and `{ [ ] }`.

## Bugs of the original fixed

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

- Without a saved `CURSLOT` registry value, character selection starts at the first slot
  instead of selecting a slot from leftover stack data and potentially disabling Game Start.
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
- The login connection entries start with their unused version and service type at 0. The
  original copied two uninitialised stack values with every entry; nothing read them.
- An STR texture animation playing in reverse wraps to the end of its textures when it
  passes frame 0. The original took the wrap from an uninitialised value, so the layer stuck
  on its first texture or showed the white one (kRO `hogg.str`, `ssnnnn2.str`,
  `fullmoon_kick.str`).
- The sprite ring of effect 941 (`m_ef05.spr`, a flying-sprite layer whose tint is 4) draws
  its sprites with colour 0. The original coloured them from an uninitialised stack value,
  since the layer's colour branches cover tints 0 and 1 only.

Overruns and bad table indexes:

- The cash shop window's button loop ran a twentieth time, past its
  19-entry tables. It took stack data as bitmap names, a likely crash.
- The length of NPC dialog text from the server is clamped to the
  buffer (0..0x7fd), and incomplete packets are rejected.
- A long item name can no longer overflow the file path of the
  package item test report.
- Bytes above 0x7f no longer index before the hex digit table, and
  non-hex characters decode as 0.
- The font lookup releases the screen DCs it takes; the original
  leaked two per lookup. An unknown font id, SCDream's fallback included, uses GDI's default
  font instead of reading past the twelve font slots.
- Fonts 12 and up pick their character set without reading past the 12-entry charset table.
  They get the values the bytes after it gave them: SCDream `DEFAULT_CHARSET`, the others
  `ANSI_CHARSET`.
- Text copied into a fixed buffer stops at the buffer: the name sent by GM `/shift` and
  `/recall` (a 24-byte field), the job name in the Star Gladiator notice, pet and monster
  talk lines from `PetTalkTable.xml` and `MonsterTalkTable.xml`, monster skill actions from
  `MonsterSkillInfo.xml` (8-byte buffers), the pet portrait file name and the pet name the
  pet windows show. The original used `strcpy` or `sprintf` and ran past the buffer or the
  packet.
- Packet fields that need not end in a NUL are read within their field: the private-airplane map
  name (16 bytes), the pet name (24) and the sound name of `ZC_SOUND` (24). The arguments of a
  format-string message (`ZC_FORMATSTRING_MSG`) are read up to the packet's length, and its `%s`
  count stops at a message that ends in a lead byte. The original read a missing or unterminated
  argument from past the packet.
- A char-server list line of 64 bytes or more is cut at a whole character, before any colour
  code the cut would split. The original's `strcpy_s` ended the client without a message, as
  it did for a private-airplane move with a full 16-byte map name.
- A map name shorter than its 4-byte `.gat` extension is kept as it is on a map or server
  move. The original's size wrapped, and the client ended with "string too long".
- A map move onto the same map returns after the warp reset fails and leaves for the login
  screen. The original went on to initialise the mode and fade in.
- An effect id from the server past the 2,385-row duration table takes its own id as its
  duration, as the unconfigured rows do. The original read past the table.
- For the Korean service, a monster with a `MonsterTalkTable.xml` entry no longer crashes the
  client. The original decoded the state name in place, a constant in read-only memory, and
  indexed the job names with the monster's job unchecked.
- An attendance banner with fewer than two images does not rotate. With none the original
  read its image from an empty table.

Format strings:

- The collection window copies the pet's name into its field. The
  original passed the name to `sprintf` as the format.
- Pet, homunculus and partner names from the server, the names typed in
  the pet and homunculus rename boxes and the default names taken from
  the job-name table are copied as text. The original passed each name to
  `sprintf` as the format, so a `%` in a name read arguments nobody passed.
- The renamed-pet prefix from the message table is printed as text, not
  used as a format; only `%%` still becomes `%`.
- The OTP text is sent as it is, not used as a `printf` format.
- The attendance error message prints the item id with `%d`. The
  original passed the id to a `%s`, which crashed on a duplicate reward.
- The equipment-swap tooltip for an item with random options gets
  the name, the option count and the amount. The original passed the name's `std::string`
  object itself.
- The couple name, the cut-in image name, the char-server and map names, the launcher
  command, monster talk lines and the saved styling-shop records are copied as text, and the
  line of message 0x784 is not formatted a second time. The original used each as a format,
  so a `%` read arguments nobody passed.
- Chat tab names are escaped in the saved chat layout. The original wrote them as they were,
  so a name with a quote or `]]` broke the file.

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
- A description line that starts with one character and a `[...]` group that just misses the
  line no longer loops forever, and a step back before a group that does not fit moves one
  character, not one byte, so it no longer splits a double-byte character.
- Window text that breaks at a lone line feed or carriage return keeps the next line's first
  character. The original's line breaker kept the CR or LF in the line and skipped the byte
  after it, which is right only for CRLF text; for LF-only books and server texts it dropped
  the first letter of every line.

Data, time and leaks:

- A vending request keeps its setup items until the next setup starts. If the server
  refuses the request and leaves the window open, retrying still sends the chosen items,
  quantities and prices instead of an empty list.
- A reputation or EZ2STR effect table that is not a JSON object fails the
  read instead of throwing an exception out of the loader.
- GRF, ACT, SPR, IMF, GND, GAT, RSW, RSM and EZ effect files are read within their own size.
  A file cut short, or whose counts or indexes do not fit the file or the tables they point
  into, fails to load. The original read such a file as far as it went: its reads past the
  end failed and left zeros or earlier values, and the counts and offsets it took from the
  file read past the end or outside its tables. An ACT that ends inside its delay table,
  a PAL or SPR too short for its palette and an RSM model that ends before or inside its
  volume boxes still load, as in the original. An EZ effect file
  with a negative frame or texture count, or a 128-byte texture name with no NUL, fails as
  well. An RSM2 model of minor version 1 to 3 whose major version is not 2 fails; the
  original loaded it and then made the kind of model actor that major version names. A water
  animation speed of 0 fails the map; the original divided by it. A sprite frame whose pixel
  stream is shorter or longer than the frame is cut at the frame's end or filled with
  transparent pixels, and the sprite still loads. A GRF record that points outside the
  archive (the kRO `data.grf` has such a deleted-file record) is left out, and the other
  files stay readable.
- A GIF (a guild emblem or a `.gif` texture) is decoded as the original does, but never read
  or written past its buffers: bytes after the end of the file read as zero, a colour index
  past the palette as 0, an LZW stream stops at the end of its data, a code chain that loops
  skips its frame, and LZW table entries past 4096 or pixels past the frame are dropped. A GIF
  that would need more than 256 MB for its frames or for one image's pixels is refused instead
  of running out of memory.
- An empty XML element written without a space before the slash, such as `<readfolder/>`
  in `clientinfo.xml`, is read as empty. The original cut the tag name at the slash before
  checking for it, so the element swallowed the rest of the file and the connections in it.
- An `&` in XML text, as in a URL's query, no longer skips everything up to the next `;`. The
  original did so outside `<registrationweb>`, and a `&` with no `;` after it swallowed the
  rest of the file.
- `ItemMoveInfoV5.txt` and `MapInfoTable.scp` are read with checked numbers and bounds. A
  malformed or short file no longer reads past its text or its rows.
- A file without an extension in the `skin` folder is skipped like other files. The original
  crashed at start-up.
- The start-up registry read closes its probe key after opening it, not before, and resets the
  value size before each query. A saved skill level outside 0..65535 is ignored.
- A framed packet length below 4 resets the connection, as an unknown packet does. With 0 the
  original read the same bytes forever and hung; 1 to 3 put every later packet out of step. A
  variable packet shorter than its table minimum is read as the original reads it, but the
  bytes up to the minimum are zero instead of an earlier packet's.
- The list of items a store cannot take (`ZC_MCSTORE_NOTMOVEITEM_LIST`, 0x08b1) shows as many
  whole names as fit in 511 bytes, and an empty list shows none. The original joined the names
  with `sprintf` into a 512-byte stack buffer, so a long list from the server overran the
  stack, and an empty one still read one item id past the packet.
- A navigation name too long for its 100-character buffer is converted only as far as the
  buffer holds. The original's name splitter passed the name's byte count as the buffer's size,
  unlike its other conversions, so a long name overran the stack.
- Player-name requests and the name cache still time out correctly
  when the millisecond clock wraps, after 49.7 days of Windows uptime.
- The emote cooldown still lets an emote through 2.5 seconds after the last one when the
  millisecond clock wraps. The original added the wait to the last emote's time and compared,
  so after the wrap every emote was refused until the next map load.

## Graphics and display

- The renderer is Direct3D 9 (fixed function) instead of DirectX 7 (DirectDraw 7 and
  Direct3D 7); DirectDraw is no longer used. Vertex processing is in hardware, with software as
  the fallback. A lost device is reset and every state the client set is replayed. The frame a
  fade-out starts from is saved into a render target instead of a plain surface.
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
  `RAGEXE_VSYNC=0` turns vsync off. The map-loading progress frames, which the original
  shows without waiting for the vertical blank, are presented without waiting too; one that
  finds the previous frame still queued is skipped.
- A device the driver reports as hung or removed, or one whose reset fails 8
  times in a row, is replaced by a new device on the same adapter, with the textures copied
  over. If the new device refuses the exclusive mode, the client falls back to windowed.
- A saved display device that is no longer attached (an unplugged
  monitor, a swapped GPU) is reset to the primary display in the registry, and a message box
  says so once.

## Performance

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
  millisecond. It still sleeps a millisecond while it draws nothing (alt-tabbed out of
  exclusive fullscreen), when no present waits for the display.
- Text measurements are cached, keyed on the font, the charset and the
  string.
- The lookup key prepared from a resource name is remembered, so per-frame
  fetches such as `shadow.spr` skip lower-casing and alias resolution. `RAGEXE_RES_KEYS=0`
  turns it off.
- Finding an actor by its id goes through an index instead of walking
  the world's actor list. Ids that are absent are remembered too.
- An actor's displayed job reads the effect-state mask once instead of
  once per rule (up to forty virtual calls).
  This and the actor index above are meant to give the original's answers; the oracle tests do
  not cover them yet.
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

## Diagnostics built in

- An unhandled exception writes `crash_report.txt` (appended, at most 2 MB)
  and a minidump `crash-<date>-<time>-<pid>-<tid>-<n>.dmp` beside the exe. The report
  includes the exception, the address as an exe offset, memory and module lists, job, map and
  cell, the last effect ids and the render checkpoint. The original's error dialog then shows
  the report, and it can be copied from there.
- With `RAGEXE_OVERLAY=1` the client draws an FPS and frame-phase overlay
  (scene, present, logic, text, busiest render lists, draw and state counts). Without
  it the overlay never shows; with it, Scroll Lock hides and shows it.
- With `RAGEXE_DRIVE_STATE=<file>` the client rewrites that file about four
  times a second with `key=value` lines. They hold the mode and screen, the map, the
  character's cell, HP, SP and level, the pickable actors with their screen boxes, the open
  and modal windows, the chat focus and the last chat lines. The file is for scripts that
  drive the client in tests.
- `RAGEXE_SWITCH_DEVICE=1` takes the device-switch path for every display change, to test it
  on a machine with one monitor.

## The C rewrite itself

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

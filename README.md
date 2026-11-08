<p align="center">
  <img src="res/icon.png" width="112" alt="Ragnarok Online icon">
</p>

<h1 align="center">2022-04-06 Ragexe in C</h1>

<p align="center">
  <img src="https://img.shields.io/badge/language-C11-00599C" alt="language: C11">
  <img src="https://img.shields.io/badge/renderer-Direct3D_9-6e40c9" alt="renderer: Direct3D 9">
  <img src="https://img.shields.io/badge/platform-Windows_x86-0078D6" alt="platform: Windows x86">
</p>

"Is there a client source?" has been the Ragnarok Online community's question for as long as
there have been servers to play on. For one client the answer is now yes. This is the Korean
client of 6 April 2022, the one the BrowEdit community recommends for its stability, as a
complete C source tree you can read, build and change. Visual Studio turns it into an exe that
plays like the original.

It draws through Direct3D 9 instead of DirectX 7, and it fixes what made the original crawl:
the equipment window, the skill tree, the shortcut bar and half a dozen other windows redrew
themselves every frame, whether anything in them had changed or not. Standing in a town, this
client redraws no window at all.

The tree is reconstructed from this exe:

| | |
| --- | --- |
| file | `2022-04-06_Ragexe_1648707856.exe` |
| SHA-256 | `656b2f274c09dbb99787b3205e51feaac884d761bc11fbe31926acd92b1971a7` |
| MD5 | `f914d47b44f11dbb73e51146d1993674` |

## C, not decompiled C++

The original is MSVC C++. A decompiler's output of it is C++ in disguise: `this` passed by
hand, `std::string` and `std::vector` read as byte offsets into the MSVC runtime's layouts,
virtual calls as jumps through fixed slots of a table. Code like that compiles, but it cannot
be changed without knowing the runtime it imitates.

This tree is written as C. Its containers are its own: `std::string`, `vector`, `list`, `map`
and `deque` became `String`, `Vec`, `List` and `Tree` in `src\std`, with plain C ownership, so
whoever creates one frees it. Classes became structs with explicit construct and destroy
functions, and virtual tables became operation tables, kept only where the implementation
really varies.

Every function is an ordinary C function. There are no `__thiscall` or `__fastcall` shims;
`__stdcall` appears only where a DLL really uses it (Miles, Granny, Bink, IJL). Static
constructors became `client_init()`, which builds the globals in the original's order. The
standard MSVC runtime replaces the image's private one, yyjson and a small BSON reader of the
tree's own replace jsoncons and JsonCpp, and an out-of-memory error ends with a message
instead of a C++ exception.

C was chosen because nothing in it runs unseen: no constructor, copy or exception the
compiler adds. What a function does is what it says, and that is what makes it possible to
hold it against the original's assembly line by line.

## Less than half the size

The original exe is 11.6 MB; this one is about 4.5 MB. Their resources are the same size,
148 KB. The difference is in what the code is made of: apart from the anti-cheat, nothing the
client does was dropped to get there.

About 2.5 MB of the original's 8.8 MB of code is not the game. It carries its own C runtime
and the exception handlers the compiler generated (1.2 MB), static constructors (470 KB),
Gravity's anti-cheat libraries for GameGuard, Themida and file checks (246 KB), BasicExcel, a
spreadsheet reader (168 KB), two JSON libraries, jsoncons and JsonCpp (190 KB), and its own
Lua, libpng and zlib (256 KB).

Here the C runtime is the system's DLL and the anti-cheat code is gone. yyjson and the tree's
own BSON reader take the JSON libraries' place, and Lua, libpng, zlib and yyjson together come
to about 360 KB.

The game code itself went from about 6.3 MB to 3.2 MB. C++ expanded the work of
`std::string`, `vector` and `map` inline at nearly every use, and kept exception bookkeeping in
every function that holds an object with a destructor: the original records its unwind state
about 57,000 times and destroys a string at about 9,000 call sites. In C a string is freed
once, where its owner is done with it, and nothing is recorded for unwinding.

Code nothing calls is removed, a rule of this tree, and the relocation table shrinks with the
code, from 641 KB to 193 KB.

## How do I get it running

Build it with Visual Studio 2022 or its Build Tools. MSVC has no C-only install: select the
x86 C++ build tools, whose compiler builds C too. From cmd or PowerShell in this folder:

    build.bat

This builds `build\ragexe.exe`. Its only argument is `dbg`: `build.bat dbg` builds
`build\ragexe_dbg.exe`, as optimized as the release, with `build\ragexe_dbg.pdb` for a debugger.
Either way `build.bat` builds the libraries in `vendor\` first (into `build\vendor\`), then
compiles every `.c` under `src\` and links.

Then copy the exe into a Ragnarok client folder, such as a kRO full client. It needs, beside
it:

- the game's DLLs: `binkw32.dll`, `cps.dll`, `granny2.dll`, `ijl15.dll`, `mss32.dll` and
  `libcurl.dll`, all in a kRO full client, and the Visual C++ 2015-2022 runtime for x86;
- `data.grf` and the rest of the game data;
- `clientinfo.xml` with your server's `<address>` and `<port>`, as `data\clientinfo.xml` or
  inside a GRF;
- `msgStringTable.txt`, from a translation or your server's data. The client has no built-in
  strings, so without it every label reads "NO MSG".

The client looks for each file in its GRFs first and in the `data\` folder after, so a copy inside
`data.grf` wins over a loose one. `<readfolder/>` in `clientinfo.xml` turns that order around.

`/account:<file>` on the command line reads another file in place of `clientinfo.xml`, looked
up the same way, so one client folder can hold the settings of several servers:

    ragexe.exe /account:myserver.xml

reads `data\myserver.xml`. The name must be shorter than 64 characters: the original copies
it without a length check, and so does this client.

No launcher, `Setup.exe` or administrator rights are needed: the client starts with any command
line and keeps its settings under `HKEY_CURRENT_USER`.

## Changes from the original client

The client is meant to behave like the original exe. [docs/changes.md](docs/changes.md) lists
every deliberate difference in detail; in short:

It runs on its own, with the usual NEMO patches built in. It needs no launcher argument and runs
no nProtect GameGuard, file-name or patcher check. It logs in with packet 0x64 to the server in
`clientinfo.xml`, reads `msgStringTable.txt` for every service, and keeps its settings under
`HKEY_CURRENT_USER`.

It draws with Direct3D 9: borderless fullscreen, 4x multisampling, resolution and fullscreen
changes without a restart, modes up to 8192x4096, and recovery from a lost or removed device.
AltGr types text in text fields instead of triggering hotkeys.

It is faster. Windows redraw only when what they show changes, draws are batched, states and
uploads the device already has are not sent again, text and resource lookups are cached, and the
exe can use 4 GB under 64-bit Windows. It also writes a crash report with a minidump, and has an
FPS overlay (`RAGEXE_OVERLAY=1`) and a state file for test scripts.

And it fixes bugs of the original: dangling pointers, uninitialised values, overruns, text used
as format strings, endless loops on malformed text, and crashes on short or malformed packets
and files.

## Contributing

The aim is a client that behaves exactly like the original exe, apart from the changes in
[docs/changes.md](docs/changes.md). If you find a reconstruction bug, anything this build does
differently from the original, feel free to open a pull request (or an issue, if you have not
tracked it down yet).

The tree keeps to a few rules:

- C11, the MSVC compiler; one declaration per function and object, in its header.
- No comments in the sources; names say what things are.
- Nothing that the client does not use: a function, global, type or macro without a user
  is removed.
- Third-party code in `vendor\` stays as published, with its licence.

Before you open a pull request, run

    tests\build.bat

It builds and runs every behavioral test, which takes about 15 minutes; each prints a line that
ends in "passed". A single area has its own `tests\build_<name>.bat`. The tests need no game
data, server or account; the ones in `tests\oracle` also run the original exe, and are skipped
unless it is at `..\client-files\2022-04-06_Ragexe_1648707856.exe` or `%ORACLE_EXE%`.

# Portable client

`purist-portable` is a long-lived deviation branch on top of
[`purist`](https://github.com/HampterDev/2022-04-06-ragexe-in-c/tree/purist): the purist client,
playable natively on 64-bit Linux, macOS and Android as well as on Windows. It is not meant to be merged
back or deleted; `purist` is merged into it as it moves. Purist's own deviations from `main` are
listed in [its README](https://github.com/HampterDev/2022-04-06-ragexe-in-c/blob/purist/README.md)
and still apply here. These are the deviations from `purist`:

- Build the client for 64-bit Linux and macOS (x86_64 and arm64) with CMake
  (`-DRAGE_CLIENT=ON`); on macOS as `ragexe.app`. Windows still builds with `build.bat`, and
  `build.bat x64` builds a 64-bit Windows client (Direct3D 9; sound through SDL3; Granny models
  from converted glTF).
- Build the client for Android (arm64 and x86_64) as an APK with Gradle (`android/`): SDL3's
  activity, OpenGL ES 3.0, the game folder in the app's own storage (filled from a folder the player
  picks, on first start), touch as the mouse and the on-screen keyboard for edit boxes; in a
  desktop window (ChromeOS, Android's desktop windowing) mouse, wheel and keyboard as on a PC.
- Run on an SDL3 application layer off Windows: window, input, IME text, clipboard and cursor.
- Draw with an OpenGL 3.3 core (macOS: 4.1, Android: OpenGL ES 3.0) backend behind `render/gfx.h`
  off Windows, scaled to the window with the aspect kept; Direct3D 9
  stays the Windows backend.
- Draw text with FreeType and Fontconfig off Windows, sized and placed as GDI does; Android finds
  its fonts by scanning the font folders.
- Convert the Windows code pages with the client's own tables of them on Android, whose C library
  has none (`tools/code_pages.py` reads them from Windows).
- Compile `char` as signed everywhere, as MSVC does.
- Play sound through an SDL3 implementation of the Miles subset the client uses off Windows.
- Reach files, threads, sockets, time, settings, memory, crash reports and the desktop through a
  platform layer; Windows keeps its registry, CryptoAPI hashes, minidumps and Win32 calls.
- Keep settings in `client_settings.ini` in the game folder off Windows.
- Replace IJL with a portable JPEG codec for screenshots and JPEG textures on every platform.
- Draw the Granny models off Windows from glTF files that `tools/granny_export` converts once from
  the game's `.gr2` files ([tools/granny_export](tools/granny_export/README.md)).
- Build 64-bit clean: pointer-width UI and game-object message arguments, standard formatting,
  string and integer functions instead of MSVC ones.
- Draw in an order that does not depend on heap addresses (texture serials, submission order,
  emitter particles in insertion order); the multitexture sort compares like with like.
- Keep `rand()` to the 15 bits the client expects where it divides or rejects by that range.
- Show the icon menu's bottom border and corner with fewer than 21 icons.
- Add the scene harness for pixel tests on every platform ([tests/scenes](tests/scenes/README.md)).
- Build and test Windows, Linux and macOS, and build Android, in CI (`.github/workflows/portable.yml`).

How it is built, what each platform layer does, how it was verified and what is left:
[docs/portability.md](docs/portability.md).

To take in new purist work:

```sh
git fetch origin
git merge origin/purist
```

# What this tree changes from the original

This tree is the 2022-04-06 client as plain C, with a small, deliberate set of changes. This
page lists them so that a difference between this client and the executable can be told apart
from a reconstruction error: a difference that matches a row here is intended; any other is a
bug in the lift. Each row says what the executable does and what this tree does, nothing more.

## Starting the client outside Gravity's service

The executable expects the service's launcher, anti-cheat and integrity files around it. The
usual patches that let a client run on its own are applied here.

| change | the executable | this tree |
| --- | --- | --- |
| launcher argument | goes on only with `1rag1`, `1sak1`, `1gld1`, `2rag2`, `1eve1` or `1pk1` on the command line, else shows a message and exits | runs with any command line (`core/winmain.c`) |
| module scan | for service korea runs the IProcess module scan and exits when it fails | does not run it |
| GameGuard | runs the nProtect GameGuard blocks of the game and login modes | the GameGuard interface is empty (`security/gameguard_mgr.c`) |
| anti-cheat scans | two scans run inside the GameGuard manager's creation and stop the start-up on a hit | the scans are not run |
| integrity list | loads `RagHash.dat` at start-up and stops when it is missing, holds fewer than two entries or fails its checksum | starts with an empty list and never reads the file (`security/gg_hash_mgr.c`) |
| GDI tamper check | a schedule compares the `TextOutW` import with a fresh lookup, looks for a debugger, and on a mismatch disconnects at random | the schedule does not exist |
| account server | chooses a `*.ragnarok.co.kr` account server by server type | takes `<address>:<port>` from `clientinfo.xml` (`platform/clientinfo.c`) |
| registry hive | settings live under `HKEY_LOCAL_MACHINE` | under `HKEY_CURRENT_USER` |
| patch applier | the start-up's patch applier opens `data.grf ` (with its trailing space) and returns before writing | the open is kept for its side effect, the applier is not (`core/archive_startup.c`) |

`tests/parity/original_patches.txt` applies the first five of these to the executable itself so
that it can run in the parity harness; the two lists are meant to describe the same set.

## Service type and messages

| change | the executable | this tree |
| --- | --- | --- |
| exe digest | sends the digest packet `0x0204` only for service 6, or for korea with server type 1, and replaces the digest with a fixed key by service and server type | always sends `0x0204` with the exe's own MD5 (`login/account.c`) |
| message strings | takes a separate path for service korea when loading the message strings | reads `msgStringTable.txt` for every service (`core/msg_str.c`) |

## Rendering

| change | the executable | this tree |
| --- | --- | --- |
| graphics API | DirectDraw 7 and Direct3D 7 | Direct3D 9 (`render/gfx_d3d9.c`) |
| UI redraw | every UI window re-composites its canvas every frame | a window re-composites only when something it shows changed (`ui/uiwindow.c`, `ui/uirender.c`) |
| scene target | draws on the back buffer | draws on a multisampled scene target, four samples, copied to the back buffer |
| draw calls | one primitive per quad | consecutive pre-transformed quads become one indexed list (`render/gfx_batch.c`) |
| device state | re-sends render and stage states at every list flush; locks the render target every frame | skips a state the device already holds; does not lock the target |
| texture uploads | re-uploads a sprite sheet or canvas whenever it is re-composited | skips an upload whose bytes did not change |
| caches | computes the billboard rotation, the angle's sine and cosine, and every text extent on each use | keeps the last angle's values and memoises text extents; resource names are looked up through a key cache |
| device loss | a hung or removed device cannot be reset | a fresh device is created on the same adapter and the textures copied across |
| display settings | fullscreen is an exclusive mode switch; a size change recreates the device; the mode list stops at 2048 x 1440; a registry GUID for a device that is gone is used as is | a borderless fullscreen window is offered (`ISBORDERLESS`); a windowed size change applies live; modes up to 8192 x 4096 are listed; a stale GUID falls back to the primary device |
| frame pacing | `Sleep(1)` after every frame | `Sleep(1)` only while the window is inactive in fullscreen, else `Sleep(0)` (`game/mode_run.c`) |
| frame save | the fade-out saves its frame into a plain surface | into a render target |

## Input and interface

| change | the executable | this tree |
| --- | --- | --- |
| AltGr | Ctrl+Alt is Ctrl+Alt | AltGr types the keyboard layout's third-level character (`ui/wndmgr/key_down.c`) |
| hit numbers | the SP-damage byte of a hit is left as the stack held it | it is zero (`game/packets_act.c`) |
| cash shop buttons | the button loop runs one past its tables | it stops at the last entry |
| timers | a timer of an unknown kind shows its object's address | it is hidden |

## Internals that answer the same

Two routines were restructured for speed and are meant to give the same answer as the
executable's: the job an actor is drawn as (`actor/getjob.c`) reads the effect-state mask once
instead of once per rule, and the world finds an actor by account id through an index beside its
list (`actor/actorindex.c`) instead of by walking the list. Neither has been proven equal by the
oracle tests yet.

## Diagnostics the executable does not have

A support log and a packet log beside the exe, a crash report with a minidump, an optional state
file rewritten a few times a second, counters around the resource caches, and a frame profiler
overlay. They record; they do not change what the client does.

## Bounds and lifetimes

The translation to C gives wire data and text buffers explicit lengths and gives objects explicit
owners. Where the executable reads or writes past a buffer, this tree stops at its end; where it
reads a value nothing had written, this tree reads zero; where it would loop forever on a glyph
wider than its row, this tree moves on. These sites are not listed one by one: they are the
difference between the executable's behaviour on malformed input and this tree's, and on
well-formed input the two agree.

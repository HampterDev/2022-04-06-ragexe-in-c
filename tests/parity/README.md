# tests/parity: record and replay sessions against a reference client

The parity harness plays a scripted session on a game client and compares what the client did
with a recording: every packet it sent, the pixels of chosen frames, and a text dump of its state
(the windows and their controls, the units on screen, the world's lists). Two clients can be held
to the same recording, so a divergence in behaviour shows up as a difference in the packets or
the state, frame for frame, without anyone watching a screen.

## The pieces

| piece | what it is |
| --- | --- |
| `parity.c` -> `parity.dll` | loaded into the client before any of its code runs; rewrites the exe's import table so the clock is virtual (16 ms per frame), the network goes to the runner, input comes from the runner, the window stays off screen, the registry is virtual, and the runner is told at every present |
| `parity_launch.c` -> `parity_launch.exe` | starts a client suspended, injects the DLL, resumes it |
| `parity.py` | the runner: `smoke`, `determinism`, `record`, `replay`, `offline`, `compare`, `bless`, `check`, `pair`, `upgrade`; its docstring is the reference |
| `scenario.py`, `inputs.py`, `scenarios/` | a scenario is a list of timed steps (keys, clicks addressed to windows the state probe finds, chat lines, captures, state dumps, the quit) plus one of three networks: offline, record (relayed to a real server), replay (the recording answers) |
| `recordings/` | per scenario, the recorded session (`NAME.json`) and the golden it is judged by (`NAME.golden.json`: pixel hashes and state texts from two agreeing runs of the reference) |
| `reference/` | `reference.py` builds the reference client from a herc-client checkout (expected beside this repository, or `--herc PATH`) with `reference/parity_probe.c` compiled in; the recordings here were made against it |
| `src/parity/` (in the client) | the state probe the DLL calls between frames: `parity_probe.c` writes the windows and the picks, `parity_world.c` the world section; the client exports `parity_probe` and `parity_call` |
| `seqfuzz.py`, `seqgen.py`, `seqtarget.py`, `seqbounds.py` | the packet-sequence differential tester: generated server packet sequences played to both clients, their answers compared |
| `variants.py`, `variants/` | data variants (messages emptied, Lua tables changed, files removed) laid over a run folder |
| `teardown.py`, `upgrade_states.py`, `editbounds.py`, `recbounds.py`, `item_texts_full.py`, `patches.py` | the other probes and sweeps the runner's docstring describes |

## Setting up

1. Build this repository's client (`build.bat`); the probe in `src/parity` is part of it.
2. Build the harness: `tests\parity\build.bat` puts `parity.dll` and `parity_launch.exe` in `build\parity`.
3. Build the reference: `python tests\parity\reference\reference.py --herc <herc-client checkout>`,
   into `%PARITY_REFERENCE%` (default `C:\parity\reference`).
4. Freeze a copy of the client data at `%PARITY_DATA%` (default `C:\parity\data`): the GRFs, BGM,
   skin and data files a client runs from. The runner never touches a client folder of your own.
5. Runs go under `%PARITY_RUNS%` (default `C:\parity\runs`), one fresh folder per run.

Then, for example:

```
python tests\parity\parity.py smoke --client ragexe
python tests\parity\parity.py check camera --client ragexe
python tests\parity\parity.py pair fighting
```

`--client` is `herc` (the reference), `ragexe` (this repository's build; `PARITY_RAGEXE_EXE`
names another build) or `original` (the untouched 2022-04-06 client, `PARITY_ORIGINAL_EXE`, by
default where `tests\oracle` expects it). The original is started as the patcher starts it
(`1rag1`), its manifest's request for elevation is ignored (`__COMPAT_LAYER=RunAsInvoker`), and
`parity.dll` applies `original_patches.txt` to it before it runs: the anti-cheat scans answer
"clean", the GameGuard blocks are never entered, the IProcess module scan cannot end the
start-up, and the hash manager accepts a missing RagHash.dat, each the behaviour the lifted
client has. Every patch names the bytes it expects, so another exe is refused and logged. The
original has no state probe, so only its pictures and its packets can be compared.

The frozen data folder is made from the client folder with hard links: the GRF, the game DLLs
(`cps`, `binkw32`, `granny2`, `ijl15`, `mss32`, `libcurl`, `mp3dec.asi`, `mssfast.m3d`), `data\`,
`System\`, `BGM\`, `AI\` and `GameGuard\`; no save data, so every run starts clean. The
arena the DLL reserves shrinks when a client's address space cannot hold a gigabyte (the original
is not large-address-aware).

## What is not here

- A server. `record` relays to a real server; `stack.py` expects a server stack of your own and
  the start script it names is not included. Everything else, `replay`, `check`, `pair` and the
  sequence tester among it, runs from the recordings with no server.
- Approved deviations. `improvements/` is empty and `saved_expected.json` holds none: this client
  is held to the reference's golden in full. A state that must differ on purpose is added with
  `parity.py expect`, with its reason.
- Coverage bookkeeping tied to feature flags; the scenarios carry no coverage claims.

## What the recordings can and cannot judge

The recordings and goldens were made against the reference, a herc-client build that carries
deliberate changes to the original: among them the login window sends the plain login request
(`0x0064`, after the exe digest and a `0x0200` queue packet) where the original, and this tree,
send the token login (`0x0acf`); a one-row character-server list is skipped; the licence screen
is hidden; the English kRO data set is read. So `check` of this tree fails at the first login
packet of every scenario that logs in, by the reference's design and not by a fault of this
tree, and the offline sweeps (`windows_login`, `probe_startup`) are the only goldens that judge
it as it is. A replay difference is a finding to be read, never a harness fault; but a golden
is only as faithful as the reference it came from.

First results on this tree (2026-11-08): `smoke --client ragexe` passes (240 frames, four
captures), `determinism --client ragexe` passes (two runs, every pixel the same), and
`check login_refused --client ragexe` diverges at frame 251 on the first packet as described
above. `smoke --client original` gets through the start-up with the patches and stops at the
renderer: the original draws through DirectDraw 7, which the DLL does not hook (it syncs frames
and captures pictures at Direct3D 9's `Present`), so the original can run in the harness only
once the DLL also treats a DirectDraw `Blt` or `Flip` to the primary surface as the frame's
end. Pictures of the two would still differ renderer for renderer; the comparison worth having
against the original is of the packets and their frames.

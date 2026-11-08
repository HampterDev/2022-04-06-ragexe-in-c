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

`--client` is `herc` (the reference) or `ragexe` (this repository's build; `PARITY_RAGEXE_EXE`
names another build).

## What is not here

- A server. `record` relays to a real server; `stack.py` expects a server stack of your own and
  the start script it names is not included. Everything else, `replay`, `check`, `pair` and the
  sequence tester among it, runs from the recordings with no server.
- Approved deviations. `improvements/` is empty and `saved_expected.json` holds none: this client
  is held to the reference's golden in full. A state that must differ on purpose is added with
  `parity.py expect`, with its reason.
- Coverage bookkeeping tied to feature flags; the scenarios carry no coverage claims.

The recordings and goldens were made against the reference, so a replay of this client can fail
on behaviour the reference has and this tree does not, or the other way round; each such failure
is a finding to be read, not a harness fault. None of the scenarios has been run against this
tree yet: the port was built and compiled on 2026-11-08, not executed.

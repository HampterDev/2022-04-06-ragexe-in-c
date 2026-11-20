# Working on this tree

This is `custom`: the Ragexe 2022-04-06 client as C from `main`, with deliberate changes on top
(features removed, local saves in place of web services, UTF-8 text, 64-bit and other platforms).
These notes are for anyone changing it, person or agent.

## Two kinds of change

A reconstruction bug is a place where the code `custom` shares with `main` does something the
original exe does not. Fix it on `main` first, by the rules in `main`'s `AGENTS.md`, then pick it
here with `git cherry-pick -x`. Where `custom`'s code differs, adapt it and say how in an
"On custom" paragraph of the commit message. Record every pick, and every fix that is not needed
here, in `docs/upstream/README.md`.

A deliberate change goes only here. Add it to the list in `README.md`, which names every way
`custom` differs from `main`, and record the decision in `docs/custom-review.md`.

When `main` reverts one of its fixes to match the original, check why before following it. If the
fix itself was wrong (it read the wrong routine, or misread what the original does), follow the
revert. If it was a deliberate improvement on the original's behaviour, `custom` may keep it; say
so in `docs/upstream/README.md`.

Read the original's assembly before deciding which kind a change is. Follow the real dispatch, the
jump table or vtable slot the code goes through, not a disassembler label's name: labels are often
wrong.

Fix every real bug you find, not a chosen few. Leftovers found on the way, such as dead code, a
half-removed feature or a name that lies, are part of the task: fix them rather than list them.

## Replacing things

When new code takes over a job, the old way goes in the same change: no fallback, opt-in switch
or second code path kept to be safe. A reference build outside the tree is fine for comparing.
When a fix needs a second patch for the same symptom, look for the structural cause instead, and
design from what the thing must do, not from how today's code does it.

New logic with one job goes into its own library in `tools/libraries.txt`, behind a narrow header
and with its own tests, rather than into a file that already does something else.

## Data from outside

Server packets, file contents and names other players chose are not under the client's control.
Bound every read and write to its buffer or packet field, copy text as text and never use it as
a format string, and carry on. Do not assert or abort on such data.

## Code

- C11 with MSVC on Windows; the CMake builds in `docs/portability.md` must keep building.
- Nothing unused stays: a function, global, type or macro without a user is removed.
- Third-party code in `vendor/` stays as published.
- The pieces under `game/` are changed as "The pieces in `game`" below says.
- Packet, file and DLL layouts stay the original's, except where `README.md` lists a change.
- Every source file belongs to a library in `tools/libraries.txt`; `build.bat check` checks the
  includes and links against it.

## The pieces in `game`

The actor rules, the records a viewer keeps, delivery, the delivery routes and the other pieces in
the submodule `game` are edited in `game`, not here: `game/CONTRIBUTING.md` says where (the `game`
submodule of your own worktree of this repository, never another worktree's and never the folder
`game` beside this repository), which commits a change needs (the piece, `game`'s pin of it, this
repository's pin of `game`, on one topic branch), and how they land. A commit here pins only a
`game` commit whose whole set passed, and a branch is merged here only when its `game` pin is on
`game`'s `main`. `python game/tools/pins.py -C game check --outer .` says whether the levels agree.

## Changes and reverts

Before reverting a commit, look for later commits built on it, such as a test changed to match
it or a later commit that replaced part of it, and handle those too.

Check every claim in a commit message or doc against the code.

## Checking your work

Before saying something works, run it the way it will be used: the option someone will turn, the
exe that will be handed on, with what it needs to start. For anything drawn, compare pictures
with the scene harness in `tests/scenes` instead of asking someone to look. Say what was not
checked. Another agent's finding, even "confirmed against the asm", is a lead until you have read
the code yourself.

Checks run with a hidden window and no sound. Never start the client fullscreen.

## Building and testing

While working, run the tests the change reaches and `build.bat check`. Run `build.bat check` and
the whole suite, `tests\build.bat`, once before committing, for everything going into the
commit. A commit that changes only documentation needs neither.

From a shell that sets `NoDefaultCurrentDirectoryInExePath` (agent shells do), clear it first, or
`cmd` will not find `build.bat` in the current directory.

Every test is declared once in `tests/CMakeLists.txt` (`rage_test`; the helpers at its top say
what each keyword does), in the section of the platforms it runs on, except the own tests of the
pieces `rules` and `delivery` from the submodule `game`, which their own `CMakeLists.txt` declares
(`rules_*`, `delivery`). `tests\build.bat` configures
`build\tests` with Ninja the first time, builds only what changed since the last run and runs the
tests with CTest on every core; then the 32-bit oracle tests in `build\oracle`. These need the
original executable, which `ORACLE_EXE` names (by default
`..\client-files\2022-04-06_Ragexe_1648707856.exe` beside the checkout); without it they are
skipped and the suite still passes, so set it for a run that has to cover them. Each client file
is compiled once, into one static library per library of `tools/libraries.txt`, so a change
recompiles the files it touches and relinks only the tests that link their library. Arguments go
to CTest:

- While working, run the tests the change reaches: `tests\build.bat -R <regex>` (tests by name,
  `ctest --test-dir build\tests -N` lists them) or `tests\build.bat -L <library>` (every test that
  compiles, links or includes a `.c` file of that library, for example `-L render`; for a header,
  the libraries whose files include it).
- Before committing, the whole suite once: `tests\build.bat`. Unchanged tests are not rebuilt.
- A change to docs only needs no suite.

Every test runs headless and silent: CTest gives it SDL's dummy video and audio drivers (`audio`
the `disk` driver, which writes files), and on Windows the message boxes of the client code
under test go to stderr (`tests/headless.h` for `platform_message_box`). A test that needs a real
window is labelled `interactive` and left out by `tests\build.bat`; tests that need the game
folder beside the checkout are labelled `client-files`. Keep new tests that way.

The netcode is checked against the real server with the playground (`tests/playground/README.md`):
the client and the server in lockstep on one virtual clock, with a hidden window and no sound.

## Commits

- Commit only the files your change touches; other work may be going on in the same checkout.
- No AI co-author or session trailers in commit messages.
- Push only when asked.

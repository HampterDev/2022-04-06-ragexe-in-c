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
- Packet, file and DLL layouts stay the original's, except where `README.md` lists a change.
- Every source file belongs to a library in `tools/libraries.txt`; `build.bat check` checks the
  includes and links against it.

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

While working, run the tests for the code you changed and `build.bat check`. Run `build.bat
check` and the whole suite, `tests\build.bat`, once before committing, for everything going into
the commit. A commit that changes only documentation needs neither.

From a shell that sets `NoDefaultCurrentDirectoryInExePath` (agent shells do), clear it first, or
`cmd` will not find `build.bat` or the test programs in the current directory.

## Commits

- Commit only the files your change touches; other work may be going on in the same checkout.
- No AI co-author or session trailers in commit messages.
- Push only when asked.

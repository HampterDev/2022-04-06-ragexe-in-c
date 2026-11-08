# Working on this tree

This tree is the Ragexe 2022-04-06 client as C, meant to behave like the original exe. These notes
are for anyone changing it, person or agent.

## What counts as a bug

A reconstruction bug is a place where this tree does something the original exe does not. Fix it
to match the original, even where the original's behaviour looks wrong.

A bug the original has too is not a reconstruction bug, and a feature or UX change is not a bug
fix at all. Make such changes only on purpose, and list each in `docs/changes.md`. When reviewing
a fork or an outside patch, take only its reconstruction bugs.

Read the original's assembly before deciding which it is. Follow the real dispatch, the jump
table or vtable slot the code goes through, not a disassembler label's name: labels are often
wrong. When the lift and a proposed fix both differ from the original, match the original.

Fix every real bug you find, not a chosen few.

## Data from outside

Server packets, file contents and names other players chose are not under the client's control.
Bound every read and write to its buffer or packet field, copy text as text and never use it as
a format string, and carry on. Do not assert or abort on such data.

## Code

- C11 with the MSVC compiler. No comments in the sources; names say what things are.
- Nothing unused stays: a function, global, type or macro without a user is removed.
- Third-party code in `vendor/` stays as published.
- Packet, file and DLL layouts stay the original's.

## Changes and reverts

Before reverting a commit, look for later commits built on it, such as a test changed to match
it or a later commit that replaced part of it, and handle those too. Run the whole suite,
`tests\build.bat`, before committing.

Check every claim in a commit message or doc against the code. A commit message says what the
original does and how that was checked.

## Commits

- Commit only the files your change touches; other work may be going on in the same checkout.
- No AI co-author or session trailers in commit messages.
- Push only when asked.

# Working on this tree

This tree is the Ragexe 2022-04-06 client as C, meant to behave like the original exe. These notes
are for anyone changing it, person or agent. Most of them come from mistakes made here before.

## What counts as a bug

A reconstruction bug is a place where this tree does something the original exe does not. Fix it
to match the original, even where the original's behaviour looks wrong.

A bug the original has too is not a reconstruction bug. Fix it only on purpose, and list it in
`docs/changes.md` under "Bugs of the original fixed". A change of behaviour, such as a feature, a
UX tweak or a policy, is not a bug fix at all. When reviewing a fork or an outside patch, take
only its reconstruction bugs.

Read the original's assembly before calling something either. Reviews here have been wrong both
ways: a "fix" that reversed a correct lift because a status code was misread, and a "lift bug"
the original turned out to have as well. When the lift and a proposed fix both differ from the
original, neither is right: match the original.

Fix every real bug you find, not a chosen few, and report each one.

## Reading the disassembly

Follow the real dispatch, the jump table or vtable slot the code goes through, not a label's
name. Disassembler labels are often wrong: a function named after a string it happens to use, a
case filed under the wrong kind.

A float constant written in hex is its IEEE-754 bit pattern. Write the decimal literal that
compiles to the same bits, and check them: `0x4008f5c2` is `2.1399999f`, not `2.14f`.

Names must say what the code does. A function named for one state that tests another has
already led to a wrong fix. When a name lies, rename it.

## Data from outside

Server packets, file contents and names other players chose are not under the client's control.
Bound every read and write to its buffer or packet field, copy text as text and never use it as
a format string, and carry on. Do not assert or abort on such data: that turns bad input into a
crash anyone can trigger.

## Code

- C11 with the MSVC compiler. No comments in the sources; names say what things are.
- Nothing unused stays: a function, global, type or macro without a user is removed.
- Third-party code in `vendor/` stays as published.
- Packet, file and DLL layouts stay the original's.

## Reverting and porting

Before reverting a commit, look for later commits that build on it: a test changed to match it,
a stub added for it, or a later commit that already replaced part of it. Revert or adjust those
too. Then run the whole suite, `tests\build.bat`; every test must end in "passed".

Ported code follows this tree's rules: strip its comments, and leave out test harnesses that
belong to the other project.

## Commits

- Commit only the files your change touches. Other work may be going on in the same checkout;
  do not stash or reset what is not yours.
- One concern per commit. The message says what the original does, how that was checked (the
  address or instruction), and what changed.
- No AI co-author or session trailers in commit messages.
- Copy commit hashes from git; never type a full hash from memory.
- Push only when asked.

## Docs

Check every claim against the code before writing it down. Drafts here have named libraries that
were not replaced and logs the client does not write.

Every deliberate difference from the original goes in `docs/changes.md`, with what the original
did. The README only summarises it.

In the README, prefer paragraphs to bullet lists. A command example shows only the command, not
a second column with its output. A Markdown table needs a real header row: GitHub draws the first
row as the header, so an empty one shows as a blank row.

When a script writes text, watch its escape sequences: `\v` in a Windows path has already turned
into a vertical tab. Check the result for control characters.

## The purist branch

`purist` is main with optional systems removed. Fixes both branches need land on main first and
are cherry-picked to purist with `-x`. When pruning purist, rank the candidates biggest first,
with size and a recommendation, and let the owner decide each one.

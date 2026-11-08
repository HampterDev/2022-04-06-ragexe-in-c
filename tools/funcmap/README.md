# tools/funcmap: original function -> C function

`function_map.tsv` maps addresses of the 2022-04-06 executable to the C functions of this tree,
so an assembly-versus-C check starts from the function, not from a search. One row per
(address, C function); columns:

| column | meaning |
| --- | --- |
| `addr` | the function's address in the original image (hex, no prefix) |
| `asm_label` | its label in the assembly listing (`sub_<addr>` or a class name), empty when the listing's inventory no longer holds it |
| `asm_chunk` | `text/<chunk>.inc:<first line>-<last line>` in the listing, empty likewise |
| `class` | the C++ class from the listing's RTTI class map, when known |
| `ref_name` | the function's name in the annotated C the translation started from |
| `final_name` | its name in this tree |
| `main_where` | `file:line` of the definition on `main` at build time, empty when it has since been renamed or removed |
| `method` | how the pair was established (below) |
| `confidence` | `high` for `cited`, `name`, `label`, `cited_hex`, `ustr`; `low` for `cited_multi`, `callee`, `jacc` |
| `ref_file` | the annotated file that cited the address |

Look up an address with `grep '^87af10' tools/funcmap/function_map.tsv`, a function with
`grep -P '\tworld_remove_skill\t'`. The assembly of any address is in the Binary Ninja dump
(`client-disassembly/2022-04-06_Ragexe_1648707856.exe.bndb_disassembly.txt`, lines start with
the 8-digit address) and in the assembly listing's `text/<chunk>.inc`.

Take a routine's entry address from the dump's function-start line, never from a listing label
or a map row of low confidence: several labels sit inside other routines, and the `label`,
`ustr`, `callee` and `jacc` rows have been wrong in practice. When a label has no address, read
the target from a call site's bytes (`tests/oracle/skill_oracle.h` does this).

## Where the pairs come from

The translation to C was done function by function from the assembly listing, and the C it
produced carries comments citing the original addresses. The map is mostly that provenance,
not inference:

| method | rows | how |
| --- | --- | --- |
| `cited` | 5166 | the comment above the function names exactly one `sub_<addr>` |
| `cited_multi` | 608 | that comment names several addresses (a function that merged several originals, or a family); every one is listed, low confidence |
| `name` | 60 | the function is itself called `sub_<addr>` |
| `cited_hex` | 1 | the comment says "lifted from 0x<addr>" |
| `label` | 19 | the name equals a class label of the listing (`CGameMode__Zc_Par_Change` -> `GameMode_Zc_ParChange`) |
| `dump_symbol` | 26 | a `label` row whose address sat inside another routine, moved to the start of the function the dump knows by that class and method name |
| `ustr` | 288 | a string literal that occurs in exactly one assembly function and exactly one C function |
| `callee` | 47 | the one unmapped callee of a mapped pair on both sides (about half right on held-out; verify before use) |
| `jacc` | 74 | mutual-best Jaccard over strings, imports and calls into mapped functions (about half right on held-out; verify before use) |
| `oracle` | 54 | the pair was driven over its input space by a test under `tests/oracle` (`oracle_pairs.tsv`) |
| `dispatch` | 383 | the packet dispatcher's jump table sends the packet id to that handler and this tree's `packet_dispatch.c` sends the same id to that function (`dispatch_pairs.tsv`, written by `dispatch_check.py`) |
| `*_inside` | 380 | any method whose address is not a function start in the dump: the label it came from sits inside another routine, so the address is wrong and the pairing suspect; low confidence |

`verify_map.py` applies the last four: it checks every address against the dump's
function-start lines, moves class-method labels to the dump's symbol, and writes the oracle and
dispatcher pairs over whatever paired those addresses or functions differently. Run it after
adding to `oracle_pairs.tsv` or regenerating `dispatch_pairs.tsv`.

`dispatch_check.py` reads the jump table of `CGameMode::PollNetworkStatus` (0x7a0570; ids 0x73
to 0xbb3 at 0x7aab40) from the executable and this tree's `packet_dispatch.c` and
`packet_dispatch2.c`: both dispatch the same 798 packet ids. For the arms that only push the
packet and call a handler, and whose case here calls exactly one function, it pairs the two;
the other arms inline their work or call several routines and are left to the oracle tests.

Annotated name -> final name comes from pairing the definitions before and after the renaming
to snake case, per file: exact, then by normalised name, then by order between anchors
(13,767 of 14,900 paired; the rest were helpers the renaming removed).

## Coverage

6,541 addresses, 5,866 high-confidence rows, 3,851 of them with a definition on `main` today.
Against the listing's inventory of the assembly (92,100 functions, 15,945 of twenty or more
instructions; the UI classes were translated first and are missing from it):

| chunk family | mapped / all | mapped / >= 20 instructions |
| --- | --- | --- |
| effects | 1739 / 4115 | 1462 / 2848 |
| gamemode | 143 / 2946 | 131 / 1569 |
| contents | 125 / 2307 | 117 / 1739 |
| session | 150 / 1765 | 110 / 895 |
| actor | 91 / 1530 | 59 / 768 |
| skills | 79 / 744 | 77 / 341 |
| world | 73 / 687 | 65 / 463 |
| render | 68 / 601 | 58 / 405 |
| gen (STL and compiler-generated) | 2 / 60374 | 1 / 1285 |
| initializers | 20 / 10575 | 17 / 1496 |

The big families are under-mapped not because the C lacks them but because the comments name a
range ("the asm's `sub_7516d4` .. `sub_751b2e` were its cases") or nothing, and the functions
carry no strings. The next lever is structural: aligning the callees of a mapped pair in call
order, and matching by size and position within a chunk. The dump lists 90,323 function starts;
5,301 of the high-confidence addresses are among them (the others are interior labels the
comments cite, such as a case body).

## Rebuilding

The inputs live outside this repository: the annotated C tree, two snapshots of the C before and
after the renaming, and the assembly listing with its inventory (`tools/function_map.jsonl`) and
class map (`tools/class_map.tsv`).

```
python refmap.py            <annotated C>/src   refmap.jsonl
python rename_map.py        <C before renaming>  <C after renaming>   rename.jsonl
python funcmap_build.py     refmap.jsonl rename.jsonl <listing>/tools/function_map.jsonl <listing>/tools/class_map.tsv <this repo>/src  map1.tsv
python fingerprint_match.py <listing>/tools/function_map.jsonl map1.tsv <this repo>/src map2.tsv
python finalize.py          map2.tsv <listing>/tools/function_map.jsonl function_map.tsv
```

## The switch sweeps

The reconstruction bugs found so far share one shape: a jump-table target dropped, a branch
attached to the wrong arm, twin cases made alike. Two scripts look for that shape across the
whole image instead of in the routines someone happens to pick.

- `case_sweep.py` reads every jump table in the listing's `text/*.inc` with the compare in
  front of it (base, range, default arm, lookup table) and turns it into the set of switch
  values it handles; it then matches each table to the C switches that share those values and
  reports the values no matched switch has (`case_sweep.tsv`; tables matched by no C switch,
  and tables whose compare the reader could not parse, are listed at the end).
- `switch_sweep.py` pairs original functions with C functions (the map, then shared string
  literals) and compares table and arm counts; coarser, and kept for functions whose C switches
  use names rather than numbers.
- `asm_arm.py TABLE VALUE...` prints the original's arm for a table's value, to read a lead.

Run of 2026-11-08 (767 tables): 688 matched, 31 with values no matched switch has, 49
unmatched, 30 without a readable compare (jsoncons and security code, and three
`CMsgEffect::OnProcess` tables jumped through with no range check). Every reported value but
one was found elsewhere in `src/`: this tree spells many of the original's switches as tables
(`ez2str_effect_files.c`'s `LOAD(...)` rows, `skill_hit_fx.c`'s per-skill rows, the
`skill_use_*.c` rows, `skill_info.c`'s ranges), which this sweep cannot see into; the one
value (`0x13af` in `SKILL_INFO::IsGoodSkillForActor`) sits inside a range row. So the switch
structure holds, and the risk moves to those table conversions, which only an oracle run of the
converted routine can check (`tests/oracle`). The unmatched tables with the most arms are the
candidates: the str-file-by-effect-type routine at `0x9997a0` (1026 values), the state-change
message switch at `0x7a0570` (798), the ez2str head-height and after-init switches at
`0x9959b0` and `0x998cf0`, the skill-use look at `0x8068e0` (15 tables, 529 arms; since proven
by `tests/oracle/test_skill_use`).

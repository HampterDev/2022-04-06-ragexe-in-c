# tools/funcmap: original function -> lifted C function

`function_map.tsv` maps addresses of the 2022-04-06 executable to the C functions of this tree,
so an assembly-versus-C check starts from the function, not from a search. One row per
(address, C function); columns:

| column | meaning |
| --- | --- |
| `addr` | the function's address in the original image (hex, no prefix) |
| `asm_label` | its label in herc-client's `text/*.inc` (`sub_<addr>` or a class name), empty when the inventory no longer holds it |
| `asm_chunk` | `text/<chunk>.inc:<first line>-<last line>` in herc-client, empty likewise |
| `class` | the C++ class from herc-client's RTTI class map, when known |
| `ref_name` | the function's name in the reference C tree (herc-client-integration `src/`) |
| `final_name` | its name in this tree (the plain-C renaming of 2026-10-30) |
| `main_where` | `file:line` of the definition on `main` at build time, empty when it has since been renamed or removed |
| `method` | how the pair was established (below) |
| `confidence` | `high` for `cited`, `name`, `label`, `cited_hex`, `ustr`; `low` for `cited_multi`, `callee`, `jacc` |
| `ref_file` | the reference tree file that cited the address |

Look up an address with `grep '^87af10' tools/funcmap/function_map.tsv`, a function with
`grep -P '\tworld_remove_skill\t'`. For a function that is not in the table, search the reference
tree (`herc-client-integration/src`) for the C name: its comment usually names the address or the
range of addresses it came from. The assembly of any address is in the Binary Ninja dump
(`client-disassembly/2022-04-06_Ragexe_1648707856.exe.bndb_disassembly.txt`, lines start with
the 8-digit address) and, for functions that were still assembly when the herc-client inventory
was taken, in `herc-client/text/<chunk>.inc`.

## Where the pairs come from

This tree is the plain-C `client/` tree that herc-client-integration generated from its
reference tree `src/` on 2026-10-29/30 (our initial commit equals that repository's
`bf307527a:client/src`, same tree hash). The reference tree was lifted from herc-client's FASM
reassembly function by function, and its comments cite the original addresses. So the map is
provenance, not inference, for most rows:

| method | rows | how |
| --- | --- | --- |
| `cited` | 5233 | the comment right above the reference function names exactly one `sub_<addr>` |
| `cited_multi` | 647 | that comment names several addresses (a function that merged several originals, or a family); every one is listed, low confidence |
| `name` | 61 | the reference function is itself called `sub_<addr>` |
| `cited_hex` | 5 | the comment says "lifted from 0x<addr>" |
| `label` | 147 | the reference name equals a herc-client class label (`CGameMode__Zc_Par_Change` -> `GameMode_Zc_ParChange`) |
| `ustr` | 420 | a string literal that occurs in exactly one assembly function and exactly one C function (26 of 26 right on held-out cited pairs) |
| `callee` | 60 | the one unmapped callee of a mapped pair on both sides (about half right on held-out; verify before use) |
| `jacc` | 118 | mutual-best Jaccard over strings, imports and calls into mapped functions (about half right on held-out; verify before use) |

Reference name -> final name comes from pairing the definitions of `client/src` before and after
the renaming commit (`615c59546`) per file: exact, then by normalised name, then by order
between anchors (13,767 of 14,900 paired; the rest were helpers the renaming removed).

## Coverage

6,541 addresses, 5,866 high-confidence rows, 3,851 of them with a definition on `main` today.
Against herc-client's inventory of the assembly that was still in `text/*.inc` when the
inventory was taken (92,100 functions, 15,945 of twenty or more instructions; the UI classes had
already been lifted out and are missing from it):

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

The big families are under-mapped not because the lift lacks them but because the reference
comments name a range ("the asm's `sub_7516d4` .. `sub_751b2e` were its cases") or nothing, and
the lifted functions carry no strings. The next lever is structural: aligning the callees of a
mapped pair in call order, and matching by size and position within a chunk. The dump lists
90,323 function starts; 5,301 of the high-confidence addresses are among them (the others are
interior labels the reference comments cite, such as a case body).

## Rebuilding

The inputs live outside this repository (paths as on the machine that built it):

```
python refmap.py            <herc-client-integration>/src   refmap.jsonl
python rename_map.py        <hci>/client/src@39e55e815       <hci>/client/src@bf307527a   rename.jsonl
python funcmap_build.py     refmap.jsonl rename.jsonl <herc-client>/tools/function_map.jsonl <herc-client>/tools/class_map.tsv <this repo>/src  map1.tsv
python fingerprint_match.py <herc-client>/tools/function_map.jsonl map1.tsv <this repo>/src map2.tsv
python finalize.py          map2.tsv <herc-client>/tools/function_map.jsonl function_map.tsv
```

`herc-client/tools/function_map.jsonl` is that repository's generated inventory
(`map_modules.py inventory`); the two client-tree snapshots come from `git archive`.

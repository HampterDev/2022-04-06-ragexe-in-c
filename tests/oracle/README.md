# tests/oracle: the original executable as the oracle

These tests map the original `2022-04-06_Ragexe_1648707856.exe` into the test process and call
its routines directly, comparing every answer with the lifted C over the whole input space. They
are the strongest check this tree has for a lookup: not a reading of the assembly, but the
assembly running.

`oracle_image.c` is the loader. It lays the image out as Windows would (sections, relocations if
the preferred base 0x400000 is taken, imports bound to the real system DLLs), and runs none of
it: no entry point, no static initialisers, no TLS. The game's own DLLs (Bink, Granny, Miles,
cps, ijl15) are left unbound on purpose, their imports pointing at a trap that ends the test if a
routine ever reaches one. Globals therefore hold the file's `.data` values; a routine that reads
state the client fills at start-up needs that state seeded (`oracle_addr`), or the reading routine
patched (`oracle_patch_return`), and the lifted side held to the same answer.

Every test is a 32-bit program linked with `/BASE:0x10000000 /FIXED /DYNAMICBASE:NO`; a fault
inside the original is caught with SEH and reported with its address in the original image
(`test_skill_poses.c` shows the pattern). `build.bat` builds and runs them all; it needs the
original executable at `%ORACLE_EXE%`, by default `..\client-files\2022-04-06_Ragexe_1648707856.exe`
beside this checkout, and `tests\build.bat` runs it only when that file exists.

## Adding a routine

1. Find its address. Use `tools/funcmap/function_map.tsv` for the C name, then confirm the start
   in the Binary Ninja dump: the function-start lines there read
   `008797a0  uint32_t __stdcall sub_8797a0(int32_t* arg1, ...)` and give the convention and the
   argument count. Do not take an address from a herc-client label or a comment without that
   check: the first attempt here used 0x879790 and landed in the `int3` padding before the
   function.
2. Declare a function-pointer type for that convention (`__stdcall` for `retn N`, `__cdecl` for a
   plain `retn`; a `thiscall` method is `__fastcall` with `this` first and a dummy second argument).
3. Stub what the lift needs and patch what the original reads, so both sides see the same world,
   and say so in the test's header comment.
4. Loop over the input space, compare, and report mismatches with both answers.

## What has been proven

| test | original | lift | inputs | result (2026-11-08) |
| --- | --- | --- | --- | --- |
| `test_skill_poses` | `sub_87af10` (use pose) | `session_get_use_effect` | skills 0..0x1518, Lua begin effect patched to none | 5,401 calls, 0 mismatches |
| `test_skill_poses` | `sub_8797a0` (cast pose) | `session_get_cast_effect` | the same skills x levels 0..7, job 0 | 43,208 calls, 0 mismatches |
| `test_job_lookups` | `sub_8760c0` | `session_remap_job` | jobs -2..0x2400 | 9,223 calls, 0 mismatches |
| `test_job_lookups` | `sub_878cb0` | `session_get_mounted_job` | jobs -2..0x2400 | 9,223 calls, 0 mismatches |
| `test_job_lookups` | `sub_8840d0`, `sub_8840a0`, `sub_869980`, `sub_8698e0`, `sub_884160`, `sub_890310` | `session_is_super_novice_job`, `_expanded_super_novice_job`, `_fourth_job`, `_third_job`, `_expanded_job`, `_doram_job` | jobs -2..0x2400 each | 55,338 calls, 0 mismatches |
| `test_job_lookups` | `sub_88dec0` | `session_get_max_job_level` | jobs -2..0x2400, the eight MaxLevelTable globals seeded with distinct values on both sides | 9,223 calls, 0 mismatches |
| `test_job_lookups` | `sub_855840` | `session_get_party_color_by_seq` | seq -1..64 | 66 calls, 0 mismatches |
| `test_job_lookups` | `sub_8771d0` | `session_get_pet_act_text` | actions -2..40, strings compared | 43 calls, 0 mismatches |

Skills from 0x1519 go to the skill-action registry, stubbed here; the comparison stops below it.
The maximum-job-level row is the test that would have caught the swapped `jobLevelBase` /
`jobLevel2nd` of the initial lift (fixed on main 2026-11-02; purist still has the swap).

`extract_functions.py` copies named production functions (and a file-local struct or helper)
into an include file, so a test links a lookup without the file it lives in; `build.bat` shows
the call for `test_job_lookups`.

## Candidates next

Pure or nearly pure lookups with large switches or tables: the ground-skill effect table
(`game_actor_am_groundskill`, once its spawn calls are stubbed), the job sprite and IMF name
builders (they read the job tables, which `.data` holds), the message-id lookups that take only
a code, the talk-option name table, and the angle and distance helpers of the effects.

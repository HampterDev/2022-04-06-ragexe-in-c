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
   argument count. Do not take an address from an assembly-listing label or a comment without that
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
| `test_ground_skill` | the routine at `0x765b90` (`CGameActor::Am_Groundskill`) | `game_actor_am_groundskill` | skills 0..0x15ff x levels 1, 5, 6, 10; the effect launches, spawns, waves, motions, state changes and the pose fields recorded on both sides through stand-ins | 22,528 calls, 45,240 events, 0 mismatches |
| `test_skill_use` | the routine at `0x8068e0` (`CGameMode::ShowSkillUse`, 15 jump tables, 529 arms) | `game_mode_show_skill_use` and the eight `skill_use_*.c` files behind it | skills 0..0x21ff x levels 1, 5, 10 x four caster/target/result variants x both effect options; every launch, spawn, wave, motion, state, turn, message, counter and window call recorded through stand-ins on both sides | 208,896 calls, 887,505 events, 0 mismatches |
| `test_skill_hit` | the routine at `0x801150` (`CGameMode::Zc_Notify_Skill2`, five jump tables) | `game_mode_on_notify_skill2` with `skill_hit_fx.c` | skills 0..0x21ff x levels 1, 5, 10 x six caster/target/damage/action variants x both effect options; the hit message to the actors, the action manager's hit, and every effect, wave, motion and state call recorded on both sides | 313,344 calls, 840,108 events, 0 mismatches |
| `test_skill_cast` | the routine at `0x7eb5f0` (`CGameMode::Zc_UseSkill_Ack`) with its cast look (`0x7eb880`) | `game_mode_zc_useskill_ack` with `skill_cast.c`, `skill_cast_look.c` and `session_cast_effect.c` | skills 0..0x21ff x five caster/target/cell/delay/property variants x both effect options; the chat line, the cast messages, the action manager's casts, every effect spawn with its stop frame, the ground scope's messages, turns and tints recorded on both sides | 87,040 calls, 820,930 events, 0 mismatches |
| `test_ez2str_files` | the routine at `0x9997a0` (`CEZ2STREffect`'s str file by effect type, one jump table of 1026 arms) | `ez2_str_effect_open_for_type` with its `str_files` table and `ez2_str_particle_open` | every effect type below 0x1000 x both effect options; the path every load receives recorded on both sides, the resource check answering "available" so the min-effect path is taken | 8,192 calls, 2,049 loads, 0 mismatches |
| `test_ez2str_head` | the routines at `0x998cf0` (head-type test), `0x9955e0` (head x offset) and `0x9959b0` (head height) | `ez2_str_effect_is_head_type`, `ez2str_head_x_offset`, `ez2str_head_height` with their range tables | every effect type below 0x1000; five str paths for the offset; no actor and eight riding/baby/doram actors for the height, the job predicates and mount height stood in on both sides | 61,440 calls, 0 mismatches |
| `test_ez2str_init` | the routines at `0x996150` (`CEZ2STREffect::Init`) and `0x996870` (its after-init, with 174 sound cues) | `ez2_str_effect_init` and `ez2_str_effect_after_init` with their type tables | every effect type below 0x1000 x both effect options x levels 1 and 3, owner 1001, a fixed position and target; the particle's creation, every str load, sound cue and zoom recorded on both sides, then the instance-key, shrink, head and camera fields of the effect and the owner, loop, scale, loops, spin, list and lifetime fields of the particle compared; the Lua questions answer no, the owner is absent, the camera heading and the screen projection are stood in with the same numbers on both sides | 16,384 calls, 21,774 events, 0 mismatches |
| `test_rag_effect_init` | the routine at `0x6e8c70` (`CRagEffect::Init`, one jump table of 224 arms) | `rag_effect_init` | every effect id the duration table covers (0..2384) x both effect options, one master with a position and heading, one offset; the duration set through the vtable and the str name asked of the resource manager recorded on both sides, then the id, master, position, heading, offset and loop-count fields compared; the resource manager answers "none" on both sides | 4,770 calls, 5,222 events, 0 mismatches |
| `test_rag_effect_process` | the routine at `0x6f85c0` (`CRagEffect::OnProcess`) with the two it dispatches to, `0x6ecd90` (effects 300 to 373) and `0x6ede30` (491 and up, one jump table of 1,883 arms) | `rag_effect_on_process`, `rag_effect_process300`, `rag_process_491`, `rag_process_800` and the `rag_skills_*.c` arms behind them | every effect id the duration table covers x both effect options, run for 64 frames (or the effect's duration) on a fake master; every prim launch, wave, texture and sprite load, action, message and spawn recorded on both sides; after each frame the effect's and the master's fields compared, after each run every launched prim's fields, particles, triangles and records (through `rag_record_map.inc`, see below), and then every launched prim is stepped 32 times through `0x68e550` (`CEffectPrim::OnProcess`, the per-kind table at `0x693fd8`) against `effect_prim_on_process` and the `prim_*.c` arms, with the same compare after each step, and drawn after each step through `0x6addc0` (`CEffectPrim::Render`, the per-kind table at `0x6af458`) against `effect_prim_render`, every triangle the two sides hand the renderer compared vertex by vertex; the thirteen effects that build whole game objects (blur and hallucination copies, the plant actor) are left out | 4,744 effect runs, 43,910 events, 13,816 prims, 419,468 prim steps and draws, 5,942,476 triangles, 0 mismatches |

Skills from 0x1519 go to the skill-action registry, stubbed here; the comparison stops below it.
The maximum-job-level row is the test that would have caught the swapped `jobLevelBase` /
`jobLevel2nd` of the initial lift (fixed on main 2026-11-02; purist still has the swap).

The ground-skill test shows the pattern for a routine with side effects: every helper the
original calls (the effect launchers, the wave player, the motion player, the Lua ground-effect
lookup, the actor's state setter through its vtable, the world queries) is redirected with
`oracle_patch_jump` to a C stand-in of the same calling convention that records the call, and the
lifted function links against stand-ins that record the same lines; the two event lists are then
compared as text. Two things it had to learn: a listing label is not a function start
(`PlayWave` sat on an epilogue's `pop esi`, `GetClientCoor` was fifty bytes early; the true entries
came from the call sites in the routine's own bytes), and a lazily constructing getter must be
left to the original rather than stubbed, or the code behind it walks an object that was never
built. The original's effect-duration table lives in `.data` and is filled at start-up, so the
test seeds it from the lift's `g_effectDurations`, which also checks the two tables index for index.

The skill handlers share their stand-ins: `skill_oracle.h` holds the fakes (a mode with its
world, caster, target and player, the effect every launch returns, the effect every spawn
returns), the recorders on both sides, the vtable traps that name the slot and the receiver when
the original calls a method the test has not provided, and `skill_oracle_setup`, which applies
every patch. Three helpers' addresses are read from the skill-use handler's own call sites
(`call_target_by_prefix`), since the listing's labels for them sit inside other routines;
two more are the call that follows a known call (`call_after`). Pointers that a message carries
(a position, the skill-use record) are printed as what they point to, since the two sides'
stacks differ. The sides call `srand(1)` before each run so `rand()` agrees.
Both sides read a fixed clock: the original's `timeGetTime` import is pointed at the test's
(`oracle_bind_import`), and the lift's, declared `dllimport`, is resolved to the same function by
a linker `/alternatename` for its import symbol, with `winmm.lib` left off the link. The cell
conversions (`GetClientCoor`, `GetServerCoor`) and the world's attribute block are stood in with
the lift's own formulas on a 5 x 5 x 5 attribute, so they are not under test here; the ground
scope the cast look makes is a fake on both sides, its constructor patched, and the world's
object list is seeded with a real sentinel node so the original's `std::list` push works.

Two shapes the cast test showed were checked by hand and are not bugs: the original passes the
caster to the action manager's cast-on-target and the target to its cast-at-cell, which the lift's
`skill_action_mgr_cast_on_target` and `skill_action_mgr_cast_at_cell` do not take. Of the 97
skill action classes in the original, two override cast-on-target (NW_ONLY_ONE_BULLET,
SKE_ALL_IN_THE_SKY) and one overrides cast-at-cell (HN_JACK_FROST_NOVA); the first two read
only the target, the third only the time and the position, and the base class returns at once.
The lift's hooks take exactly what is read.

The str-file test stands in the original's min-effect path transform: it is a `std::regex`
(`^(.+\\)(\w+\.str)$`) whose locale set-up reads C runtime state no entry point initialised, so
it cannot run in the mapped image. The stand-in is the regex written out (folder up to the last
backslash with no line break, a word name, `.str`), and the lift's `split_str_path` is held to
it; which row of the table reaches a load, with which path and under which option, is the
original's own code.

`extract_functions.py` copies named production functions (and a file-local struct or helper)
into an include file, so a test links a lookup without the file it lives in; `build.bat` shows
the call for `test_job_lookups`.

`test_rag_effect_process` takes `ORACLE_ONLY=<effect id>` to run one effect, `ORACLE_TRACE=1` to
print every recorded event as it happens (with the original's launch sites), `ORACLE_VERBOSE=1`
for a progress line per frame, `ORACLE_FIRST_LOOK=1` to report the first fault with its stack
before the handlers see it, and `ORACLE_REPORT=<n>` to show more than forty mismatches.

The prim steps run on the same fakes as the frames, plus an act resource whose action `n` has
`2 + n mod 5` motions (the original's `CActRes` keeps the actions as a vector of 12-byte entries
at `+0x110`, each a vector of 68-byte motions), a camera latitude on the fake view, and the
effect launch at `0x74bf80` stood in to record its arguments. A field that holds NaN on both
sides counts as equal (the sign of a NaN depends on which instruction produced it).

The draws run with the original's renderer pointer (`0xee5be0`) on a fake whose projection
scale is the lift's, its triangle submit (`0x4e6440`) stood in to record the triangle, the
ground attribute quad (`0x6572c0`) stood in the same way, a map attribute of 100 x 100 cells of
5 units for the cell lookup (`CWorld::GetServerCoor` runs as is against it), and a motion with no
clips for every sprite lookup, so the sprite kinds are stepped but draw nothing yet. Vertex
positions are compared to a thousandth, since the two sides round differently in the last bit.

The original keeps a prim's records as four slots of 180 bytes with one field layout for every
prim kind; this tree keeps a union of per-kind structs (`effects/rag/prim_records.h`), so the
test compares the records through a map: `rag_record_map.inc` says, per union member and field
unit, where in a slot the original keeps it, with a per-kind entry where the kinds differ (the
textured square of kind 82 keeps its height in `point[0].y`, which the original keeps where the
other textured kinds keep `point[2].y`). Two lift fields that share one slot field (the
billboards' `wander` and `speed`, say) pass when either holds the slot's value. The map is
derived from the data, not from reading the original's code: `rag_record_map.py` takes a record
dump written by the test with `ORACLE_RECORD_DUMP=<file>` and the union layout, and maps each
field unit to the slot offset that holds its bytes in every prim of that kind; fields no sampled
arm sets stay unmapped and uncompared. A row of size 0 excludes a field for one kind: the strip
ring draws (kinds 62 and 73) keep their corner scratch in the slot bytes other ring kinds use
for `offset_x` and its neighbours, where this tree keeps them in locals. The generator keeps
those rows. Regenerate it after a change to the union or to an arm:

    ORACLE_RECORD_DUMP=build\tests\oracle\records.txt build\tests\oracle\test_rag_effect_process.exe <exe>
    python tests\oracle\rag_record_map.py . build\tests\oracle\records.txt tests\oracle\rag_record_map.inc

## Candidates next

The last large table the switch sweep (`tools/funcmap/case_sweep.py`) could not match to a
numeric C switch is the packet dispatcher at `0x7a0570` (798 arms). It cannot be driven here: each
arm hands a received packet to a handler that works on the world, the actors and the windows.
What can be checked statically is checked by `tools/funcmap/dispatch_check.py`: both sides
dispatch the same 798 ids, and the handlers the simple arms call are paired with this tree's
handler functions in the function map. The handlers themselves are the next oracle candidates,
one at a time as the three skill handlers were. Smaller pure lookups: the job sprite and IMF
name builders (they read the job tables, which `.data` holds), the message-id lookups that take
only a code, the talk-option name table, and the angle and distance helpers of the effects.

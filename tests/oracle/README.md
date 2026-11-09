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
| `test_rag_effect_process` | the routine at `0x6f85c0` (`CRagEffect::OnProcess`) with the two it dispatches to, `0x6ecd90` (effects 300 to 373) and `0x6ede30` (491 and up, one jump table of 1,883 arms) | `rag_effect_on_process`, `rag_effect_process300`, `rag_process_491`, `rag_process_800` and the `rag_skills_*.c` arms behind them | every effect id the duration table covers x both effect options, run for 64 frames (or the effect's duration) on a fake master; every prim launch, wave, texture and sprite load, action, message and spawn recorded on both sides; after each frame the effect's and the master's fields compared, after each run every launched prim's fields, particles, triangles and records (through `rag_record_map.inc`, see below), and then every launched prim is stepped 32 times through `0x68e550` (`CEffectPrim::OnProcess`, the per-kind table at `0x693fd8`) against `effect_prim_on_process` and the `prim_*.c` arms, with the same compare after each step, and drawn after each step through `0x6addc0` (`CEffectPrim::Render`, the per-kind table at `0x6af458`) against `effect_prim_render`, every triangle the two sides hand the renderer compared vertex by vertex and every sprite quad they submit compared field by field; the thirteen effects that build whole game objects (blur and hallucination copies, the plant actor) are left out | 4,744 effect runs, 44,834 events, 13,816 prims, 419,468 prim steps and draws, 6,461,354 triangles and sprite quads, 0 mismatches |
| `test_packet_handlers` | fifty-six packet handlers of `CGameMode`: the eighteen actor and effect notifications `0x7cc580` (vanish), `0x7d6650` and `0x7f5320` (sprite change), `0x7fae70` (fast move), `0x7f9bb0` (blade stop), `0x7f4d40` (actor init), `0x7ce200` (par change by user), `0x7f4470` (names), `0x7ba170` (combo delay), `0x7ff750`, `0x7ff460`, `0x7ff730` and `0x7f4ed0` (notify effect 1 to 4), `0x7fa8b0` (clear effect), `0x7fac60` (congratulation), `0x803b30` (efst update), `0x7ba9d0` (devotion list), `0x7bac20` (dispel), and the twenty-five entry notifications `0x7c9ce0`, `0x7c8c50`, `0x7c91d0` and `0x7c9750` (new entry 1 to 4), `0x7cbbf0` (stand entry), `0x7cc180` (stand entry npc), `0x7cacc0`, `0x7cb1d0` and `0x7cb6e0` (stand entry 2 to 4), `0x7ca7b0` (stand entry 5), `0x7c87b0` (new entry 5), `0x7c8280`, `0x7c7150`, `0x7c76d0` and `0x7c7c70` (move entry 1, 2, 3 and 6), `0x7ca510` (ranking), `0x7f1ba0` (new entry 7), `0x7f2a90` (stand entry 8), `0x7f0d40` (move entry 9), `0x7f0a90`, `0x7f0be0`, `0x7f1900`, `0x7f1a50`, `0x7f27e0` and `0x7f2930` (move, new and stand entry 10 and 11), and the thirteen par changes `0x7cd5c0` (par change), `0x7c4530` and `0x7fce30` (long and long long par change), `0x7d7110` (status), `0x7d7560` (status change ack), `0x7b33a0` (status for a GM), `0x7dd890` (couple status), `0x7f09f0` and `0x7f0a40` (hp info), `0x7f4290` and `0x7f4330` (ABR init and par change), `0x7c6ba0` and `0x7ff9b0` (notify exp 1 and 2) | the handlers of `packets_actor.c`, `effect_notify.c`, `packets_entry.c`, `packets_entry_hp.c`, `par_change.c`, `status_values.c`, `character_stats.c` and `packets_hp.c` | every handler x the caster, the target, the player and an unknown aid x three fillers for the rest of the packet x both effect options; the actor and effect notifications sweep sixteen values and the handler's own interesting values in the first field after the aid, the entry notifications sweep the object type, eighteen jobs across every job range the handlers test and the pet head, with a name of 0, 4 or 15 bytes in the variable-length packets, the par changes sweep the parameter type and twelve values across the weight, level and manner thresholds of the handlers; every helper the handler calls recorded through stand-ins on both sides, then the session, the mode, the three actors, the spawned effect and the actor the handler created diffed against a snapshot and every changed field reported by name | 164,352 calls, 2,151,982 events, 0 mismatches |

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
5 units for the cell lookup (`CWorld::GetServerCoor` runs as is against it), a sprite resource
(prim `+0x104`) of six images per list, held as the original's vectors of image pointers and
this tree's arrays, four motions picked by action and motion with none to three 36-byte clips
(`CMotion::GetClip` at `0x9f1020` runs as is), and the sprite quad submit (`0x751160`) stood in
to record the quad, its image, palette and list flags. The stack below each draw is zeroed
first, so a local the original reads before writing holds zero on both sides (the flying
sprites' colour under a tint other than 0 and 1, which this tree sets to 0; see
`docs/changes.md`). Vertex positions are compared to a thousandth, since the two sides round
differently in the last bit.

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

The packet handlers run on the skill fakes plus a diff of the state they touch.
`test_packet_handlers` hands each handler a packet built from the id, an aid, a swept value in
the first field after the aid and a filler for the rest, the same bytes on both sides, with every
helper the handler calls stood in as a recorder (the name cards, the session's look and job
setters, the window manager's quit, open and message box, the effect manager's remove, the
actor's look setters through its vtable, the connection). After the call the original's session
block (`0x1133c50`, 0x83b0 bytes), the mode, the caster, the target, the player and the spawned
effect are compared with a snapshot taken before it, and every changed field is reported by name
through a map from this tree's field to the original's offset (`fields[]` in the test; the
layouts differ, the session's own aid sits at `0x129c` here and `0x15bc` there). A change outside
the map is reported with its raw offset on either side, so the sides disagree until the field is
mapped, which is how the map was filled. A vtable slot the test does not provide is reported as
an event and the call abandoned (`trap_jump`), so a handler reaching new ground shows as a
mismatch rather than ending the run. The `std::string` members of the fake effect are
pre-constructed empty (capacity 15), since a handler assigns into them. Every handler is called
with the mode in `ecx`: the arms that take only the packet (`retn 4`) still read the mode from
`ecx`, left there by the dispatcher. `ORACLE_ONLY=<handler name>` runs one handler,
`ORACLE_TRACE=1` prints every event list, `ORACLE_REPORTS=<n>` raises the number of mismatches
printed.

The entry notifications add a seventh region, the actor a handler creates: `operator new` and the
`CPC` constructor are stood in to hand out one fixed buffer on either side (`N`), the list node
allocation of the world's object and actor lists is stood in as well, and the two list sizes are
compared after the call. The entry and move-entry records the handlers send to the actor are
rendered field by field (their layouts match the original's); the fields a handler leaves unset
are left out of the rendering. One sweep value picks the object type, the job and whether the
head is the pet head. A stand-in must pop exactly what the original pops: a thunk has its
target's convention (`0x74bf70` jumps to the `__stdcall` `0x8699b0`), and a mismatch does not
fault, it leaves the caller's saved registers wrong after the call, which only shows in a caller
that still reads them.

The par changes put the parameter type in the packet's own type field and use the aid loop for
twelve values that cross the weight, level and manner thresholds of the handlers; the session
fields they read (hit points, weight, experience, pet state, levels) are seeded alike on both
sides so that every threshold is reached. The original's basic info, status and party windows
are fake objects whose `SendMsg` slot records, present or absent with the effect option; the
original's `rand` import is bound to the test's own `rand`, so the pet talk chance rolls the same
on both sides. Lookups into the session's maps (the advanced status values and the ABR hit
points) are stood in on both sides, and the value written through the returned slot is reported
after the call.

## Candidates next

The last large table the switch sweep (`tools/funcmap/case_sweep.py`) could not match to a
numeric C switch is the packet dispatcher at `0x7a0570` (798 arms). It is checked statically by
`tools/funcmap/dispatch_check.py`: both sides dispatch the same 798 ids, and the handlers the
simple arms call are paired with this tree's handler functions in the function map. The handlers
themselves are driven one family at a time by `test_packet_handlers`, the actor and effect
notifications, the entry notifications and the par changes so far; the next families are the
item lists and the NPC dialogs, each with its own stand-ins. Smaller pure lookups:
the job sprite and IMF name builders (they read the job tables, which `.data` holds), the
message-id lookups that take only a code, the talk-option name table, and the angle and distance
helpers of the effects.

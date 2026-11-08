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

## On custom

`custom` removed job and skill ranges on purpose, so these tests compare it with the original
everywhere except there:

- `custom_removed.h` lists the removed jobs (third, fourth and expanded classes, Doram) and
  skills (third-class, expanded, 4th-class, Homunculus S and elemental), with the ranges the
  removals were made with in `docs/custom-review.md`. Every test leaves those inputs out and says
  how many.
- `test_job_lookups` compares the remap, mounted-job, Super Novice, maximum-job-level and
  party-colour lookups. The third, fourth, expanded, expanded Super Novice and Doram predicates
  and the pet act text are gone from custom, so they are not compared.
- custom removed the skill and object halves of the skill-action manager (all 121 registrations
  were 4th-class), so its call sites take the not-found path without calling it. The original's
  calls are still answered "none" in `skill_oracle.h`, but no longer recorded.
- custom always runs as service type 20 (the langtype 20 lock), so `skill_oracle_setup` sets the
  original's `g_serviceType` (`0x1226950`) to 20. Under service type 0 the original launches
  Korea's Slow Poison effect, which custom removed with the other service types.

Results on custom, 2026-11-08:

| test | calls compared | left out (removed on custom) | result |
| --- | --- | --- | --- |
| `test_skill_poses` | 40,680 | 881 skills | 0 mismatches, 0 faults |
| `test_job_lookups` | 36,174 | 768 jobs | 0 mismatches, 0 faults |
| `test_ground_skill` | 18,612 | 979 skills | 0 mismatches, 0 faults |
| `test_skill_use` | 182,928 | 2,164 skill/option pairs | 0 mismatches, 0 faults |
| `test_skill_hit` | 274,392 | 2,164 skill/option pairs | 0 mismatches, 0 faults |
| `test_skill_cast` | 76,220 | 2,164 skill/option pairs | 0 mismatches, 0 faults |

The table below is `main`'s.

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

Skills from 0x1519 go to the skill-action registry, stubbed here; the comparison stops below it.
The maximum-job-level row is the test that would have caught the swapped `jobLevelBase` /
`jobLevel2nd` of the initial lift (fixed on main 2026-11-02, and on custom by `b64030b7`).

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

`extract_functions.py` copies named production functions (and a file-local struct or helper)
into an include file, so a test links a lookup without the file it lives in; `build.bat` shows
the call for `test_job_lookups`.

## Candidates next

Lookups with large switches or tables: the job sprite and IMF name builders (they read the job
tables, which `.data` holds), the message-id lookups that take only a code, the talk-option name
table, and the angle and distance helpers of the effects. With the stand-in pattern, the other
large per-skill switches: the skill-use handler, the cast-start handler and the hit handler.

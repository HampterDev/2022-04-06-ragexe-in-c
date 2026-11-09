# Custom client review

The `custom` branch, called `purist` until 2026-11-08 (the entries below use that name), is the
permanent, deliberately reduced client base. Review the
published client in execution order, explain each behavior, and let Patrick decide
what stays. Future composer and gameplay deviations start from this base.

## Commit checkpoint (2026-11-01)

The accumulated purist changes are captured together through START-180. START-181
(skill-point preview and Apply/Reset) was undecided and unchanged at this checkpoint;
Patrick later removed it ([skill-point preview removal](#skill-point-preview-removal-start-181-2026-11-02)).
The branch README then listed 93 brief deviations from `main`; this ledger retains the
detail.

The [foundation index](upstream/README.md) links the 27 separately committed fixes
and the emblem-cache fix already on `main`. A follow-up merchant regression update
brings `main` to `db18fe4b65c59b46ade4908780faadf71341d058`. Earlier notes saying
"prepared" or "not committed" describe the state at that review step.

Validation: release builds passed on both branches. All commands in both central
behavioral suites passed, resuming from the first failure after correcting two
stale harnesses: removed HTTP includes in the purist whisper test, and old restore
expectations/missing shop lookup in the foundation merchant test. The additional
COM, emote timing, NPC music, pet portrait and skill-tab suites passed on both;
purist headgear/accessory and foundation attendance-banner checks also passed.

Matching server changes remain separate patch files under `docs/server`; no server
checkout was modified. Resume the feature review at START-181 after this checkpoint.

## Size-ranked review (2026-11-02)

Patrick replaced the sequential walk with a size-ranked triage: Claude measured the
remaining optional systems (own source/header lines, including UI, packet, session
and Lua files), skipped everything already decided above, and presented the largest
first so that removed systems need no detailed review. START-181, pending at first,
was removed on 2026-11-02 ([skill-point preview removal](#skill-point-preview-removal-start-181-2026-11-02)).

| ID | System (approx. lines) | Recommendation | Patrick's decision | Implementation |
| --- | --- | --- | --- | --- |
| SIZE-01 | Navigation: route window, `/navigation`, navi Lua tables, airship (9.3k) | Keep; quest NPC navigation and facility buttons use it | Keep, 2026-11-02 | Unchanged |
| SIZE-02 | Achievements: window, tracker, rewards, catalog (4.0k) | Patrick's call | Keep, 2026-11-02 | Unchanged |
| SIZE-03 | Styling shop (3.8k) | Patrick's call | Keep, 2026-11-02 | Unchanged |
| SIZE-04 | Barter and expanded barter shops (3.6k) | Keep | Keep, 2026-11-02 | Unchanged |
| SIZE-05 | Cash shop and mileage (3.4k) | Patrick's call | Remove, 2026-11-02 | [Cash shop removal](#cash-shop-removal-2026-11-02) |
| SIZE-06 | Party Booking and Seek Party (2.8k) | Remove; the Agency replaces them | Remove, 2026-11-02 | [Party Booking and Seek Party removal](#party-booking-and-seek-party-removal-2026-11-02) |
| SIZE-07 | 4th-class skill handlers, with their effects and job tables (2.7k) | Patrick's call | Remove 4th classes, 2026-11-02 | [4th-class removal](#4th-class-removal-2026-11-02) |
| SIZE-08 | Lapine synthesis/upgrade (2.3k) | Remove | Remove, 2026-11-02 | [Lapine removal](#lapine-removal-2026-11-02) |
| SIZE-09 | Refine window (2.5k) | Keep | Keep, 2026-11-02 | Unchanged |
| SIZE-10 | Macro detector, reporter and captcha (2.0k) | Lean remove | Remove, 2026-11-02 | [Macro detector and captcha removal](#macro-detector-and-captcha-removal-2026-11-02) |
| SIZE-11 | Store search (2.0k) | Keep | Keep, 2026-11-02 | Unchanged |
| SIZE-12 | World map (1.7k) | Keep | Keep, 2026-11-02 | Unchanged |
| SIZE-13 | "Item collection" (1.6k) | Remove | Remove, 2026-11-02 | Not applicable: the files are the core item-description window and monster-capture slot animation; [review](#item-collection-review-2026-11-02-not-removed) |
| SIZE-14 | Second password (PIN) (1.3k) | Keep if the server uses PINs | Remove, 2026-11-02 | [Second password removal](#second-password-removal-2026-11-02) |
| SIZE-15 | Grade enchant and item grades (1.0k, read in 39 files) | Remove | Remove, 2026-11-02 | [Grade removal](#grade-enchanting-and-item-grade-removal-2026-11-02) |
| SIZE-16 | Dress room / equipment preview (1.0k) | Patrick's call | Keep, 2026-11-02 | Unchanged |
| SIZE-17 | Swap equipment (1.0k) | Patrick's call | Remove, 2026-11-02 | [Swap equipment removal](#swap-equipment-removal-2026-11-02) |
| SIZE-18 | Dead code paths: unreachable windows, unreferenced functions/data, dead branches | Remove | Remove, 2026-11-02 | [Dead-code sweep](#dead-code-sweep-2026-11-02) |

SIZE-13 was mislabeled in the presentation: `item_collection*.c` is the ordinary
right-click item description window, not the 2021 collection book, which this
client does not contain. Nothing was removed under it.

### Implementation and merge

The eight approved feature removals were implemented as five independent changes
on isolated copies of `d725b49`, each with its own release build, packet-dispatch
derivation and full behavioral suite (sections below), then merged into `purist`.
Conflicts were confined to shared tables, window-ID lists and test expectations;
every conflict was a pair of independent deletions and was resolved by keeping
both. One semantic overlap: the cash-shop removal deleted the limited-sale list
request (`0x0b4c`) from `src/game/msg_swap_equip.c`, so the swap-equipment
removal's relocation of that request to a new file was dropped and the file is
deleted outright.

Merged validation, before the dead-code sweep:

- Release build passed without compiler or linker warnings:
  `build/session2-merge5-build.log`.
- Packet dispatch: all 1,179,800 cases; digest `a3fd444e45d2becf`. A comparison
  of the merged trace with the baseline (`build/packet-dispatch-before-session2.bin`)
  and the five per-removal traces shows 498 changed cases, each identical to the
  removal that owns it, and no other differences:
  `build/session2-merged-dispatch-combination.log`.
- Object types: 519 concrete classes and 92 retired identities; 3,087 prior-runtime
  comparisons and 227 accepted casts; 373 C diagnostic names and 352 window names.
- The complete behavioral suite `tests/build.bat` passed:
  `build/session2-merged-full-tests.log`.

The five pre-removal trees are under `build/before-*`; per-removal logs keep their
prefixes (`cash-shop-`, `party-booking-`, `fourth-class-`, `item-systems-`,
`no-lapine-`, `no-grade-`, `antibot-pin-`). No live client/server smoke test was run.

### Follow-up decisions surfaced during removal

- AU-bot auto-reply (`0x0a29`/`0x0a2a`), formerly in `macro_detector.c`, has no UI
  and was moved unchanged to `src/game/au_bot.c`. Neither Hercules nor rAthena sends
  it. Removed, 2026-11-02 (Patrick): [AU-bot, DdukDdak and Party Recruit removal](#au-bot-reply-ddukddak-box-test-and-party-recruit-removal-2026-11-02).
- DdukDdak box test (`0x0bb3`), a GM-only statistics report independent of the
  Lapine windows. Removed, 2026-11-02 (Patrick): [AU-bot, DdukDdak and Party Recruit removal](#au-bot-reply-ddukddak-box-test-and-party-recruit-removal-2026-11-02).
- Grade-enchant effects `0x647`-`0x649` and 4th-class status visuals, ABR/Bionic
  summons and shared effect data remain because server packets can still request
  them or because they are woven into shared summon code. Removed, 2026-11-02
  (Patrick): [remaining 4th-class content and grade-effect removal](#remaining-4th-class-content-and-grade-effect-removal-2026-11-02).
- Party Recruit packet rows (`0x08e5`-`0x0971`) have no client code. Removed,
  2026-11-02 (Patrick): [AU-bot, DdukDdak and Party Recruit removal](#au-bot-reply-ddukddak-box-test-and-party-recruit-removal-2026-11-02).

### Foundation-fix candidates found during removal

Fixed on `main` on 2026-11-02, one commit each, and ported to `purist` where the
code remains; exceptions are noted. The series is indexed in
[docs/upstream](upstream/README.md#foundation-fixes-from-the-size-ranked-review-2026-11-02).

- `rag_connection_get_packet` accepts a variable or unknown packet length below 4.
  A length of 0 makes the receive loop re-read the same bytes forever (client hang);
  1 desynchronizes framing. Treat lengths below 4 as a protocol error.
  Fixed: `main` `ebb1887196`, `purist` `e731da3cf1`.
- `enter_map` in `src/login/login_states.c` leaves the 4-byte `unset` field of map
  entry `0x0436` uninitialized, sending stack bytes.
  Fixed: `main` `f21e5cb13b`, `purist` `d477f7b91b`.
- On `main`, `ui_window_mgr_get_window(0x12d)` falls back to the cash-shop window
  when no name plate exists, apparently a decompiled fall-through.
  Fixed: `main` `ad2c6075c8`; already correct on `purist`, which returns NULL there
  since the cash-shop removal.
- On `main`, the swap-equipment window's default position writes a 16-byte
  rectangle into an 8-byte point, overwriting the friend-list drag plate size.
  Fixed: `main` `685ebfa786`; not needed on `purist`, code removed.
- On `main`, a Lapine upgrade failure checks the synthesis window (`0x122`) instead
  of the upgrade window (`0x12e`), leaving the upgrade box waiting.
  Fixed: `main` `3e9d6ce8fe`; not needed on `purist`, code removed.
- `jobIdNames` is filled at startup but never read (removed on `purist` by the
  [dead-code sweep](#dead-code-sweep-2026-11-02)). Write-only data with no wrong
  behavior; left unchanged on `main`.
- A saved `OPTIONWNDINFO.SHOW` makes `make_basic_info` request window `0xd`, which no
  factory creates; ESC then opens the ESC menu instead of cancelling navigation
  guidance. Fixed: `main` `07135fd6d1`, `purist` `7bde71e3b3`.
- ESC menu button `ESC_ID_1C1` toggles nonexistent window `0xec`. Dead code, left
  unfixed: no ESC menu button has ID `0x1c1` (its seven buttons are `0x16b`,
  `0x1c2`, `0x16a`, `0xf3`, `0xb1`, `0x159` and `0x16c`, here and in the original
  executable), and the original's MakeWindow has no case for `0xec`, so nothing
  sends the command and its intended window cannot be determined. Left on `main`;
  removed on `purist` by the [leftover cleanup](#leftover-cleanup-2026-11-03).
- A status window saved minimized is restored at full height (pre-existing).
  Fixed: `main` `5da401b8f2`, `purist` `fb6d501f97` (purist version for its
  single-height window).
- `rag_effect_init` reads `g_effectDurations[effect_id]` for effect IDs 2385-60000,
  past the end of the 2,385-row table. Fixed: `main` `e26afaf764`, `purist`
  `6095736a2c`; such IDs now last their ID in frames, like the unconfigured rows.
- `rag_connection_get_packet` accepts variable-length packets whose framed length
  is at least 4 but below the table minimum, so handlers read stale
  `g_packetBuffer` bytes. Fixed: `main` `301c8c62a6`, `purist` `321284a664`.
- `ui_window_mgr_make_window` adds tracked IDs to the ESC list before anything is
  created, so any request that creates no window leaves a stuck entry, not only
  `0xd`. Fixed: `main` `1150faabf3`, `purist` `8cc259bdc5`.

## Starting point

- Repository: https://github.com/HampterDev/2022-04-06-ragexe-in-c
- Foundation branch: `main`
- Starting commit: `61a425e6ae4762a5daaa1e62bbaf007002484902`
- Review started: 2026-11-01
- Baseline release build: passed on 2026-11-01.
- Baseline behavioral suite: not run.

## Review method

- Follow startup, initialization, login, the game loop and its subsystems, then shutdown.
- Explain observable behavior and dependencies before recommending keep or remove.
- Record recommendations separately from Patrick's decisions. A recommendation is
  not authorization to remove a feature.
- Review a system's initializers with its behavior; retaining the initialization
  mechanism does not automatically retain every system it currently initializes.
- After a removal decision, trace callers, state, packet handling, resources, build
  inputs and tests. Make a focused change and run the relevant checks.
- Keep broadly useful fixes separate from purist policy changes so they can go to `main`.

## Agreed direction

- Keep essential rendering setup.
- Remove unwanted local-patcher integration if present. The published baseline has
  already removed the Patchup.exe/Sakup.exe/Indoorup.exe startup check; README.md
  documents this and the application sources have no matches for those names.
- Review remaining optional systems individually; no complete gameplay feature set
  has been selected yet.

## Startup review

Entry point: `src/core/winmain.c`, `WinMain`, starting at line 121 in the baseline.
The first four operations are at lines 133-139.

| ID | Behavior and source | Recommendation | Patrick's decision | Implementation |
| --- | --- | --- | --- | --- |
| START-001 | `client_init()` prepares shared state and registers cleanup functions. Its individual systems include UI, resources, guilds, navigation, weather and item data. `src/core/client_init.c:79`. | Keep the initialization/cleanup mechanism; review each registered system separately. | Pending | Unchanged |
| START-002 | `create_data_archive_if_missing()` opens or creates `data.grf` and closes it. A missing file becomes zero bytes; existing content is not rewritten. It does not create a valid GRF archive. Baseline `src/core/archive_startup.c:4`. | Remove automatic empty-file creation. Keep existing archive loading. | Remove, 2026-11-01 | Removed the call, helper, header and dedicated test/build script. Release build and existing GRF ownership suite passed. |
| START-003 | `_BinkSetSoundSystem()` selects DirectSound for Bink movie audio. Bink also serves the video window and `.bik` textures on world models. Baseline `src/core/winmain.c:135`, `src/ui/media/video_playback.c`, `src/resources/model_res_queries.c:17`, `src/render/tex_cache.c:159`. | Review the complete video feature together. | Remove all Bink and video playback, including video surfaces, 2026-11-01 | Removed playback, world ownership/update hooks, video texture handling, model video metadata, video UI and input interception, exclusive drawing helpers, import library/link input, and dedicated tests. Release build and full remaining behavioral suite passed; executable dependencies confirm Bink is absent. |
| START-004 | `crash_report_install()` installs crash diagnostics; startup currently aborts if installation fails. Baseline `src/core/winmain.c:136`, `src/diag/crash_report.c:410`. | Keep crash reports and dumps for diagnosis. | Keep, 2026-11-01 | Retained |
| START-005 | Replay launch mode, recording and playback, saved session/world/UI state, packet capture and replay timing. Baseline `src/core/winmain.c:141`, `src/replay/`, `src/game/msg_replay.c`. | Review the complete replay feature together. | Completely remove replay, 2026-11-01 | Removed launch mode, recorder/player, file handling, Replay directory creation, recording and playback UI, menu buttons, hotkey actions, snapshot serializers, capture hooks, outgoing-packet suppression, recording flags and synthetic packet `0x08ae`. Removed helpers whose only production callers belonged to replay. Live gameplay paths remain. Release build, full remaining behavioral suite and focused checks after helper cleanup passed. |
| START-006 | Launcher switches select `Dev`/`QmSakray`, token login, PC-cafe mode, alternate config files and official server overrides; graphics settings can hand back to a launcher page. Baseline `src/core/winmain.c`, `src/core/app.c`, `src/login/login_states.c`, `src/session/session_lua.c`. | Remove internal test modes. | Remove all launcher modes and go directly to `clientinfo.xml`, 2026-11-01 | Removed argument parsing and launcher state, `1rag1` no-op, `1qm1`/`1sak1` overrides, `1roc1` routing, `-t:` auto-login, `/account:` selection, `pc` login modifier, `ApplyNPGE` early exit and launcher-page handoff. Login always uses the configured address. Release rebuild and login OTP, talk-option Lua, game-layout and packet-dispatch checks passed. |
| START-007 | Disabled nProtect still has GameGuard state and lifecycle calls; GGHash checks file access and queues Lua filenames, but login/game loops ignore its errors. Baseline `src/security/gameguard_mgr.c`, `src/security/gg_hash_mgr.c`, `src/initializers/gg_hash.c`, `vendor/lua-5.1.4/src/lauxlib.c`. | Remove the leftover integration. | Remove GameGuard/GGHash remnants, 2026-11-01 | Removed managers, private MD5, startup/shutdown and loop calls, file checks, Lua hook and queue, guard packet entries and handlers, and cleanup of legacy nProtect helper files. Release rebuild, final relink and full remaining behavioral suite passed. |
| START-008 | If `AI_patch` and `AI` both exist, startup deletes an existing `AI_backup`, renames `AI` to `AI_backup`, then renames `AI_patch` to `AI`. Baseline `src/core/app.c`, `app_swap_ai_folder()`. | Remove the built-in AI folder updater. | Remove automatic AI folder replacement, 2026-11-01 | Removed the startup call, helper, declaration and exclusive includes. Changed units recompiled and the release executable linked successfully. Normal homunculus and mercenary AI loading is unchanged. |
| START-009 | Session startup writes the executable's folder to `HKCU\Software\Gravity\RagnarokOnline\RagPath`. Nothing in this client reads it. Baseline `src/session/session_init_state.c`, `write_rag_path()`. | Remove the legacy installation-path write. | Remove the RagPath registry write, 2026-11-01 | Removed the helper and its startup call. The changed unit recompiled and the release executable linked successfully. Remaining session initialization is unchanged. |
| START-010 | Missing `RenewSetup` or client registry keys trigger an endless loop that tries to launch `Setup.exe`. Baseline `src/core/winmain.c`, `read_registry()` in `src/core/startup.c`. | Remove the external setup dependency; read saved settings or use defaults. | Remove Setup.exe dependency; use saved settings or defaults, 2026-11-01 | Removed the launch/retry loop, setup marker and now-unused process-launch helper. Startup settings load once with per-value defaults, checked types/sizes and bounded strings. Release rebuild and focused settings/Lua option checks passed. |
| START-011 | Korean-service startup scans running processes for Ragnarok executable names through Toolhelp/PSAPI, but ignores the result. Baseline `src/core/winmain.c`, `create_depends_dll_msg()` and `i_process_construct()` in `src/core/startup.c`. | Remove the unused process scan and its OS-selection helper. | Remove the unused process scan, 2026-11-01 | Removed the startup call, both scanners, their private name helpers/API types, Windows 9x selector, and unused IProcess state/header. Release rebuild, source-reference audit and diff checks passed. |
| START-012 | Loads `ResNameTable.txt` to redirect requested resource filenames to actual filenames; map-name resolution also uses these aliases. Missing alias tables leave ordinary names in use. `src/fileio/res_cache.c`, `res_mgr_read_res_name_table()`. | Keep compatibility with existing data and configurable resource replacements. | Keep resource filename redirects, 2026-11-01 | Retained; no code changes. |
| START-013 | The Themida manager only tracks a started flag, rejects duplicate startup and clears the flag at shutdown. It performs no protection or scanning. `src/security/themida_mgr.c`; called from `app_init_managers()` and `app_destroy_singletons()`. | Remove the empty protection wrapper. | Remove the empty Themida wrapper, 2026-11-01 | Removed the implementation/header, startup failure branch, shutdown call, and dedicated test/build entry. Release rebuild, source-reference audit and diff checks passed. |
| START-014 | `ReassemblyPacketMgr` allocates packet-key state. All three key values start/reset to zero, and nothing loads nonzero values, so the outgoing packet-ID XOR always uses zero. `src/ui/net/packet_crypt.c`, `src/contents/rag_connection.c`. | Remove the inactive packet-ID obfuscation manager and key/reset calls; preserve normal packet transport. | Remove inactive packet-ID obfuscation, 2026-11-01 | Removed the implementation/header, startup/shutdown calls, login/map key resets and outgoing XOR. Real-key baseline, post-removal packet/queue checks, login/dispatch checks and release rebuild passed. |
| START-015 | The game dispatcher answers 84 `REASSEMBLY_AUTH` challenge IDs with paired reply headers; these handlers are independent of the removed key manager. `src/game/packet_dispatch2.c`, `src/game/net.c`. | Remove the legacy handshake for the purist client; servers expecting the replies would need matching changes. | Remove legacy reassembly authentication replies, 2026-11-01 | Removed all 84 handlers, their three exclusive reply helpers, incoming ID constants, 168 packet names/length rows, and unused test sinks. Dispatch trace, packet/queue checks, release rebuild and source/link audits passed. |
| START-016 | Loads `ContentData/EffectData/EZ2STREffect.bson`, mapping effect names to STR animations, offsets, loops, sound and delays. The effect registry and skill code use these entries, including Hell's Drive. `src/effects/ez2str_effect_mgr.c`, `src/effects/effect_registry.c`, `src/skills/hyper_novice.c`. | Keep named skill-effect support; review individual skills separately. | Keep named STR skill effects, 2026-11-01 | Retained; no code changes. |
| START-017 | The effect manager registers effect types, creates effects, tracks their actors, updates/draws them and cleans them up. Particle pools reuse plain, sprite, STR and multi-texture particles. `src/effects/effect_mgr.c`, `src/effects/particle_pool.c`. | Keep the shared visual-effect lifecycle and particle pools; review individual effects separately. | Keep effect manager and particle pools, 2026-11-01 | Retained; no code changes. |
| START-018 | The battleground entry queue loads a Lua catalog, offers solo/party/guild applications, displays waiting status and admission prompts, sends cancellation/answer/ranking requests and handles server queue notifications. `src/ui/social/entry_queue_mgr.c`, `src/ui/social/entry_queue_catalog.c`, `src/ui/social/entry_queue_events.c` and related windows. | Remove this optional queue feature from the purist base unless the server will use its built-in matchmaking flow. | Remove battleground entry queues, 2026-11-01 | Removed manager, catalog, four windows and their list control, startup/shutdown/login hooks, Lua registration/loading, menu buttons, saved settings, queue packets and exclusive helpers/tests. Focused checks, release rebuild, final helper relink and source/link audits passed. |
| START-019 | The time scheduler runs due jobs and frees finished ones. Current jobs handle connection pings/timeouts, delayed ranking display, captcha uploads and timed shutdown notices, plus an inactive GDI-check job. `src/contents/schedule.c`, `src/contents/time_scheduler_tick.c`, `src/game/ping_schedule.c`. | Keep the shared scheduler; review each job separately. | Keep scheduler; review individual jobs, 2026-11-01 | Retained; no code changes. |
| START-020 | Game-mode initialization adds a GDI-check job with a 60-second interval. When it runs, it only marks itself finished; it performs no graphics checks. `src/game/ping_schedule.c`, `gdi_check_run()` in `src/contents/schedule.c`. | Remove the empty scheduled job and its exclusive helpers. | Remove empty GDI-check job, 2026-11-01 | Removed job creation, callback table, helper functions/declarations and the random schedule mode whose only production user was this job. Scheduler checks, changed-unit compilation, release relink and source/link audits passed. |
| START-021 | The game-mode heartbeat periodically checks the connection, sends packet `0x0b1c` after more than 10 seconds without an outgoing packet, and disconnects/opens window `0x14c` after more than 60 seconds without an incoming packet. `src/game/ping_schedule.c`, `ping_run()`. | Keep connection keepalive and timeout handling. | Keep heartbeat and timeout handling, 2026-11-01 | Retained; no code changes. |
| START-022 | Login packet `0x0986` supplies a deadline and server time. The client displays a dated shutdown notice and schedules further chat reminders. This job does not disconnect or close the client. `src/contents/shutdown_notice.c`, partial-shutdown functions in `src/contents/schedule.c`. | Remove this optional legacy countdown from the purist client. | Remove shutdown countdown notices, 2026-11-01 | Removed notice handler/header, scheduled reminders, shutdown-only types/helpers/constants/includes, login cases and packet entries for `0x08e4`/`0x0986`, and exclusive tests. Scheduler/login checks, release rebuild and source/link audits passed. |
| START-023 | The CAPTCHA registration window lists BMP images from the client's `captcha` directory, previews a selected image and submits its answer/size. After server approval, a scheduled job uploads the compressed image in chunks. `src/ui/chat/captcha.c`, `src/game/packets_macro_detector.c`, `captcha_upload_*()` in `src/contents/schedule.c`. | Remove the optional image registration/upload tool; review receiving and answering challenges separately. | Remove CAPTCHA registration/upload tool, 2026-11-01 | Removed registration UI/factory/state, local BMP selection and compression, upload job, four packet entries and two incoming handlers. CAPTCHA, scheduler, dispatch, object-type and window-ownership checks and release rebuild passed. |
| START-024 | Rankings sent as character IDs wait up to about three seconds for player names, then print the top ten and personal score in chat. Styles cover Blacksmith, Alchemist, Taekwon and Killer rankings. `src/contents/ranking.c`, `src/session/session_ranking.c`, `src/game/mode_ranking.c`. | Keep this gameplay display support. | Keep delayed ranking display, 2026-11-01 | Retained; no code changes. Existing scheduler checks passed during START-023 validation. |
| START-025 | Loads `System/Towninfo.lub` with named facility coordinates and icons. Minimap views use it for shops, Kafra, inns, guides and other town markers; facility buttons can start navigation. `src/contents/town_info.c`, `src/ui/map/minimap_wnd.c`, `src/ui/quests/minimap_zoom.c`. | Keep useful town navigation and configurable map markers. | Keep town facility markers, 2026-11-01 | Retained; no code changes. |
| START-026 | The shared cooldown drawing manager tracks timed status-icon overlays and queues countdown shading over skill buttons in shortcut/quick-slot bars. `src/resources/state_icon_sheet.c`, `src/ui/cooldown_timer.c`, `src/ui/hotkeys/hotkey_bar_common.c`, `src/effects/rag/prim_screen_sprites.c`. | Keep visible status durations and skill cooldown feedback. | Keep countdown overlays, 2026-11-01 | Retained; no code changes. |
| START-027 | The quest manager combines local quest descriptions, NPC locations and reward information with server-supplied quest state, hunting progress and deadlines. It supports the journal and on-screen quest tracker. The same manager also loads recommended-quest data, which needs a separate decision. `src/contents/renew_quest_info.c`, `src/game/quest_list.c`, `src/game/quest_hunting.c`, `src/ui/quests/`. | Keep the core quest journal and tracking support; review recommended quests and service-specific loading separately. | Keep journal and tracker; review extras separately, 2026-11-01 | Retained; no code changes. |
| START-028 | A separate Recommended tab loads a local Lua quest-guide catalog with titles, summaries, artwork, three text sections and NPC navigation. Its checkmarks reorder entries and persist in `SaveData/RecommendedActiveList.lua`; they are local preferences. `src/contents/renew_quest_info.c`, `src/contents/renew_quest_lua.c`, `src/contents/renew_quest_table.c`, `src/ui/quests/quest_window.c`, `src/ui/quests/quest_panes.c`. | Remove this optional guide catalog from the purist base; retain the approved journal and tracker for server-supplied quests. | Remove recommended-quest catalog, 2026-11-01 | Removed catalog/state, Lua registration/loading, saved preferences, Recommended tab and detail pane. Journal/loading, ownership, quest packet/deadline and object-type checks, release rebuild and source/link audits passed. |
| START-029 | Quest startup selects `system/OngoingQuestInfoList_Sakray` when the clientinfo server type is `sakray`, otherwise `system/OngoingQuestInfoList_True`. `src/contents/renew_quest_info.c`, `src/platform/clientinfo.c`. | Remove the alternate test-server quest-data selection and always load the normal `_True` table, which remains customizable. | Remove Sakray selection; use normal quest table, 2026-11-01 | Quest startup always loads `_True`; removed the server-type branch and exclusive include/constant. Loading checks across 24 server-type values, changed-unit compilation, release relink and source/diff checks passed. |
| START-030 | Clan state supplies an information window, leader/home-map details, online/member counts, ally/enemy lists, emblems and clan chat. `src/contents/clan_info.c`, `src/game/packets_clan.c`, `src/ui/social/clan.c`. | Remove this optional social system from the purist base unless the server will use it; ordinary player guilds are separate. | Remove the clan system, 2026-11-01 | Removed manager/lifecycle, windows/state, actor emblems, chat commands/routing/selector/filter, hotkey action, clan status-image fallbacks, guild restrictions and five protocol entries. Guild-title and dressroom handlers moved to surviving modules. Release rebuild, relevant tests and source/link audits passed. |
| START-031 | Loads `System/monster_size_effect_new.lub` to configure sprite sizes and visual effects for listed monster types. Rendering and actor-entry/state paths use this table. `src/contents/champion_monster.c`, `src/actor/render_sprite.c`, `src/actor/attr_state.c`, `src/game/packets_entry_hp.c`. | Keep configurable monster presentation; the table can be customized for the server. | Keep monster size/effect configuration, 2026-11-01 | Retained; no code changes. |
| START-032 | Stores map-rule flags used by player targeting, PvP/GvG/siege/battleground handling, visual-effect policy and vending availability. Server map-property messages update the table; startup also reads local `MapInfoTable.scp`, to be reviewed separately. `src/actor/map_type_table.c`, `src/game/map_property.c`, `src/session/predicates.c`, `src/game/target_actor.c`, `src/ui/wndmgr/make_window_trade.c`. | Keep shared map-rule handling so client behavior follows server map restrictions; review the local table loader separately. | Keep map-rule handling; review local loader separately, 2026-11-01 | Retained; no code changes. |
| START-033 | Startup reads `MapInfoTable.scp` into up to 30 map-type rows of 27 values. This supplies defaults used by older map-property messages. Newer server packet `0x099b` supplies all 11 property IDs currently read by this client. A missing local file produces an error message for service type 0. `src/session/session_init.c`, `src/game/map_property.c`. | Remove the local fallback and use server-supplied flags for the purist client; this requires the server to send `0x099b`, which has not been verified against a running server. Keep shared rules and server handling. | Remove local fallback; use server map flags, 2026-11-01 | Removed startup load, parser, declaration and exclusive include. Shared map rules and server handlers remain. Map-flag ownership checks, changed-unit compilation, release relink and source/link audits passed. A separate bounded-parser correction is prepared for the foundation. |
| START-034 | RODEX provides the in-game mailbox, unread notifications, reading/writing/deleting messages and sending/claiming item and zeny attachments. It distinguishes player and server mail. `src/ui/mail/rodex_box.c`, `src/ui/mail/rodex_inbox.c`, `src/ui/mail/rodex_compose.c`, `src/game/msg_rodex.c`, `src/game/packets_rodex.c`. | Keep in-game mail and attachment delivery as useful gameplay support. | Keep RODEX in-game mail, 2026-11-01 | Retained; no code changes. |
| START-035 | Roulette has a launcher icon and prize-board window, gold/silver/bronze coin balances, animated spins, bonus-item display and prize claims. The server supplies the prize catalog, results and balances; the client sends open/spin/claim requests. `src/ui/misc/roulette_state.c`, `src/ui/shops/roulette.c`, `src/game/packets_roulette.c`, `src/game/mode_roulette.c`. | Remove this optional reward minigame from the purist base. | Remove roulette, 2026-11-01 | Removed manager/lifecycle, icon and prize-board windows, rendering, state, slash command and saved-option mapping, 11 protocol entries and six incoming handlers. Release rebuild, relevant tests and source/link audits passed. A separate prize-ordering correction is prepared for the foundation. |
| START-036 | Shared HTTP support loads `libcurl.dll`, builds text/file POST requests, collects replies and converts text to/from UTF-8. At review start, users included guild emblems, account/character settings, merchant-store configuration, party recruitment and Twitter. Startup fails if the HTTP library cannot initialize. `src/web/curl.c`, `src/web/http_post.h`. | Review consumers before choosing transport removal; start with guild emblem loading and animation. | Remove after preserving the chosen consumers, 2026-11-01 | Resolved by START-037 through START-042: emblems and Agency use game packets, preferences use local files, Twitter is removed. Curl and the unused HTTP worker stack are removed; the embedded browser is separate. |
| START-037 | Guild emblem HTTP requests transfer raw BMP/GIF files; the decoder, animation and versioned cache are independent of HTTP. | Keep BMP/GIF support and move transfers to game packets; export the matching server changes separately. | Approved packet transport and a small foundation fix; client only, separate server patch, 2026-11-01 | Implemented requests `0x0b46`, raw uploads `0x0153`, bounded `0x0b36` assembly and removal of emblem HTTP jobs/lifecycle. Client release build and relevant tests passed. Separate Hercules patch prepared and checked without changing existing server checkouts. Cache naming fix committed locally on `main` as `7fe9620`. |
| START-038 | HTTP account settings sync stores key bindings, chat/emotion macros and blocked-whisper names under the account/world. `src/web/account_settings.c`, `src/web/service_saves.c`. | Replace this remote sync with local files per server/account, preserving preferences without HTTP; settings would no longer follow the account between PCs. | Local settings per server/account, 2026-11-01 | Replaced account HTTP sync with complete UTF-8 JSON snapshots under `SaveData/Accounts`, isolated by login address/port/world and account ID. Removed account HTTP jobs, partial-save state, shared hotkey/registry writes and shutdown wait. Release build, relevant tests and source/link audit passed. Character skill-use preferences were reviewed separately under START-039. |
| START-039 | HTTP character settings sync remembers chosen casting levels for player and homunculus skills. `src/web/char_user_data.c`, `src/web/service_saves.c`. | Store these preferences locally per server/account/character alongside local account settings. | Local settings per character, 2026-11-01 | Replaced character HTTP sync with complete local skill-level snapshots, isolated by login address/port/world, account ID and character ID. Removed HTTP jobs, owner fields, dirty markers and the unused skill-tree visitor. Release build/relink, local file tests, retained service/skill/game-mode tests and source/link audit passed. |

| START-040 | Merchant shop presets save a shop title, item identities, quantities and prices, and refill vending/buying-shop setup from matching current inventory. `src/contents/mcstore_backup.c`, `src/contents/mcstore_json.c`, `src/ui/shops/merchant_shop_make_messages.c`. | Keep this convenience in local files per server/account/character, removing its HTTP dependency. | Keep presets in local files, 2026-11-01 | Replaced merchant HTTP load/save with separate local UTF-8 vending/buying files per server/account/character. Added strict transactional loading, preserved damaged files, made UI results synchronous, and corrected item-option matching and restored-row price alignment. Full release build, focused tests and source/link audit passed; independent foundation restore patch prepared. |

| START-041 | Adventurer's Agency publishes advertisements and lets players browse/filter parties by level, role and purpose, then apply to join. | Initial removal recommendation superseded by Patrick's choice to keep the UI with game-packet transport. | Port to normal packets; prepare server changes separately as for emblems, 2026-11-01 | Implemented versioned binary requests/replies, removed Agency HTTP/JSON jobs, preserved existing join packets, added stale-reply/timeout handling. Client and isolated server builds and focused tests passed. Separate Hercules patch applies alone or with the emblem patch; existing server checkouts untouched. |
| START-042 | Twitter integration opens account authorization and posts text plus a captured screenshot through the server web service. `src/ui/media/connect_sns.c`, `src/contents/twitter.c`. | Remove this optional social integration; it is now the last HTTP consumer. | Remove, user approved 2026-11-01 | Removed Twitter UI, commands, authorization state, screenshot crop helper, upload worker and the now-unused curl/async stack. Release build and focused regression checks passed; details below. |
| START-043 | Embedded Internet Explorer browser. Its remaining entry point is `/web <address>`, with an address bar and popup windows. `src/game/talk_type.c`, `src/browser/`, `src/ui/wndmgr/wndmgr_web_browser.c`. | Remove this optional in-client browser. | Remove, user approved 2026-11-01 | Removed browser control, commands, popup/address-bar code, window-manager state and lifecycle hooks. Shared UI text drawing moved intact to `src/ui/controls/window_text_styles.c`. Validation below. |
| START-044 | Player-name cache resolves character IDs into names for friends, guild lists, chat links, rankings and shop records. Missing names are requested through game packets, with duplicate suppression and timeout tracking. `src/contents/player_names.c`. | Keep this shared gameplay support. | Keep, user approved 2026-11-01 | Retained unchanged; character-name lookup and caching remain on the game connection. No code changes or additional tests needed. |
| START-045 | Local map signboards. `SignBoardList` Lua entries place pictures, animated sprite icons or colored text boards at fixed map coordinates and heights. `src/ui/quests/signboard_data.c`, `signboard_list.c`, `signboard.c`. | Remove this optional client-side annotation/decorative layer from the purist base. | Remove, user approved 2026-11-01 | Removed Lua imports/data owner, placement/rendering, signboard-only objects/windows, resize exception and lifecycle hooks. Full release build, focused tests and reference audit passed; details below. |
| START-046 | Reputation display loads group/name/limit definitions from BSON and displays server-supplied positive or negative points, progress bars, group filtering, search and change notifications. `src/contents/repute_*.c`, `src/game/repute.c`. | Remove this optional progression display from the purist base. | Remove, user approved 2026-11-01 | Removed data loading/ownership, UI/menu/hotkey, lifecycle hooks and packets `0x0b8d`/`0x0b8e`. Release build, focused tests and reference audit passed; details below. |
| START-047 | Item-name lookup loads `ItemDBNameTbl.lub` and calls `ItemDB_To_ItemID` to translate configured internal item names into numeric item IDs for reforming and enchanting. `src/contents/item_db_to_id.c`. | Keep this shared lookup while reviewing its two consumers next. | Keep; review its users next, user approved 2026-11-01 | Retained unchanged. No code changes or additional tests needed. |
| START-048 | Item reforming loads local conversion recipes, lists eligible inventory equipment, previews the replacement and materials, and sends a conversion request to the server. Recipes describe refine requirements/changes, random options, card retention and grade retention. `src/contents/item_reform.c`, `item_reform_wnd.c`, `item_reform_list.c`, `src/game/mode_item_reform.c`. | Remove this optional equipment-conversion system from the purist base; ordinary refining is separate. | Remove, user approved after viewing a reference screenshot, 2026-11-01 | Removed recipes/manager, five UI identities, lifecycle hooks, window blocker and four protocol messages. Full release build, focused tests and audits passed; details below. |
| START-049 | Dedicated enchanting window uses `EnchantList_f.lub`/`EnchantList.lub` to display eligible equipment, slot choices, material/zeny costs and four operations: random enchant, chosen enchant, upgrade and reset. Sends requests through the game connection and applies server results. `src/contents/enchant_mgr.c`, `enchant_wnd.c`, `enchant_offers.c`, `src/game/item_upgrade.c`. | Remove this optional dedicated window and its catalog; existing item enchant data/display and ordinary NPC dialogs are separate. | Remove, user approved after viewing a reference screenshot, 2026-11-01 | Removed catalog/manager, nine UI types, lifecycle and blocking checks, seven protocol messages and five newly unused helpers. Release build, final link, focused tests and audits passed; details below. |
| START-050 | Revisit the item-name lookup retained under START-047: reforming and the dedicated enchanting catalog were its only consumers, and both are now removed. Startup still loads `ItemDBNameTbl.lub`, but nothing queries it. `src/contents/item_db_to_id.c`. | Remove the now-unused lookup and its startup/shutdown hooks. | Remove, user approved 2026-11-01 | Removed lookup, lifecycle hooks and its now-unused LuaScript wrapper, superseding START-047. Changed-unit release compilation/link, Lua and map tests, and reference audit passed; details below. |
| START-051 | Map sky settings load `MapSkyData` and apply per-map background colors, configurable clouds, optional stars and legacy sky/weather effects on map entry. `src/core/map_sky.c`. | Keep this configurable visual support for existing and custom maps. | Keep, user approved 2026-11-01 | Retained unchanged. No code changes or additional tests needed. |
| START-052 | Selectable item packages load `selectpackageitem` Lua data. Using a configured inventory box opens a window listing reward bundles and previewing their items, quantities, refine levels and rental details. Confirmation sends the box/index and selected bundle to the server through `0x0baf`. `src/core/select_package.c`, `src/ui/items/choice_package.c`, `src/game/msg_items.c`. | Remove this optional reward-choice window and its local catalog from the purist base; ordinary item use and NPC dialogs are separate. | Remove, user approved after viewing Gravity's reference demo, 2026-11-01 | Removed catalog/manager, two UI types, lifecycle hooks, hotkey blocker and outgoing `0x0baf`. Normal item use remains. Release compilation/link, item-use and type checks, and source/protocol audits passed; details below. |
| START-053 | Winsock self-check loads `ws2_32.dll` again and compares the addresses of `connect`, `closesocket`, `send` and `select` with the already-loaded module. Startup ignores the result. `src/contents/connection.c:connection_ws2_unhooked`. | Remove this unused check; preserve socket initialization and normal game traffic. | Remove, user approved 2026-11-01 | Removed ignored startup call, check function, address-name table and declaration. Release compilation/link, packet tests and source/link audit passed; details below. |
| START-054 | Game-step timer uses elapsed milliseconds to calculate update steps for effects, actors, water and camera. Startup requests 60 steps/s, implemented with a 16 ms integer interval. It resets on map entry and focus regain. `src/contents/frame_timer.c`, `src/game/mode_run.c:run_logic_steps`. | Keep core timing support; any pacing redesign should be a separate renderer/game-loop change. | Keep, user approved 2026-11-01 | Retained unchanged; no code changes or additional tests needed. |
| START-055 | Saved UI layout reads positions, sizes, selected tabs, window visibility and chat/minimap preferences from the current-user `UIRectInfo` registry key at startup, and writes them on normal exit. A missing key uses built-in layout defaults. `src/ui/wndmgr/registry.c`. | Keep layout persistence so players do not need to rearrange their interface each launch. | Keep, user approved 2026-11-01 | Retained unchanged; no code changes or additional tests needed. Shortcut assignments in the same source file are a separate behavior. |
| START-056 | Shared Granny animation cache owns and reuses skeletal animations for `.gr2` models. Actor creation selects the 3D path when an NPC/monster resource name contains `.gr2`; model playback reads this cache. `src/resources/granny_res.c`, `granny_playback.c`, `src/game/world_objects.c`. | Keep animation support for existing 3D actors; review the specific startup guild-flag preload separately. | Keep, user approved 2026-11-01 | Retained unchanged; no code changes or additional tests needed. |
| START-057 | Startup preloaded the guild-flag animation into `[5][0]`, but `Guildflag90_1.gr2` requests `[1][0]` and the normal loader skipped model ID 1. Emblem rendering uses a separate texture replacement. | Use normal on-demand animation loading and preserve the emblem renderer. Prepare the same correction for the foundation. | Approved and implemented, 2026-11-01 | Removed preload and slot-1 exclusion. Resource lifecycle regression and release build passed; emblem renderer unchanged. Separate `docs/upstream/guild-flag-lazy-loading.patch` applies cleanly to `main`. Live visual check remains outstanding. |
| START-058 | Startup lists custom interface skins. ZIPs previously invoked `runzip.exe`, used `_tmpskin_`, and recursively deleted that temporary directory at shutdown. Folder skins load directly. | Keep folder-based skins and remove ZIP extraction and its external helper dependency. | Keep folders; remove ZIP extraction, user approved 2026-11-01 | ZIP scanner branch, extraction call/helper, temporary cleanup and flags removed. Retained saved selection, folder paths and ordinary resource fallback. Skin manager tests and full release build passed; no retired production references remain. |
| START-059 | `math_init_degree_tables()` precomputes angle conversions and sine/cosine values, used by sprite/effect rotations, camera calculations, water waves and 3D audio orientation. `src/render/math3d.c`. | Keep this shared mathematical support for retained rendering, world and sound systems. | Keep, user approved 2026-11-01 | Retained unchanged; no additional tests needed. |
| START-060 | Regional startup selected a warning image or a `licence.txt` Agree/Disagree page before connection selection. | Remove the legacy warning/licence startup screens and enter the normal configured connection-selection/login flow. | Remove, user approved 2026-11-01 | Initial state is now connection selection (2). Removed selector, warning/licence states, text loader/cache, dedicated warning window and obsolete render/disagree branches. Shared NPC dialogue and login-back action remain. Release rebuild and UI type checks passed. |
| START-061 | Personal minimap notes used a Record button, placement cursor, editor and per-map files under `Memo`. `src/ui/map/minimap_wnd.c`. | Initially kept with save fixes; Patrick later chose removal from purist. | Remove, revised decision 2026-11-01 | Removed note controls, editor, storage, startup folder creation, window-manager fields and feature tests. Window tests and full release rebuild passed. Independent foundation save-error/newline patch retained. |
| START-062 | Startup creates `Navigationdata`; the navigation window imports local text lists of named destinations, with map names and coordinates, for route guidance. | Initially recommended keeping; Patrick chose removal. | Remove, approved 2026-11-01 | Removed startup folder creation, file buttons/import handlers/storage from both navigation windows, the import parser and its unused directory-list helper. Full release build, final helper relink and navigation tests passed. |
| START-063 | The login/game lifecycle loop creates the active mode, initializes it, runs it, and cleans it up when changing maps, returning to login or quitting. `src/world/mode_loop.c`. | Keep this core lifecycle machinery; review individual login/game behaviors separately. | Keep; proceed with individual behaviors, approved 2026-11-01 | Retained unchanged; existing lifecycle tests pass. |
| START-064 | Service type `0xb` opens the hardcoded official website on exit from three paths: mode quit message, manager quit and Windows close message. `src/world/mode_mgr.c`, `src/core/wndproc.c`. | Remove automatic official-website launch on exit. | Remove, approved 2026-11-01 | Removed all three branches and the now-unused shell include in the mode manager. Changed-unit release build/relink, lifecycle tests and source audit passed. |
| START-065 | `<viewhomepage>` in `clientinfo.xml` stored a configurable URL that opened when the main window was destroyed. `src/platform/clientinfo.c`, `src/core/wndproc.c`. | Remove this separate automatic browser launch on exit. | Remove, approved 2026-11-01 | Removed XML option loading, URL buffer, destruction-time launch, private helper and exclusive includes. Window cleanup tests and full release rebuild passed. |
| START-066 | Print Screen or Scroll Lock requests a numbered JPEG in the local `ScreenShot` folder. The shared capture also serves optional trade screenshots and applies stamps/branding. `src/world/mode_mgr.c`, `src/render/frame_capture.c`. | Keep screenshot capture; review stamps, branding and automatic trade capture separately. | Keep capture; review extras separately, approved 2026-11-01 | Capture retained. Missing-logo dependency and misleading/silent failure reporting fixed during START-069; independent foundation patch prepared and tested. |
| START-067 | With the Korean service setting, screenshot text included a letter code derived from the character ID beside the timestamp. `watermark_code`, `mode_screen_shot` in `src/world/mode_mgr.c`. | Remove this legacy character-ID stamp; review timestamp/logo separately. | Remove, approved 2026-11-01 | Removed encoder and service/character-dependent formatting. Screenshots use plain date/time pending its separate review. JPEG capture tests and release rebuild/relink passed. |
| START-068 | Screenshot capture drew the local date, weekday and time into the bottom-left of the image. `mode_screen_shot` in `src/world/mode_mgr.c`. | Remove the visible timestamp for an unobstructed image; files retain ordinary filesystem timestamps. | Remove, approved 2026-11-01 | Removed date/time generation, weekday table, text buffer and all five drawing calls. Changed-unit release rebuild/relink and JPEG capture tests passed. |
| START-069 | Screenshot capture composites `scr_logo.bmp` into the bottom-right, treating magenta as transparent. It also required that resource before capturing. `src/render/frame_capture.c`. | Remove the screenshot logo overlay and its resource dependency from purist. | Remove, approved 2026-11-01 | Removed lookup, compositor and bitmap-specific include. Capture and save-notification regression tests and release relink passed. Foundation patch preserves optional logo rendering and fixes failed-save reporting. |
| START-070 | An unchecked-by-default trade-window checkbox requests a screenshot when Exchange is clicked, before sending the trade commit request. `src/ui/shops/trade.c`. | Remove this optional automatic screenshot behavior and checkbox; retain manual screenshots and ordinary trading. | Remove, approved 2026-11-01 | Removed checkbox, label, state and automatic request; removed the now-unused custom screenshot prefix and base destructor. Full release build, manual screenshot tests, mode lifecycle tests and trade-command source audit passed. |
| START-071 | Startup detects graphics adapters and available display modes, populates graphics-setting choices and finds the selected adapter's monitor coordinates. `src/render/render_clients.c`, `gfx_d3d9.c`, `display_devices.c`. | Keep this shared graphics support. | Keep, approved 2026-11-01 | Retained unchanged; used by graphics settings and coordinate conversion for the selected monitor. |
| START-072 | Startup creates `savedata` if it is missing. The directory holds local options, chat layouts, quest-tracker selections, party-panel records and local account/character preferences. `src/core/winmain.c` and the individual settings writers. | Keep this shared storage setup; review individual saved features separately. | Keep, approved 2026-11-01 | Retained unchanged. Several retained systems use this directory. |
| START-073 | Startup calls `_controlfp(0x20000, 0x30000)`, selecting 24-bit x87 floating-point precision in this 32-bit client. `src/core/winmain.c`. | Keep the existing numerical environment; review any math modernization separately. | Keep, approved 2026-11-01 | Retained unchanged. This is a CPU arithmetic setting, not a visual effect or data format. |
| START-074 | Startup requests finer Windows timer resolution with `timeBeginPeriod`, aiming for 1 ms within supported limits; normal shutdown calls `timeEndPeriod`. `src/core/winmain.c`. | Keep while the client uses `timeGetTime` for logic, animation and timeouts; review timing modernization separately. | Keep, approved 2026-11-01 | Retained unchanged. This improves timing granularity, not the configured update rate. |
| START-075 | `CoInitialize(0)` initializes Windows COM services on the main thread. Tested middleware paths work without it, but remaining external-page launches use `ShellExecute`, whose handlers can require COM. `src/core/winmain.c`, `src/core/app.c` and URL handlers. | Keep while shell launchers remain and fix cleanup ownership. | Keep COM; fix cleanup, 2026-11-01, after reviewing external-page features | WinMain owns one cleanup after successful initialization, including returning startup-failure paths. Removed duplicate window-shutdown cleanup. Purist/foundation regression cases pass; independent foundation patch prepared. |
| START-076 | Under the Korean service setting, the third wrong-password refusal shows a special notice; acknowledging it opens Hangame/Ragnarok's site and quits the client. `src/login/refuse.c`, counter in `src/login/account.c`. | Remove this regional three-attempt browser/exit behavior and use ordinary wrong-password error handling. | Remove, approved 2026-11-01 | Removed the counter, special notice, browser launch and extra exit branch. Ordinary wrong-password handling now applies to every attempt/service. Changed-unit release rebuild/relink, existing login OTP tests and isolated source audit passed. |
| START-077 | Login refusal code 16 under the Korean service setting shows a password-change notice; acknowledging it opens Gravity's password-change webpage and quits. `src/login/refuse.c`. | Remove the browser launch and extra forced exit; retain the notice and normal rejection handling. | Remove, approved 2026-11-01 | Removed the hardcoded URL launch and extra quit. The existing notice now uses common refusal handling. Changed-unit release rebuild/relink and isolated source comparison passed. |
| START-078 | Login refusal code 110 displays message-table notice `0x929`; acknowledging it launches URL `0x92a` and quits. The local runtime table describes a password change and points to Ragnarok's website. `src/login/refuse.c`. | Remove this second password-change redirect and extra forced exit; retain its notice through ordinary refusal handling. | Remove, approved 2026-11-01 | Code 110 now selects its existing notice for common refusal handling. Removed the unused shell helper/includes. Changed-unit release rebuild/relink and isolated source comparison passed. |
| START-079 | Authentication result code 900 shows account-security notice `0xd86`, launches GNJOY using `iexplore.exe`, then quits. `src/login/auth.c`. | Remove the browser launch and forced exit; retain the notice and use normal failed-authentication handling. | Remove, approved 2026-11-01 | Retained the exact notice and routed code 900 through existing failure cleanup. Removed the shell include and unused exit label. Changed-unit release rebuild/relink and isolated diff review passed. |
| START-080 | Disconnect reason 6 asks whether to pay for the account; choosing Yes quits and opens a regional billing/registration page. `src/login/notify.c`. | Remove the payment offer, webpage launch and extra quit; use the ordinary disconnect notice and normal disconnect handling. | Remove, approved 2026-11-01 | Code 6 now uses ordinary notice 3 and common disconnect handling. Removed the exclusive regional URL helper and shell/application includes. Changed-unit release rebuild/relink and isolated diff review passed. |
| START-081 | The login window's Join/Register button quits the client and opens the selected connection's `registrationweb` URL; Korean Hangame settings override it with an official page. `src/ui/character/login.c`, `src/platform/clientinfo.c`. | Remove the button and its web-link setup from the purist base. | Remove, approved 2026-11-01 | Removed button, click handler, page helpers, URL setting and XML name exceptions. Release rebuild/relink and XML regression checks passed. A separately tested foundation patch corrects structural XML parsing of ignored fields containing raw ampersands. |
| START-082 | The cash shop's Charge button opens message-table URL `0xce5` in Internet Explorer. `src/ui/shops/cash_shop.c`. | Remove this web-only button and browser launch; review purchases and banners separately. | Remove, approved 2026-11-01 | Removed button creation, its resource lookups, command branch and three exclusive constants. Changed-unit release rebuild/relink, reference check and isolated diff review passed. Purchases, balances and banners are unchanged. |
| START-083 | Cash-shop banners show local images from a Lua catalog, rotate roughly every five seconds and open configured webpages in Internet Explorer when clicked. `src/ui/shops/cash_shop.c`, `src/session/session_lua.c`, `src/contents/client_lua_funcs.c`. | Remove this optional promotional banner strip. | Remove entirely, approved 2026-11-01 | Removed banner window/owner, images/rotation/selectors, webpage launch, Lua catalog/callback and type registration. Release rebuild/relink, type checks and retained-function audit passed. Item catalog, cart, balances and purchases remain. |
| START-084 | Attendance-window banners rotate local Lua-configured images and open either webpages or in-game windows when clicked. `src/ui/quests/attendance.c`, `src/session/session_lua.c`, `src/contents/client_lua_funcs.c`. | Remove this optional banner strip; keep the attendance calendar and reward claims for separate review. | Remove entirely, approved 2026-11-01 | Removed banner UI, Lua loading/callback, storage helper and type registration. Uses the existing compact layout. Release rebuild/relink, type checks and calendar/claim source audit passed. A separately tested foundation patch fixes rotation with zero/one banner. |
| START-085 | Some authentication-error dialogs add a website button beneath the message. It displays message-table URL `0xd75` and opens it through the Windows shell. `src/ui/controls/message_box.c`, `src/login/auth.c`. | Remove this extra website link and use plain OK dialogs while retaining the error messages. | Remove, 2026-11-01 | Removed the link control and shell handler; all three callers use ordinary OK dialogs. Changed units compiled and release relinked; caller/source audit passed. |
| START-086 | Shared text viewers turn `<URL>` tags into clickable website links, including NPC dialog and quest text. `src/ui/controls/text_viewer_links.c`, `src/ui/quests/tag_text_viewer.c`. | Remove external-page launching and retain readable labels; keep in-game item, navigation, tip and quest links. | Keep clickable website links, 2026-11-01 | Retained unchanged. The two remaining `ShellExecuteA` call sites provide a reason to retain COM under START-075. |
| START-087 | Shared Miles audio backend plays streamed background music and sampled effects, including positional sound; it manages voices, volume, fades and cleanup. `src/contents/sound.c`, startup call in `src/core/winmain.c`. | Keep audio playback; review settings and startup-failure handling separately. | Keep audio playback; review policies separately, 2026-11-01 | Retained unchanged. Startup-failure policy is reviewed under START-088. |
| START-088 | Failure to create the sound manager displays `Audio System Init Failed` and stops startup before the game window opens. Output-driver and positional-provider/listener failures can trigger this. `src/core/winmain.c`, `src/contents/sound.c`. | Warn once and continue without sound for that run, preserving saved audio preferences. | Keep stopping startup on audio failure, 2026-11-01 | Existing notice and startup stop retained unchanged; COM cleanup remains balanced. No silent fallback added. |
| START-089 | Startup reads `PROVIDERNAME` from the registry into a 1,024-byte buffer, but positional audio overwrites it with `Miles Fast 2D Positional Audio` before using it. Other audio modes never use the saved name. `src/core/startup_settings.c`, `src/contents/sound.c`, `src/contents/sound.h`. | Remove the ineffective saved-name read and mutable buffer; retain the existing positional provider through a constant name. | Remove the unused setting, 2026-11-01 | Removed the read, reset, buffer and declaration; provider matching uses the same literal. Settings and sound-manager tests pass; changed units compiled and release relinked. |
| START-090 | With no saved `SOUNDMODE`, startup previously defaulted to disabled audio. In-game music/effect toggles do not open the backend. `src/core/startup_settings.c`, `src/contents/sound.c`, `src/ui/render/graphic_setting.c`. | Default to positional audio when the saved value cannot be read; honor existing saved sound modes, including disabled. | Default to positional audio, 2026-11-01 | Fallback changed from 0 to 2. Saved modes 0/1/2 still override it. Settings checks and release rebuild passed; startup-failure policy unchanged. |
| START-091 | Registry options select 22,050/11,025/8,000 Hz and 16/8-bit output. Missing settings use 22,050 Hz and 16-bit. `src/core/startup_settings.c`, `src/contents/sound.c`. | Remove the lower-quality format options and always use the existing 22,050 Hz, 16-bit default. | Use fixed 22,050 Hz, 16-bit output, 2026-11-01 | Removed format selectors, globals and registry reads. Existing settings and sound tests pass; changed units compiled and release relinked. |
| START-092 | Actor-animation/world sounds pass fixed full/max distance parameters of 40/250 world units, independent of server visibility. Patrick reported distant visible monsters becoming inaudible in SpringRO. `src/actor/motion.c`, `src/contents/sound.c`, actor/effect callers. | Review smooth attenuation and audible-threshold culling before choosing a configurable cutoff; verify the real Miles provider's maximum-distance behavior. | Deferred; Patrick asked to resume pruning, 2026-11-01 | Ordinary-path cutoff and positional distance parameters confirmed. A hard positional mute at 50 cells is not verified. Production behavior remains unchanged. |
| START-093 | `NUMSAMPLETYPE` selects a requested pool of 16, 32 or 48 simultaneous sound effects; the default is 16. Playback skips new effects when no allocated channel is free. `src/core/startup_settings.c`, `src/contents/sound.c`. | Remove the legacy selector and request up to the existing 48-slot capacity, accepting fewer when the backend cannot supply more. | Remove selector; request up to 48 channels, 2026-11-01 | Removed the setting/global and private count field. Allocation/playback use the existing 48-slot capacity. Settings/sound tests and release build pass; backend limits still apply. |
| START-094 | Startup reads `SPEAKERTYPE` and supplies it to the Miles positional provider when its speaker-type API is supported. `src/core/startup_settings.c`, `src/contents/sound.c`. | Keep the saved speaker-layout preference so supported positional providers can honor the existing audio setup. | Keep the saved preference, 2026-11-01 | Retained unchanged. Missing settings use 0; ordinary/disabled audio do not use this provider setting. |
| START-095 | `/tingonly` filters sampled audio to the button-ding WAV and enables that ding when a non-owner joins a chat room. Music is separate. `src/contents/sound.c`, `src/game/talk_commands.c`, `src/ui/chat/chat_room_wnd.c`, command registration/dispatch. | Remove this optional mode and its conditional chat-room entry alert; preserve normal music/effect controls and ordinary button sounds. | **Remove**, approved and implemented 2026-11-01 | Removed command, flag, filter, toggle notices and join alert. All 11 changed/dependent units rebuilt and release relinked; sound-manager and talk-option suites passed. |
| START-096 | Server-requested NPC music: `0x07fe` supplies a local track name, played from `bgm\<name>.mp3` when music is enabled. `src/game/packets_npc.c`, `src/game/mode_run.c`. | Keep scripted music; verify the stream status and handoff together. | **Keep and fix**, approved and implemented 2026-11-01 | Real DLL reports playing=4, done=2. Corrected the wrongly named constant and reversed handoff together; normal completion stays intact, missing tracks now fall back. Regression and release rebuild passed; independent foundation patch prepared. |
| START-097 | Server-requested WAV effects: `0x01d3` requests a local sound once, repeats it at an interval, or removes a queued repeat by name. Supports NPC-relative playback. `src/game/packets_npc.c`, `src/game/mode_run.c`. | Keep these script-controlled cues and ambient sounds, respecting the effects toggle. | **Keep**, approved 2026-11-01 | Unchanged. Initial repeated playback is centered; later repeats can follow an NPC. Sound-distance policy remains deferred under START-092. |
| START-098 | Music volume fades: screen-transition code starts a fade toward silence or back toward the selected music volume. `src/contents/sound.c`, `src/world/mode_mgr.c`, `src/world/mode_draw.c`. | Keep smoother music-volume transitions. Sampled effects are separate. | **Keep**, approved 2026-11-01 | Unchanged. A cancellable worker adjusts volume one unit per approximately 10 ms and is joined before replacement/shutdown. |
| START-099 | Unused audio-output flag: `g_sound_use_waveout` is zero-initialized and only read by `_AIL_set_preference(15, ...)`; no client code changes or exposes it. `src/contents/sound.c`. | Remove the unused variable and pass the same zero value directly, preserving the existing backend request. | **Remove**, approved and implemented 2026-11-01 | Removed the global; the same preference call now receives literal zero. Sound suite and release rebuild passed. |
| START-100 | Main item-information catalog: `System/iteminfo_true.lub` supplies identified/unidentified names, descriptions, artwork names, slots, view classes, costume flags and effect IDs. `src/contents/item_info_mgr.c`, `src/iteminfo/item_table.c`. | Keep this shared support for displaying ordinary and custom items. Review additional item tables/features separately. | **Keep**, approved 2026-11-01 | Unchanged. This is distinct from the unused `ItemDBNameTbl.lub` lookup removed under START-050. |
| START-101 | Legacy `item.txt` / `eventItem.txt` selection: finding either file loads old-format names/descriptions and skips applying the Lua definitions to the item table. `src/iteminfo/item_loader.c`. | Remove this alternate selection and its parser; always take the Lua import path. Review the other item text tables separately. | **Remove**, approved and implemented 2026-11-01 | Removed alternate loader, selection state, exclusive description-copy helper and now-unused record parsing mode. Seven units rebuilt, release relinked, existing item/Lua and table-reader tests passed. |
| START-102 | Four supplemental `Event*` name/description text tables load before Lua, adding entries and prepending description lines to matching Lua items. `src/iteminfo/item_loader.c`. | Remove these supplemental loaders and keep names/descriptions in the retained Lua catalog. Review resource-name overrides separately. | **Remove**, approved and implemented 2026-11-01 | Removed four calls, two private loaders and unused dependencies/test stubs. Existing item/Lua and table-reader suites and release rebuild passed. |
| START-103 | Two `Event*ItemResNameTable.txt` tables overwrite identified/unidentified artwork names after Lua imports. These names select item icons, dropped-item sprites and drag artwork. `src/iteminfo/item_loader.c`, `src/session/session_names.c`. | Remove these legacy overrides and configure the artwork names in the Lua catalog. | **Remove**, approved and implemented 2026-11-01 | Removed two calls, private loader and unused test stub. Lua resource-name assertions, table-reader tests and release rebuild passed. Card illustrations and general filename redirects remain. |
| START-104 | NPC-shop ATK/DEF estimate bar uses `itemSlotTable.txt` equipment masks and `itemParamTable.txt` numeric pairs to display current/after estimates. `src/ui/items/param_change.c`, `src/iteminfo/item_loader.c`. | Remove the optional estimate bar and its two exclusive local tables; keep ordinary shop purchases, item descriptions and side-by-side equipment comparison. | **Remove**, approved and implemented 2026-11-01 | Removed UI, lifecycle/registry hooks, table loads and exclusive item fields. All 1,175 client units rebuilt and release relinked; item/Lua, table-reader, type and window-cleanup tests passed. |
| START-105 | Card-ID recognition: `CardItemNameTable.txt` seeds the card catalog; card artwork/naming tables also add entries. Membership checks support inserted-card detection, card-only search and card-specific displays. `src/iteminfo/item_tables.c`, `src/iteminfo/item_name.c`, `src/iteminfo/item_search.c`. | Keep shared card recognition; review illustrations and naming separately. | **Keep**, approved 2026-11-01 | Unchanged. This catalog is separate from server-side card effects and slotting rules. |
| START-106 | Card illustrations: the View button in a card's item-information window opens its local BMP artwork, selected by `Num2CardIllustNameTable.txt`. `src/ui/items/illustrations.c`, `src/ui/items/item_collection.c`, `src/iteminfo/item_loader.c`. | Keep this classic card-art viewer and configurable artwork mapping; review prefix/suffix naming separately. | **Keep**, approved 2026-11-01 | Unchanged. Uses the shared resource manager; no network download is involved. |
| START-107 | Card prefixes/suffixes: local tables supply the names added around an equipment name for inserted cards, with localized labels for repeated copies. Card aliases also participate in item searches. `src/iteminfo/item_loader.c`, `src/iteminfo/item_name.c`, `src/iteminfo/item_search.c`. | Keep this familiar, configurable equipment naming and search support. | **Keep**, approved 2026-11-01 | Unchanged. Display/search metadata is separate from server-side card effects. |
| START-108 | Readable books: an item-description button or server packet opens local `book/<itemID>.txt` in a paged reader with saved bookmarks, or an automatic text display. `BookItemNameTable.txt` selects items with book buttons. Former reader: `src/ui/items/book_wnd.c`. | Keep the reader for lore and quest text; review the automatic line-by-line display mode separately. | **Remove**, approved and implemented 2026-11-01 | Removed reader, automatic display, bookmarks, buttons, catalog and packet handler. All 1,173 client units rebuilt; item, window, text and packet suites passed. Legacy packet framing metadata remains. |
| START-109 | Forging-window instructions: `MetalProcessItemList.txt` supplies per-item text lines drawn in the classic crafting confirmation window. `src/iteminfo/item_loader.c`, `src/ui/items/make_target.c`. | Keep configurable instructions/material notes while reviewing crafting behavior separately. | **Keep**, approved 2026-11-01 | Unchanged. These lines are presentation data; they do not determine recipes, consume materials or calculate success. |
| START-110 | Item restriction tooltip: `ItemMoveInfoV5.txt` supplies per-item flags displayed on hover in item information, covering dropping, trading, storage, cart, mail, guild storage and NPC sales. `src/ui/items/item_move_info.c`, `src/ui/items/move_info_tooltip.c`. | Keep useful restriction notices, with the local catalog maintained to match the server. Review loader handling separately. | **Keep**, approved 2026-11-01 | Tooltip kept. Fixed lost numeric values at newline/EOF and incomplete-record buffer overreads; regression and release rebuild passed. Independent foundation patch prepared. |
| START-111 | Missing restriction catalog: failure to open `ItemMoveInfoV5.txt` shows a generic "Can't open file" popup, then startup continues without loading restriction notices. `src/iteminfo/item_loader.c`. | Treat the optional display catalog as absent without this generic popup. | **Keep**, approved 2026-11-01 | Patrick chose to retain the popup. Warning unchanged; START-110's parser fix preserves the current missing-file policy. |
| START-112 | Illustrated Tip Box guide: `System/tipbox.lua` or `.lub` defines searchable topics, text pages, pictures/sprites and animated annotations. It supports automatic opening, topic links and server-requested topics. Formerly `src/ui/tips/tipbox_mgr.c`, `src/ui/tips/tipbox.c`. | Remove this optional guide from the purist base; ordinary hover tooltips and the separate legacy Tip of the Day system are outside this decision. | **Remove**, approved 2026-11-01 | Removed catalog, window, illustrations/annotations, automatic opening, menu/shortcut/chat/server entry points and clickable guide links. Legacy labels remain plain text. Full release rebuild and focused checks passed. |
| START-113 | Legacy Tip of the Day: an older text-only window picked a random tip from regional text files and offered previous/next buttons plus a saved startup-display checkbox. Formerly `src/ui/tips/tip_of_the_day.c`; catalog accessors in `src/contents/tips.c`. | Remove this older optional guide and its now-unused startup-display preference; shared GuildTip parsing remains for separate review. | **Remove**, approved 2026-11-01 | Removed window, catalog state/accessors, cleanup, slots/type registration and startup-display preference. Full release rebuild, focused checks and source-scope audit passed. |
| START-114 | Guild help text: the Help button in the panel shown to guildless characters opened local `GuildTip.txt` in a scrollable text window. `src/ui/social/guild_helper.c`, former `src/contents/tips.c`. | Remove this optional manual and its Help button/catalog; preserve creating, joining and managing guilds. | **Remove**, approved 2026-11-01 | Removed manual window, Help button, exclusive loader/parser/state and cleanup. Create Guild and OK remain. Full release rebuild, guild/type/window checks and source-scope audit passed. |
| START-115 | Unused random-value bookkeeping: `session_reroll_values` wrote random indices and zeros into arrays with no readers, then rechecked unchanged HP/SP values. Formerly called at startup, map entry and recovery events. `src/session/session_state.c`. | Remove this unused helper/state and its calls; preserve actual HP/SP updates and their UI refreshes. | **Remove**, approved 2026-11-01 | Removed helper, private helpers/flag, unused 260-byte state block and five calls. HP/SP/effect logic unchanged outside the removed calls. Full release rebuild, session/packet checks and source-scope audit passed. |
| START-116 | Unused initialization counter: `g_initTableRuns` incremented each time `session_init_table` ran, but no code read or reported it. `src/session/session_tables.c`. | Remove this unused counter and increment; keep table initialization. | **Remove**, approved 2026-11-01 | Removed definition and increment. All initialization calls remain unchanged. Changed source rebuilt and release executable relinked successfully. |
| START-117 | Player hit-impact sounds: startup initializes cloth, wood and metal sound names, selected by the struck player's job during combat. `src/session/job_sounds.c`, `src/actor/actor_sendmsg.c`. | Keep this combat feedback; review the separate weapon-hit sound catalog later. | **Keep**, approved 2026-11-01 | Unchanged. The selection is presentation-only and does not calculate damage or inspect actual armor materials. |
| START-118 | Map background-music catalog: `mp3NameTable.txt` pairs map names with local music paths, used on map entry and when NPC music ends. `src/session/session_tables.c`, `src/session/session_names.c`. | Keep per-map music selection; review the missing-entry fallback separately. | **Keep**, approved 2026-11-01 | Catalog and selection retained unchanged. Review the hardcoded fallback under START-119. |
| START-119 | Missing map-music entry: the lookup substituted `bgm\01.mp3` when a map had no catalog match, including when the catalog failed to load. `src/session/session_names.c`. | Remove the hardcoded fallback and leave unmapped maps without background music; explicitly stop/clear previous map music. | **Silence for unmapped maps**, approved 2026-11-01 | Lookup now returns NULL; map entry/restoration clear previous playback and remembered track. Configured/NPC music retained. Regression, sound-manager checks and release rebuild/relink passed. |
| START-120 | Indoor-map camera rules: `indoorRswTable.txt` selects maps with narrower rotation/tilt/zoom ranges and separate saved indoor camera settings. `src/session/session_tables.c`, `src/game/camera.c`. | Keep this configurable behavior for interior maps; review detailed per-map camera presets separately. | **Keep**, approved 2026-11-01 | Unchanged. Missing entries use the ordinary outdoor camera policy; per-map presets can override limits. |
| START-121 | Per-map camera presets: `ViewPointTable.txt` sets initial distance, rotation and tilt plus permitted adjustment ranges for named maps. `src/session/session_tables.c`, `src/game/camera.c`, `src/game/mouse_camera.c`. | Keep this configurable support for maps requiring a particular view. | **Keep**, approved 2026-11-01 | Unchanged. A matching preset overrides the usual indoor/outdoor camera bounds; unmatched maps retain their ordinary policy. |
| START-122 | Skill visual-effect mappings: startup builds two lookup tables connecting skill IDs to launch and hit effects. `src/session/session_init.c`, `src/actor/skill_checks.c`. | Keep this shared combat presentation support; review individual skills separately. | **Keep**, approved 2026-11-01 | Unchanged. These select effects, not skill damage or gameplay rules. |
| START-123 | Shared interface text catalog: `msgStringTable.txt` supplies numbered messages used by labels, dialogs, errors and notices across the client. `src/core/msg_str.c`, `src/session/session_tables.c`. | Keep this core customizable text support; review word filtering and other reload steps separately. | **Keep**, approved 2026-11-01 | Unchanged. Missing messages use the existing `NO MSG` fallback. |
| START-124 | Local word blacklist: `manner.txt` supplied terms that the client masked in chat/names and rejected in some text submissions, including character names, mail and NPC input. Formerly `src/contents/manner_words.c`, `src/contents/manner_filter.c`. | Remove this optional local censorship policy; preserve separate input-format checks and server behavior. | **Remove**, approved 2026-11-01 | Removed loader, matcher, state, masking/rejection calls and exclusive tests. Separate validation retained. Full release build, name-display/mail/encoding checks and source-scope audit passed. |
| START-125 | Pet dialogue: `PetTalkTable.xml` supplied random lines selected by pet type, hunger and activity, displayed above the pet and in chat through pet-action messages. `src/session/talk_tables.c`, `src/game/packets_pet.c`. | Remove this optional chatter; preserve pet care, gameplay and separate emotion icons. | **Remove**, approved 2026-11-01 | Removed catalog/state, speech requests and presentation, and exclusive lookups/replacements. Emotion handling and timing retained. Full release rebuild, focused checks and source-scope audit passed. |
| START-126 | Monster skill animation catalog: `MonsterSkillInfo.xml` selects a sprite action and looping mode for a monster/skill pair. `src/session/talk_tables.c`, `src/game/skill_cast.c`, `src/game/skill_use.c`. | Keep this configurable animation support for existing and custom monsters; review monster speech separately. | **Keep**, approved 2026-11-01 | Unchanged. This chooses presentation, not monster AI, damage or skill availability. |
| START-127 | Local monster speech: `MonsterTalkTable.xml` supplied random dialogue shown in an actor balloon and chat when the attack path requested a line. `src/session/talk_tables.c`, `src/game/packets_act.c`. | Remove this optional locally generated chatter; preserve combat, monster skill animations and server-supplied speech. | **Remove**, approved 2026-11-01 | Removed catalog/state, lookup/formatting and attack-triggered speech. Pet emotions and combat retained. Full release rebuild, focused checks and source-scope audit passed. |
| START-128 | Readable map names: shared lookups turn map identifiers into display names for the minimap, character selection, party locations, quests, warp lists and the world map. `src/contents/map_info.c`, `src/session/session_names.c`. | Keep shared map-name support; review the text/Lua data sources and map-entry banners separately. | **Keep**, approved 2026-11-01 | Shared lookup retained. START-129 consolidates name data into Lua; instance-style names also resolve. |
| START-129 | Text map-name catalog: `mapNameTable.txt` supplied map/display-name pairs to the shared lookup; Lua could override entries afterward. `src/session/session_tables.c`. | Keep the text source; Patrick chose consolidation into Lua instead. | **Remove; consolidate into Lua**, approved 2026-11-01 | Removed the text loader. Added a byte-preserving converter and prepared populated Lua catalogs. Release compile/link, map-owner and migration checks, actual catalog round trips and scope audit passed. |
| START-130 | Map-entry banners: optional Lua metadata drew a decorative image and two title lines after map entry. Formerly `src/contents/map_name_display.c`. | Remove this optional decoration; keep readable names in ordinary displays. | **Remove**, approved 2026-11-01 | Removed renderer, metadata/state, entry/frame hooks, checkbox and saved preference. Legacy Lua decoration directives are inert for name-catalog compatibility. Full release rebuild, focused checks, catalog round trips and scope audit passed. |
| START-131 | Empty skill-info hook: text reload called `skill_info_nop`, whose body was empty. `src/session/session_tables.c`, `src/skills/skill_info.c`. | Remove the call, empty function and declaration; retain actual skill data loading. | **Remove**, approved 2026-11-01 | Removed all three references. Both affected C files compiled and the release executable linked; scope/reference and whitespace checks passed. |
| START-132 | Closing an open Setup window: startup looked for a window titled `Ragnarok Setup`, displayed an error notice and sent it `WM_CLOSE`. `src/session/session_tables.c`. | Remove this legacy interference with an external settings window. | **Remove**, approved 2026-11-01 | Removed title search, notice, close request and local window handle. Release compile/link and scope/reference/whitespace checks passed. |
| START-133 | Per-map fog: `fogParameterTable.txt` supplies fog start/end distances, color and density; map entry and the Fog toggle apply it. `src/session/session_tables.c`, `src/game/talk_commands.c`. | Keep this configurable map atmosphere and player toggle. | **Keep**, approved 2026-11-01 | Unchanged. Missing map entries disable fog; the lookup handles instance-style names. |
| START-134 | Shared session Lua runtime: opens the interpreter and loads catalogs/helper functions used by skills, appearances, status icons, hotkeys, options and navigation. `src/session/session_lua.c`, `src/contents/client_lua.c`. | Keep the shared runtime; review each loaded catalog and helper separately. | **Keep**, approved 2026-11-01 | Unchanged. This decision does not retain every script or optional feature in the initializer. |
| START-135 | Automatic file relocation: `ChangeDirectoryList` called a C helper to move listed loose files into new directories when the destination was absent. `src/session/session_lua.c`, `src/contents/client_lua_funcs.c`. | Remove this legacy startup migration; package files in their intended directories. | **Remove**, approved 2026-11-01 | Removed load, callback/registration, path joining and unused directory-creation helper. Release compile/link, Lua checks and source-scope audit passed. |
| START-136 | Lua UI text catalog: `MsgString_KR` defines named strings used by later Lua files, including hotkey labels and command help. `src/session/session_lua.c`. | Keep this shared, translatable text source; review its individual consumers separately. | **Keep**, approved 2026-11-01 | Unchanged. The filename does not force Korean text; the inspected local copy is English. |
| START-137 | Common Lua helpers: `Common` supplies table counting, default-value helpers and help-text lookup used by skill/emotion lists, hotkeys, effect registration and command help. `src/session/session_lua.c`. | Keep these shared helpers; review optional consumers and unused calls separately. | **Keep**, approved 2026-11-01 | Unchanged. Active callers exist in both C and loaded Lua catalogs. |
| START-138 | Lua window defaults: `wndInfo` supplies initial sizes and positions for windows including hotkey settings, quick slots, shop search and the basic button panel. `src/session/session_lua.c`, `src/ui/wndmgr/make_window.c`. | Keep these configurable defaults; review individual windows and legacy entries separately. | **Keep**, approved 2026-11-01 | Unchanged. The creation helper calls `GetWindowInfo`; saved window state remains separate. |
| START-139 | Lua hotkey definitions and matching: `HotKey` supplies default bindings, key labels, action lookup and remapping helpers used by input and the settings window. `src/session/session_lua.c`, `src/ui/wndmgr/keyboard.c`. | Keep shared customizable hotkey support; review individual actions and startup reset policies separately. | Keep, approved 2026-11-01 | Definitions, matching and remapping retained. Per-account local storage under START-038 remains separate; ledger-only change. |
| START-140 | Automatic empty `data` folder creation after loading HotKey. `src/session/session_lua.c`. | Remove this directory-creation leftover; GRF reads and existing loose-resource folders do not depend on it. | Remove, approved 2026-11-01 | Removed the lookup, directory creation and exclusive local variables. Changed unit compiled and release linked; source scope and whitespace verified. |
| START-141 | Legacy shared hotkey seed import: `SaveData/UserKeys.lub` or `.lua` supplied initial user bindings before per-account settings load. `src/session/session_lua.c`. | Remove this old shared import; use HotKey defaults and retained local per-account settings. | Remove, approved 2026-11-01 | Removed the one startup load. Release compiled/linked; existing local account persistence/isolation tests and scope audit passed. |
| START-142 | Forced startup hotkey resets: two `ResetTheHotKey` calls could remove user overrides involving Alt+OEM_3 and Alt+Home. `src/session/hotkeys.c`, `src/session/session_lua.c`. | Remove this legacy conflict cleanup; preserve Lua defaults and chosen account bindings. | Remove, approved 2026-11-01 | Removed startup call, exclusive C helper and declaration. Both changed units compiled and release linked; exact source scope and references verified. |
| START-143 | Shared emotion definitions: `EmotionList_F` and `EmotionList` map emote IDs to animations, text aliases and picker entries. `src/session/session_lua.c`, `src/game/actor_marks.c`, `src/session/session_lookups.c`. | Keep classic shared emote support; review the picker and optional extras separately. | Keep, approved 2026-11-01 | Shared definitions, commands and animation mapping retained. Ledger-only change; individual consumers remain separate decisions. |
| START-144 | Emotion picker: Alt+L opens a paged icon grid; clicking copies an emote command into chat or a selected macro field, and double-clicking requests the emote. `src/ui/chat/emotion.c`, `src/ui/wndmgr/key_down.c`. | Keep this convenient way to browse and use the retained emotes. | Keep, approved 2026-11-01 | Picker, paging, hotkey, macro-editor entry point and saved layout retained. Ledger-only change. |
| START-145 | Dice emote: `/dice` or `/e19` selects one of six dice faces on the client and sends its emotion ID through the game connection. `src/contents/client_lua_funcs.c`, `src/game/talk_type.c`. | Keep this familiar social emote; review the separate yut-style random emote next. | Keep, approved 2026-11-01 | Dice request paths, random face selection and animation mapping retained. Ledger-only change. |
| START-146 | Yut-style random emote: `/dbc` and a Korean alias requested one of seven locally selected emotion IDs. `src/contents/client_lua_funcs.c`, external EmotionList Lua catalog. | Remove this optional request variant; keep ordinary dice and received emotion rendering. | Remove, approved 2026-11-01 | Removed randomization; Lua callback and outgoing sender reject IDs 81..87. Received rendering and dice preserved. Both changed units compiled and release linked; exact scope verified. |
| START-147 | Emote cooldown: requires more than 2,500 ms since game-mode initialization or the previous sent emote. `src/game/msg_misc.c`, `src/game/mode_initialize.c`. | Keep the shared rate limit; review the separate pending-animation wait afterward. | Keep, approved 2026-11-01 | Cooldown retained; fixed 32-bit tick rollover and prepared a separate foundation patch. Original/fixed regression checks and release build passed. |
| START-148 | Pending-emote lock: after sending, further requests waited for the local player's emotion animation to finish or an emotion refusal. `src/game/msg_misc.c`, `src/effects/skills/msg_effect_process.c`. | Remove this extra lock while keeping the approved cooldown; it has no independent timeout when completion is missing. | Remove, approved 2026-11-01 | Removed field, check/set/reset sites and completion notifications. All source units rebuilt and release linked; timing and game-mode layout regressions passed. Foundation timing patch unchanged. |
| START-149 | NPC/monster identity table: `NPCIdentity` defines named numeric IDs used by Lua appearance and pet catalogs. `src/session/session_lua.c`, external DataInfo Lua files. | Keep this shared identifier catalog; review appearance helpers and pet-specific data separately. | Keep, approved 2026-11-01 | Identity table load and dependent named-ID support retained. Ledger-only change; individual catalogs remain separate reviews. |
| START-150 | Actor appearance-name catalog and lookup: `jobName` and `ReqJobName` map numeric types to resource names used for actor sprites/animations. `src/session/session_job_name_table.c`, `src/actor/npc.c`. | Keep this configurable appearance support; review pet-specific helpers and data separately. | Keep, approved 2026-11-01 | Catalog, lookup and active consumers retained. Ledger-only change; pet-specific helpers remain separate decisions. |
| START-151 | Unused pet-food precheck: an optional command argument enabled a Lua food lookup and inventory check, but all traced pet-command callers passed zero. `src/game/msg_misc.c`, `src/session/pet.c`. | Remove the dead precheck and its exclusive food lookup; retain ordinary feeding and server-result handling. | Remove, approved 2026-11-01 | Removed check, unused argument, lookup/declaration, private notice color and obsolete test stub. Release build, affected packet tests and exact scope audit passed. |
| START-152 | Static pet portraits: the pet window and evolution preview select local BMP illustrations through `PetIllustNameTable_Eng`/`GetPetIllustName_V3`. `src/session/pet.c`, `src/ui/items/pet_info.c`. | Keep customizable portraits; correct filename copying and prepare that fix for the foundation. Review evolution separately. | Keep portraits; fix filename handling, approved 2026-11-01 | Literal, capacity-aware copy with no-image fallback for missing/oversized names; both callers pass their buffer size. Release build, 18 regression cases, isolated foundation patch checks and scope audit passed. |
| START-153 | Pet accessories: Lua identifies accessory items and selects their animation; server updates drive the worn appearance, and pet controls show/remove equipment. `src/session/lua_tables.c`, `src/session/pet.c`, `src/game/packets_pet.c`. | Keep classic pet customization; review the legacy fallback separately. | Keep; review fallback separately, approved 2026-11-01 | Retained. Ledger-only decision; no source or asset changes. |
| START-154 | Missing/unknown pet accessory appearance falls back to the Chonchon gas-mask ACT in both Lua and C. `src/session/pet.c` and external `jobName_F`. | Remove the unrelated substitute and show the normal pet appearance, preserving server equipment state and valid accessory mappings. | Use normal pet appearance, approved 2026-11-01 | Explicit mapping check bypasses the Lua default; C returns no animation on lookup/file failure. Four packet paths restore the normal appearance through a shared helper. Release build, real-Lua behavior checks and exact scope audit passed. |
| START-155 | Pet evolution: recipe-driven menu entries open before/after portraits and material requirements, then request evolution through the game connection. Former `src/contents/pet_evolution.c`, `src/ui/character/pet_evolution_wnd.c`. | Remove this optional progression UI and recipe support; review automatic feeding separately. | Remove, approved 2026-11-01 | Removed window, menu, recipes, exclusive lookups, packets and type registration. Auto-feed eligibility is isolated in its own manager, with a no-op legacy recipe callback for combined catalogs. Full release rebuild, manager/dispatch/type checks and scope audit passed. |
| START-156 | Pet automatic feeding: eligible pets get a menu toggle that requests server-managed feeding and displays the server-confirmed state. `src/contents/pet_auto_feed.c`, `src/session/session_auto_feed.c`. | Keep this optional convenience; review homunculus auto-feeding separately. | Keep, approved 2026-11-01 | Eligibility catalog, menu toggle, requests and server-state handling retained. Ledger-only change. |
| START-157 | Homunculus automatic feeding: the status window checkbox requests server-managed feeding through configuration kind 3; replies update state and chat feedback. `src/ui/character/companion_info.c`, `src/session/session_auto_feed.c`. | Keep this optional convenience. | Keep, approved 2026-11-01 | Checkbox, request and server-state handling retained. Ledger-only change. |
| START-158 | Headgear appearance catalogs: `AccessoryId`, `accName_F` and `accName` map appearance IDs to sprite/animation filenames for character headgear. `src/session/session_job_names.c`, `src/ui/resource/resource_paths.c`. | Keep this shared artwork lookup; review fallback and ID limits separately. | Keep; review fallback and limits, approved 2026-11-01 | Catalogs, lookup and active consumers retained. Ledger-only change. |
| START-159 | Hardcoded headgear substitute: initial name slots and out-of-range appearance IDs used a Chick Hat suffix. `src/session/session_job_names.c`, `src/session/sprite_names.c`. | Remove the substitute and show no headgear for unresolved appearances; preserve equipment state and valid mappings. Review the ID limit afterward. | Show no headgear for unresolved IDs, approved 2026-11-01 | Removed both substitute values. Valid mappings and equipped IDs are unchanged. Changed-unit release build, existing resource-path checks and exact scope audit passed. |
| START-160 | Headgear catalog limit: startup only queries appearance IDs 1-2999, so configured higher IDs never enter the name cache. `src/session/session_job_names.c`. | Remove this arbitrary catalog cap; preserve packet formats and review their limits separately. | Remove fixed catalog cap, approved 2026-11-01 | Replaced the cache with on-demand `ReqAccName` lookups into owned strings. Full release rebuild, high-ID/Lua-boundary tests, session-field/resource-path checks and exact scope audit passed. |
| START-161 | Doram headgear fallback: if the race-specific artwork is absent, the client tries the same headgear name in the standard folder. `src/ui/resource/resource_paths.c`. | Keep this resource reuse while Doram support remains; review the race itself separately. | Keep, approved 2026-11-01 | Doram-specific priority and same-hat standard-folder fallback retained. Ledger-only change. |
| START-162 | Extra startup script: `xofyldas` loads an optional Lua chunk after headgear catalogs, if present. `src/session/session_lua.c`. | Remove this unexplained load: neither loose assets nor either inspected GRF contains the script. | Remove, approved 2026-11-01 | Removed the single startup call. Changed-unit release build/relink and exact scope audit passed. |
| START-163 | Chat tabs and detached windows: Lua settings restore names, message filters, separate-window geometry and the layout lock. `src/session/session_lua.c`, `src/session/chat_tabs_layout.c`, `src/ui/chat/chat_window.c`. | Keep customizable chat layouts and local persistence; review individual policies separately. | Keep; review policies separately, approved 2026-11-01 | Chat layouts, defaults, local saves and consumers retained unchanged. Ledger-only decision. |
| START-164 | Chat-panel count limit: ten combined main tabs and detached windows; UI creation warns at capacity, and Lua import stops adding records at that count. `src/ui/chat/chat_window_msg.c`, `src/session/chat_tabs_lua.c`. | Keep the current ten-panel capacity while retaining customizable layouts; revisit if more panels are needed. | Keep, approved 2026-11-01 | Ten-panel UI/import limit retained unchanged. Ledger-only decision. |
| START-165 | Chat layout lock: the padlock toggle prevents dragging and docking changes, blocks detached-window resizing/renaming, and hides layout-editing buttons. Its state is saved locally. `src/ui/chat/chat_window.c`, `src/ui/chat/sub_chat.c`. | Keep the player-controlled lock to prevent accidental rearrangement. | Keep, approved 2026-11-01 | Padlock toggle, interaction guards and saved preference retained unchanged. Ledger-only decision. |
| START-166 | Chat-layout save scope: every server/account using the same client folder reads and writes one `SaveData/ChatWndInfo_U` layout. | Proposed local layouts per server/account; Patrick prefers the shared layout. | Keep one shared layout per client folder, approved 2026-11-01 | Shared filename and lifecycle retained; no account/character storage changes. |
| START-167 | Chat-layout name serialization: raw tab names in Lua long brackets/quotes could break the saved script or change the restored name. `src/session/chat_tabs_layout.c`. | Preserve names literally with escaped Lua strings; prepare the general fix independently for the foundation. | Correctness fix for retained chat layouts, 2026-11-01 | Reproduced failures, corrected both panel types, expanded existing round-trip tests; release build and purist/foundation suites passed. Two-file foundation patch prepared. |
| START-168 | Compiled chat-layout priority: startup prefers `ChatWndInfo_U.lub` over the `.lua` file written by the client. `src/contents/client_lua.c`, `src/session/session_lua.c`. | Remove compiled-layout import for this user-settings file and load the saved `.lua` directly. | Remove .lub import for chat layouts, approved 2026-11-01 | Added exact local-file loading for the chat layout; packaged defaults and other catalogs retain their loading rules. Release build, extended Lua suites and scope audit passed. |
| START-169 | Chat-layout save failures: the writer truncated the old file before writing and ignored write results. | Preserve the old file until a complete replacement is saved; show one clear warning on failure. | Show one clear warning, approved 2026-11-01 | Checked temporary-file saves and post-cleanup warning implemented. Release build, real-file round trips, 126 write-fault positions and other failure cases passed. Foundation patch prepared after START-167. |
| START-170 | Job-inheritance catalog: Lua identifies earlier job stages used to populate the skill window's class trees/tabs. `src/session/session_lua.c`, `src/session/lua_tables.c`. | Keep this shared skill-window support; review individual skill catalogs and presentation policies separately. | Keep, 2026-11-01 | Catalog, inherited-job lookup and skill-window consumers retained. Ledger-only decision. |
| START-171 | Skill-ID definitions: Lua assigns readable symbols to numeric skill IDs for other skill catalogs. `src/session/session_lua.c`. | Keep this shared lookup; review skill metadata, descriptions and tree layouts separately. | Keep, 2026-11-01 | Skill-ID startup load retained. Ledger-only decision. |
| START-172 | Skill-information catalog: names, icon identifiers, level costs, prerequisites and client targeting/level-selection data. `src/session/session_lua.c`, `src/skills/skill_info.c`. | Keep this shared catalog; review descriptions and individual skill policies separately. | Keep, 2026-11-01 | Skill-information catalog and consumers retained. Ledger-only decision. |
| START-173 | Skill-description text: local Lua descriptions displayed in skill information windows and hover panels. `src/session/session_lua.c`, `src/ui/character/skill_dialogs.c`. | Keep customizable skill descriptions; review display policies separately. | Keep, 2026-11-01 | Description catalog and display support retained. Ledger-only decision. |
| START-174 | Skill Lua helpers: expose catalog values, build inherited skill-tree records and prerequisites, and format descriptions. `src/session/session_lua.c`, `src/session/lua_tables.c`, `src/skills/skill_info.c`. | Keep this shared connection between skill catalogs and client behavior; review individual policies separately. | Keep, 2026-11-01 | Skill helper load and callers retained. Ledger-only decision. |
| START-175 | Per-job skill-tree layouts: Lua assigns skills to cells in each class tree. `src/session/session_lua.c`, `src/contents/client_lua_funcs.c`, `src/ui/character/skill_tree.c`. | Keep configurable skill-tree layouts; review individual display policies separately. | Keep, 2026-11-01 | Configurable per-job skill-tree layouts retained. Ledger-only decision. |
| START-176 | Compact skill-list view: players can switch the skill window between a class-tree grid and a narrower vertical list, with the choice saved locally. `src/ui/character/skill_tree.c`, `src/ui/character/skill_tree_draw.c`. | Keep the optional compact view and saved preference. | Keep both views, 2026-11-01 | Tree and compact list views, switch and saved preference retained. Ledger-only decision. |
| START-177 | Skill hover descriptions: a checkbox controls automatic description panels in the skill window and the choice is saved locally. `src/ui/character/skill_tree.c`. | Keep player-controlled hover descriptions; manual description windows remain separate. | Keep hover panels and toggle, 2026-11-01 | Hover panels, checkbox and saved preference retained. Ledger-only decision. |
| START-178 | Skill prerequisite highlighting: hovering a skill marks required skills and displays their required levels in the tree grid. `src/ui/character/skill_tree_points.c`, `src/ui/character/skill_tree_draw.c`. | Keep prerequisite guidance; review right-click pinning and tab-selection assumptions separately. | Keep, 2026-11-01 | Prerequisite highlighting and required-level labels retained. Ledger-only decision. |
| START-179 | Right-click prerequisite pinning: freezes the highlighted prerequisite selection across hover/tab changes and blocks adding planned skill points while pinned. `src/ui/character/skill_tree.c`, `src/ui/character/skill_tree_points.c`. | Remove this hidden pinning mode and its point-allocation lock; keep ordinary hover highlighting. | Keep prerequisite pinning, 2026-11-01 | Pinning, cross-tab persistence and its point-add lock retained per Patrick. Ledger-only decision. |
| START-180 | Prerequisite tab hints: hardcoded skill-ID ranges choose which class tab to color, without checking its skill list. `src/ui/character/skill_tree_points.c`. | Keep the hints but resolve tabs from their actual displayed skill lists; prepare a separate foundation correctness fix. | Use actual tab contents; prepare foundation fix, approved 2026-11-01 | Actual displayed lists now select tab hints. Release build and 17 regression checks passed; standalone four-file foundation patch prepared and tested. |
| START-181 | Skill-point preview: stage level increases locally, Apply confirms and sends upgrade requests, Reset discards unsubmitted changes. `src/ui/character/skill_tree_points.c`, `src/ui/character/skill_tree.c`. | Keep preview and Apply/Reset controls; review automatic prerequisite allocation separately. | Removed, 2026-11-02 (Patrick): raise immediately with one ordinary upgrade request | Preview, planned levels, working copies, pending display, Apply/Reset, confirmation and automatic prerequisite planning removed; the raise control sends `0x0112` when points and session prerequisite levels allow; START-179/180 kept. [Skill-point preview removal](#skill-point-preview-removal-start-181-2026-11-02) |

## Review position

START-002 is approved, implemented and checked. Existing archive loading opens files read-only with
`OPEN_EXISTING` (`src/fileio/grf_mapfile.c:26`); a missing archive fails to open and
does not get created. No replacement archive or new startup requirement was added.

START-003 is approved and implemented. Remaining references to movie textures use
the existing missing-texture fallback: the resource registry does not recognize
`bik`, so no decoder or file load is dispatched. The two retired UI type names
remain only in historical test fixtures and assertions that they are no longer
registered, alongside other retired baseline types.

START-004 is approved to keep. START-005 is approved and implemented. Shared return-to-login
helpers formerly in `msg_replay.c` now live in `src/game/msg_login.c`. Live quest,
inventory, navigation and name-card owners remain; their replay serialization is removed.
The retired replay UI type names remain only in historical fixtures and tests that
assert they are no longer registered. `replay_state()` in `src/render/gfx_d3d9.c`
restores Direct3D state after a device reset and is unrelated to game replay.

START-006 is implemented. The Windows command line is ignored. Startup registers
the data archive and loads `clientinfo.xml`; its service/server settings continue
to select the normal language, resources and login flow. Server-issued authentication
responses remain part of that flow. Helper processes no longer receive launcher
tokens or metadata. Display-setting changes that require exiting no longer open
an official launcher webpage. A leftover close call for the removed replay
indicator was also removed from character selection.

START-007 is implemented. Startup proceeds from archive registration directly to
`clientinfo.xml`. Lua's file loader uses its stock behavior; ordinary resource and
script loading remain. The checker-only MD5 was removed, while login password
hashing and the executable hash packet retain their separate `contents/md5ctx.c`
implementation. Shutdown no longer deletes `nconfig.ini`, `ffindhack.exe`,
`gfindhack.exe`, `removenp.exe` or `cfindhack.exe`.

The independent Themida startup flag was initially retained during START-007;
START-013 subsequently approved and removed it together with its temporary
dedicated test.

START-008 is implemented. The client no longer renames or deletes those AI folders
at startup. `src/actor/npc.c` still loads `AI/AI.lua`, `AI/AI_M.lua` and their
`AI/USER_AI` variants. Validation: `build/purist-no-ai-folder-swap-build.log`;
no updater references remain in application sources or tests.

START-009 is implemented. Validation: `build/purist-no-ragpath-build.log`; no
`RagPath` references remain in application sources or tests. Existing registry
values were not touched; only the client's future write was removed.

START-010 is implemented. `src/core/startup_settings.c` reads the client registry
key once; a missing or inaccessible key keeps defaults and does not launch another
program. Missing, malformed or failed individual reads keep that setting's default
without discarding valid neighboring settings. Graphics options still load through
the normal session option path, whose fallback is a 1024x768 window. The sound-mode
default was initially retained as disabled (`0`); START-090 subsequently changes
the fallback to positional (`2`). Saved sound settings remain honored.
The stale-display notice now points to the in-game graphics settings.

START-011 is implemented. Session startup now proceeds directly to resource-name
aliases and window creation. No application source or test references remain to
the scanners, their OS-selection helper or `IProcess` state. No dedicated behavioral
test was added for removal of this unused result; the release build validates callers
and linkage (`build/purist-no-process-scan-build.log`, passed without compiler or
linker warnings). The final link map contains none of the retired scan symbols.
No live-client runtime smoke test has been performed.

Still to review within `client_init()`: individual initialization and cleanup
registrations in `src/core/client_init.c:81-143`. The overview above does not mark
those systems as approved.

Client configuration remains in accordance with the direct-`clientinfo.xml`
decision; its individual options and regional behaviors still need review.
START-012 is approved to keep. Resource alias loading, lookup and map-name
resolution remain unchanged.

The following `app_create_window()` registers the native window and message
handler, assigns its icon/cursor, uses the saved size and position or centers it,
keeps its position within the primary screen bounds, and shows it. Recommendation:
keep this essential window setup; no changes were made.

START-013 is implemented. No application, test or build references remain to the
Themida wrapper. The release rebuild passed without compiler or linker warnings
(`build/purist-no-themida-build.log`); no Themida symbols remain in the final link
map. No live-client runtime smoke test has been performed.
No new test was added for removal of the empty wrapper; the dedicated flag-only
test was removed with it.

START-014 is implemented. Sending no longer allocates, resets or reads packet-key
state. Packet ID bytes, payloads, immediate/held sends, disconnected-send behavior,
receive framing and the existing special packet-length rules remain unchanged.

START-015 removes the separate `REASSEMBLY_AUTH` handshake: incoming IDs
`0x085a`-`0x0883` and `0x0917`-`0x0940`, with their corresponding replies
`0x0884`-`0x08ad` and `0x0941`-`0x096a`. These IDs are no longer supported;
servers must not require them. The three exclusive reply helpers are gone,
including `src/game/net.c` and its header. Other packet entries and the existing
`0x02bf`/`0x02c0` special-length rules remain unchanged.

START-016 is approved to keep. Named STR effect loading, registration and skill
callers remain unchanged. This retains the support mechanism; individual skills
still need review. No additional tests were needed for this keep decision.

START-017 is approved to keep. The shared effect manager and particle pools remain
unchanged; retaining them does not preselect individual effects for retention.
No additional tests were needed for this keep decision.

START-018 is implemented. Queue UI types `0x9d`, `0xd1`, `0xd2` and `0xd3` are no
longer created. Both menu layouts omit the entry button. The client no longer
loads `Lua Files/EntryQueue/EntryQueueList`, registers `AddEntryQueue`, or reads and
writes the queue window position or `ShowBattleFieldIcon` setting. Existing host
settings were not edited. Two helpers used only by queue admission checks were
removed: `game_mode_has_state_icon` and `session_count_online_party_members`.

Queue packets `0x08d7`-`0x08e1`, `0x090a` and `0x090e` are unsupported. A server
must not depend on this queue protocol. Ordinary battleground team, score,
position, HP/SP and chat handling remains, as does the separate ranking system.

START-019 is approved to keep. Scheduler creation, ticking, job ownership and
cleanup remain unchanged. The individual jobs were reviewed in START-020 through
START-024; heartbeat and ranking display are retained.

START-020 is implemented. Game-mode scheduling now creates only the heartbeat job.
The empty GDI job and its otherwise unused random interval mode are gone. The
shared scheduler, repeating jobs and one-shot support remain. Existing ping packet
bytes and timeout behavior are unchanged. Scheduler tests cover the two remaining
allocation failure points and verify that the sole game-mode job is the heartbeat.

START-021 is approved to keep. Heartbeat scheduling, keepalive packets and the
60-second receive timeout remain unchanged. No additional tests were needed for
this keep decision.

START-022 is implemented. The deadline popup and timed chat reminders are gone,
along with their only caller, scheduling state and legacy unused `ShutDownSchedule`
type. Both the active notice (`0x0986`) and already-ignored shutdown-info packet
(`0x08e4`) are now unsupported. Other login cases are unchanged. Normal application
shutdown and the retained connection heartbeat/timeout remain.

START-023 is implemented. The registration window, local BMP browser/compressor,
upload scheduling, window-manager slots, UI-open selector and protocol entries
`0x0a52` through `0x0a55` are removed. The stray challenge-window close action
that targeted the registration window is removed too. Challenge receive/answer,
server-image previews and reporting windows remain for separate review.

START-024 is approved to keep. Delayed ranking display remains unchanged. The
scheduler review is complete.

START-025 is approved to keep. Town facility data, minimap markers, tooltips and
facility navigation remain unchanged. No additional tests were needed for this
keep decision.

START-026 is approved to keep. Timed status-icon and shortcut-bar countdown
overlays remain unchanged. No additional tests were needed for this keep decision.

START-027 is approved to keep. The core quest journal, on-screen tracker, quest
progress, descriptions, rewards and NPC navigation remain unchanged. No additional
tests were needed for this keep decision.

START-028 is implemented. The recommended catalog, Lua callbacks and loading,
local checkmark persistence, extra manager state, detail pane, artwork path and
journal tab are gone. Active, Paused and Other remain. Their stock skin sections
are remapped to three click regions without requiring replacement game assets.
Existing saved preference files are no longer read or written.

START-029 is implemented. Quest startup always loads
`system/OngoingQuestInfoList_True`, including when clientinfo selects Sakray.
The shared quest callbacks and saved tracking settings remain. Other server-type
behavior has not been changed by this decision.

START-030 is implemented. Clan manager/lifecycle, UI/state, chat routing and
commands, destination/filter entries, hotkey action, emblems, hardcoded status
images and protocol are gone. Guild creation/invitation no longer checks clan
membership. Ordinary guilds remain; the existing guild-title and dressroom packet
handlers now live in their respective guild/UI modules.
The 21 retained chat-filter rows keep their original setting IDs, so existing
saved options do not shift. The generic 22-slot saved-settings format remains
compatible; the removed filter's slot has no UI row or clan-message producer.

START-031 is approved to keep. The monster presentation table, sprite scaling and
configured effects remain unchanged. No additional tests were needed for this
keep decision.

START-032 is approved to keep. Shared map-rule storage, queries and server updates
remain unchanged. No additional tests were needed for this keep decision.

START-033 is implemented. Startup no longer opens `MapInfoTable.scp`, parses its
fallback values or shows its missing-file popup. The map-rule manager and server
packet handling remain. The client now depends on the server's `0x099b` message
for these flags; no live-server verification has been performed.
The removed parser's out-of-bounds read was reproduced, and a separate foundation
patch preserves local-table support with bounded parsing (validation below).

START-034 is approved to keep. The RODEX mailbox, messages, notifications and
item/zeny attachments remain unchanged. No additional tests were needed for this
keep decision.

START-035 is implemented. Roulette no longer has a manager, launcher icon, prize
board, animation, coin/prize state, slash command, saved-option mapping or protocol
handling. Historical type fixtures retain the old names only to verify that the
retired windows are rejected. The prize-ordering defect found during removal has
a separate foundation correction (validation below).

Current review: START-037 through START-043 are implemented, START-044 is
approved to keep unchanged, and local map signboards and reputation are removed
under START-045/046. The guild initializer manages the emblem versions, cache and
requests already retained under START-037. The Agency and party-application queue
initializers are retained under START-041. Item reforming and the dedicated
enchanting window are removed under START-048/049. Their unused item-name lookup
is now removed under START-050, superseding the earlier START-047 keep decision.
Map sky settings are approved to keep unchanged under START-051. Selectable item
packages are removed under START-052, finishing the manager initialization list.
Essential graphics-device and renderer creation stay under the initial keep
direction. Network startup initializes Winsock and the shared game connection.
Its ignored Winsock address check is removed under START-053. The game-step timer
is approved to keep unchanged under START-054, as are saved UI layouts under
START-055. The shared Granny animation cache is approved to keep unchanged under
START-056. START-057 now loads guild-flag animations normally on demand, with
emblem rendering retained and a separate foundation fix prepared. START-058 keeps
folder skins and removes ZIP extraction; tests and the full release build passed.
The shared angle lookup tables are approved to keep under START-059. START-060
removes the regional warning/licence screens and starts at connection selection.
Patrick revised START-061 to remove personal minimap notes. The feature is now
removed from purist; the earlier save-error/newline fix remains an independent
foundation patch for clients retaining notes.
Local navigation destination lists are removed under START-062. The login/game
lifecycle loop is kept under START-063. The hardcoded regional website launches
on exit are removed under START-064, and the configurable `viewhomepage` launch is
removed under START-065. Built-in screenshot capture is kept under START-066.
The character-ID stamp, visible timestamp and logo are removed under START-067/068/069.
Automatic trade capture is removed under START-070, completing the screenshot
extras review. Screenshot correctness fixes recorded under START-066 are
implemented and prepared as an independently tested foundation patch.
Graphics-adapter enumeration and saved-data directory creation are kept under
START-071/072. The floating-point precision setting is kept under START-073.
Timer resolution is kept under START-074. START-075 DLL investigation is complete:
tested middleware does not require caller COM initialization, but retained shell
URL links provide a documented reason to keep it. After reviewing external-page
features, Patrick kept COM and approved fixing cleanup. The Korean three-password refusal
behavior is removed under START-076, and both password-change redirects are
removed under START-077/078. The authentication-result GNJOY redirect is removed
under START-079, and the account-payment prompt/redirect is removed under
START-080. The registration button and web-link setup are removed under START-081.
The cash-shop Charge button and promotional banners are removed under
START-082/083. Attendance banners are removed under START-084, and the extra
authentication message-box website button is removed under START-085. Shared
text-viewer website links are kept under START-086. COM cleanup is corrected on
purist with an independent foundation patch. Shared audio playback is kept under
START-087. Stopping startup on audio initialization failure is kept under
START-088. The ineffective saved audio-provider name is removed under START-089.
The fallback audio mode is now positional under START-090; output format is fixed
at 22,050 Hz, 16-bit under START-091. The distant-monster sound investigation is
recorded under START-092 and deferred at Patrick's request to resume pruning.
The simultaneous-effect selector is removed under START-093; startup requests
up to 48 channels subject to backend availability. The saved speaker-layout
preference is kept under START-094. The optional `/tingonly` mode and its chat-room
join alert are removed under START-095. Server-requested NPC music is kept under
START-096, with stream-state and handoff corrections also prepared for the
foundation. Server-requested WAV effects and music volume fades are kept under
START-097/098. The unused audio-output flag is removed under START-099. Review
now continues inside `session_startup`. The main item-information catalog is kept
under START-100. Its legacy `item.txt` / `eventItem.txt` alternative is removed
under START-101. Supplemental `Event*` item names/descriptions are removed under
START-102. Item artwork overrides and the NPC-shop ATK/DEF estimate bar with its
local tables are removed under START-103/104. Shared card recognition is kept
under START-105, card illustrations under START-106, and card prefix/suffix
naming and search aliases under START-107. Readable books, including their
automatic display mode, are removed under START-108. Forging-window instructions
are kept under START-109. Item restriction notices are kept under START-110,
with loader correctness fixes prepared separately for the foundation. Its
missing-file popup is kept under START-111. The illustrated Tip Box guide is
removed under START-112, and the legacy Tip of the Day window and startup-display
preference under START-113. Guild-help text is removed under START-114 and unused
random-value bookkeeping under START-115. The unused initialization counter is
removed under START-116. Job-based player hit-impact sounds are kept under
START-117. The map background-music catalog is kept under START-118; unmapped
maps now use silence under START-119. Indoor-map camera rules are kept under
START-120, per-map camera presets under START-121, and skill visual-effect
mappings under START-122. The shared interface text catalog is kept under
START-123. The local word blacklist is removed under START-124, and pet dialogue
under START-125. The monster skill animation catalog is kept under START-126.
Local monster speech is removed under START-127. Shared readable map names are
kept under START-128; the text catalog is removed and names consolidated into
Lua under START-129. Map-entry banners are removed under START-130 and the
empty skill-info hook under START-131. Closing an open Setup window is removed
under START-132. Per-map fog is kept under START-133, and the shared session
Lua runtime under START-134. Automatic file relocation is removed under
START-135. The Lua UI text catalog is kept under START-136 and Common helpers
under START-137, Lua window defaults under START-138, and Lua hotkey definitions
and matching under START-139. Automatic empty `data` folder creation is removed
under START-140, the legacy shared hotkey seed import under START-141, and
forced startup hotkey resets under START-142. Shared Lua emotion definitions
are kept under START-143, the emotion picker under START-144, and the dice
emote under START-145. Local Yut requests are removed under START-146. The emote
cooldown is kept under START-147, with a tested clock-rollover correction also
prepared for the foundation; the extra pending-animation lock is removed under
START-148. The NPC/monster identity table is kept under START-149 and actor
appearance lookup under START-150. The unused pet-food precheck is removed
under START-151. Static pet portraits are kept under START-152, with bounded
literal filename copying also prepared as a standalone foundation patch. Pet
accessory support is kept under START-153. Missing/unknown accessory appearances
now fall back to the normal pet under START-154. Pet evolution is removed
under START-155. Pet automatic feeding is kept under START-156, and homunculus
automatic feeding under START-157. Headgear appearance catalogs are kept under
START-158. The hardcoded Chick Hat substitute is removed under START-159.
The fixed headgear catalog cap is removed under START-160; protocol field widths
remain unchanged. Doram headgear fallback is kept under START-161. The optional
`xofyldas` startup load is removed under START-162. Chat tabs, detached windows
and local layout settings are kept under START-163. The combined ten-panel
limit is kept under START-164 and the layout lock under START-165. Patrick
kept one shared layout per client folder under START-166. Literal name saving
is corrected under START-167, with a separate tested foundation patch. Compiled
chat-layout import is removed under START-168. Checked file replacement and one
clear save-failure warning are implemented under START-169, with a tested
foundation patch following START-167. Job-inheritance support is kept under
START-170, shared skill-ID definitions under START-171, and the skill-information
catalog under START-172. Skill descriptions are kept under START-173 and
skill Lua helpers under START-174. Configurable per-job skill-tree layouts are
kept under START-175. Both the tree and compact list views are kept under
START-176. Hover descriptions and their toggle are kept under START-177.
Prerequisite highlighting is kept under START-178. Patrick kept right-click
pinning and its point-add lock under START-179. Prerequisite tab hints now use
actual displayed skill lists under START-180, with a tested standalone foundation
patch. Resume at skill-point preview and Apply/Reset (START-181), then automatic
prerequisite allocation and remaining skill-window policies before StateIcon.
Continue with the individual loads and
startup policies in `session_init_lua_files`, followed by the remaining steps
in `session_init_table`.

## Local account settings (START-038)

Account preferences now live in
`<client>/SaveData/Accounts/<server-id>/<numeric-account-id>.json`. The server ID is
SHA-256 of the configured login address, port and world name, separated by NUL
bytes. World names never become path components. Changing that configured identity
selects a different directory; aliases for the same endpoint are not merged.

The version-1 UTF-8 JSON file contains complete key-binding, ten chat/emotion macro,
and whisper-block-list snapshots. Explicitly empty lists/bindings/macros remain
empty after reloading. Text is converted at the string level before serialization;
malformed encoded strings are rejected. Files are limited to 1 MiB.

- Capture the initial preferences after startup Lua and registry loading, before
  the first account is loaded. Lua hotkey defaults and existing registry macros
  seed new accounts; the old shared `UserKeys.lua`/`.lub` import was subsequently
  removed under START-141. Later account switches restore this snapshot before
  applying the selected account's file, preventing settings from the previous
  account from becoming a new account's defaults.
- Save through the existing quit/character-selection paths, on account changes,
  and when applying key bindings or chat macros. Saving a complete snapshot also
  records deletions. A small synchronous file write replaces the old HTTP jobs.
- Write and flush a temporary file in the destination directory, close it, then
  replace the target. Failed writes/flushes/replacements leave the previous file
  intact. Temporary files are removed on failure.
- Missing files use the initial preferences and become writable. Unreadable,
  oversized or invalid files are preserved; that account uses initial preferences
  with saving disabled until a later successful load. The game-entry notice and
  hotkey save result report local failures; debug output records save failures.
- Account changes no longer write shared `UserKeys.lua` or the shared chat-macro
  registry key. Registry macros remain an initial-preference source; UserKeys
  import was removed under START-141. Remote-only preferences are not downloaded
  or migrated. These files stay on this
  PC unless copied with the client.
- Removed account HTTP endpoints, work types/payloads, URL/auth-token plumbing,
  async-save counters, the account shutdown wait, hotkey save bookkeeping, whisper
  change counters and the duplicate loaded-macro snapshot. The local owner and
  serialization live under `src/contents`; remaining HTTP services are separate.

Validation:

- Full release build and subsequent changed-unit relinks passed without compiler
  or linker warnings (`build/purist-local-account-final-build.log`,
  `build/purist-local-account-final-link.log`,
  `build/purist-local-account-empty-macro-link.log`).
- `tests/build_local_account.bat` exercises the actual Lua runtime, JSON code and
  Windows filesystem in a fresh temporary directory. It checks account, address,
  port and world isolation; fresh-owner reload; initial settings; Korean UTF-8
  round trips; empty-state persistence; invalid schemas/encoding/corrupt files;
  invalid account selection; and injected write/flush/replace failures preserving
  the old bytes (`build/purist-local-account-tests.log`). No HTTP code is linked.
- Hotkey ownership/failure checks, whisper edits and packets, remaining service
  ownership/submissions, JSON/character/shop saves and async work checks passed
  (`build/purist-local-account-hotkey-tests.log`,
  `build/purist-local-account-whisper-tests.log`,
  `build/purist-local-account-service-tests.log`,
  `build/purist-local-account-json-tests.log`,
  `build/purist-local-account-async-tests.log`). Async submissions cover 10 retained
  payload cases and 185 allocation-failure scenarios.
- Game-mode layout passed with digest `cb530c74`. Packet dispatch passed 1,179,800
  cases with unchanged digest `e183046ac7a0f56f`
  (`build/purist-local-account-mode-tests.log`, `build/purist-local-account-dispatch.log`).
- Source/link searches find no account HTTP jobs/endpoints or retired save helpers.
  The only reduced-reference helper, `get_hotkey_info`, retains its live caller
  (`build/purist-local-account-reference-audit.log`). `git diff --check` passed.
  No interactive game-data/login/UI smoke test was performed.

A separate foundation issue surfaced: empty chat macros were skipped when applying
edits and when reading registry values. The purist path now accepts them; the
independent [empty-chat-macro.patch](upstream/empty-chat-macro.patch) contains the
two foundation source corrections and an empty-registry-value regression assertion.
That assertion fails with the original reader (`build/purist-empty-macro-before.log`)
and passes after the correction. The patch passes `git apply --check` against
`main`; it has not been applied or committed there. The existing foundation commit
`7fe9620` remains the only committed fix from this review. No server changes or
pushes were made for local account settings.

## Local character settings (START-039)

Selected casting levels for player and homunculus skills now live in
`<client>/SaveData/Accounts/<server-id>/<account-id>/Characters/<character-id>.json`.
The server ID uses the same login-address/port/world hash as account preferences;
account files keep their existing path. `settings_file.c` now owns both paths and
the shared bounded read/atomic replacement code.

The version-1 JSON object contains `player` and `homunculus` arrays of `{id, level}`
entries. Only nonzero levels are written; omitted IDs mean zero/default. Both arrays
are required, including when empty. This is a complete preference snapshot, not a
list of learned skills or a change to character progression. The game server still
supplies learned skill levels, and the existing UI clamps selected casting levels.

- Load synchronously at the existing map-server connection step. A different file
  first saves the previous character to its frozen path and clears both preference
  arrays. Reloading the already loaded identity preserves unsaved edits.
- Existing `SkillUseLevelInfo` registry entries for the selected character slot seed
  characters without a file; absent values use zero/default. The prior mercenary
  registry initialization remains. Registry reads now reject non-DWORD, wrong-sized,
  negative or greater-than-65535 values. No registry writes were added.
- Valid files replace both arrays together after schema/range/duplicate-ID checks
  and staging allocation. Malformed or unreadable files use registry/default values
  with saving disabled so the original file is preserved. Errors go to debug output.
- Save through the existing quit, character-selection and window-close paths, and
  before switching/resetting the loaded identity. Saving reads the complete chosen
  level arrays even if the skill tree is empty or a homunculus is not summoned.
  Explicitly empty arrays remain empty after reloading.
- Save targets do not depend on session IDs that may already have changed during a
  transition. Selecting an invalid identity clears the writable path. Missing files
  become writable. The common 1 MiB limit and temporary-write/flush/replace behavior
  preserve prior files on failed writes; account files are unaffected.
- Removed `/charconfig` requests, async work records/pools/payloads, URL/token fields,
  pending-save counters, the HTTP owner, skill dirty markers 7/8, and the now-unused
  skill-tree visitor and its tests. The other skill-tree/container tests remain.
  Remote-only preferences are not downloaded or migrated; local files can be copied
  with the client. No matching server change is needed for local preferences.

Validation:

- Full release build passed; after extracting the registry reader and pruning the
  visitor, all changed units were rebuilt and the client relinked without warnings
  (`build/purist-local-character-build.log`, `build/purist-local-character-link.log`).
- `tests/build_local_character.bat` checks real JSON and temporary files, with mocked
  registry access and skill setters. It covers server/account/character/port/world
  isolation, slot-specific defaults, full replacement, empty lists, both ID-range
  endpoints, fresh-owner reload, preservation of unavailable skills, frozen save
  targets, invalid identities, twelve malformed files and staging-allocation
  failure. The actual registry reader also rejects wrong types/sizes and invalid
  levels. No host registry was accessed (`build/purist-local-character-tests.log`).
- The existing account suite passed after extracting shared file storage, including
  Korean text and write/flush/replace failure preservation
  (`build/purist-local-character-account.log`).
- Remaining HTTP owners/submissions, JSON/shop saves, async work and skill/container
  tests passed (`build/purist-local-character-services.log`,
  `build/purist-local-character-json.log`, `build/purist-local-character-async.log`,
  `build/purist-local-character-skills.log`). Async coverage now has eight payload
  cases and 150 allocation-failure cases. Game-mode layout retained digest
  `cb530c74` (`build/purist-local-character-mode.log`).
- Source/link searches found no character HTTP endpoints/jobs, retired registry
  wrapper or skill-tree visitor. The reduced-reference helpers in
  `build/purist-local-character-reference-audit.log` all retain live callers.
  `git diff --check` passed. No live login or in-game UI smoke test was performed.

Foundation finding: [skill-level-registry-validation.patch](upstream/skill-level-registry-validation.patch)
is an independent, one-file validation fix for the original registry reader.
The same malformed-type case fails with its original unchecked reader
(`build/purist-skill-seed-before.log`) and passes with the correction. The patch
passes `git apply --check` against `main`; it is prepared only, with no new commit
or push. All START-039 implementation remains uncommitted on `purist`.

## Local merchant shop setups (START-040)

The existing save-on-close and load-previous-setup feature now uses
`<client>/SaveData/Accounts/<server-id>/<account-id>/Characters/<character-id>.vending.json`
and the equivalent `.buying.json`. They share the existing server identity hash and
atomic file writer with account/character preferences, and keep those files intact.
The version-1 UTF-8 document contains a `data` object with the shop `Title` and an
`Item` array of IDs, quantities, prices, identification/refinement, cards and random
options. Vending and buying files are independent. No server change is required.
Remote-only shop setups are not automatically downloaded or migrated.

- Save synchronously when closing a normal player shop, before its window is closed
  or its live item list is cleared. Keep the existing exclusion for shop assistants.
  UI values needed after closure are captured before closing the old window.
- The existing load button calls local storage directly and applies the result to
  that setup window. It no longer disables itself while waiting for an HTTP callback
  or guesses a destination through unrelated global window pointers. Chat reports
  success, missing saved setups, read errors and save failures.
- Parse and stage the entire snapshot before replacing the owned title/items.
  Missing or invalid files leave the current setup untouched. Saving validates an
  existing file first, so a damaged or unknown-version file is preserved even if
  the user closes a shop without first pressing Load. Previous bytes also survive
  write, flush and replacement failures.
- Require the version, string title, item array, correctly typed/bounded numeric
  fields, boolean identification flag and exactly the declared zero-to-five random
  options. Reject embedded NULs and malformed text. Limit snapshots to the existing
  UI capacities (13 vending or 5 buying rows) and files to 1 MiB. Empty snapshots
  remain valid and can clear a prior setup. Text conversion now lives in shared
  `settings_json.c`, with unchanged account-file behavior.
- Restore only matching available items. Equipment must match every random-option
  field, including when all five option slots are used. Invalid counts are rejected.
  Skip missing/zero-index/empty/already-added candidates; retain only rows actually
  inserted, in insertion order, so saved prices and quantities align with the UI.
  The row limit counts successful inserts, and vending quantities cannot exceed
  the currently available stack.
- Removed merchant HTTP worker types, pool registrations, request producers,
  endpoint/token plumbing, callbacks, pending counters and URL state from the
  session. The obsolete web save module and its HTTP-save tests are gone; local
  storage tests replace that coverage. Removed the now-unused JSON boolean coercion
  accessor. At this checkpoint party recruitment and Twitter still required shared
  HTTP; START-041 subsequently moves recruitment to packets.

Validation:

- Full release build and final JSON-helper relink passed without warnings
  (`build/purist-local-shop-final-build.log`, `build/purist-local-shop-link.log`).
- `tests/build_local_shop.bat` exercises real JSON, encoders and temporary files,
  with controlled shop-list inputs: Korean/escaped titles, vending/buying isolation,
  account/character/address/port/world isolation, fresh-owner reload, empty state,
  type/range/encoding failures, invalid IDs, corrupt-file preservation, and injected
  write/flush/replace failures (`build/purist-local-shop-tests.log`). No HTTP linked.
- `tests/build_mcstore_backup.bat` covers staged allocation rollback, invalid and
  empty snapshots, missing and zero-index items, quantity clamping, and restored
  price order. `tests/build_inventory_match.bat` tests the actual matcher with
  mismatched option fields, five options, invalid counts and item/card/refinement/
  identification differences (`build/purist-local-shop-backup.log`,
  `build/purist-local-shop-match.log`).
- Account and character local storage, remaining HTTP services, JSON/shop codec,
  async work and game-mode layout passed (`build/purist-local-shop-account.log`,
  `build/purist-local-shop-character.log`, `build/purist-local-shop-services.log`,
  `build/purist-local-shop-json.log`, `build/purist-local-shop-async.log`,
  `build/purist-local-shop-mode.log`). The async suite now covers six request cases
  and 115 allocation failures; game-mode layout digest remains `cb530c74`.
- Source/link audit found no merchant HTTP jobs/endpoints or newly orphaned helpers
  (`build/purist-local-shop-reference-audit.log`). `git diff --check` passed.
  No live server or in-game shop UI smoke test was performed.

Foundation fixes: [merchant-restore.patch](upstream/merchant-restore.patch) carries
the independent matching and row-restoration corrections against `main`, without
local-file behavior. The option-matching assertion and restored-row assertion both
fail with their original implementations (`build/purist-shop-match-before.log`,
`build/purist-shop-restore-before.log`) and pass after correction. The patch passes
`git apply --check` against `main`. It has not been applied or committed there.
START-040 remains uncommitted on `purist`; no push or server modification was made.

## Guild emblem packet transport and foundation fix

Patrick approved keeping BMP and animated GIF emblems while replacing their HTTP
transport. He requested the generally useful cache fix as a small foundation
commit and chose a separate server patch instead of edits to either server checkout.

The reduced client now follows this path:

1. Actor/guild information supplies a guild ID and emblem version. A cache miss
   queues a request; `guild_emblem_request` sends `0x0b46` over the game connection.
   A rejected submission stays queued. Successfully submitted guild/version pairs
   retain the existing approximately 60-second suppression period.
2. The existing `0x0b36` receiver assembles clear/data/complete fragments, now capped
   at 51,200 bytes. It compresses the raw image into the versioned `.ebm` cache.
   Oversized or failed allocations cancel assembly without saving partial data.
3. `emblem_res.c` decompresses the cache. The existing bitmap/GIF loader recognizes
   the file signature and requires a 24x24 canvas. GIF palettes, transparency,
   interlacing, composition, disposal and animation updates remain.
4. The guild window reads its selected BMP/GIF once. `guild_emblem_upload` validates
   it through the actual image loader and sends `0x0153` with raw file bytes,
   bounded to 51,200 bytes. The old preview cast from a generic resource to
   `EmblemRes` is removed. The server remains responsible for upload permissions.
5. Emblem HTTP endpoints, manager ownership, startup/shutdown/login hooks, async
   jobs and payloads are removed. Their now-unused HTTP-status accessor and curl
   export binding are also removed. Other HTTP consumers remain.

The independent legacy `0x0152` receiver previously wrote a cache filename using
its raw version, while the reader adds `0x7fffffff`. For example, version 1 wrote
`world_7_1.ebm` but lookup requested `world_7_2147483648.ebm`. Both now call
`guild_emblem_name`. This fix and its regression test are committed locally on
`main` as **`7fe9620febaabe966c9ddf1356e4f5ab1b9bd144`**, using the approved
HampterDev identity. The same fix is present in the uncommitted purist work.
[emblem-cache-name.patch](upstream/emblem-cache-name.patch) exports that commit.
No push was made; the nine previously prepared foundation patches remain separate
and uncommitted to `main`.

The [separate Hercules patch and protocol notes](server/README.md) target pristine
Hercules `84da0df5ea655c8339430d2065dbe3b0b1a40577`. Both existing server checkouts
were left untouched. The patch adds modern raw BMP/GIF upload validation, retains
guild-master and gameplay restrictions, and serves existing compressed BMP database
entries as raw images. It also corrects an assembled-buffer handoff and an existing
MSVC-dependent BMP pixel-stride bug encountered while testing validation. Apply it
before using packet uploads; an unmodified server rejects those uploads.

Validation:

- Foundation regression test fails with the original writer and passes with the
  shared filename helper. It covers unsigned version boundaries and saved payloads
  (`build/foundation-emblem-cache-before.log`, `build/foundation-emblem-cache-tests.log`).
- Full client release rebuild passed, followed by successful changed-unit compile
  and relink after the download cap and HTTP cleanup
  (`build/purist-emblem-packets-build.log`, `build/purist-emblem-packets-final-link.log`).
- Actual BMP/GIF upload bytes, malformed/truncated input, every upload allocation
  failure, disconnected/rejected sends and queue retry/deduplication passed
  (`build/purist-emblem-transport-tests.log`). No HTTP code is linked into this test.
- Download ordering, guild/version matching, allocation failure, exact 50 KiB
  assembly, oversized cancellation and cleanup passed
  (`build/purist-emblem-download-tests.log`).
- Service ownership/submissions, worker pools, guild/quest owners, remaining HTTP
  and async stress checks passed (`build/purist-emblem-packets-checks.log`,
  `build/purist-emblem-async-tests.log`, `build/purist-emblem-remaining-http-tests.log`).
  Guild/quest digest remains `1210ed3a`; packet dispatch passed 1,179,800 cases with
  unchanged digest `e183046ac7a0f56f` (`build/purist-emblem-packets-dispatch.log`).
- Existing image tests passed 23 fixtures and 2,735 truncated prefixes. A separate
  isolated check passed a five-frame 24x24 GIF through the actual emblem loader and
  animation updates without HTTP (`build/purist-emblem-review-bitmap-tests.log`,
  `build/purist-emblem-review-no-http-tests.log`).
- Patched Hercules map/char servers compiled and linked in an isolated archive
  with `PACKETVER=20220406`. Modern/legacy upload-handler tests passed; patch
  application check passed. Details and remaining validation are in the server notes.
- Source/link audit found no retired emblem HTTP symbols. Shared HTTP still needs
  `libcurl.dll`. No live client/server transfer, persistence or animation smoke test
  has been performed.

Deferred reliability observations: an image that fails cache decoding becomes a
blank resource and is remembered in `bad_emblems`; this does not trigger a fresh
cache-miss download. The existing request timer's wrap behavior is also unchanged.
Neither issue is claimed fixed by moving transport.

## Roulette removal validation and foundation fix

- Full release rebuild passed without compiler or linker warnings
  (`build/purist-no-roulette-build.log`). No roulette symbols or source/object
  inputs remain in the executable map or build response files.
- Relevant object-type, window ownership/drawing, talk-option Lua/storage, session
  fields/packets, game-mode layout, RODEX and packet transport checks passed
  (`build/purist-no-roulette-checks.log`). The type registry now has 589 live and
  22 retired identities.
- Packet dispatch passed 1,179,800 cases with digest `e183046ac7a0f56f`
  (`build/purist-no-roulette-dispatch.log`). The expected trace was derived from
  the pre-removal trace by changing only the six retired incoming packet IDs to
  the existing unknown-packet behavior: 72 cases changed. The actual and expected
  trace files match byte for byte.
- Exactly 11 length rows and 11 names were removed. All 1,380 surviving length
  rows and all surviving names retain their original values and order. Source
  comparison found 855 byte-identical surviving functions across changed source
  and test files (`build/purist-no-roulette-source-audit.log`).
- The reference audit found no newly orphaned helper; its remaining candidates
  have live callers (`build/purist-no-roulette-reference-audit.log`). Application
  sources contain no roulette identifiers or exclusive aliases. Numeric IDs used
  by unrelated mail messages, skills and mode commands remain. `git diff --check`
  passed. No live client/server smoke test has been performed.

The standalone [roulette-prize-order.patch](upstream/roulette-prize-order.patch)
fixes the foundation's prize-tree comparator. Previously it required both row and
position to be smaller, which makes key equivalence nontransitive. The correction
compares row first and position within equal rows, preserving the existing signed
comparison. This is independent of removing roulette from the purist branch.

An isolated C harness compiled the actual original and corrected comparators
alongside the real tree implementation. It reproduced the original ordering-law
violation, checked the correction across signed boundaries and exercised 42
separate test keys with forward, reverse and shuffled insertion, lookup and
removal of duplicate keys (`build/purist-roulette-foundation-tests.log`). All nine
foundation patches together pass `git apply --check` against the starting sources.
They have not been applied to `main` or pushed.

## Local map-table removal validation and foundation fix

- The two changed source units compiled and the release executable linked
  successfully, without warnings or errors (`build/purist-no-map-table-build.log`).
- Map-property ownership, flag reads/writes, registry records, retained actions
  and cleanup checks passed with digest `76c1ce8f`
  (`build/purist-no-map-table-tests.log`).
- Source comparison confirms only the loader, its startup call/declaration and
  exclusive include were removed. All surviving functions in `session_init.c`
  are byte-identical. No loader symbol remains in the application sources or link
  map (`build/purist-no-map-table-source-audit.log`). `git diff --check` passed.
- No live client/server smoke test has been performed. The server must supply
  `0x099b` map flags; older map-type messages alone do not replace the removed data.

The standalone [map-info-table-bounds.patch](upstream/map-info-table-bounds.patch)
retains the foundation's local-table loading and missing-file behavior. It bounds
comment/token scans, removes the fixed token buffer, accepts a final integer
without trailing whitespace, checks integer range and stops at malformed tokens
or unfinished block comments. Failed reads are cleaned up without parsing.

An isolated harness compiled the actual original and corrected loader functions
with file/map boundary stubs and a protected page immediately after the input
allocation. The original loader faults on a trailing `//` comment. The correction
passed guarded empty/truncated input, every-byte truncation of a mixed fixture,
CRLF/LF tables, comments, EOF tokens, signed limits, 300-byte tokens, the 30-by-27
limit, missing-file behavior and read-failure cleanup
(`build/purist-map-table-foundation-tests.log`). The complete patched source unit
also compiled (`build/purist-map-table-foundation-compile.log`). All eight
foundation patches together pass `git apply --check` against the starting commit.
They have not been applied to `main`.

## Clan removal validation

- Release rebuild passed without compiler/linker warnings or errors
  (`build/purist-no-clans-build.log`). Source/object lists and the final link map
  contain no clan implementation. `git diff --check` passed.
- Packet dispatch passed 1,179,800 cases with digest `f9651e3d215d4f37`.
  Its complete trace equals the preceding trace with only the four incoming clan
  IDs changed to the existing unknown-packet behavior. All 1,391 remaining packet
  length/minimum-size rows match the preceding source in the same order; exactly
  five clan rows were removed (`build/purist-no-clans-dispatch.log`,
  `build/purist-no-clans-dispatch-expectation.log`,
  `build/purist-no-clans-source-audit.log`).
- Guild/quest ownership retains digest `1210ed3a`. Guild-state/request, packet
  transport, name-card ownership, actor-action, game-layout, window-cleanup,
  cooldown and chat-settings/Lua round-trip checks passed
  (`build/purist-no-clans-checks.log`, `build/purist-no-clans-neighbors.log`).
  Type checks cover 591 live classes and 20 retired identities.
- Source comparison confirms 377 surviving functions in changed files are
  byte-identical. The moved guild-title handler is unchanged; the dressroom
  handler only inlines its existing window ID. All 21 retained chat-filter
  labels still map to their original settings IDs for reads and writes.
  The reference audit found no newly orphaned helpers
  (`build/purist-no-clans-reference-audit.log`).
- No live client/server or game-skin visual smoke test has been performed.

## Normal quest-table loading validation

- The production loading function registers both ongoing-quest callbacks and loads
  the normal `_True` table plus the existing support/saved tracking files across
  24 server-type values. Journal skin-layout checks also passed
  (`build/purist-normal-quest-table-tests.log`).
- The changed source unit compiled and the release executable linked successfully,
  with no warnings or errors (`build/purist-normal-quest-table-build.log`).
- Source comparison confirms only the table-selection branch and its exclusive
  include/constant changed. No alternate quest-table path remains in application
  or test sources. `git diff --check` passed. No live client/server smoke test
  has been performed.

## Recommended-quest removal validation and foundation fix

- Ongoing quest values, deep copies, growth/shrink and repeat cleanup passed. Packet
  checks passed for both list/add formats, actual spans, every-byte truncation,
  bounded hunt counts/names, 32/64-bit updates and active flags. Deadline checks
  passed 78 comparisons across three time zones, including DST and 2038 boundaries
  (`build/purist-no-recommended-quests-packets.log`).
- Guild/quest ownership checks passed. The full trace matches the preceding trace
  byte for byte after removing exactly 150 catalog-only records
  (`build/purist-no-recommended-quests-owners.log`,
  `build/purist-no-recommended-quests-source-audit.log`).
- New journal-layout checks verify all pixels of the three retained skin layouts,
  including transparent pixels and clearing the unused fourth tab area. Loading
  checks retain the two ongoing callbacks, descriptions and saved tracking settings
  across 24 server-type values. The new checks are part of `tests/build.bat`
  (`build/purist-no-recommended-quests-journal.log`).
- Object-type checks passed for 593 live classes and 18 retired identities. The
  removed pane remains only in historical fixtures and its rejection check
  (`build/purist-no-recommended-quests-types.log`). The reference audit found no
  newly orphaned helpers; surviving single-caller helpers remain in use.
- Release rebuild passed (`build/purist-no-recommended-quests-build.log`) with no
  compiler/linker warnings. Source and link-map audits contain no recommended-quest
  implementation. `git diff --check` passed. No live client/server or game-skin
  visual smoke test has been performed.

The journal's lookup-by-ID loop previously searched only Active and Recommended,
so an existing quest in Paused or Other could produce a missing-quest notice. It
now searches all retained tabs. The independent
[quest-journal-tab-lookup.patch](upstream/quest-journal-tab-lookup.patch) makes the
same correction in the foundation while retaining all four original tabs. It
passes `git apply --check` against the original source alongside the six earlier
foundation patches. An isolated regression
harness compiled the actual lookup function from each source: the baseline missed
two tabs, the foundation correction found all four, and purist found all three
(`build/purist-quest-tab-lookup.log`). The patch has not been applied to `main`.

## CAPTCHA registration/upload removal validation

- CAPTCHA checks passed for seven retained packet messages, truncation/chunk
  boundaries, image validation and decompression, corrupt-image rollback, binary
  keys, window allocation failures, acknowledgments and answer submission
  (`build/purist-no-captcha-upload-captcha.log`). Tests construct compressed server
  fixtures directly with zlib; production upload compression is gone.
- Scheduler ownership, allocation failures, clocks, heartbeat/timeouts and ranking
  checks passed (`build/purist-no-captcha-upload-scheduler.log`).
- Object-type checks passed for 594 live classes and 17 retired identities. The
  historical registration-window identity is rejected. Window-owner cleanup also
  passed (`build/purist-no-captcha-upload-types.log`,
  `build/purist-no-captcha-upload-window-ownership.log`).
- All 1,179,800 dispatch cases passed with digest `8c7c59de612c089f`. The expected
  trace was independently derived from the preceding trace by changing only the
  two retired upload replies to unknown-packet handling. The complete actual trace
  matches that expectation byte for byte. All 1,396 remaining packet length rows
  retain their IDs, lengths, minimum sizes and order
  (`build/purist-no-captcha-upload-dispatch-expectation.log`,
  `build/purist-no-captcha-upload-dispatch.log`,
  `build/purist-no-captcha-upload-source-audit.log`).
- Release rebuild passed (`build/purist-no-captcha-upload-build.log`). Source and
  link audits found no remaining registration/upload code or orphaned helpers.
  Historical type fixtures retain the removed class name. `git diff --check`
  passed. No live client/server smoke test has been performed.

## Shutdown countdown removal validation

- Scheduler checks passed for the remaining queue ownership, callback changes,
  allocation failures, clocks, heartbeat/timeouts, CAPTCHA upload and ranking jobs
  (`build/purist-no-shutdown-notices-scheduler.log`).
- Login OTP checks passed, including exact packet bytes, bounded input, wait ordering
  and 5,000 generated cases (`build/purist-no-shutdown-notices-login.log`).
- The login dispatcher differs from its pre-removal source only by the two retired
  shutdown cases and two unused includes. All 1,400 remaining packet length rows
  preserve ID, length, minimum size and order
  (`build/purist-no-shutdown-notices-source-audit.log`).
- Release rebuild passed (`build/purist-no-shutdown-notices-build.log`) without
  compiler/linker warnings. Source/link audits and `git diff --check` passed.
  No live client/server smoke test has been performed.

## Empty GDI job removal validation and foundation cleanup

- Scheduler checks passed for queue ownership, callback changes, clocks, allocation
  failures, heartbeat packet bytes and timeout handling, captcha chunks, ranking
  display and shutdown notices (`build/purist-no-gdi-job-tests.log`).
- Both changed implementation units compiled and the release executable linked
  without compiler or linker warnings (`build/purist-no-gdi-job-build.log`). Source,
  tests and the final link map contain no GDI-job identifiers or random schedule
  mode. `git diff --check` passed. No live client/server smoke test was performed.
- The foundation has the same empty job. The standalone
  [empty-gdi-schedule.patch](upstream/empty-gdi-schedule.patch) contains this cleanup
  and its test adjustments without purist feature removals. All six foundation
  patches together pass `git apply --check` against the starting commit's sources.
  They have not been applied to `main`.

## Entry queue removal validation

- Captured the pre-removal dispatch trace with digest `cdc29a8d9e368063`. Replaced
  only the eight retired incoming queue IDs with the original unknown-packet path
  (96 cases across both dispatchers and six payload patterns). Post-removal dispatch
  passed all 1,179,800 cases with digest `bf389cf447fe62e7`, and the complete trace
  matches that independent expectation by SHA-256
  (`build/purist-no-entry-queue-dispatch.log`). This includes unchanged battleground
  dispatch and the other supported packet paths.
- All 1,402 remaining packet length rows retain their IDs, lengths, minimum sizes
  and order. Exactly 13 queue protocol rows and their names were removed.
- Object-type checks passed for 595 remaining classes and 16 retired identities,
  including all five queue UI types. The surviving 17 type queries passed 3,695
  comparisons against the original runtime; fixture source masks were projected
  onto those surviving queries (`build/purist-no-entry-queue-types.log`).
- Window cleanup, game-mode layout, session fields, talk-option storage/Lua and
  Lua loading/boundary checks passed (`build/purist-no-entry-queue-windows.log`,
  `build/purist-no-entry-queue-layout.log`, `build/purist-no-entry-queue-fields.log`,
  `build/purist-no-entry-queue-options.log`, `build/purist-no-entry-queue-lua.log`).
  The former queue-icon option is now checked as unbound. Historical queue class
  names remain only in reference fixtures and assertions of their removal.
- Release rebuild and final helper cleanup compilation/linkage passed without
  compiler or linker warnings (`build/purist-no-entry-queue-build.log`,
  `build/purist-no-entry-queue-final-link.log`). Source and link-map audits contain
  no queue symbols or removed helpers. Ordinary battleground handlers remain linked.
  `git diff --check` passed. No live game-data/server smoke test has been performed.

## Reassembly authentication removal validation

- Captured the current pre-removal dispatch trace with verified digest
  `f3d4ace16b87c15f`. Derived the expected result by replacing only the 84 retired
  challenge IDs with the original unknown-packet dispatch path: 1,008 cases across
  both dispatchers and six payload patterns; all other traces remain unchanged.
- Post-removal dispatch passed all 1,179,800 cases with digest `cdc29a8d9e368063`
  (`build/purist-no-reassembly-auth-dispatch.log`). The complete output trace also
  matches the independently transformed baseline by SHA-256. The dispatcher test
  explicitly checks that these IDs take the unknown-packet path.
- All 1,415 remaining packet length rows retain their ID, size, minimum size and
  order; exactly 168 challenge/reply rows were removed
  (`build/purist-no-reassembly-auth-table.log`).
- Existing packet/queue checks passed, including receive framing, partial reads,
  invalid sends, immutable payloads, 196,608 packet-ID/send-mode cases and 50,000
  mixed queue operations (`build/purist-no-reassembly-auth-packets.log`).
- Release rebuild passed without compiler or linker warnings
  (`build/purist-no-reassembly-auth-build.log`). Source/tests and the final link map
  contain no retired authentication handlers or reply helpers. `git diff --check`
  passed. No live client/server smoke test has been performed.

## Packet-key removal validation

- The original packet test supplied an artificial nonzero key. Before removing the
  manager, the updated checks ran against the actual production key implementation
  and confirmed unchanged packet bytes for all 65,536 IDs across immediate, held
  and disconnected sends: 196,608 cases (`build/purist-packet-keys-baseline.log`).
- The same cases passed after removing the manager. Receive framing, partial
  reads, immutable inputs, invalid-send rejection and the byte queue's 50,000 mixed
  operations also passed (`build/purist-no-packet-keys-tests.log`).
- Login OTP checks passed (`build/purist-no-packet-keys-login.log`). Packet dispatch
  passed 1,179,800 cases with the unchanged digest `f3d4ace16b87c15f`
  (`build/purist-no-packet-keys-dispatch.log`).
- Release rebuild passed without compiler or linker warnings
  (`build/purist-no-packet-keys-build.log`). The source audit and final link map have
  no remaining packet-key manager symbols. `git diff --check` passed. No live
  client/server smoke test has been performed.

## Foundation cleanup found during process-scan removal

The published foundation already ignores both scanners' results, so removing that
startup work also fits its existing behavior. The standalone
[unused-startup-process-scan.patch](upstream/unused-startup-process-scan.patch)
removes the scanners, OS selector and unused process state while preserving the
foundation's other startup features, including its separate helper-file cleanup.
All five foundation patches together pass `git apply --check` against the starting
commit. They have not been applied to `main`.

## Optional startup settings validation

- Focused tests passed for missing/inaccessible keys, saved settings, defaults for
  each failed or malformed value, safe string termination, independent buffer sizes
  and closing only opened handles (`build/purist-no-setup-settings-tests.log`).
  The tests use simulated registry access and do not change the host's settings.
- Existing talk-option storage and Lua checks passed
  (`build/purist-no-setup-options-tests.log`).
- Release rebuild passed without compiler or linker warnings
  (`build/purist-no-setup-build.log`). The link map contains the new settings reader
  and no retired setup launcher/reader. `git diff --check` passed. No client/server
  runtime smoke test has been performed.

## Foundation fixes found during setup removal

The original reader closed `probe` even when opening its registry key failed, and
reused the previous value's output size for subsequent DWORD reads. The standalone
[startup-registry-reads.patch](upstream/startup-registry-reads.patch) fixes those
handle and buffer-size errors while retaining the foundation's setup requirement.
It passes `git apply --check` against the original foundation sources alongside
the other three patches. It has not been applied to `main`.

## GameGuard removal validation

- Release rebuild passed (`build/purist-no-gameguard-build.log`). The final
  guard-packet and helper-file cleanup recompiled and linked successfully
  (`build/purist-no-gameguard-final-link.log`). Subsequent AI-updater and RagPath
  removals also recompiled and linked successfully, as recorded above.
- Full remaining behavioral suite passed (`build/purist-no-gameguard-tests.log`),
  including GRF ownership, file loading, Lua data owners, session fields and packet
  dispatch. No game-data/server runtime smoke test has been performed.
- Packet dispatch passed 1,179,800 cases with digest `f3d4ace16b87c15f`. The
  expectation was derived from the pre-removal trace, replacing only packets
  `0x0227` and `0x09cf` with the existing unknown-packet path. Other traces are
  unchanged (`build/purist-no-gameguard-dispatch-expectation.log`).
- All 1,583 remaining packet length rows match the foundation's ID, size,
  minimum size and order. Exactly eight GameGuard rows were removed
  (`build/purist-no-gameguard-packet-table-audit.log`).
- Application sources, tests and vendor code have no remaining GameGuard/GGHash
  hooks or identifiers. The link map has no guard manager or Lua-hook symbols.
  The reference audit found no helpers left unused by this removal
  (`build/purist-no-gameguard-reference-audit.log`). `git diff --check` passed.

## Foundation fix found during replay removal

Startup searched for an empty pathname when checking `Navigationdata`, then tested
the handle for `Replay` instead of the navigation handle. The reduced client now
checks `Navigationdata` and uses its own handle. The same two-line correction is
available independently in [navigation-directory.patch](upstream/navigation-directory.patch).
It applies to the starting foundation commit without any purist removals;
`git apply --check` passed against that original source. This patch has not been applied to `main`.

## Foundation fixes found during launcher removal

- The foundation README promises that login uses the configured address, but
  server type 13 still replaced it with hardcoded US/Thai addresses. The standalone
  [configured-login-address.patch](upstream/configured-login-address.patch) removes
  those overrides and their `1roc1` selector from the foundation.
- `app_run()` used its command text as a formatting string in the normal path.
  [launch-command-format.patch](upstream/launch-command-format.patch) copies it
  through `%s`, preserving literal percent characters. The reduced client initially
  included that correction; START-010 later removed the helper with its last caller.
- Both patches, together with the navigation correction, pass `git apply --check`
  against the original foundation sources. They have not been applied to `main`.

## Direct configuration startup validation

- Login OTP checks, talk-option Lua checks (including neighboring session
  fields), game-mode layout and packet dispatch passed
  (`build/purist-direct-clientinfo-tests.log`). Packet dispatch retains the verified
  1,179,800-case digest `6ada076359d113ff`.
- Application sources and tests have no remaining launcher mode strings, launcher
  argument fields, alternate-config flag, command-line PC-cafe flag or launcher-page flag.
- Release rebuild passed (`build/purist-direct-clientinfo-build.log`), with no compiler or linker warnings. `git diff --check` passed.
- No game-data/server runtime smoke test has been performed.

## Replay validation

- Release build passed (`build/purist-no-replay-clean-build.log`). The final
  packet-table cleanup was recompiled and linked successfully
  (`build/purist-final-link.log`); all 1,591 packet ID, size and minimum-size tuples
  remain identical to the foundation, in the same order. Only recording flags changed.
- Full remaining behavioral suite passed (`build/purist-no-replay-final-tests.log`).
- Focused checks after helper cleanup passed for packet framing/sending, game context,
  path storage and real grid searches, navigation, and name-card ownership
  (`build/purist-replay-cleanup-tests.log`).
- Packet dispatch passed 1,179,800 cases. Its expected digest was derived from the
  pre-removal trace, replacing only the retired synthetic packet's cases with the
  existing unknown-packet behavior. Ordinary packet traces remain unchanged.
- Reference audit found no remaining functions whose only production callers were
  removed with replay. The surviving two-reference functions are ordinary static
  helpers with live callers (`build/purist-replay-reference-audit.log`).
- Executable dependencies contain no Bink import; the link map retains crash-report
  installation. `git diff --check` passed.
- No game-data/server runtime smoke test has been performed.

## Adventurer's Agency packets (START-041)

Patrick identified this as the Adventurer's Agency UI and chose to keep it, move
its transport onto the game connection, and leave the server implementation as a
separate patch like the emblem work.

- `src/contents/adventurer_agency.c` owns bounded binary requests, validated replies,
  cached listings, per-operation request serials and ten-second timeouts. The
  game connection adapter is separate from `src/ui/social/agency_results.c`.
- Custom extension `0x0bf0`/`0x0bf1` carries list/search, own ad, detail, registration
  and deletion. Existing application and invitation packets remain. This requires
  the matching server implementation; these are not stock Hercules packets.
- Removed Agency HTTP endpoints, JSON decoder, seven worker registrations, request
  producers/callbacks and the shutdown wait for HTTP recruitment jobs. Twitter was
  then the only HTTP worker; START-042 subsequently removed it and shared curl support.
- Results cannot reopen a closed board or apply through a closed/stale application
  window. Failed/time-out list requests restore the last successful page and its
  controls. Empty lists become page 1 of 1. Disconnect/login/game exit reset state.
- The server owns advertiser identity and authorizes registration/deletion against
  party leadership. The client no longer tries deleting another account's listing
  when an existing join response reports a stale leader.
- `docs/server/hercules-agency-packets.patch` is independent of the emblem patch.
  It uses in-memory party records in one map-server process, with authoritative
  ownership, pagination, filtering and per-operation throttles. Logout, leader
  changes, missing leaders and full parties remove listings. Multi-process shared
  discovery and restart persistence are not implemented. See
  [protocol and deployment notes](server/agency-packets.md).

Two existing board defects were also fixed: the maximum-level input was guarded
by the minimum-level field's emptiness, and the Next button was not consistently
disabled on the last page. `docs/upstream/agency-board.patch` contains just those
two changes and passes `git apply --check` on `main`. It is prepared for review;
no additional foundation commit or push was made.

Validation:

- Complete release build, then final build after cleanup and the board fixes.
- Client packet ownership/validation tests, including all truncated reply prefixes,
  malformed fields, allocation failure, stale/duplicate replies, cancellation,
  clock wraparound and disconnect/reset.
- UI completion tests for closed/stale windows, failed list recovery, own-ad state,
  level/party membership checks and the existing join operation.
- Actual patched Hercules request handler compiled in a fixture; its real output
  is decoded by the client test. Ownership, filters, paging, malformed requests,
  throttling and stale/full advertisements are covered.
- Isolated Win32 Release Hercules map-server builds passed for Agency alone and
  combined with emblems. Both patch application orders passed checks against the
  baseline source and produced identical combined files.
- All 1,179,800 dispatcher traces were compared against the pre-change executable;
  only the 12 cases for the new reply changed (direct and forwarded dispatch).
- Remaining HTTP services, async submissions/queue, JSON and game-mode layout
  checks passed. Game-mode layout digest remains `cb530c74`.

The original state is backed up under `build/before-agency-packets`; validation
logs are `build/purist-agency-*.log`. No live account/server/database was used.
The purist edits remain uncommitted, and both existing server checkouts are
untouched. Live two-client validation is still required before deployment.


## Twitter and unused HTTP removal (START-042)

Patrick approved removing Twitter integration. With the earlier packet transports
and local settings in place, Twitter was the final consumer of the curl worker
stack, so that unused stack is removed too.

- Removed the composer, authorization controls, screenshot crop selector, special
  edit/scroll controls, menu entry, slash commands and keyboard action.
- Removed OAuth URL handling, stored authorization flags/web tokens, Twitter data
  and uploads. Keyed login packet fields still occupy their original wire bytes;
  server-list parsing retains its existing offsets.
- Removed curl initialization/shutdown, HTTP POST/form handling, background worker
  queues and pools, service endpoint discovery and vendored curl headers. The
  executable no longer requires `libcurl.dll` at startup.
- Kept full screenshots, JSON/local settings, packet-based emblems and Agency,
  normal name-card lookups and the generic embedded browser pending START-043.
- Removed tests solely for retired HTTP workers. Kept generic owned text/booking
  coverage in `tests/test_owned_booking.c`, and retained screenshot, Lua, browser
  and retired UI identity checks.

Validation:

- Full release build passed without compiler/linker warnings:
  `build/purist-twitter-final-build.log`.
- Focused checks passed for browser navigation/ownership, screenshot pixels and
  failure cleanup, UI extensions, Lua boundaries, owned text/booking, local
  account/character/shop settings, emblem transport/cache, and Agency packets/UI.
- Name-card requests/expiry and ownership checks passed. Game-mode layout retains
  digest `cb530c74`; all 1,179,800 packet-dispatch cases retain digest
  `66be6cec6121c903`.
- Source and link-map checks found no Twitter, HTTP worker or curl integration.
  Historical type fixtures keep retired names solely for rejection checks.
  `git diff --check` passed.

The pre-removal tree is backed up in `build/before-twitter-removal`; focused test
logs are `build/purist-twitter-*-tests.log`. No live game/server smoke test was run.
Purist edits remain uncommitted; `main` and both existing server checkouts are
unchanged by this removal.


## Embedded browser removal (START-043)

Patrick approved removing the optional embedded Internet Explorer browser.

- Removed `/web` parsing, navigation and address-bar actions, COM browser hosting,
  popup handling, browser window procedures and URL conversion helpers.
- Removed the window manager's browser registry, active-browser pointers, deferred
  deletion queue, per-frame updates, input capture and teardown paths. Fullscreen
  activation no longer exempts browser windows from normal focus handling.
- Moved the unrelated shared text-drawing code from `src/browser/browser_text.c`
  to `src/ui/controls/window_text_styles.c`; all twelve function bodies are
  unchanged. Their declarations now live with ordinary UI drawing declarations.
- Removed the browser-only test suite and unused `wininet.lib` build input.
  General Windows COM initialization and external-page launch behavior remain
  separate from this embedded-browser decision.

Validation:

- Focused UI extensions, window-reference cleanup, chat-tab layout/persistence,
  text rendering, drag/drop, talk-option and named-client-dispatch tests passed.
- Shared text helpers were compared against the pre-removal source: all twelve
  function bodies match exactly.
- Full release build passed without compiler/linker warnings. Source, test and
  tool scans and the link map contain no embedded-browser references; the shared
  text-drawing symbols remain. `git diff --check` passed. See
  `build/purist-no-browser-build.log` and `build/purist-no-browser-audit.log`.

The previous tree is backed up under `build/before-browser-removal`; test/build
logs use `build/purist-no-browser-*.log`. No live game/server smoke test was run.
Edits remain uncommitted on `purist`; this change does not modify `main` or either
existing server checkout.


## Local map signboard removal (START-045)

Patrick approved removing the optional map decoration/annotation system.

- Removed the `SignBoardList_F` and `SignBoardList` Lua imports, owned data table,
  startup/map-entry placement and shutdown cleanup.
- Removed picture/text signboard windows, world-position projection, animated
  signboard icons, operation tables and declarations. Removed the picture
  window's resize exception.
- Removed the dedicated data-loader test. Historical type fixtures retain the old
  names so tests verify that both retired UI identities are rejected.
- Shared actor rendering, map conversion, NPCs, quest tracking and town markers
  remain. The callee audit found no newly orphaned production helpers.

Validation:

- Full release build passed without compiler/linker warnings.
- Type-query checks passed for 582 remaining classes and 29 retired identities.
  Quest journal/loading, town markers, actor rendering and Lua boundary checks
  passed; town-info digest remains `762fe954`.
- No signboard references remain in production source or the executable's link
  map. `git diff --check` passed.

The pre-removal tree is in `build/before-signboard-removal`; logs are
`build/purist-no-signboards-*.log`, including the final reference audit. No live
client/server smoke test was run. Purist changes remain uncommitted; `main` and
both existing server checkouts are unchanged by this removal.


## Reputation removal (START-046)

Patrick approved removing the optional reputation progression display.

- Removed reputation BSON loading, group/name/limit definitions, score ownership,
  startup/shutdown hooks, window creation and drawing, search/filter/progress
  controls, change notifications, and the menu/hotkey entry.
- Removed both incoming packet definitions, dispatch handlers and packet-table
  entries: `0x0b8d` and `0x0b8e`. Servers must stop sending these messages: they
  now take the existing unknown-packet path, which resets connection queues.
  No server checkout was modified.
- Removed newly orphaned array-loader, integer-parsing and list/combo text-search
  helpers after checking their callers. Shared BSON, JSON, string and list
  machinery remains. Historical type fixtures retain the retired window name
  solely to verify that its identity is rejected.

Validation:

- Full release build passed without compiler/linker warnings. The final helper
  cleanup was recompiled with release flags and the complete executable relinked
  successfully. Logs: `build/purist-no-reputation-build.log` and
  `build/purist-no-reputation-final-link.log`.
- All 1,179,800 packet-dispatch cases passed, digest `85e0a8e98fba8ebf`. An
  independently derived trace replaces only the 24 cases for the two retired
  packets with the existing unknown-packet result; its SHA-256 matches the new
  executable's trace. All 1,380 retained packet-table rows are unchanged.
- Content-manager tests passed, digest `2fed0ead`; the trace matches the previous
  trace after removing only reputation records and its cleanup owner. Type
  checks passed for 581 classes and 30 retired identities.
- JSON/local-storage, UI tabs, strings, list controls and retained BSON effect
  loading passed. The named STR effect digest remains `817beee3`.
- Source and final link-map checks found no reputation code or newly retired
  helper references. `git diff --check` passed.

The pre-removal tree is in `build/before-reputation-removal`; focused logs use
`build/purist-no-reputation-*.log`. No live client/server smoke test was run.
Changes remain uncommitted on `purist`; `main` and both existing server checkouts
are unchanged by this removal.


## Item reforming removal (START-048)

Patrick approved removal after seeing an official iRO reference screenshot of the
equipment-conversion window.

- Removed `ItemReformSystem.lub` loading, recipe/material ownership, manager
  startup/shutdown, inventory eligibility filtering, result previews, five UI
  types and the window's creation/blocking hooks. Seven source/header files were
  deleted; the caller audit found no newly orphaned production helpers.
- Removed the incoming open/result packets `0x0b8f`/`0x0b92` and outgoing
  close/request packets `0x0b90`/`0x0b91`, including dispatch and packet-table
  entries. Servers must stop sending the two incoming messages: they now use the
  existing unknown-packet path, which resets connection queues.
- Kept the approved item-name lookup, ordinary refining, enchanting pending its
  own review, and effects still used by grade enchanting and other systems.
- Kept historical type fixtures to verify that all five retired UI identities
  are rejected. Remaining manager tests retain real Lua loading, failure cases
  and ownership/cleanup checks.

Foundation correction: the constants `MSG_ITEM_REFORM_RESULT` and
`CHAT_CHANNEL_ITEM_REFORM` in `src/game/system_messages.c` were misleading names.
Message-table entry `0x0b92` is a blocked-call notice, verified in both local
`client-files/data/msgstringtable.txt` and
`ROenglishRE/Ragnarok/data/msgstringtable.txt` resources. It is unrelated to the
packet with the same number. Renamed the constants to `MSG_CALL_BLOCKED` and
`CHAT_CHANNEL_CALL_NOTICE`, preserving all behavior. The separate
[`call-message-names.patch`](upstream/call-message-names.patch) passes
`git apply --check` against the foundation worktree; it has not been applied or
committed there.

Validation:

- Full release build passed without compiler/linker warnings:
  `build/purist-no-item-reform-build.log`.
- All 1,179,800 packet-dispatch cases passed, digest `ce7bc0b0b2f9a4d7`. The
  actual trace matches an independently derived baseline with only the 24 cases
  for the two retired incoming packets changed to unknown-packet handling.
  All 1,376 retained packet-table rows are unchanged.
- Remaining content-manager traces exactly match the baseline after removing
  reform records and its cleanup owner; digest `8b8d87ec`. Type checks passed for
  576 classes and 35 retired identities, including the five reform window types.
- All twelve item-lookup and ordinary-refining implementation/header files are
  byte-identical to the pre-removal tree. No reform references remain in
  production source or the final link map. The generic chat file differs only in
  the two constant names. `git diff --check` passed.

The pre-removal tree is in `build/before-item-reform-removal`; focused logs and
the audit use `build/purist-no-item-reform-*.log`. No live client/server smoke test
was run. Changes remain uncommitted on `purist`; `main` and both existing server
checkouts are unchanged by this removal.


## Dedicated enchanting window removal (START-049)

Patrick approved removal after seeing an official iRO reference screenshot.

- Removed the two `EnchantList` Lua files' loading/registration, catalog ownership,
  eligible-item and offer lists, slot/material/zeny displays, all four operation
  tabs, nine UI identities and startup/shutdown hooks. Thirteen source/header
  files were deleted.
- Removed the window's creation path and checks that blocked mail, banking or
  other item operations while it was open. Refining and grade-enchanting blockers
  remain.
- Removed incoming `0x0b9a`/`0x0b9f` and outgoing `0x0b9b` through `0x0b9e` plus
  `0x0ba0`. Despite its generic `UI_OPEN_V3` name, the `0x0b9a` handler supported
  only this enchanting window. Servers must stop sending the two incoming
  messages: they now take the existing unknown-packet path, which resets
  connection queues.
- Removed five newly unused helpers: three Lua global/registration wrappers,
  `session_inventory_count` and `tree_less_uint64`. Shared Lua loading, inventory
  access and trees remain. Lua tests now install their observation callback
  directly and retain guarded loading, stack, error and allocation-failure checks.
- Kept item enchant representation/display, ordinary refining, grade enchanting,
  NPC dialogs and the shared effect catalog. Historical UI fixtures remain for
  rejection checks. The item-name lookup stays under START-047's keep decision;
  its last consumer is now gone, so START-050 asks whether to remove it.

Validation:

- Full release build passed without compiler/linker warnings. After the final
  comparator cleanup, `tree.c` was recompiled with release flags and the complete
  executable relinked successfully. Logs:
  `build/purist-no-enchant-window-build.log` and
  `build/purist-no-enchant-window-final-link.log`.
- All 1,179,800 packet-dispatch cases passed, digest `0d544dd9bfa2d4e7`. The actual
  trace matches an independently derived baseline with only 24 cases for the two
  retired incoming messages changed. All 1,369 retained packet-table rows match.
- Remaining-manager loading/failure/cleanup tests passed, digest `3a78f56e`;
  their trace exactly matches the baseline with only enchanting records removed.
  Type checks passed for 567 classes and 44 retired identities, including all
  nine enchanting UI types.
- Lua calls, script loading and allocation failures, skill/container values,
  shared containers and UI drag/drop checks passed. Thirty-two item-information,
  refining, grade-enchanting and item-lookup files match the backup byte for byte.
- No retired UI/catalog/handler/helper symbols remain in production source or
  the final link map. Lookup startup remains; the unused lookup function is
  eliminated by the release linker, while its source/tests stay pending review.
  `git diff --check` passed.

The pre-removal tree is in `build/before-enchant-window-removal`; focused logs and
the audit use `build/purist-no-enchant-window-*.log`. No live client/server smoke
test was run. Changes remain uncommitted on `purist`; `main` and both existing
server checkouts are unchanged by this removal.


## Unused item-name lookup removal (START-050)

Patrick approved removing the lookup after its only two consumers, item reforming
and the dedicated enchanting catalog, were removed. This supersedes START-047's
earlier decision to retain it while reviewing those consumers.

- Removed the lookup manager, `ItemDBNameTbl.lub` loading, `ItemDB_To_ItemID`
  queries, singleton ownership and startup/shutdown calls.
- Removed the separate `LuaScript` wrapper because this lookup was its last user,
  along with three unused includes and the wrapper-only test. Five source/header
  or test files were deleted. Normal `ClientLua` and item-information loading
  remain unchanged.
- Adapted the existing Lua and manager tests to remove only the retired lookup
  cases. No packet messages, UI identities or retained structure layouts changed.

Validation:

- All five changed retained production units compiled with release flags, and the
  complete release executable relinked successfully. Build input lists were
  regenerated to exclude both deleted implementation objects. This was an
  incremental rebuild after START-049's full release build.
- Lua calls passed with diagnostics enabled and disabled. Lua boundary checks
  passed, including 419 initialization and 40 query/load allocation failures.
- Remaining-manager loading/failure/cleanup tests passed, digest `f0fa1aba`.
  The trace matches an independently derived pre-removal trace with only the
  three lookup-owner cleanup records removed.
- Named client dispatch checks passed for cloud settings/restart, effect callback
  registration and quad order.
- No retired lookup/wrapper references remain in source/tests, build inputs or the
  final link map. All retained production files outside the five changed units
  match the backup byte for byte, including normal ClientLua, item information
  and packet handling. `git diff --check` passed.

The pre-removal tree is in `build/before-item-lookup-removal`; focused logs and the
audit use `build/purist-no-item-lookup-*.log`. No live client/server smoke test was
run. Changes remain uncommitted on `purist`; `main` and both existing server
checkouts are unchanged by this removal.


## Selectable item-package removal (START-052)

Patrick approved removal after viewing the selection-box demonstration in
[Gravity's developer note](https://ro.gnjoy.com/news/devnote/View.asp?category=3&curpage=2&seq=4139188).

- Removed `selectpackageitem` catalog loading, package/bundle/item ownership and
  startup/shutdown hooks, the reward-choice window and its unused package-name
  list type. Four source/header files were deleted.
- Removed the inventory-use interception that opened the chooser. Item use now
  sends the ordinary `0x0439` request after the existing checks. The server decides
  whether to award anything or open a scripted choice; this client-only change
  does not provide replacement server scripts.
- Removed the dedicated selection message, outgoing `0x0baf` packet, chooser
  window creation, chooser-specific hotkey blocker and its now-unused helper.
  No incoming packet handler or shared item-use restriction was changed.
- Historical type fixtures retain both retired UI names for rejection tests.
  Added an item-use regression test and wired it into the existing suite.

Validation:

- All nine changed retained production units compiled with release flags and the
  complete release executable relinked. Build input lists were regenerated so
  deleted implementations are excluded. No retained structure layout changed.
- Item-use tests passed for ordinary items and former selection boxes, exact
  normal request bytes, duplicate-request suppression, trade/blocking windows,
  pending drops, death, burrow/body-state restrictions, item-specific exceptions,
  refusal notices and temporary-item cleanup.
- Type checks passed for 565 concrete classes and 46 retired identities, with
  3,455 prior-runtime comparisons, 419 diagnostic names and 398 window names.
- All 1,368 retained packet-table rows match the backup; only outgoing `0x0baf`
  was removed. Incoming dispatch, normal ClientLua and connection implementation
  match the backup byte for byte. The normal item-use function matches after
  removing only the chooser interception and its two local variables.
- No retired feature references remain in production source, release build inputs
  or the final link map. `git diff --check` passed.

The pre-removal tree is in `build/before-select-package-removal`; focused logs and
the audit use `build/purist-no-select-package-*.log`. No live client/server smoke
test was run. Changes remain uncommitted on `purist`; `main` and both existing
server checkouts are unchanged by this removal.


## Unused Winsock check removal (START-053)

Patrick approved removing the check. Removed its ignored startup call,
`connection_ws2_unhooked`, the four-entry address-name table and the public
declaration. Winsock startup/shutdown, loading the transfer functions, fallback
handling, connection ownership, send/receive and packet framing remain.

The changed connection unit compiled with release flags and the complete release
executable relinked successfully. Existing packet tests passed, including framing,
partial reads, immutable sends, held/disconnected sends and all 196,608
packet-ID/send-mode cases. The source audit confirms the connection implementation
matches the backup after removing only the check, its name table and its call.
No retired references remain in production source or the final link map;
`git diff --check` passed.

Backup: `build/before-winsock-check-removal`. Logs and audit:
`build/purist-no-winsock-check-*.log`. No live connection smoke test was run.
Changes remain uncommitted on `purist`; `main` and server checkouts are unchanged.

## Guild-flag preload investigation (START-057)

Patrick asked why the flag cannot load normally and whether its guild emblem
requires special handling. The emblem does require a special drawing path, but
that path does not consume the startup animation preload:

- Job 722 selects the emblem renderer in `src/actor/granny_model.c`. The renderer
  uses the normal animated mesh preparation and replaces the second mesh's texture
  with the guild emblem in `src/actor/granny.c:431`. Animation stepping runs before
  choosing between the emblem and ordinary renderers.
- Local `revel.grf` maps `JT_GUILD_FLAG = 722` to `Guildflag90_1.gr2`. The model
  loader parses the `_1` suffix as animation-cache model ID 1; playback requests
  that ID. The slot-1 job-name lookup also maps back to job 722 and the same file.
- Startup instead loads the flag animation into `[5][0]`. No model with suffix
  `_5` exists among the `.gr2` assets in the local `data.grf` and `revel.grf`.
  The cache-miss loader resolves the slot-1 filename but refuses to load it because
  of `if (job != 1)` in `src/resources/granny_res.c:328`.
- Original executable disassembly confirms the preload's `push 5` at `0x008a86fe`
  and slot-1 exclusion at `0x009ff2a0`, as well as filename-derived model IDs and
  matching cache indexing. These are inherited behaviors, not introduced by the
  purist pruning. The original author's reason for them is unknown.

Before the fix, two focused x86 probes passed. One exercised the then-unchanged
production cache code through the existing lifecycle fixture: cold `[1][0]` lookup fails, preloading
`[5][0]` still fails to satisfy it, and using the same standard loader at `[1][0]`
succeeds. The other loads the actual 56,340-byte flag asset from local `data.grf`
with the installed `granny2.dll`: it contains one model, one animation and two
meshes. Standard SDK model instantiation and animation start succeed without any
emblem, graphics or startup initialization.

Patrick approved normal loading with emblem rendering preserved. Removed the
startup slot-5 preload and the slot-1 exclusion from the ordinary cache-miss
loader. Guild flags now request the animation only when needed through the same
job-name lookup and file-loading path as other Granny models. The shared cache,
model creation, animated mesh preparation and emblem texture drawing remain.
The two emblem-rendering source files match the pre-change backup byte for byte.

Validation:

- Resource lifecycle tests passed, including the new flag regression: job-722
  name resolution, successful loading into `[1][0]` without a preload, retry after
  a missing file, cache reuse and reloading after cleanup. Existing allocation,
  SDK failure and resource ownership cases also passed.
- Both modified production units compiled with release flags and the complete
  release executable relinked. `git diff --check` passed.
- [guild-flag-lazy-loading.patch](upstream/guild-flag-lazy-loading.patch) contains
  only the two production changes and their regression test, based on `main`.
  It passes `git apply --check` against the clean foundation worktree. Its loader
  and test baseline match the tested purist files. It has not been applied to
  `main`, committed or pushed.

Read-only asset extraction, pre-change reproduction and the real-DLL probe are
under `build/guild-flag-investigation`. Backup:
`build/before-guild-flag-lazy-loading`. Validation logs:
`build/purist-guild-flag-tests.log` and `build/purist-guild-flag-build.log`.
Changes remain uncommitted on `purist`. The game installation, foundation worktree
and server checkouts were not modified. No live in-game visual check was run.

## Folder-only interface skins (START-058)

Patrick approved keeping folder skins and removing ZIP extraction. The startup
scan now accepts directories under `skin` and ignores ordinary files, including
ZIP archives. Saved folder selection, the basic skin, color-chip lookup, bitmap
cache invalidation when switching skins, and fallback for absent custom graphics
remain. A saved ZIP-only choice no longer matches an entry and uses the basic skin;
unpacking that skin into a same-named folder makes it available again.

Removed ZIP extraction, the `runzip.exe` call, `_tmpskin_` paths and cleanup,
per-entry ZIP/extraction flags and the unused extension helper. The recursive
`dir_remove_tree` helper and its private cleanup callback had no remaining callers
and were removed. Other directory listing/creation functions remain unchanged.
Existing temporary skin directories in the game installation were not touched.

The existing skin manager test was updated to omit retired ZIP state. Startup,
selection, bitmap cache invalidation and balanced shutdown ownership passed.
The resource-path function is byte-for-byte unchanged except for removal of the
ZIP extraction call; retained file-utility functions and skin accessors are also
unchanged. The folder scanner branch matches the backup after removing only ZIP
flags and its redundant final `continue`. All production sources compiled and the
complete release executable linked successfully. No retired references remain in
production source or the link map; `git diff --check` passed.

Backup: `build/before-zip-skin-removal`. Logs:
`build/purist-no-zip-skins-tests.log` and `build/purist-no-zip-skins-build.log`.
No live UI smoke test has been run. Changes remain uncommitted on `purist`;
`main`, the game installation and server checkouts are unchanged by this removal.

## Warning and licence startup screen removal (START-060)

Patrick approved removing both startup screens. The initial `g_loginStartMode`
is now connection selection (2), and the service/server-specific selector is gone.
The existing `hideaccountlist` behavior still proceeds to the login form, while
visible connection lists still let players choose a configured server. Returning
to login (3) and character restart (5) keep their existing entry paths.

Removed the two screen functions and dispatch cases, warning-image names, licence
file path, lazy text loader/cache and shutdown cleanup. The warning-specific
`UINoticeConfirmWnd` implementation, factory entry, untracked-ID entry, declarations
and type registration are removed. The shared NPC dialogue window used to display
the licence is retained. Login's `0x2722` back-to-connection-selection message stays;
only its now-unreachable Disagree/quit branch was removed. Login rendering no
longer tests for the removed warning state.

Validation:

- All eight changed retained production units compiled with release flags and the
  complete release executable relinked. Regenerated build inputs exclude the
  deleted warning-window implementation.
- UI type checks passed: 564 retained concrete classes, 47 retired identities,
  3,447 prior-runtime comparisons, 418 diagnostic names and 397 window names.
  Historical fixtures retain the old warning-window identity to test rejection.
- Source comparison confirms every retained login-state dispatch branch and its
  implementation is unchanged. Connection loading, login-window controls, return
  to login, character restart and NPC dialogue match the backup byte for byte.
- No retired symbols remain in production source or the release link map.
  `git diff --check` passed.

Backup: `build/before-startup-notices-removal`. Logs:
`build/purist-no-startup-notices-build.log` and
`build/purist-no-startup-notices-types.log`. No live login/UI smoke test was run.
Changes remain uncommitted on `purist`; `main`, the game installation and server
checkouts are unchanged by this removal.

## Personal minimap notes (START-061, removed after initial save fixes)

Current decision: Patrick subsequently requested removing personal minimap notes
from purist. This supersedes the initial keep decision described in the history
below. The independent foundation save-error/newline patch is retained.

Patrick initially reported a Store-button failure in a 2016 SpringRO client.
He later clarified that the feature works: he had not recognized that Record
switches the cursor into placement mode. His manually entered note also lacked a
final newline, so the next note saved in-game was appended to the same record.
There is no remaining confirmed SpringRO Store-button failure.

The 2022 source opens the expanded map from the small minimap's Maximize button;
Record arms placement, a map click opens the editor, and Store saves the note.
The editor limits input to 20 characters. The save filename is
`<client-directory>/Memo/$$<map-name>.txt`, for example `$$prontera.txt`;
map-entry code removes the `.rsw` extension before storing the session map name.

An isolated probe extracts the actual Store handler, save function and reload
function unchanged from `minimap_wnd.c`. With the Memo directory absent, Store
reproduces `editor=open markers=0 errno=2`; creating only that directory makes the
same command return `editor=closed markers=1 errno=0`. The 2022 save path therefore
silently ignores file-open failures, leaving the editor open without an error.
This independently established a weakness in the 2022 client, rather than
reproducing the initially reported SpringRO behavior.

Read-only inspection found `C:/Games/springro/game/memo` present and empty, with
inherited Modify permission for Authenticated Users. That does not establish the
running client's actual save path. No SpringRO files were modified during the
investigation or the implementation below.

Probe, generator, executable and exact command/output log are under
`build/memo-store-investigation`, `build/make_memo_store_probe.py` and
`build/run_memo_store_probe.bat`. Only isolated directories under `build` were
created or written for the reproduction.

Follow-up: Patrick manually created `game/memo/$$mjo_dun02.txt`. Read-only inspection
confirmed that exact filename and zero bytes of content. The SpringRO executable
contains both `%s\Memo\$$%s.txt` and `%d\t%d\t%s\r\n`, confirming the filename and
record format match the 2022 source. An empty file supplies no marker records.

Patrick then added a tab-separated note record and confirmed that SpringRO loaded
and displayed its marker and tooltip. This validates its filename, record format
and rendering path for that map. His subsequent correction about placement mode
and the missing newline supersedes the initial failure report.

Patrick initially approved keeping notes and improving save-error reporting.
Before the later removal, the implementation checked file opening, writes and closing (including buffered
flush failures). Errors name the target file and suggest checking the folder,
write access and free space. Failed additions keep the editor and its text open;
markers refresh and the editor closes only after a successful save. Deletion
omits the selected note while writing, and changes the visible markers only after
the rewrite succeeds. The filename and tab-separated record format stay the same.

Appending also checks the last byte: an unterminated manual record gets a CRLF
separator, a trailing CR gets its LF, and LF/CRLF endings get no extra separator.
Position/read failures during this check are reported through the same error path.
These remain ordinary append/rewrite operations, not atomic file replacement;
an actual I/O failure can leave partial bytes on disk even though the UI remains
available for retry.

`tests/build_minimap_memos.bat` extracts the production Store handler and save
functions unchanged and runs them with real temporary streams, injected I/O
failures and UI-boundary spies. The original source failed both the missing-error
assertion and missing-newline regression before implementation. Tests now cover
open/write/close failures, seek/tell/read failures, separator-write failure,
retained editor text, success after a failed attempt, empty and terminated files,
deletion errors, omitting one note, and deleting the final note. This does not
claim live in-game UI coverage; the deletion caller's refresh gate was inspected.

Validation passed on 2026-11-01:

- Purist regression suite: `build/purist-memo-save-errors-tests.log`.
- Changed release source compiled and the full executable relinked:
  `build/purist-memo-save-errors-build.log`.
- Independent patch `docs/upstream/minimap-memo-save-errors.patch` passes
  `git apply --check` against `main`. Its exact patched source compiles against
  foundation headers, and its tests pass with foundation string/vector code:
  `build/purist-memo-save-errors-foundation.log`.

`main` remains clean and the foundation patch is unapplied. No server or
game-installation files were changed. Backups of the pre-fix source and test runner
are under `build/before-memo-save-errors`.

The subsequent purist removal deletes the Record button, armed placement cursor,
twenty note-marker controls, note editor (`UIEditctrlWnd`, window ID `0x112`),
file loading/saving/deletion and startup `Memo` directory creation. Editor factory,
lookup, cleanup and type-registration entries are removed, as are the unused
placement coordinates and editor pointer in shared UI state. The dedicated note
test/extractor/build files and suite entry are removed from purist; their complete
versions remain inside the foundation patch.

The expanded minimap keeps its existing title-bar dragging behavior and uses the
standard arrow cursor. Map drawing, player/boss/party/guild/warp markers, town and
quest icons, navigation and saved window-layout handlers remain unchanged. No
existing Memo folders or player-authored notes were deleted from game installs.

Removal validation on 2026-11-01:

- Window identity and cleanup tests passed: 563 retained classes, 48 retired
  identities, 3,439 comparisons, 417 diagnostic names and 396 window names.
  Log: `build/purist-no-minimap-notes-tests.log`.
- Source audit confirms retained minimap drawing, marker, navigation and layout
  handlers are byte-identical. Log: `build/purist-no-minimap-notes-audit.log`.
- Full release compile/link passed, recompiling all users of changed UI
  structures. Log: `build/purist-no-minimap-notes-build.log`.
- The retained `docs/upstream/minimap-memo-save-errors.patch` still passes
  `git apply --check` against clean `main`.

Removal backups are under `build/before-minimap-note-removal`, including the
previously untracked test files. The isolated diff is
`build/purist-no-minimap-notes.diff`. Purist changes remain uncommitted.

## Local navigation destinations (START-062, removed)

Before removal, `Navigationdata/*.txt` supplied named places to the navigation window. Each row
contains a map name, an `x/y` coordinate token, and a label, for example
`prontera 150/180 Meeting point`. The user selects a file from the navigation
window's file button, then selects one of its destinations for guidance. This
imports destinations rather than a saved sequence of route steps. The inspected
code reads these files; it does not export them.

Patrick chose to remove this feature. Both the current and older navigation
windows now omit the file-open button, file enumeration/selection handlers,
filename vectors and file readers. Startup no longer creates `Navigationdata`.
The import-only `navi_manager_add_place` function and the now-unused
`dir_list_names_vector` helper are removed. The old window has five buttons rather
than six; the current window's structure offsets were updated for removed fields.

Ordinary search, route guidance, chat links, coordinate destinations and town
facilities remain. Personal minimap notes were subsequently removed under the
revised START-061 decision. The coordinate parser and place
result type still have active users and were retained. No existing files in game
installations were touched.

Patrick also requested an example file and asked about naming. The reference
fixture `build/examples/navigation-destinations.txt` has three tab-separated
meeting points. For clients retaining this feature, any `.txt` basename works;
the picker displays the filename, while each row supplies its map and coordinates.
These follow-up questions did not revoke the removal decision.

Validation passed on 2026-11-01:

- Full release compile/link after window structure changes:
  `build/purist-no-navigation-files-build.log`.
- Existing navigation state/event suites, including coordinate commands, search,
  ownership and UI dispatch: `build/purist-no-navigation-files-tests.log`.
- Final helper-only compile/relink after removing its last callers:
  `build/purist-no-navigation-files-helper-build.log`.
- Source audit found no feature references in `src` or `tests`; 60 retained
  function bodies in affected C files are byte-identical to the pre-change copies.
  Only startup, file-related UI creation/layout/dispatch and lifecycle cleanup
  changed. Audit: `build/purist-no-navigation-files-audit.log`.

Backups are under `build/before-navigation-file-removal`; the isolated change is
recorded in `build/purist-no-navigation-files.diff`. Changes remain uncommitted on
`purist`. This is an opinionated removal; no foundation patch is needed for it.

## Login/game lifecycle loop (START-063, kept)

`mode_mgr_run` starts the login mode with `login.rsw`, then creates, initializes,
runs, exits and destroys the requested login or game mode. `mode_mgr_switch`
ends the active loop and records the next type/map. Quitting stops the outer loop.
This is core application flow, distinct from the removed command-line modes.
Patrick approved continuing with individual behaviors. The core loop is retained
unchanged; its existing transition, cleanup, allocation-failure and quit tests
passed during START-064 validation.

## Hardcoded official website on exit (START-064, removed)

Service type `0xb` previously launched `http://www.ragnarok.co.in/index.php` from
`mode_send_msg_base` on `MODE_MSG_QUIT`, `mode_mgr_quit`, and the main window's
`WM_CLOSE` handler. All three branches are now removed. Account/character saves,
Agency booking cancellation, loop termination and the global quit request are
unchanged. The mode manager no longer needs its `shellapi.h` include.

Validation passed on 2026-11-01:

- Both modified C units compiled and the release executable relinked:
  `build/purist-no-official-exit-page-build.log`.
- Existing mode lifecycle suite passed:
  `build/purist-no-official-exit-page-tests.log`.
- Source audit verifies each affected function differs only by removal of its
  service check/browser launch; all other function bodies in both files are
  unchanged. No hardcoded URL references remain in source.
  Log: `build/purist-no-official-exit-page-audit.log`.

Backups: `build/before-official-exit-page-removal`. Changes remain uncommitted on
`purist`. Foundation and server checkouts are unchanged.

## Configured homepage on exit (START-065, removed)

The clientinfo option handler previously read `<viewhomepage>` into the window
manager's 256-byte URL buffer. On `WM_DESTROY`, a nonempty buffer launched that URL
in the system browser. Patrick approved removing this behavior as well.

The XML option branch, URL buffer, launch branch, private `open_page` helper and
its exclusive headers are removed. `WM_DESTROY` now returns normally without
opening a browser. Existing XML files may retain the unused tag; other clientinfo
settings load as before. This completes removal of both exit-page mechanisms.

Validation on 2026-11-01:

- Existing window cleanup suite passed with the smaller UI-manager structure:
  `build/purist-no-configured-exit-page-tests.log`.
- Full release compile/link passed, recompiling all users of the smaller shared
  UI-manager structure:
  `build/purist-no-configured-exit-page-build.log`.
- Isolated diff/source audit confirms only the option/launch handling and buffer
  changed; 28 other function bodies across the two C files are unchanged. No
  `viewhomepage`, `view_homepage` or hardcoded regional exit URL references remain
  in `src` or `tests`. Logs: `build/purist-no-configured-exit-page-audit.log` and
  `build/purist-no-configured-exit-page.diff`.

Backups: `build/before-configured-exit-page-removal`. Changes remain uncommitted on
`purist`; foundation, game installations and server checkouts are unchanged.

## Built-in screenshots (START-066, kept; extras under review)

The keyboard handler uses Print Screen or Scroll Lock to request a screenshot.
Login/game rendering consumes that request, chooses an unused numbered JPEG name
under `<client>/ScreenShot`, captures the rendered frame and reports the filename
in chat. The shared implementation also receives optional trade-window requests.

Patrick approved keeping capture and reviewing its extras individually. No
production changes or new test runs were needed for that keep decision.

Two foundation bugs were identified alongside the screenshot review:
`renderer_screenshot` gates frame capture on finding `scr_logo.bmp`, although the
pixel-composition helper already handles a null logo; `mode_screen_shot` ignores
the capture result and unconditionally prints a saved notification. Failure to
create the screenshot directory is also silent. The existing JPEG suite covers
lock/allocation/codec/write failures, but always returns a logo object from its
resource stub and does not exercise the caller's notification behavior. These
were reproduced by new regression tests and fixed during START-069. See
[the independent foundation patch](upstream/screenshot-save-failures.patch).

## Screenshot character-ID stamp (START-067, removed)

For service type zero (`korea` in the clientinfo service table), a nonzero
`g_session1.ownGid` was converted into a letter code and drawn beside the screenshot
date/time. This is the character ID: the map-login packet assigns `ownAid` to
`account_id` and `ownGid` to `char_id` in `src/game/mode_connection.c`. The stamp is
visible text, not JPEG metadata. Other service types draw the timestamp without
this code.

Patrick approved removing the character-ID code. The encoder and both
service/character-dependent formatting branches are removed. Date/time formatting
became uniform across services and did not require a selected character; the
visible overlay was subsequently removed under START-068. Filename selection, JPEG capture,
notifications, ownership/cleanup and every other retained function in the source
are unchanged.

Validation passed on 2026-11-01:

- Changed C unit compiled and the release executable relinked:
  `build/purist-no-screenshot-id-build.log`.
- Existing JPEG capture suite passed for 16/32-bit frames, pitches, logo
  composition, failure cleanup and unlocks: `build/purist-no-screenshot-id-tests.log`.
- Source audit confirms the ID encoder is gone and capture/save code after stamp
  drawing is byte-identical: `build/purist-no-screenshot-id-audit.log`.

Backup: `build/before-screenshot-id-removal/mode_mgr.c`. Isolated diff:
`build/purist-no-screenshot-id.diff`. This is a purist-only removal; no foundation
patch is needed for this policy choice. The separate capture correctness issues
recorded under START-066 were subsequently fixed during START-069.

## Screenshot timestamp (START-068, removed)

The capture function previously drew local date, weekday and time at the
bottom-left of the rendered frame, using white text with a dark outline, before
saving the JPEG. Patrick approved removing this visible stamp.

Removed the weekday table, `SYSTEMTIME`, timestamp buffer, clock read/formatting,
five text-drawing calls and now-unused string header. Screenshot requests now
proceed directly to naming/capture. File timestamps, numbered JPEG filenames,
capture and cleanup remain unchanged. Logo overlay and trade-triggered captures
remain separate decisions.

Validation passed on 2026-11-01:

- Changed C unit compiled and the release executable relinked:
  `build/purist-no-screenshot-timestamp-build.log`.
- Existing JPEG capture suite passed:
  `build/purist-no-screenshot-timestamp-tests.log`.
- Source audit confirms date/time generation and text drawing are gone, and all
  code from clearing the capture request through file saving/cleanup is unchanged:
  `build/purist-no-screenshot-timestamp-audit.log`.

Backup: `build/before-screenshot-timestamp-removal/mode_mgr.c`. Isolated diff:
`build/purist-no-screenshot-timestamp.diff`. Changes remain uncommitted on purist;
foundation and game/server installations are unchanged by this removal.

## Screenshot logo (START-069, removed)

`renderer_screenshot` loads `scr_logo.bmp` and composites its pixels at the
bottom-right of the JPEG, with magenta (`0xff00ff`) treated as transparent. This
is separate from the rendered game/UI and the now-removed text stamps.

Patrick approved removing the overlay. The resource lookup, pixel compositor and
bitmap-specific include are removed. JPEG capture now copies the rendered frame
without any logo resource access. Tests abort on resource-manager access and
verify all pixels, including the bottom-right corner, for both 16/32-bit frames.

The shared failure-reporting fixes also landed in purist: failed frame capture or
JPEG writing displays a save-error message naming the target path, and only a
successful write prints the saved notification. Failure to create `ScreenShot`
also reports the error. The request and optional filename prefix are consumed
after failure so it does not retry every frame or contaminate a later manual shot.

The independent [foundation patch](upstream/screenshot-save-failures.patch)
contains only those correctness fixes: a missing logo no longer blocks capture,
and failed saves are reported accurately. Foundation logo rendering, timestamp,
character-ID stamp, region capture and other existing features remain. The patch
includes its regression tests and test-runner entry. It is prepared against main
at `7fe9620`, not applied or committed there.

Validation on 2026-11-01:

- Before fixes, regressions reproduced missing-logo capture failure, a false
  success notice after capture failure, and silent directory creation failure:
  `build/screenshot-missing-logo-red.log`,
  `build/screenshot-save-notice-red.log`,
  `build/screenshot-save-directory-red.log`.
- Purist JPEG tests passed for pixel conversion, pitches, resource independence,
  write/codec/allocation/lock failures and cleanup:
  `build/purist-no-screenshot-logo-tests.log`.
- The production request handler passed tests for success/failure notifications,
  folder failure, collision skipping, filename sanitization, request consumption
  and a manual request following a failed trade capture:
  `build/purist-screenshot-save-tests.log`.
- Changed production units compiled and the release executable relinked:
  `build/purist-no-screenshot-logo-build.log`.
- Foundation patch applies cleanly to main. Both patched production units compile
  against foundation headers; its capture suite passes with present/absent logos
  and region captures, and its request-handler tests pass:
  `build/foundation-screenshot-save-tests.log`.

Backups: `build/before-screenshot-logo-removal`. Changes remain uncommitted on
purist. No live game screenshot was taken; game/server installations are unchanged.

## Automatic trade screenshots (START-070, removed)

The trade window creates a checkbox with `notify` initially zero. When enabled,
clicking Exchange queues a screenshot with a filename prefix formatted using the
trade partner's name, then sends the trade commit request. It does not wait for a
successful server reply. The checkbox state is local to that window.

Patrick approved removing this feature. Removed the checkbox and label, its state
and command handler, automatic screenshot request and private message constant.
Trade item/zeny operations, lock/cancel/commit requests and duplicate-click guards
are unchanged.

The trade request was the only caller supplying a custom screenshot filename
prefix. Removed that string from `Mode`, its initialization and cleanup, and its
formatting/request handling. Manual captures retain the normal numbered
`screen<server><number>.jpg` names. The base destructor became empty and was removed
with its declaration and login/game calls. The other mode cleanup remains intact.
The foundation screenshot-error patch retains its original prefix handling and
tests; this purist-only removal is not part of that patch.

Validation on 2026-11-01:

- Full release build passed after the shared `Mode` structure changed:
  `build/purist-no-trade-screenshots-build.log`.
- Existing manual screenshot request tests passed for successful and failed saves,
  directory failure, collision skipping, filename sanitization and request cleanup:
  `build/purist-no-trade-screenshots-capture-tests.log`.
- Existing mode lifecycle tests passed for initialized state, callbacks, cursors,
  owned strings and cleanup. The expected allocation trace was updated by exactly
  one allocation/free pair per iteration (160 fewer words across 32 iterations):
  `build/purist-no-trade-screenshots-lifecycle-tests.log`.
- Source audit confirms 17 other trade function bodies are unchanged, and trade
  command handling differs only by removing the checkbox/screenshot branches and
  unused checkbox argument. No screenshot-prefix state or empty base destructor
  remains: `build/purist-no-trade-screenshots-audit.log`.

Backups: `build/before-trade-screenshot-removal`. Isolated diff:
`build/purist-no-trade-screenshots.diff`. Changes remain uncommitted on purist.
No live trade was performed; foundation and game/server installations are unchanged.

## Graphics-adapter detection (START-071, kept)

Startup enumerates graphics adapters, gathers supported resolutions and color
depths, and stores the results for the in-game graphics settings window. It also
records the selected adapter's monitor rectangle, used to convert window
coordinates for rendering on that monitor. The current renderer obtains mode
information through Direct3D 9.

Patrick approved keeping this shared graphics support. Production code is
unchanged; no new build or test run was needed for this keep decision.

## Saved-data directory (START-072, kept)

Startup checks for `savedata` in the working directory and attempts to create it
when missing. It creates the folder, not the individual settings files. Existing
contents are left in place. On the normal Windows filesystem this is the same
directory the writers spell `SaveData`.

Current users include game options (`OptionInfo.lua`), chat layout
(`ChatWndInfo_U.lua`), quest-tracker choices (`OngoingSimpleViewList.lua`), party
panel records (`MiniPartyInfo.lua`), styling-shop favorites (`LikeListInfoFile.lua`)
and the local account/character JSON preferences under `Accounts`. Keeping the
directory does not decide whether to retain every individual feature or file
format; those can be reviewed separately.

Patrick approved keeping this shared storage setup for retained local preferences.
Production code is unchanged; no new build or test run was needed for this keep
decision.

## Floating-point precision (START-073, kept)

The next startup call is `_controlfp(0x20000, 0x30000)`, equivalent to selecting
`_PC_24` with the `_MCW_PC` precision mask. The build targets x86. This selects
24-bit significand precision for legacy x87 arithmetic, affecting intermediate
results for code using that floating-point unit. It is not a blanket change to
every floating-point operation or to stored C data types.

The constants and precision-control behavior were verified against
[Microsoft's runtime documentation](https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/control87-controlfp-control87-2?view=msvc-170).
The precision mask does not request changes to rounding direction or exception
masks. Which individual client routines or external libraries depend on this
setting has not been established in this review.

Patrick approved preserving the existing numerical environment. Production code
is unchanged; no new build or test run was needed for this keep decision. Changes
to arithmetic behavior should receive a separate compatibility review.

## Windows timer resolution (START-074, kept)

Startup calls `timeGetDevCaps`, clamps a requested 1 ms interval to the reported
limits, then calls `timeBeginPeriod`. Normal shutdown calls `timeEndPeriod` with
the same interval. The game-step timer, cursor animation, fades and numerous
timeouts use `timeGetTime`, so their elapsed-time readings benefit from finer
granularity. This request does not set the frame rate or the logic update rate.

[Microsoft documents](https://learn.microsoft.com/en-us/windows/win32/api/timeapi/nf-timeapi-timegettime)
that `timeBeginPeriod` can improve `timeGetTime` precision.
The [timer-resolution documentation](https://learn.microsoft.com/en-us/windows/win32/api/timeapi/nf-timeapi-timebeginperiod)
also describes the scheduling/power tradeoff and limits on the guarantee for
occluded or minimized applications on Windows 11. The request is not a guarantee
that the client will run every millisecond.

Patrick approved keeping the request with the current timing implementation.
Production code is unchanged; no new build or test run was needed for this keep
decision. Clock replacement, frame pacing and background behavior remain separate
review topics. No runtime timing measurements were made for this review.

Cleanup audit candidate: startup currently ignores the `timeBeginPeriod` result,
and early initialization failures bypass the normal `timeEndPeriod` call. These
paths should be considered with startup resource cleanup rather than treated as
a reason to remove useful timing support. No cleanup changes made here.

## COM initialization (START-075, kept; cleanup corrected)

`CoInitialize(0)` initializes COM on the main thread as a single-threaded
apartment. The embedded-browser removal eliminated the source-level OLE/browser
activation paths. A search of the remaining source found no `CoCreateInstance`,
`CoGetClassObject`, OLE activation or shell COM creation calls. Direct3D still
uses COM interfaces through its own factory; the existence of those interfaces
alone does not demonstrate a dependency on this initialization.

The repository contains import libraries for external middleware, including Miles
audio, rather than their DLL implementations. The initial recommendation was to
retain initialization provisionally until those DLLs were checked. Patrick chose
to investigate their requirements before deciding; the results follow below.

An independent cleanup defect is visible in source: `WinMain` calls
`CoInitialize` once without checking the result, then normal exit calls
`CoUninitialize` in both `app_last_pump` and `WinMain`. Early failure paths can
bypass both. [Microsoft requires one matching cleanup for each successful COM
initialization](https://learn.microsoft.com/en-us/windows/win32/api/objbase/nf-objbase-coinitialize),
including `S_FALSE`. Centralize ownership and balance cleanup as a separate
foundation correction; do not treat removing all COM support as the bug fix.
The duplicate cleanup was initially identified in source. The later regression
checks and correction are recorded below; no live-client failure was reproduced.

### DLL investigation, 2026-11-01

Used the runtime set in `C:\Users\Patrick Kuhlmann\GIT\client-files`, also used by
the earlier real Granny asset probe. PE import/export tables and relevant dynamic
loader strings were inspected for `mss32.dll`, `granny2.dll`, `ijl15.dll`,
`mssfast.m3d`, `mp3dec.asi` and `cps.dll`. None imports COM initialization or object
activation functions. Miles dynamically resolves `DirectSoundCreate` from
`dsound.dll`. The active Miles DLL matches its original pre-proxy backup byte for
byte. The current purist executable does not actually import `cps.dll`, despite
the remaining link input.

Exact paths, SHA-256 hashes, imports and exports are recorded in
`build/com-dll-investigation/dll-report.json`; inspection script:
`build/inspect_com_dlls.py`. Runtime probes used matching copies under the build
directory, with no changes to the installed DLLs or game data.

A 32-bit probe compiles the current production sound-manager and JPEG code, the
production GDI+ font backend, and calls the real middleware DLLs. Each case runs
in a fresh process, confirms COM initially reports `CO_E_NOTINITIALIZED`, and
compares no explicit initialization against an explicitly initialized STA:

| Case | Without application COM initialization | With application COM initialization |
| --- | --- | --- |
| Miles ordinary audio, DirectSound and waveOut backends | Both pass; 48 sample handles each | Both pass |
| Miles positional audio, DirectSound and waveOut backends | Both pass; matching Fast 2D Positional Audio provider, listener and 16 supported sample handles | Both pass |
| Real MP3 stream opening/closing in all four audio cases | Pass; no stream or sample playback started | Pass |
| Real guild-flag Granny asset: model and animation creation, cleanup | Pass | Pass |
| Production JPEG writer/reader with actual IJL DLL | Pass | Pass |
| BSTR allocation and production GDI+ font-family lookup | Pass | Pass |
| Direct3D 9 factory and hardware-capability query | Pass | Pass |

All 16 process runs pass (four audio cases plus four other cases, each with/without
initialization). During the no-COM DirectSound run, Windows reports an implicit
MTA after audio startup; after audio teardown it again reports
`CO_E_NOTINITIALIZED`. This is not a requirement for the client to preinitialize
COM. The other no-COM middleware probes remain uninitialized throughout.
[Microsoft's DirectSound documentation](https://learn.microsoft.com/en-us/previous-versions/windows/desktop/ee416773(v=vs.85))
also states that explicit COM initialization is unnecessary unless using effect
DMOs. These probes do not play sound, run a complete game session or establish
behavior for other DLL versions/audio plugins.

Probe sources, executable and individual `none-*.log`/`sta-*.log` outputs are in
`build/com-dll-investigation`. Initial probe-only failures were corrected: querying
a not-yet-created DirectSound sample buffer, expecting every 3D provider to supply
48 handles, and treating library-created implicit MTA state as a failure. Final
checks retain fresh-process validation and verify successful creation/cleanup.

### Remaining shell dependency

The earlier direct COM-activation search was insufficient to establish that COM
is unused: the current executable still imports `ShellExecuteA`. Source callers
include login registration, account/refusal notices, cash-shop charging, attendance
banners, message-box links and URL tags in ordinary/quest text viewers. Four sites
explicitly launch `iexplore.exe`; the others hand a URL to the Windows shell.

[Microsoft's ShellExecute documentation](https://learn.microsoft.com/en-us/windows/win32/api/shellapi/nf-shellapi-shellexecutea)
recommends initialized COM because shell extensions can use COM, and some require
an STA. Not every launch needs it, but successful middleware probes do not establish
that removing initialization preserves all shell handlers. No websites or external
applications were launched by this investigation.

Revised recommendation: retain COM for the remaining shell-launch behavior and
correct its cleanup ownership, or review those URL-launch features first. The
middleware audit itself found no reason to preserve the global initialization.

Patrick first selected reviewing external-page launches before the COM decision.
After keeping clickable text links under START-086, he approved keeping COM and
fixing cleanup, with the correctness fix prepared separately for the foundation.

### Cleanup correction, 2026-11-01

`WinMain` now saves the initialization result and owns its matching cleanup.
`S_OK` and `S_FALSE` each receive exactly one `CoUninitialize`; failed attempts
receive none. The final window pump no longer releases COM. Returning failures
after audio, window, manager, graphics-device, renderer or connection setup all
reach the matching cleanup. The two failures before manager creation skip the
uninitialized `InitResult` string. Existing process return values and the policy
of continuing after COM initialization failure are preserved.

Cleanup is after the remaining shutdown work. In the foundation this includes
its retained exit-page launches and asynchronous manager destruction. This fix
does not redesign other subsystem cleanup or intercept `exit()`/fatal process
termination inside called subsystems.

Regression tests extract the exact production startup tail from immediately
before COM initialization through the function return, plus `app_last_pump`.
Subsystems and COM calls are controlled test substitutes. They check normal
shutdown and six returning startup failures for `S_OK`, `S_FALSE`,
`RPC_E_CHANGED_MODE` and `E_FAIL`, including cleanup counts, consumer ordering,
return values and valid string cleanup. Foundation checks also exercise both
exit-page branches. The tests open no windows, devices, sockets or websites.

Validation:

- Original purist flow fails the regression checks (28 cases, 44 failed checks);
  corrected flow passes all 28 cases: `build/purist-com-lifecycle-before.log` and
  `build/purist-com-lifecycle-tests.log`.
- Original foundation snapshot fails (36 cases, 104 failed checks); corrected
  snapshot passes all 36 cases: `build/foundation-com-lifecycle-before.log` and
  `build/foundation-com-lifecycle-tests.log`.
- Changed production units compiled and the release executable relinked:
  `build/purist-com-lifecycle-build.log`. Source whitespace checks passed.
- [com-lifecycle.patch](upstream/com-lifecycle.patch) contains only the cleanup
  correction and its standalone regression tests. Prepared against foundation
  snapshot `4be93f5fade82d346d88582be594faf5943ce4b7`; `git apply --check` passes
  against that clean foundation checkout. The patch is not applied or committed.
  Earlier foundation patches retain their own recorded base revisions.

Purist backup: `build/before-com-lifecycle-fix`. Isolated production diff:
`build/purist-com-lifecycle.diff`. Both source variants use the same verified
transformation; no purist feature removals are included in the foundation patch.
No full live-client startup or browser launch was tested.

## Regional wrong-password browser/exit behavior (START-076, removed)

For login refusal code 1, service type 0 increments `g_wrongPasswordCount`.
The first two failures use the ordinary wrong-password notice (`msg_str(0x10b)`).
On the third failure, the client instead shows a Korean notice saying three
passwords were entered incorrectly and the client will close. Acknowledging it
opens `http://www.hangame.com` for channel 1 or `http://ragnarok.co.kr` otherwise,
then calls `mode_mgr_quit`. Successful account/server-list processing resets the
counter. Other service types already use ordinary wrong-password handling.

Patrick approved removing this behavior. Refusal code 1 now selects the existing
ordinary wrong-password notice for every service and attempt. Removed the special
branch and the exclusive global counter, declaration and successful-login reset.
Server authentication, rejection packets and the normal configured
return-to-login/exit behavior remain. Other refusal-code redirects and the
registration button will be reviewed separately.

Validation on 2026-11-01:

- Changed C units compiled and the release executable relinked:
  `build/purist-no-three-password-build.log`.
- Existing login OTP packet/input tests passed:
  `build/purist-no-three-password-tests.log`. These cover retained authentication
  behavior, not the removed refusal branch.
- Isolated source comparison confirms the only changes are refusal code 1 and its
  counter. All other refusal handling and account processing are identical; no
  counter references remain: `build/purist-no-three-password-audit.log`.
- Whitespace checks passed. No live login attempts were made.

Backup: `build/before-three-password-removal`. Isolated diff:
`build/purist-no-three-password.diff`. This policy change remains uncommitted on
purist; foundation and game/server installations are unchanged.

## Password-change webpage redirect (START-077, removed)

Refusal code 16 under service type 0 shows a Korean notice asking the player to
change their password before reconnecting. Clicking OK opens the hardcoded
`http://www.ragnarok.co.kr/setNewPW/password_change_agreement.asp` URL and calls
`mode_mgr_quit`. Other services skip this special branch.

Patrick approved removing this legacy webpage launch and extra forced exit.
Removed the case-specific modal handling, URL launch, quit call and message
clearing. The existing Korean-service gate and password-change notice remain;
the common refusal path now displays the notice and follows the configured
return-to-login/exit policy. The client can still exit under that normal policy.

Validation on 2026-11-01:

- `src/login/refuse.c` compiled and the release executable relinked successfully:
  `build/purist-no-password-change-redirect-build.log`.
- Isolated comparison against the immediate backup confirms only the five
  approved lines were removed; the rest of the file is byte-identical, including
  every other refusal branch and common login-error handling:
  `build/purist-no-password-change-redirect-audit.log`.
- Whitespace checks passed. No live login or website launch was performed.

Backup: `build/before-password-change-redirect-removal`. Isolated diff:
`build/purist-no-password-change-redirect.diff`. This is an uncommitted purist
policy change; no foundation correction is needed for this removal.

## Message-table password-change redirect (START-078, removed)

Refusal code 110 displays `msg_str(0x929)`. Clicking OK opens `msg_str(0x92a)`
through the Windows shell and calls `mode_mgr_quit`. Unlike code 16, this branch
has no service-type restriction. It then reaches the shared login-error policy.

The local runtime `client-files/data/msgstringtable.txt`, parsed with the same
field rules and zero-based indices as `table_reader.c`, contains "Please change
your password" at `0x929` and `http://www.ragnarok.co.kr` at `0x92a`. Other data
sets can supply different text and URLs; the launch behavior is confirmed in
source independently of those local values.

Patrick approved removing this second password-change browser launch and extra
forced exit. Code 110 now selects `0x929` for the common refusal-message path.
The normal configured return-to-login/exit policy remains. Removed the now-unused
`open_page` helper and shell/application headers; included `platform/language.h`
directly for the retained service-type check. No shell launch remains in
`src/login/refuse.c`.

Validation on 2026-11-01:

- `src/login/refuse.c` compiled and the release executable relinked successfully:
  `build/purist-no-table-password-redirect-build.log`.
- Isolated source comparison confirms only case 110 and its exclusive helper/
  includes changed. All other refusal branches and common login-error handling
  remain unchanged: `build/purist-no-table-password-redirect-audit.log`.
- Whitespace checks passed. No live login or website launch was performed.

Backup: `build/before-table-password-redirect-removal`. Isolated diff:
`build/purist-no-table-password-redirect.diff`. This remains an uncommitted purist
policy change; foundation and game/server installations are unchanged.

## Authentication-result GNJOY redirect (START-079, removed)

`login_mode_on_auth_result` handles packet `0x0ae3`. For result code 900 it shows
`msg_str(0xd86)` in a native error message box, tries to launch `iexplore.exe` with
`http://ro.gnjoy.com/`, then sends mode message 2 to quit the client. The local
runtime message table describes an account-security/MOTP homepage login request.
This branch has no service-type restriction. The alternate result handler for
packet `0x0ad1` has no equivalent redirect.

Other failed results show their notices and use `after_message` to close the wait
window, clear authentication-wait state, disconnect and return to login, unless
the request already timed out. Successful token processing is separate.

Patrick approved removing the GNJOY launch and forced exit. The existing native
error notice is retained exactly. After it closes, code 900 now calls the existing
`after_message(self, packet->code)` helper. Its timeout guard and failure cleanup
remain unchanged. The branch then reaches ordinary string cleanup without
entering the other messages' formatting path. Removed the unused shell include
and `out` label. Successful token handling and the alternate result handler are
unchanged; this rejection does not become a successful login.

Validation on 2026-11-01:

- `src/login/auth.c` compiled and the release executable relinked successfully:
  `build/purist-no-gnjoy-auth-redirect-build.log`.
- Reviewed the isolated diff against the immediate backup: only the code 900
  redirect/quit sequence and unused include/label changed. All other result and
  token handling is unchanged: `build/purist-no-gnjoy-auth-redirect.diff`.
- Whitespace checks passed. No live authentication exchange or browser launch
  was performed.

Backup: `build/before-gnjoy-auth-redirect-removal`. This remains an uncommitted
purist policy change; foundation and game/server installations are unchanged.
An initial combined verification command was rejected with a generic policy
message. Separate diff inspection and compilation/linking completed successfully.

## Account-payment disconnect redirect (START-080, removed)

`login_mode_on_disconnect_reason` disconnects the transport before handling its
reason code. Code 6 shows a Yes/No prompt from message `0x1b6`. The local runtime
table says the account has not been paid for and asks whether to pay now. Choosing
Yes quits the client and invokes `open_account_page`.

The page depends on service/channel: Korean settings use message-table URLs
`0x210` or `0x51c` (Ragnarok billing or Hangame payment in the local table); service
1 uses a hardcoded iRO registration page; service 3 uses a Kunlun voucher page;
service 2 opens no page. Other services use `0x210`. The shared return-to-login/
exit policy runs afterward.

Patrick approved removing the payment offer, regional webpage helper and extra
forced exit. Code 6 now selects message 3, the ordinary disconnected-from-server
notice; the common message path appends the original reason code `(6)`. The
transport disconnect and configured return-to-login/exit policy remain unchanged.
Removed `open_account_page`, its shell include and its application-header
dependency. Included `platform/language.h` directly for the retained service-type
check in reason 107. Other disconnect reasons and play-time notices are unchanged.

Validation on 2026-11-01:

- `src/login/notify.c` compiled and the release executable relinked successfully:
  `build/purist-no-account-payment-redirect-build.log`.
- Reviewed the isolated diff against the immediate backup: only reason 6, its
  exclusive regional URL helper and the related includes changed:
  `build/purist-no-account-payment-redirect.diff`.
- Whitespace checks passed. No live server disconnect or browser launch was
  performed.

Backup: `build/before-account-payment-redirect-removal`. This remains an
uncommitted purist policy change; foundation and game/server installations are
unchanged.

## Login registration webpage button (START-081, removed)

The login window creates a Join/Register text button using `bt_join_*` images and
message `0xd56`. Its click handler calls `login_join`. Korean service/channel 1
quits and opens a hardcoded Hangame signup URL. Other Korean connections first
ask for confirmation through message `0xd08`; a Yes response continues to the
ordinary path. Other services go straight to that path, which quits and opens
`g_registrationWeb` through the Windows shell.

The selected connection's `<registrationweb>` entry in `clientinfo.xml` supplies
that address. The button is created even when the address is empty. Its helper
and configuration global have no other source users. The XML reader also has
registrationweb-specific handling for raw ampersands. This feature opens a web
page; it does not create accounts over the game connection.

Patrick approved removing the button and its web-link setup. Removed the button
creation, bitmap dependencies, command branch, registration helper, regional
confirmation/URL handling and exclusive constants. Removed `g_registrationWeb`,
its declaration and connection-setting read. No source references remain to the
registration feature. The login controls and their coordinates are otherwise
unchanged; the former button area is unused.

Removed the XML reader's registration-specific exceptions as well. A retired
`registrationweb` element can remain in an old clientinfo file as ignored data,
including URLs with raw ampersands, without hiding later connection fields. Text
entity decoding for retained fields still works. The regression suite covers
both that retired field and an unrelated ignored field, followed by a readable
server address, description containing `&amp;`, and port.

### XML parsing correction for the foundation

Investigation reproduced a shared parser defect: while building the element
tree, a scan from `&` to the next `;` can consume closing tags and subsequent
settings inside an ignored field. The existing registration exception avoided
it only for that particular tag name. The test with an unrelated ignored URL
field fails against the original parser because the following address is no
longer a child of the connection. The baseline parser is identical to `main`.

Removed this unnecessary entity scan from structural parsing. Entity decoding
belongs to the existing node-text reader; it does not need to run while finding
element boundaries. This is a targeted compatibility correction, not a claim
that the parser now validates arbitrary malformed XML.

Prepared [xml-ignored-field-entities.patch](upstream/xml-ignored-field-entities.patch)
against foundation `7fe9620`. It contains only that structural correction and the
regression tests. Foundation registration UI, configuration and URL-text behavior
are preserved. The patch passes `git apply --check` against the clean foundation
worktree; an isolated copy of its patched parser and tests passes with the
foundation's headers/string/memory implementation. It has not been applied or
committed to `main`.

Validation on 2026-11-01:

- The three changed C units compiled and the release executable relinked:
  `build/purist-no-registration-button-build.log`.
- Regression failure before the parser correction:
  `build/purist-registration-xml-before.log`.
- Existing empty/nested XML checks and new ignored-URL/entity-text cases pass:
  `build/purist-registration-xml-tests.log`.
- Isolated foundation-patch checks pass:
  `build/foundation-xml-ignored-fields-tests.log`.
- Source-reference and whitespace checks passed. No live login UI or installed
  clientinfo file was modified or tested.

Backup: `build/before-registration-button-removal`. Isolated diff:
`build/purist-no-registration-button.diff`. Purist changes remain uncommitted;
the foundation worktree and installed game/server files remain unchanged.

## Cash-shop Charge webpage button (START-082, removed)

The cash shop creates a Charge button using `btn_charge_*` images and caption
`0xce3`. Its command handler opens `msg_str(0xce5)` by launching `iexplore.exe`.
There is no game-connection purchase or points request in this handler. The local
runtime message table uses the caption "Charging" and points to an rAthena
donations page; other data sets can configure another URL.

Patrick approved removing this button and its webpage launch. Removed the button
creation, `btn_charge_*` resource lookups, command branch, control ID and the two
message constants. Item browsing, cart purchases, point balances, refresh and
banner behavior are unchanged. The shell include remains necessary for the
separate banner link. The former Charge-button area is unused; the other controls
retain their positions.

Validation on 2026-11-01:

- `src/ui/shops/cash_shop.c` compiled and the release executable relinked:
  `build/purist-no-cash-charge-button-build.log`.
- Reviewed the isolated diff: only button creation, its command branch and three
  exclusive constants changed. No Charge-button references remain in source:
  `build/purist-no-cash-charge-button.diff`.
- Whitespace checks passed. No live cash-shop UI, purchase or browser launch was
  performed.

Backup: `build/before-cash-charge-button-removal`. This remains an uncommitted
purist policy change; foundation and game/server installations are unchanged.

## Cash-shop promotional banners (START-083, removed)

Startup loads `Lua Files\\datainfo\\tb_cashshop_banner`. The shop's banner-load
handler invokes `set_cashshop_banner`, whose `add_cashshop_banner` callback supplies
local image filenames and URLs. The banner child window draws images from the skin's
`cashshop` directory, creates selector buttons and advances roughly every five
seconds. Hovering shows a hand cursor; clicking launches `iexplore.exe` with the
current banner URL. Images are loaded through ordinary local resources, not
downloaded by this feature.

Patrick approved removing the banners entirely. Removed the child window,
constructor/destructor and callbacks, image/URL storage, rotation, selectors,
mouse cursor/click handling and the remaining cash-shop shell launch. Removed
the shop's banner field, creation/process hooks and load-message branch, plus
the startup Lua file load, registered Lua callback, public declaration and runtime
type registration. The shop's private structure is now `0xe0` bytes rather than
`0xe4`; its offset assertions and local allocation use the new size. Other files
access it through the existing opaque window API.

The item catalog, cart, purchases and point balances remain. The former banner
area is empty; other shop controls and background layout retain their positions.
Shared Lua, radio-button and window machinery remain in use. Historical type
fixtures retain the old banner identity to verify that it is now rejected.

Validation on 2026-11-01:

- Four changed C units compiled and the release executable relinked:
  `build/purist-no-cash-banners-build.log`.
- Existing type-query checks passed with the banner added to the retired list:
  562 active fixture classes, 49 retired identities, 3,431 prior-runtime
  comparisons and 256 accepted casts; guarded/null/unknown-object checks passed:
  `build/purist-no-cash-banners-types.log`.
- An isolated source audit confirms 29 retained shop functions are unchanged,
  including cart purchase and drag handling. Only four retained shop functions
  changed to remove banner ownership/lifecycle/message handling. The attendance
  Lua callback and every other startup Lua load are unchanged:
  `build/purist-no-cash-banners-audit.log`.
- No retired cash-shop banner references remain in production source or the
  executable's link map. Whitespace checks passed. No live shop UI or purchase
  was exercised.

Backup: `build/before-cash-banners-removal`. Isolated diff:
`build/purist-no-cash-banners.diff`. Changes remain uncommitted on purist;
foundation and game/server installations are unchanged.

## Attendance-window banners (START-084, removed)

Startup loads `Lua Files\\datainfo\\tb_checkAttendance_banner`. The attendance
state handler calls `set_checkAttendance_banner`; its callback supplies local
image names, optional URLs and in-game window targets. The banner child draws
those images and advances roughly every five seconds, with selector buttons and
a hand cursor. A nonempty URL launches Internet Explorer. With an empty URL, the
click handler can open another in-game window, including a cash-shop request
through the mode handler.

The attendance window already has layouts and background images for both with-
banner and without-banner states. Its calendar, item details and check-in reward
request are separate from the promotional strip.

Patrick approved removing the banners entirely. Removed the child window and
callbacks, image/URL/target storage, selectors, rotation and both click actions.
Removed the owner field, startup Lua load, registered callback, declarations,
type registration, `StdBanner38` and its exclusive free helper/header. Achievement
sorting in `quests_std.c` and the other quest panes remain.

The calendar always uses its existing compact `0x1e8` by `0x19d` layout and
`attendance_bg.bmp`. Removed the banner-dependent vertical offsets consistently
from text, item icons, overlays and slot hit testing. The private attendance
window is now `0xac` bytes, with the close button at `0xa8`. Its own factory uses
the new size; other source files use the opaque window API. Calendar input,
tooltips/item details, blinking, claim-packet handling and attendance-state
updates remain.

### Single-banner rotation correction for the foundation

The selector builder creates no buttons when there are zero or one banners, but
the original rotation callback reads a selector after five seconds regardless.
A test using the exact production rotation/message callbacks reproduced an
invalid selector read with one banner. Returning early when the count is at most
one fixes that path; multiple banners retain their existing timing, selection
updates and wraparound. This guard is unnecessary in purist because the entire
feature is removed.

Prepared [attendance-banner-rotation.patch](upstream/attendance-banner-rotation.patch)
against foundation `7fe9620`, containing the guard, source-extracted regression
tests and a standalone test runner. It preserves the foundation's attendance UI
and replay callbacks. `git apply --check` passes against clean `main`, and the
patched callbacks pass in an isolated fixture using foundation headers. The patch
has not been applied or committed there.

Validation on 2026-11-01:

- Six changed C units compiled and the release executable relinked:
  `build/purist-no-attendance-banners-build.log`.
- Existing type-query tests passed with 561 active fixture classes, 50 retired
  identities, 3,423 prior-runtime comparisons and 256 accepted casts:
  `build/purist-no-attendance-banners-types.log`.
- Source audit confirms nine retained attendance functions are unchanged,
  including mouse handlers, tooltips/item details and blinking. Claim-packet
  handling and state updates are unchanged; overlay/hit testing use the same
  coordinates as the original no-banner layout:
  `build/purist-no-attendance-banners-audit.log`.
- The original single-banner rotation fails in
  `build/attendance-banner-probe/before.log`. Zero/single-banner and retained
  multi-banner timing/wraparound checks pass after the guard, including the
  independent foundation fixture: `build/foundation-attendance-banner-tests.log`.
- No attendance-banner references remain in production source or the link map.
  Whitespace checks passed. No live attendance window or reward claim was tested.

Backup: `build/before-attendance-banners-removal`. Isolated diff:
`build/purist-no-attendance-banners.diff`. Purist changes remain uncommitted;
foundation and game/server installations are unchanged.

## Authentication message-box website button (START-085, removed)

Message-box type 5 (`MSGBOX_OK_LINK`) creates an OK button plus a clickable text
button whose caption and target both use `msg_str(0xd75)`. Clicking the extra link
calls `ShellExecuteA`; it does not itself close the client. Current direct callers
are three authentication-result message-box sites in `src/login/auth.c`.

The local runtime message table sets `0xd75` to `http://rathena.org`; other data
sets can supply another address. This extra control is separate from inline URL
tags handled by the shared text viewers.

Patrick approved removing the extra website link. Removed the type-5 creation
branch, link constants and text-button shell handler. The three authentication
call sites now request type 0, the existing ordinary OK dialog. All other
authentication code, message text, dialog flags and subsequent error handling
are unchanged. The message box directly includes the renderer declaration it
still uses instead of the application header used by the removed shell call.

Validation on 2026-11-01:

- Both changed C units compiled and the release executable relinked:
  `build/purist-no-message-box-link-build.log`.
- Isolated source comparison verifies the authentication file differs only in
  the three dialog-type arguments. Direct callers, the five local wrapper
  families and message-box constants were audited; no remaining type-5 request
  was found. The modeless wrapper has no production callers.
- Removed link symbols and the shell call are absent from the message box;
  isolated whitespace checks passed:
  `build/purist-no-message-box-link-audit.log`.
- No live authentication dialog was exercised.

Backup: `build/before-message-box-link-removal`. Isolated diff:
`build/purist-no-message-box-link.diff`. Changes remain uncommitted on purist.
This policy removal introduces no separate foundation fix.

## Inline website links in text viewers (START-086, kept)

The shared ordinary and tagged text viewers turn markup such as
`<URL>Website<INFO>https://example.com</INFO></URL>` into a clickable label.
Clicking passes the target to `ShellExecuteA` to open through the Windows shell.
The tagged viewer is used by NPC dialog (`say_dialog.c`) and quest panes
(`quest_panes.c`). These are the two remaining shell-launch call sites found in
production source after START-085.

The ordinary viewer creates these controls in `add_url_links`; the tagged
viewer receives them from the shared line builder in `line_info.c`. Both also
handle separate in-game links for navigation, item details, tips and quests.
The ordinary viewer already unwraps URL markup to readable text underneath its
clickable controls. Removing the tagged viewer's clickable controls would also
require preserving their visible labels in the line builder.

The recommendation was to remove website launching and retain readable labels.
Patrick instead chose to keep clickable website links. Both implementations,
markup parsing and the other in-game link kinds remain unchanged. This retained
shell behavior motivated keeping COM and correcting cleanup under START-075.

Since 2026-11-02, both viewers open a link only when its target is an
`http://` or `https://` URL; files, UNC paths and other protocol handlers
stay unopened, and their labels remain visible. Fixed on `main` in
`5238a9f021` and on `purist` in `d80fc8010d`.

## Shared audio backend (START-087, kept)

`sound_mgr_create` constructs the shared Miles audio manager. When sound is
enabled, it opens the selected output driver and allocates reusable effect
voices. The existing settings choose ordinary or positional audio. Music uses
a streamed file with loop and volume controls; effects use wave resources and
can take world position/distance. The manager also owns music fades, stream
synchronization and shutdown of handles/provider/driver.

With sound disabled, the manager can exist without opening an audio driver.
If manager creation fails, `WinMain` currently displays `Audio System Init Failed`
and stops startup. Whether to allow silent startup after an audio-device failure
is separate from keeping playback support.

Patrick approved keeping the shared music/effects playback machinery and
reviewing policies separately. Audio source and settings remain unchanged;
no rebuild or new tests are needed for this keep decision.

## Audio initialization failure policy (START-088, kept)

`WinMain` currently treats a false result from `sound_mgr_create` as fatal to
startup: it shows `Audio System Init Failed`, balances COM initialization, and
returns before creating the main window. The audio manager reports failure for
allocation/event/cleanup-registration errors, an output driver that cannot open,
or a selected positional provider/listener that cannot open. Partial audio
resources are released by its failure path. A missing music/effect file later
during gameplay is separate and does not follow this startup branch.

Playback, volume and cleanup APIs guard an absent manager, but simply ignoring
the return value is insufficient: absent-manager volume getters return zero,
and `session_option_save.c` reads them when saving options. A silent fallback
must preserve the configured preference state as well as avoid invalid handles.
Missing provider enumeration and partial sample allocation are separate cases:
they currently return success and are not covered by this startup error notice.

The recommendation was to warn and continue without sound for that run, retaining
saved preferences. Patrick instead chose to keep stopping startup on audio
failure. The existing notice, return path and balanced COM cleanup remain
unchanged. No silent fallback was implemented. No rebuild or new tests are
needed for this keep decision.

## Ineffective saved audio-provider name (START-089, removed)

`read_startup_settings` resets `g_sound_provider_name` and loads the registry's
`PROVIDERNAME` string into that 1,024-byte buffer. In positional mode,
`open_manager` immediately replaces its contents with the literal
`Miles Fast 2D Positional Audio`, then enumerates providers to find that name.
Disabled and ordinary audio do not consult the saved string. The source audit
found no other production consumers of the global buffer.

Patrick approved removing this unused setting. Removed the registry read/reset,
1,024-byte global buffer and public declaration. Provider enumeration compares
against the same literal name directly. The registry reader's scratch buffer
now uses the largest remaining value, the 40-byte device name; its capacity,
type and size guards remain. Updated the existing settings fixture to remove
only the retired field and its expectations.

Validation on 2026-11-01:

- Changed `startup_settings.c` and `sound.c` compiled and the release executable
  relinked: `build/purist-no-provider-name-build.log`.
- Existing startup-settings checks passed for saved values, missing/inaccessible
  keys, malformed types/sizes, partial failures, bounded strings and handle
  ownership: `build/purist-no-provider-name-settings.log`.
- Existing sound-manager tests passed, including positional-provider selection,
  playback requests, failure rollback, fades and joined shutdown:
  `build/purist-no-provider-name-sound.log`.
- Isolated source audit confirms all other sound implementation is unchanged.
  No retired symbol or registry-read references remain in production source or
  tests, and the executable link map has no retired global. Whitespace checks
  passed: `build/purist-no-provider-name-audit.log`.
- No installed registry values were changed; no live-client playback was tested.

Backup: `build/before-provider-name-removal`. Isolated source/test diff:
`build/purist-no-provider-name.diff`. Changes remain uncommitted on purist.
This removal introduces no separate foundation correctness fix.

## Default audio mode (START-090, changed to positional)

Before this change, `read_startup_settings` initialized `g_soundMode` to zero
before attempting to read `SOUNDMODE`. A missing/inaccessible registry key or an
unreadable/malformed value therefore left audio disabled. The setting distinguishes disabled (0),
ordinary (1) and positional (2) playback. The positional path opens a stereo
driver and the existing Miles positional provider.

The in-game BGM/effect checkboxes update playback flags; they do not initialize
an output driver or change `g_soundMode`. Thus a fresh configuration with no
saved sound mode stays silent even if those checkboxes are enabled.

Patrick approved positional audio as the fallback. The only production change
is assigning 2 instead of 0 before the existing registry read. Saved settings
still override that fallback, including an explicit disabled mode. The registry
reader retains its existing type/size checks; this change does not introduce
numeric range validation. Other format/voice settings and the policy of stopping
startup on audio failure remain unchanged.

Validation on 2026-11-01:

- Updated the existing settings fixture's default and explicitly exercised saved
  modes 0, 1 and 2. Saved mode 1 makes later fallback checks distinguishable from
  the new default. Missing/inaccessible keys, empty keys and malformed/failed
  individual reads all pass: `build/purist-positional-audio-default-settings.log`.
- The changed C unit compiled and the release executable relinked:
  `build/purist-positional-audio-default-build.log`.
- Isolated comparison confirms the one-line production change; whitespace
  checks passed: `build/purist-positional-audio-default-audit.log`.
- No installed registry values changed and no live-client audio was played.

Backup: `build/before-positional-audio-default`. Isolated source/test diff:
`build/purist-positional-audio-default.diff`. Uncommitted purist policy change;
no separate foundation correctness fix was introduced.

## Legacy audio format choices (START-091, fixed format)

Startup reads `DIGITALRATETYPE` and `DIGITALBITSTYPE`. The sound manager maps the
rate choice to 22,050, 11,025 or 8,000 Hz, and the bit-depth choice to 16 or 8 bits
when opening the output driver. Both settings default to zero, selecting
22,050 Hz and 16-bit. The in-game audio controls do not edit these registry values.

Patrick approved fixed 22,050 Hz, 16-bit output. Removed both registry reads,
default assignments, global selectors and declarations. The driver call now
uses those values directly; its ordinary/positional channel choice is unchanged.
Adjusted existing test fixtures to remove the retired fields and verify the
fixed format at the actual backend-call boundary. The sound test still covers
all three audio modes and all retained effect-voice count choices.

Validation on 2026-11-01:

- Changed C units compiled and the release executable relinked:
  `build/purist-fixed-audio-format-build.log`.
- Existing settings tests passed: `build/purist-fixed-audio-format-settings.log`.
- Existing sound-manager tests passed, including positional playback requests,
  volume, fades, provider enumeration and failure cleanup:
  `build/purist-fixed-audio-format-sound.log`.
- Isolated comparison confirms the sound source differs only in format
  selection. Retired symbols/reads are absent from source/tests and the link
  map; whitespace checks pass: `build/purist-fixed-audio-format-audit.log`.
- No installed registry values changed; no live-client audio was played.

Backup: `build/before-fixed-audio-format`. Isolated source/test diff:
`build/purist-fixed-audio-format.diff`. Uncommitted purist policy change; no
separate foundation correctness fix introduced.

## World-sound distance and server visibility (START-092, deferred)

Patrick reported that increasing server visibility exposes distant monsters
whose positional sounds are not heard. The observation was in SpringRO, with
the exact client version, distance and server setting unavailable. That is a
lead for checking this client, not proof of identical SpringRO implementation.

The diagnosing-bugs workflow was used to build a focused range probe under
`build/audio-range-probe`. It compiles the actual `render_object_process_sound`
and `motion_sound` code with the actual sound manager, using the existing
controlled Miles/resource backend and one synthetic animation event. Each
distance starts with fresh voices; the production process function advances
the frame and dispatches the sound. No game installation is modified or played.
Initial harness link errors were resolved with the missing dependency stubs
and `winmm` link input before collecting the following results.

Command: `build/check_audio_range_probe.bat`. Output:
`build/audio-range-probe/result.log`. The diagnostic deliberately returns 1
when a sound 300 world units away lies outside the animation caller's requested
range; it is not part of the normal regression suite.

| Distance (world units) | Ordinary playback starts | Positional playback request | Requested full/max distances |
| --- | --- | --- | --- |
| 40 | Yes, volume 100 | Yes | 40 / 250 |
| 100 | Yes, volume 40 | Yes | 40 / 250 |
| 249 | Yes, volume 16 | Yes | 40 / 250 |
| 250 | No | Yes, at requested maximum | 40 / 250 |
| 300 | No | Yes, beyond requested maximum | 40 / 250 |

The ranked checks were fixed attenuation radius, an earlier animation-event
cutoff, and exhausted voices. The range sweep preserves the animation event;
fresh voices eliminate contention in this probe. A control changes only the
requested maximum from 250 to 500 at distance 300: ordinary playback starts
(volume 13), and positional playback receives maximum 500 with full distance 40.
This isolates the caller's range contract for the tested animation path.

`src/resources/attr.c` initializes cell zoom to 5; `game/cell_coordinates.c`
converts each cell to that many world units. The usual 40/250 limits therefore
correspond to 8/50 cells. Animation sounds and many actor/effect callers pass
these literals; their sound-distance parameters do not follow server visibility.
The ordinary path explicitly rejects distance >= maximum. The positional path
delegates attenuation using those bounds to Miles. The probe observes requests,
not physical output from the real Miles DLL, and does not reproduce SpringRO's
unknown live scenario. Other effect-specific range checks remain separate.

The initial recommendation was to centralize world-sound distance and expose a
configurable maximum. Patrick questioned why positional audio needs a fixed cap
instead of dynamically becoming quieter with distance. He has not approved a
range change.

Clarification: fixed distance parameters define a changing attenuation curve;
they do not mean volume is fixed. The ordinary path uses full volume through
40 world units, then scales by 40/distance, and abruptly rejects new playback at
250 units. For example, ignoring integer rounding and source volume scaling,
16 cells gives 50% volume, 32 cells gives 25%, and 50 cells is rejected.

The earlier user-facing wording overstated the positional finding: we verified
the maximum-distance parameter passed to Miles, not a hard mute at that distance
inside the real provider. The diagnostic's "covered" label and failure at 300
units describe being beyond that requested parameter only. They must not be
treated as proof of physical silence. A maximum-distance parameter does not
universally mean muting: [Microsoft's DirectSound documentation](https://learn.microsoft.com/en-us/previous-versions/windows/desktop/ee418662(v=vs.85))
distinguishes attenuation clamping from optional maximum-distance muting. That
documentation explains the distinction, not the exact behavior of the Miles
Fast 2D provider used here.

Revised design direction to discuss: smooth attenuation, with inaudible sounds
culled and limited voices managed separately from server visibility. A quiet
monster and a loud explosion need not have identical audible reach. The exact
reason for the original 40/250 constants is not established. No production
attenuation/cutoff change is authorized by this clarification; real-provider
behavior remains an investigation step before claiming a positional fix.

Patrick then asked to move on with pruning. Preserve the existing distance
behavior and these findings for later; no range redesign is currently underway.

## Simultaneous sound-effect limit (START-093, selector removed)

Startup reads `NUMSAMPLETYPE`: 0 requests 48 effect handles, 1 requests 32, and
other values request 16. The default is 2, hence 16. The manager's arrays already
hold 48 slots for ordinary and positional effects. Allocation stops when the
backend cannot provide another handle. Playback searches allocated idle handles
and skips a new effect when none is available. Streamed background music uses
its own stream rather than one of these effect handles.

Recommendation: remove the registry selector/default/global and its declaration,
and request up to the existing 48-slot capacity. Preserve partial-allocation
handling and all playback/cleanup behavior. This does not promise 48 simultaneous
positional sounds: the earlier real-DLL probe supplied 48 ordinary handles but
only 16 positional handles even when 48 were requested. Evidence is recorded in
`build/com-dll-investigation/none-audio-1-0.log` and `none-audio-2-0.log`.

Patrick approved removing the selector and requesting up to 48 channels.
Removed the registry read/default, global and public declaration. Removed the
now-redundant private `sample_count` field and replaced its allocation/playback
loop bounds with the existing `SOUND_SAMPLES` constant. The manager is defined
and allocated entirely in `sound.c`; no public structure layout changed.
Allocation still stops at the first unavailable handle, playback still skips
null/busy handles, and cleanup visits only allocated handles.

Updated the existing settings fixture to drop the removed field. The sound
suite now varies backend availability through 0, 16, 32 and 48 handles for each
audio mode, verifying allocation counts and corresponding releases. Existing
small partial allocations, positional playback, volume, fades and failure
rollback checks remain.

Validation on 2026-11-01:

- Changed units compiled and the release executable relinked:
  `build/purist-no-sound-channel-selector-build.log`.
- Settings and sound-manager tests passed:
  `build/purist-no-sound-channel-selector-settings.log` and
  `build/purist-no-sound-channel-selector-sound.log`.
- Isolated comparison confirms the sound changes are limited to selector/count
  removal and loop bounds. No retired setting/global remains in source/tests or
  the link map. Whitespace checks passed:
  `build/purist-no-sound-channel-selector-audit.log`.
- No installed registry values or game installations changed; no live-client
  playback was tested. Streamed music and distance behavior are unchanged.

Backup: `build/before-sound-channel-selector-removal`. Isolated diff:
`build/purist-no-sound-channel-selector.diff`. Changes remain uncommitted on
purist; no separate foundation correctness fix was introduced. How to prioritize
or replace sounds when all channels are busy remains unreviewed.

## Saved speaker-layout preference (START-094, kept)

`read_startup_settings` initializes `g_sound_speaker_type` to 0 and attempts to
read `SPEAKERTYPE` from the client registry key. When positional audio opens its
provider, it calls `_AIL_3D_speaker_type`; if the result is not -1, it supplies
the saved value through `_AIL_set_3D_speaker_type`. This forwards the requested
speaker arrangement to the provider rather than selecting a different provider.
Ordinary and disabled audio do not follow this path. The current in-game audio
controls do not edit this startup setting.

Patrick approved keeping the saved preference while retaining this positional
backend, so supported speaker layouts can honor existing configuration. Exact
layout support is provider-specific and has not been compared through live
playback. No code changes, rebuild or additional tests were needed for this
keep decision.

## Button-sounds-only mode (START-095, removed)

The `/tingonly` command is registered as type `0xc1` and reaches
`talk_cmd_sound`, which toggles `g_sound_settings.button_sound_only` and prints
an enabled/disabled notice. The sound manager then rejects sampled sounds unless
their resource name exactly matches the existing button-ding WAV. The earlier
master effects-enabled check still applies; the mode does not enable muted audio.
Background music uses a separate stream and is unaffected by this filter.

When a non-owner is added to a chat room's member list, the window requests that
button ding only when effects and this mode are both enabled. Normal UI button
sounds use the same resource through their own entry points. The mode starts
off; enabling effects through `/sound` clears it, as does the corresponding
settings-reset path when effects are enabled.

Recommendation: remove `/tingonly`, its flag/filter/toggle notices and the
chat-room entry alert that exists only for this mode. Preserve ordinary button
sounds, independent music/effect toggles and volume controls.

Patrick approved removal. Removed the command registration and dispatch case,
toggle branch and notices, `SoundSettings.button_sound_only`, the sampled-sound
filter, its reset assignments and the conditional chat-room join ding. Ordinary
member-list updates, button sounds, music/effect toggles and volume controls stay.
The two default settings initializers retain the equivalent values for the
remaining fields. The existing sound test now verifies ordinary and button WAV
playback without the retired filter.

Validation on 2026-11-01:

- Rebuilt all 11 changed/dependent units, including every consumer of the
  shifted `SoundSettings` fields, and relinked the release executable:
  `build/purist-no-tingonly-build.log`; unit list:
  `build/tingonly-rebuild-units.txt`.
- Existing sound-manager and talk-option storage/real-Lua suites passed:
  `build/purist-no-tingonly-sound.log` and
  `build/purist-no-tingonly-talk.log`. The latter tests option storage, not the
  removed command or live chat-room behavior.
- No `tingonly` or `button_sound_only` references remain in source/tests.
  Isolated source comparison and whitespace checks passed:
  `build/purist-no-tingonly-audit.log`.
- No live chat-room/audio session was exercised. Installed games and registry
  values were not changed. No separate foundation correctness fix arose here.

Backup: `build/before-tingonly-removal`. Isolated diff:
`build/purist-no-tingonly.diff`. Changes remain uncommitted on purist.

## Server-requested NPC music (START-096, kept and playback state corrected)

`HEADER_ZC_PLAY_NPC_BGM` (`0x07fe`) reaches `game_mode_zc_play_npc_bgm`.
If music is enabled, the handler constructs `bgm\<name>.mp3` from the server's
track name, requests playback with loop count 1 and sets `bgm_overridden`.
These are local music assets; the packet does not download audio. Normal map
music is selected separately through the map-to-MP3 lookup on map entry.

Patrick approved keeping this support and investigating the apparent early
handoff. The initial source reading was incomplete: the production header
defined `MSS_STREAM_PLAYING` as 2, while the shipped Miles DLL reports 4 during
playback and 2 on completion. The wrong constant and reversed `restart_bgm`
condition canceled each other for ordinary track completion. An early cutoff
was therefore NOT established for the actual DLL; changing only one of these
two lines would introduce it.

Confirmed with Miles 6.1a (`client-files/mss32.dll`, SHA-256
`D0DB426EBAC97DC410B5130C9C74D19BC04151236DF85B012823D6CB3D7FB87C`): a generated
half-second silent WAV, played at volume zero, reports 4 while running and 2
after completion. Loop count 1 finishes once (about 563 ms in this run); loop
count 0 still plays at 1.8 seconds. Probe source and output:
`build/npc_bgm_miles_probe.c`, `build/check_npc_bgm_miles.bat`,
`build/npc-bgm-miles-status.log`. The probe opens an audio device but makes no
audible sound and changes no installed assets or saved settings.

Corrected `MSS_STREAM_PLAYING` to 4 and changed `restart_bgm` to wait while the
stream is playing. This makes the status helper truthful, preserves the normal
event-track-to-map-track handoff, and restores map music when the event file
cannot be opened. Previously that no-stream case left the override pending with
no automatic map-music restoration. Updated the existing Miles test stub to
return the measured playing value rather than repeat the incorrect constant.

`tests/build_npc_bgm.bat` runs a focused regression using the exact production
packet handler, stream operations and restoration helper extracted by
`tests/extract_npc_bgm.py`, plus the real string implementation. Only hardware
and map lookup are substituted. Its fake backend uses independent measured
values 4/2, so it cannot silently agree with a wrong production constant.
It checks playing/completed status, several updates during the event, one-time
restoration, no override, music disabled, disabling music during an event and
failure to open the event track.

Validation on 2026-11-01:

- The regression failed four checks before the fix and passes afterward, both
  on purist and the pinned foundation source. Logs:
  `build/purist-npc-bgm-before.log`, `build/purist-npc-bgm-tests.log`,
  `build/foundation-npc-bgm-before.log`, `build/foundation-npc-bgm-tests.log`.
- Existing sound-manager suites passed on both versions:
  `build/purist-npc-bgm-sound.log`, `build/foundation-npc-bgm-sound.log`.
- Rebuilt the changed game unit and both Miles-header consumers, then relinked
  the release executable: `build/purist-npc-bgm-build.log`.
- Whitespace and isolated change checks passed. No full live-client/server
  event or audible music playback was tested.

Foundation patch: `docs/upstream/npc-bgm-playback-state.patch`, based on
`54ca05e3785e1ab2771362b150f8050c2a832b9a`, with two production-line corrections,
the stub correction and regression files. `git apply --check` passed against
the foundation checkout; the patch remains unapplied and that checkout is
unchanged. Snapshot and patch preparation are recorded under
`build/foundation-npc-bgm` and `build/prepare_npc_bgm_foundation.py`.
Purist backup: `build/before-npc-bgm-fix`; isolated diff:
`build/purist-npc-bgm.diff`. No commit or push was made.

## Server-requested WAV effects (START-097, kept)

`HEADER_ZC_SOUND` (`0x01d3`) supplies a 24-byte local WAV name, an action, repeat
interval and optional NPC actor ID. The once action plays at the NPC's relative
position when it is found, otherwise at the listener. Repeat queues the name,
interval and NPC ID, and plays an initial centered sound. Later updates replay
it at the interval: attached sounds follow the NPC when it exists, while sounds
without an NPC ID play centered. Stop removes the first queued matching name;
it does not interrupt a sample already playing. The game-mode destructor frees
the queue. Sample playback follows the shared effects toggle and volume.

Patrick approved keeping this support for scripted sound cues and ambient
effects. These requests use local assets through the game connection. Existing
distance/attenuation policy remains deferred under START-092. No code changes or
additional tests were needed for this keep decision.

## Music volume fades (START-098, kept)

The shared mode transition routines call `sound_bgm_fade_out` as the screen
fades out, and `sound_bgm_fade_in` when the next fade-in begins. The sound manager
uses a cancellable worker to change music volume one unit per approximately
10 ms: down toward zero or up toward the selected `bgm_fade_target`. Duration
therefore depends on the current/target volume and scheduling; it is not tied
exactly to the visual fade's 255 ms. A replacement fade cancels and joins the
previous worker; shutdown does the same before closing the stream. Sampled
effects are separate from these music-volume changes.

Patrick approved keeping this support for smoother music-volume transitions.
The existing sound-manager suite exercises fades, cancellation and shutdown and
passed during START-096. No code changes or additional tests were needed for
this keep decision.

## Unused audio-output flag (START-099, removed)

`g_sound_use_waveout` is a file-scope integer in `sound.c`, initialized to zero
by static storage. Its only other source reference passes it to
`_AIL_set_preference(15, ...)` before opening the digital driver. It has no
header declaration, assignments, registry reader or UI control in the client.
Consequently this startup request always supplies zero. Historical standalone
DLL probes under `build` changed the variable directly; those probes are not
client configuration or production callers.

Recommendation: remove this unused state and pass zero directly to the same
Miles preference call. This preserves the current output request, audio modes,
speaker layout and device-opening failure policy. The exact interpretation of
preference 15 is not needed to establish that equivalence.

Patrick approved removal. Deleted the global and replaced its one read with
literal zero. No other production code or tests changed. Historical DLL probes
under `build` retain their original source as evidence and are not part of the
client build.

Validation on 2026-11-01:

- Recompiled `sound.c` and relinked the release executable:
  `build/purist-no-audio-output-flag-build.log`.
- Existing sound-manager tests passed, including the assertion that preference
  15 receives zero: `build/purist-no-audio-output-flag-sound.log`.
- Exact isolated comparison confirms only the global removal and argument
  replacement. No retired symbol remains in source/tests or the release link
  map; whitespace checks passed:
  `build/purist-no-audio-output-flag-audit.log`.

Backup: `build/before-audio-output-flag-removal`. Isolated diff:
`build/purist-no-audio-output-flag.diff`. No installed settings changed or live
audio was exercised. Changes remain uncommitted on purist; no independent
foundation correctness fix arose from this cleanup.

## Main item-information catalog (START-100, kept)

The first operation in `session_startup` loads `System\\iteminfo_true.lub`.
The Lua script registers entries through `AddItem`, description callbacks,
`AddItemIsCostume` and `AddItemEffectInfo`. Entries contain identified and
unidentified names, artwork resource names and description lines, along with
slot count, view class, costume flag and effect ID. The item table imports these
definitions for names, icons, descriptions and appearance lookup; other consumers
read costume/effect information from the manager. At initial review the Lua
import was conditional on neither `item.txt` nor `eventItem.txt` being found;
START-101 subsequently removed that alternate selection. Other overlays remain
for separate review. This is the main Lua presentation
catalog, separate from the internal-name-to-item-ID lookup previously removed
after its only users (reforming and enchanting) were pruned.

Patrick approved keeping this shared support for ordinary and custom items. It lets
server data assign item IDs while local data supplies their names and visual
presentation. Additional item tables and specialized item features remain for
individual review. No code changes or additional tests were needed for this
keep decision.

## Legacy item-file selection (START-101, removed)

Previously, `item_tables_load` looked for `item.txt` and `eventItem.txt` through
the normal data-file resolver. For each existing file it invoked
`load_item_file`, which parsed identified/unidentified names and descriptions,
then set `haveItemFile` unconditionally, even if no records were loaded. Either
file's presence skipped the branch containing `item_table_apply_lua` and the
four `Event*` name/description table loads. The Lua catalog itself was still
loaded earlier; this switch prevented importing it into the shared item table.

Recommendation: remove these two legacy file checks, their parser and the
selection flag, and always use the existing Lua-import branch. This avoids a
leftover old-format file silently bypassing the catalog Patrick kept. The
separate `Event*` tables, item-slot/parameter tables, card tables and other
support files remain unchanged for individual review.

Patrick approved removal. Removed the two checks, `load_item_file`, its error
message and include, and the selection state. The existing branch containing
the four supplemental text-table loads followed by the Lua import now runs
unconditionally. Removed `item_table_use_identified_description`, its declaration
and the test stub because the retired parser was its only production caller.
The parser was also the only caller of `TABLE_RECORDS`; removed that enum member
and the null-separator insertion branch from the shared table reader. The two
remaining modes retain their original numeric values and structure layouts.

Updated existing tests to drop the retired description-copy/record-mode checks.
Preserved table-reader growth and borrowed-buffer lifetime coverage using
20,000 ordinary fields, and retained independent identified/unidentified
description storage and unknown-item copying checks.

Validation on 2026-11-01:

- Rebuilt the three changed C units and four other direct consumers of the
  edited headers, then relinked the release executable:
  `build/purist-no-legacy-item-loader-build.log`.
- Existing real-Lua item-definition/ownership tests and message/table-reader
  tests passed: `build/purist-no-legacy-item-loader-tests.log`. These exercise
  Lua definitions and table storage/parsing, not a live client loading assets.
- Isolated source comparison confirms the old Lua branch is now unconditional
  and all other item loaders retain their order and implementation. Retired
  symbols/resource references are absent from source/tests; whitespace checks
  passed: `build/purist-no-legacy-item-loader-audit.log`.

Backup: `build/before-legacy-item-loader-removal`. Isolated diff:
`build/purist-no-legacy-item-loader.diff`. No game-data files were changed and
no live client session was exercised. Changes remain uncommitted on purist;
this policy removal does not introduce a separate foundation patch.

## Supplemental event item names and descriptions (START-102, removed)

Before this removal, `item_tables_load` read these optional text tables before Lua:

- `EventNum2ItemDisplayNameTable.txt` and `EventNum2itemDescTable.txt` for
  unidentified names and descriptions.
- `EventIdNum2ItemDisplayNameTable.txt` and `EventIdNum2itemDescTable.txt` for
  identified names and descriptions.

They could create entries absent from Lua. For matching item IDs, Lua replaced
the text-table names but appended its description lines after the existing
text-table lines, so supplemental descriptions could appear before the Lua text.
These files were read through the normal data resolver rather than downloaded.

Recommendation: remove these four supplemental loaders and keep item names and
descriptions in `iteminfo_true.lub`. Resource-name overrides run afterward and
remain separate, as do card and other item tables.

Patrick approved removal. Removed the four startup calls and the private
`load_display_names` / `load_descriptions` functions, their two unused includes
and corresponding unused test stubs. Shared description storage and Lua import
remain. The existing item-definition test no longer injects an event-table line
before importing Lua; it now verifies the identified/unidentified descriptions
from its Lua fixture directly, retaining the ownership and reload checks.

Validation on 2026-11-01:

- Recompiled `item_loader.c` and relinked the release executable:
  `build/purist-no-event-item-text-build.log`.
- Existing item-definition/real-Lua and message/table-reader suites passed:
  `build/purist-no-event-item-text-tests.log`.
- Exact source comparison confirms only the two loaders, four calls and unused
  includes changed in production. Retired references are absent from source
  and tests; all other loaders and their order remain unchanged. Whitespace
  checks passed: `build/purist-no-event-item-text-audit.log`.

Backup: `build/before-event-item-text-removal`. Isolated diff:
`build/purist-no-event-item-text.diff`. No game-data files changed or live client
session was exercised. Changes remain uncommitted on purist; no separate
foundation correctness fix arose from this policy removal.

## Item artwork-name overrides (START-103, removed)

Before removal, after importing the Lua catalog, `load_res_names` read
`EventNum2ItemResNameTable.txt` for unidentified resources and
`EventIdNum2ItemResNameTable.txt` for identified resources. Each ID/name pair
replaced the corresponding item-table resource name, including names already
supplied by Lua. The file could also create a new table entry with only artwork
information. These were filename mappings, not artwork downloads.

`session_get_item_bitmap_file_name` uses the selected resource name for the
item icon BMP. `session_get_item_display_name` supplies the item resource path
used for dropped-item SPR/ACT pairs and drag artwork. This does not replace the
item's display name, equipped view class or separate card illustration mapping.

Recommendation: remove the two overrides and their private loader; configure
identified/unidentified resource names in `iteminfo_true.lub`. General resource
filename redirects retained under START-012 are separate.

Patrick approved removal. Deleted `load_res_names`, its two calls and the
now-unused file-existence stub in `message_tables_support.c`. The item table
continues to import Lua resource names, and all shared icon/sprite resolution,
card data and other item tables remain unchanged.

Validation on 2026-11-01:

- Recompiled `item_loader.c` and relinked the release executable:
  `build/purist-no-item-artwork-override-build.log`.
- Existing item-definition/real-Lua tests passed, including the identified and
  unidentified resource-name assertions. Message/table-reader tests also passed:
  `build/purist-no-item-artwork-override-tests.log`.
- Exact source comparison confirms only the private loader and its two calls
  changed in production. Retired references are absent from source/tests and
  whitespace checks passed: `build/purist-no-item-artwork-override-audit.log`.

Backup: `build/before-item-artwork-override-removal`. Isolated diff:
`build/purist-no-item-artwork-override.diff`. No game-data files changed or live
rendering session was exercised. Changes remain uncommitted on purist; no
separate foundation correctness patch arose from this policy removal.

## NPC-shop ATK/DEF estimate bar (START-104, removed)

Before removal, `itemSlotTable.txt` populated `slotTableValue`. Despite the
name, this field was an equipment-location mask used by the old stat-estimate
window, not the Lua card-slot count. `itemParamTable.txt` supplied numeric pairs
stored in each item's `params`. Their only gameplay reader was
`UIItemParamChangeDisplayWnd` in `src/ui/items/param_change.c`.

Opening the NPC purchase window (`0x17`) also opened this 280-by-34-pixel bar
(`0x32`) through `make_window_trade.c`. It displayed two current/after values
with up/down/same markers. Message IDs `0x10d` and `0x10e` label them; the local
`client-files/data/msgstringtable.txt` resolves these to `ATK` and `DEF`.

The window totaled pairs from eight equipped-item positions, used the masks to
avoid counting overlapping locations, and estimated the result of substituting
the inspected item for a currently worn counterpart. Item inspection and
equipment changes refreshed it. This was local table arithmetic, not a request to
the server to calculate the character's resulting combat stats. It is separate
from the normal item-description window and the side-by-side comparison opened
through `itemCompare` / `WND_ITEM_COLLECTION_CMP`.

Recommendation: remove this optional estimate bar, its startup tables and
exclusive stored fields/helpers. Keep NPC purchases, ordinary item information
and side-by-side comparison.

Patrick approved removal. Deleted the window module, factory/type registration,
creation/closing/refresh hooks, window-manager pointer and saved-layout state,
and registry read/write hooks. Removed the two table loads, `slotTableValue`,
`ItemParameters`, parameter allocation/copy/cleanup and append helper. The
retained Lua `slotCount` and equipped view class are unchanged. Window IDs for
other features were not renumbered, and existing registry values were not edited.

Existing item tests now exercise table growth through retained view-class data
and fallback ownership through description arrays instead of the removed
parameter arrays. The original type-reference fixture remains historical evidence;
the object-type test now verifies that identity 354 is rejected as retired.

Validation on 2026-11-01:

- Rebuilt all **1,175 client C units** to account for the changed shared item
  and window-manager layouts, and relinked the release executable:
  `build/purist-no-item-estimate-bar-build.log`.
- Existing real-Lua item-definition/ownership and table-reader suites passed:
  `build/purist-no-item-estimate-bar-items.log`.
- Object-type tests passed for 560 retained classes and 51 retired identities,
  with 3,415 prior-runtime comparisons; window-reference cleanup tests passed:
  `build/purist-no-item-estimate-bar-types.log`.
- Isolated checks confirm NPC purchase creation, equipment handling and
  side-by-side comparison changed only by removal of estimate-bar hooks. Card
  and remaining item loaders are unchanged. Retired references are absent from
  production source/link symbols; whitespace checks passed:
  `build/purist-no-item-estimate-bar-audit.log`.

Backup: `build/before-item-estimate-bar-removal`. File inventory:
`build/item-estimate-bar-files.txt`. Isolated diff:
`build/purist-no-item-estimate-bar.diff`. No live shop session was tested and no
installed assets or registry values changed. Changes remain uncommitted on
purist; this policy removal introduces no separate foundation patch.

## Shared card recognition (START-105)

Decision: **keep**, approved 2026-11-01.

`CardItemNameTable.txt` adds IDs to the shared card catalog. Subsequent card
illustration, prefix and postfix loaders also create entries as they attach
their metadata. `card_affix_has` tests membership independently of whether a
name or illustration was supplied.

Item display code uses membership to recognize cards in the item's four slot
fields, with separate handling for named/forged and other special items. It
also participates in item-name coloring, filters card-only searches and controls
card-specific display behavior such as the illustration button. These are
client presentation/lookup decisions, not the server's card bonus or slotting
rules.

The catalog and recognition support remain unchanged. Card illustrations and
prefix/postfix naming are reviewed separately. This keep decision changes only
the review ledger; no additional build or tests were needed.

## Card illustrations (START-106)

Decision: **keep**, approved 2026-11-01.

`Num2CardIllustNameTable.txt` maps card IDs to artwork basenames. The View button
in a card's item-information window opens a separate draggable, closable window
that displays the card's item name and static BMP illustration. The resource
path is `UI_SKIN "cardBmp\\"` followed by the mapped name and `.bmp`, loaded
through the ordinary resource manager. No HTTP request or download is involved.

The viewer is implemented in `src/ui/items/illustrations.c`, alongside a
separate generic illustration window. Its callers are in
`src/ui/items/item_collection.c` and `item_collection_text.c`; the mapping loads
in `src/iteminfo/item_loader.c`. Card recognition remains independently approved
under START-105, with card prefix/postfix naming kept under START-107.

The viewer and its artwork mapping remain unchanged. This keep decision changes
only the review ledger; no additional build or tests were needed. No live viewer
test was performed.

## Card prefix/suffix naming (START-107)

Decision: **keep**, approved 2026-11-01.

`CardPrefixNameTable.txt` maps card IDs to the names used on equipment containing
those cards. `CardPostfixNameTable.txt` lists which IDs should use that text after
the equipment name instead of before it; it does not contain a second set of
name strings.

For identified equipment with recognized cards, the displayed name combines
refinement, card prefixes, the base equipment name, card suffixes and slot count.
Repeated copies of the same card share one affix with a localized multiplicity
label (two, three or four copies). The name search also accepts card prefixes
and suffixes as aliases for the card's own name.

The loaders are in `src/iteminfo/item_loader.c`; composition is in
`src/iteminfo/item_name.c`, and alias matching is in
`src/iteminfo/item_search.c`. This is display/search support, separate from
server-side card bonuses and slotting rules.

The naming tables, equipment-name composition and search aliases remain
unchanged. This keep decision changes only the review ledger; no additional
build or tests were needed. No live display/search test was performed.

## Readable books (START-108)

Decision: **remove**, approved and implemented 2026-11-01. Patrick chose removal
instead of the initial keep recommendation. The former behavior is described
below for review history.

`BookItemNameTable.txt` marks item IDs that expose book buttons in the item
information window when the character carries the item. The ordinary reading
button opens `book/<itemID>.txt` through the existing resource loader, with
regional/language path handling. The title comes from the item's name. The
reader wraps text, displays 19 lines per page, and supplies previous/next and
bookmark controls. An optional leading `%RRGGBB` line chooses the background
color.

Bookmarks are saved in the Windows registry beneath the configured client key,
server name and character name, keyed by book title. The close button also saves
the current page. Server packet `0x0294` (`ZC_READ_BOOK`) can open a book by ID at
a requested page, independently of the item-description button/catalog check.
The packet does not transfer book text.

The same window also has a separate automatic display mode: it hides the paged
reader and shows successive lines in local chat and an on-screen text bubble
roughly every four seconds. This is text display, not recorded or synthesized
speech. This mode was removed together with the reader.

Sources: `src/iteminfo/item_loader.c`, `src/ui/items/item_collection.c`,
`src/ui/items/item_collection_text.c`, `src/ui/items/book_wnd.c`,
`src/ui/items/book_reading.c`, and `src/game/ui_open.c`.

Removed the reader and automatic display modules/header, bookmark registry
access, both item-description buttons and their animation callback/state,
the book-ID catalog/load/init/cleanup, and the inventory-check helper whose only
caller was the book-button code. Removed the window factory, type registration,
manager pointer and lifecycle/input hooks, and the book-open packet handler,
dispatch case and declarations. Item-description variants now use the existing
empty base processing callback. The card illustration button and other item
display behavior remain.

Packet `0x0294` retains its ten-byte framing and diagnostic name in the shared
packet table so incoming data can still be consumed; it follows the existing
unused-packet dispatch path. Ordinary item data, book weapons, card artwork and
naming/search, crafting, and NPC purchases are not removed. Existing installed
book assets and saved registry values were not modified.

Removed the exclusive book tests and their build steps, preserving the shared
plain-line/text-format tests. Updated existing item-table coverage to remove
book-only checks, retired type identity 220 in the type regression, and added
`0x0294` to the existing retired-packet checks. The historical type fixture stays
as rejection evidence.

Validation:

- Rebuilt all **1,173 client C units** after changing shared window/item-display
  layouts and relinked the release executable:
  `build/purist-no-readable-books-build.log`.
- Existing item/Lua and table-reader suites passed:
  `build/purist-no-readable-books-items.log`.
- Type regression passed for 559 retained classes and 52 retired identities,
  with 3,407 prior-runtime comparisons; window cleanup passed:
  `build/purist-no-readable-books-types.log`.
- Compared all **1,179,800 before/after packet cases**: only the six valid
  book-open cases changed, one per payload pattern. Every other trace matched.
  Updated the expected digest only after that comparison; the suite passed:
  `build/purist-no-readable-books-packets.log` and
  `build/readable-books-packet-comparison.txt`.
- Retained plain-line and text-format suites passed, including 455,175 SDK
  character pairs and 5,000 generated cases:
  `build/purist-no-readable-books-lines.log` and
  `build/purist-no-readable-books-text.log`.
- Source, protected-file hashes, link inputs/symbols and whitespace checks
  passed: `build/purist-no-readable-books-audit.log`.

Backup: `build/before-readable-books-removal`. Inventory:
`build/readable-books-files.txt`. Isolated diff:
`build/purist-no-readable-books.diff`. No live game session was tested. Changes
remain uncommitted on purist; this policy removal introduces no foundation patch.

## Forging-window instructions (START-109)

Decision: **keep**, approved 2026-11-01.

`MetalProcessItemList.txt` is a local text catalog keyed by target item ID.
Startup reads numeric IDs followed by text lines. The sole production reader,
`ui_make_target_process_wnd_on_draw` in `src/ui/items/make_target.c`, displays the
matching lines below the chosen item's headings in the classic crafting
confirmation window. Authors can use these lines for instructions or material
requirements.

The lines do not select recipes, validate/consume materials, or calculate success;
the window's selection/request behavior and server results are separate. Missing
entries simply supply no additional text. The loader/storage live in
`src/iteminfo/item_loader.c` and `src/iteminfo/item_tables.c`.

The instruction catalog and display remain unchanged; crafting behavior will be
reviewed separately. This keep decision changes only the review ledger; no
additional build or tests were needed. No live forging-window test was performed.

## Item restriction notices (START-110)

Decision: **keep**, approved 2026-11-01. Tooltip behavior remains; verified
loader correctness fixes were implemented and prepared separately for the
foundation.

Startup loads `ItemMoveInfoV5.txt` as item IDs with eight numeric flags. For an
item listed in the table, its information window adds an "Item move restrictions"
label. Hovering opens a tooltip that lists restrictions marked by the local
flags. The current UI displays seven categories: dropping, personal storage,
cart, mail attachments, player trading, guild storage and NPC sales. The eighth
flag (index 6, auctioning) is stored but not displayed by this tooltip.

The category meanings were checked against the message IDs in
`src/ui/items/move_info_tooltip.c` and the local runtime message table. All
production users of `item_move_info_get` are item-information layout or tooltip
code. These entries do not block actions or change server permissions, and they
are not synchronized from the server. The catalog must match the server to give
accurate notices. Missing entries do not expose a restriction label.

Sources: `src/iteminfo/item_loader.c`, `src/iteminfo/item_tables.c`,
`src/ui/items/item_collection.c`, `src/ui/items/item_collection_text.c`,
`src/ui/items/item_move_info.c`, and `src/ui/items/move_info_tooltip.c`.

The local catalog and tooltip remain. Server permission enforcement is unchanged.
The missing-file popup is retained under START-111.

### Verified loader correction

A guarded regression executes the actual `read_int_file` function and its
startup record-consumption loop, extracted directly from the source, using the
real vector implementation. Before the fix, `501 1 0 1 0 1 0 1 0\n` parsed only
eight integers: the parser emitted values only at spaces/tabs, discarding the
last value at a newline or end-of-file. The consumer still tried to read an ID
and eight flags. The shorter input `501 1 ` independently reproduced the same
buffer overread from an incomplete record. Repeating the newline case reproduced
the failure; a trailing-space version parsed nine integers correctly. Allocation
sizes matched parsed counts, ruling out vector shrinking as the cause.

`read_int_file` now reads numeric values across ordinary whitespace, final EOF
and line comments, without generating zero values from repeated separators.
It rejects invalid/out-of-range integers, negative file sizes and failed reads,
clearing partial parsed output. Startup processes only complete nine-integer
records; an incomplete trailing group is not consumed. Existing table storage,
tooltip flag meanings, other item loaders and the missing-file warning are
unchanged.

The regression covers legacy trailing-space input, LF/CRLF, tabs/repeated
whitespace, EOF, comments, complete and incomplete groups, malformed/overflow
values, integer limits, failed/missing resources, localization and cleanup. It
is integrated into `tests/build.bat` via `tests/build_item_restrictions.bat`.

Validation:

- Purist pre-fix failures: `build/item-restrictions-newline-before.log`,
  `build/item-restrictions-newline-repeat.log`, and
  `build/item-restrictions-incomplete-before.log`. The isolated parser probe is
  under `build/`, with results in `build/item-restrictions-parse-probe.log`.
- New regression passed: `build/purist-item-restrictions-tests.log`.
- The same newline failure reproduced on the pinned foundation source, and the
  corrected snapshot passed the full regression:
  `build/foundation-item-restrictions-before.log` and
  `build/foundation-item-restrictions-tests.log`.
- Existing item/Lua and table-reader suites passed for both versions:
  `build/purist-item-restrictions-items.log` and
  `build/foundation-item-restrictions-items.log`.
- Recompiled the changed client unit and relinked the release executable:
  `build/purist-item-restrictions-build.log`.
- Source-scope checks, identical foundation/purist fixes and regressions,
  foundation patch apply-check and whitespace checks passed:
  `build/purist-item-restrictions-audit.log`.

Independent foundation patch:
[item-restriction-loader.patch](upstream/item-restriction-loader.patch), based on
`54ca05e3785e1ab2771362b150f8050c2a832b9a`. It contains the loader correction and
regression/build integration, without purist feature removals. The actual
foundation checkout was not modified. Evidence snapshots and preparation script:
`build/foundation-item-restrictions` and
`build/prepare_item_restrictions_foundation.py`. Purist backup:
`build/before-item-restriction-loader-fix`; isolated diff:
`build/purist-item-restrictions.diff`. No live tooltip test, commit or push was
performed. The regression's parser-to-startup seam now catches both dropped
values and incomplete-record traversal.

## Missing restriction-catalog popup (START-111)

Decision: **keep**, approved 2026-11-01. Patrick chose to retain the warning
instead of the initial recommendation to remove it.

If `ItemMoveInfoV5.txt` cannot be opened, the numeric reader displays a generic
"Can't open file" message without identifying the file, then returns failure.
Startup continues without adding restriction notices from that file. This
reader's sole production caller is the restriction-catalog loader.

The popup and continuation behavior remain unchanged. The loader regression
added under START-110 already verifies this warning on a missing resource. This
keep decision changes only the review ledger; no additional build or tests were
needed.

## Illustrated Tip Box guide (START-112)

Decision: **remove**, approved 2026-11-01.

The removed initializer in `session_startup` called `tipbox_load`. It loaded
`System/tipbox.lua`, falling back to `.lub`, into a local guide catalog. Topics
contain titles, text pages, bitmap or sprite illustrations, and optional animated
annotations. The guide window supports title search, page navigation and links
to other topics. Its initial topic is selected randomly from listed entries with
IDs below 1000.

Entry points include a menu icon, a configurable shortcut, a chat-command
dispatch entry, `<TIPBOX>` text links, quest links and server UI-open type 5
with a topic ID. On initial map-server entry, the client can open the guide
according to `SHOWTIPSATSTARTUP`; the default is enabled. The guide's checkbox
saves that preference. Startup script failures can produce both a script error
and the generic "TipBox file Init" popup.

This is separate from ordinary item/skill/status hover tooltips. The older
`UITipOfTheDayWnd` and its text-file catalog are also separate, although they
share the startup-display preference and save helper. Review those separately;
do not remove shared state while a retained caller still uses it.

Sources: `src/session/session_startup.c`, `src/ui/tips/tipbox_mgr.c`,
`src/ui/tips/tipbox.c`, `src/ui/tips/tipbox_picture.c`,
`src/game/mode_initialize.c`, `src/game/ui_open.c`,
`src/ui/controls/text_viewer_links.c`, and `src/ui/quests/tag_text_viewer.c`.

Removed the guide catalog, window, picture/annotation rendering and their
startup/shutdown hooks. Deleted its menu icon, `/tip` and Korean-keyboard alias,
window factory/slot/type registration, keyboard action and first-map opening.
The hotkey settings filter now also hides index 54 (`0x36`) in tab 2; the local
`HotKey_v2.lua` identifies this as `MSI_HK_TIPBOXWND_ONOFF` and maps it to action
154 (`0x9a`). Other action IDs and saved binding indices are unchanged.

Server UI-open type 5 no longer opens a guide; the shared packet handler and
other types remain. Removed guide-link parsing/actions from both text viewers.
Existing `<TIPBOX>` labels still display as plain text with their `<INFO>`
arguments hidden. The line-layout regression verifies that these labels do not
create buttons, including Korean labels between active URL and quest links.
Ordinary hover tooltips and URL/item/navigation/quest links remain supported.

The separate old Tip of the Day window remains pending START-113. Its registry
save helper was moved intact into its own source file and made private; the
startup-display setting and registry reader remain for that decision. Six
exclusive guide source/header/test files were deleted. The type-query fixture
retains the historical guide identity to assert that it is rejected now.

Validation:

- Rebuilt all 1,170 current client C sources and linked the release executable:
  `build/purist-no-tipbox-build.log`. Compile/link response files contain no
  removed guide sources or objects.
- Line layout passed 2,240 layout/style/link/placement cases, 97,656 escape
  combinations and the new plain-guide-label assertions. Tagged text actions,
  readable links, text markup, markup helpers and plain-text fitting passed:
  `build/purist-no-tipbox-build_line_layout.log`,
  `build/purist-no-tipbox-build_tag_text_viewer.log`,
  `build/purist-no-tipbox-build_readable_links.log`,
  `build/purist-no-tipbox-build_text_markup.log`,
  `build/purist-no-tipbox-build_markup_helpers.log`, and
  `build/purist-no-tipbox-build_fit_plain_text.log`.
- User-hotkey ownership/Lua tests passed:
  `build/purist-no-tipbox-build_user_hotkeys.log`.
- Type queries passed for 558 concrete classes and 53 retired identities,
  including 3,399 prior-runtime comparisons and 253 accepted casts; window
  cleanup passed: `build/purist-no-tipbox-check_readable_books_types.log`.
- Source/reference, compile/link inventory, whitespace and unrelated-change
  checks passed: `build/purist-no-tipbox-audit.log`.

Backup: `build/before-tipbox-removal`; inventory: `build/tipbox-files.txt`;
isolated diff: `build/purist-no-tipbox.diff`; audit helper:
`build/audit_tipbox.py`. No live game/UI test, asset edits, commit or push was
performed. This is a purist policy removal, with no foundation patch; the
foundation checkout remains clean and unchanged.

## Legacy Tip of the Day (START-113)

Decision: **remove**, approved 2026-11-01.

This older, text-only window (`0x26`) picks an initial random tip and offers
previous/next buttons. Depending on the service/language setting it reads loose
`tipOfTheday.txt`, `tipoftheday.txt` or `tipOfTheday_Eng.txt`, or a language-selected
`tipofday.txt` resource. Its checkbox reads and writes `SHOWTIPSATSTARTUP`.

The illustrated guide removed under START-112 contained the only automatic
opening check found for this preference. The old window's factory and cleanup
still exist, but no dedicated opening caller was found. Removing the old guide
would also allow removal of the remaining preference and registry handling.
The text-file parser is shared with the separate guild-help window's
`GuildTip.txt`, so that machinery must remain until its own review.

Sources: `src/ui/tips/tip_of_the_day.c`, `src/contents/tips.c`,
`src/core/startup_settings.c`, `src/core/client_init.c`, and
`src/ui/wndmgr/make_window_hud.c`.

Removed the window implementation and its now-unused header, creation/close
slots, teardown and forget-window entries, type registration, and the basic
panel's entry for closing this window. Removed `g_tipOfTheDay`, its lazy-read/get
accessors and exit cleanup. The separate `GuildTip.txt` parser, loading, copying
and cleanup behavior remain byte-for-byte unchanged.

Removed `g_showTipsAtStartup`, its initialization/normalization and registry
read, and the save helper that was temporarily retained under START-112. The
client no longer reads or writes `SHOWTIPSATSTARTUP`. Existing external files
and registry values were not edited. Other startup preferences retain their
existing defaults and saved-value handling.

Updated the existing startup-settings and tip-file regressions to cover their
remaining behavior, and added the old window identity to the type-query test's
retired list. Historical fixture metadata remains as evidence that the old type
is now rejected.

Validation:

- Startup settings passed saved/default values, missing/inaccessible keys,
  malformed types/sizes, partial failures, bounded strings and key ownership:
  `build/purist-no-tip-of-day-build_startup_settings.log`.
- Guild-tip lazy loading, absent/failed resources, deep copies and nested cleanup
  passed: `build/purist-no-tip-of-day-check_remaining_guild_tips.log`.
- Type queries passed for 557 concrete classes and 54 retired identities,
  including 3,391 prior-runtime comparisons and 252 accepted casts; window
  cleanup passed: `build/purist-no-tip-of-day-check_readable_books_types.log`.
- Ordinary tooltip creation/reuse, styling, positioning, expiry and storage
  checks passed: `build/purist-no-tip-of-day-build_mode_tooltip.log`.
- Rebuilt all 1,169 current client C sources and linked the release executable:
  `build/purist-no-tip-of-day-build.log`. Compile/link response files contain no
  retired Tip of the Day source/object.
- Source/reference, unchanged guild parser, compile/link inventory, whitespace
  and unrelated-change checks passed: `build/purist-no-tip-of-day-audit.log`.

Backup: `build/before-tip-of-day-removal`; inventory:
`build/tip-of-day-files.txt`; isolated diff: `build/purist-no-tip-of-day.diff`;
source-scope audit helper: `build/audit_tip_of_day.py`. This is a purist policy
removal, with no foundation patch. No live game/UI test, asset edit, commit or
push was performed.

## Guild help text (START-114)

Decision: **remove**, approved 2026-11-01.

Opening the guild interface without a guild shows a small panel with Help,
Create Guild and OK buttons. Help toggles window `0xd6`, which loads
`GuildTip.txt` from disk for service type 0, or a language-selected game resource
for other service types. It displays the file's text in a scrollable viewer with
a Close button. This is a local manual; it does not create a guild or handle
membership, permissions, guild skills or server messages.

The loader/parser and `TipFile` storage remaining after START-113 now serve only
this guild manual. The Create Guild action and the shared helper/create/disband
window code in `guild_helper.c` must remain when removing the help text.

Sources: `src/ui/social/guild_helper.c`, `src/ui/wndmgr/make_window_social.c`,
`src/ui/character/menu_icon_wnd.c`, `src/ui/wndmgr/key_down.c`, and
`src/contents/tips.c`.

Removed the manual window implementation, its type/factory/close registration,
manager slot and cleanup references. Removed Help from the panel and its command
handler; Create Guild now occupies the left button position and OK remains on
the right. The Create Guild action and the complete create/disband window
implementation are unchanged. Guild membership, management and server traffic
are outside this removal.

Removed the now-exclusive `contents/tips.c` and `.h`, catalog state and exit
cleanup, plus the obsolete tip-file regression and support file/build commands.
No local/GRF assets were edited. The historical guild-tip type fixture is kept
to check that the removed window is rejected.

Validation:

- Guild state, owned containers, member updates, teardown and all 13 member
  sorting orders passed: `build/purist-no-guild-tips-check_guild_state.log`.
- Guild/quest queues, retry timing, maps, values and failure cleanup passed with
  digest `1210ed3a`: `build/purist-no-guild-tips-build_guild_quest_owners.log`.
- Type queries passed for 556 concrete classes and 55 retired identities,
  including 3,383 prior-runtime comparisons and 251 accepted casts; window
  cleanup passed: `build/purist-no-guild-tips-check_readable_books_types.log`.
- Rebuilt all 1,168 current client C sources and linked the release executable:
  `build/purist-no-guild-tips-build.log`. Compile/link response files contain no
  removed tip-file loader.
- Source/reference, unchanged create/disband code, compile/link inventory,
  whitespace and unrelated-change checks passed:
  `build/purist-no-guild-tips-audit.log`.

Backup: `build/before-guild-tips-removal`; inventory:
`build/guild-tips-files.txt`; isolated diff: `build/purist-no-guild-tips.diff`;
source-scope audit helper: `build/audit_guild_tips.py`. This is a purist policy
removal, with no foundation patch. No live game/UI test, asset edit, commit or
push was performed.

## Unused random-value bookkeeping (START-115)

Decision: **remove**, approved 2026-11-01.

After seeding the random generator, startup calls `session_reroll_values`.
The same helper runs during map entry and HP/SP recovery/effect notifications.
It writes five random indices into `rerollValues`, zeroes twelve `rerollZeros`
entries, and consumes extra `rand()` results. On its first call it additionally
fills the random-index array once before overwriting it.

No readers of either array were found. The helper saves HP/max HP/SP/max SP and
passes the identical values to a private setter; since those fields were not
changed, the setter returns without updating stats or refreshing the UI.
Recovery handlers calculate HP/SP changes and refresh the display separately.
Adjacent `vital_slot_tables` storage also has no named users and can be checked
for removal with the obsolete bookkeeping if this decision is approved.

Sources: `src/session/session_state.c`, `src/session/state.h`,
`src/session/session_startup.c`, `src/game/mode_initialize.c`,
`src/game/effect_notify.c`, and `src/game/packets_recovery.c`.

Removed `session_reroll_values`, its declaration, private `rand12`/`set_watched`
helpers and first-call flag, and all five calls in startup, map initialization,
healing effects and recovery packets. Removed the two arrays and adjacent
`vital_slot_tables` block after confirming no named or matching literal-offset
users: 260 bytes of unused session fields in total. The session's other fields
remain unchanged and all consumers are rebuilt for the new layout.

The random seed remains. Removing these discarded draws changes subsequent
client-side random sequences; it does not change server stat calculations.
Source comparison confirms that recovery arithmetic, HP/SP fields, conditional
UI refreshes, displayed healing numbers and effect calls are identical outside
the removed helper calls. No packet layout or dispatch logic was changed.

Validation:

- Session field checks passed bounded character arrays, names, packed hotkeys
  and six 64-bit costs: `build/purist-no-reroll-build_session_fields.log`.
- Session scalar checks passed bank balances/failures, companion fields/names,
  redraw ordering and timer wraparound:
  `build/purist-no-reroll-build_session_scalar.log`.
- The existing emblem-cache test, which builds `session_state.c`, passed:
  `build/purist-no-reroll-build_emblem_cache.log`.
- Packet dispatch passed all 1,179,800 cases with the unchanged digest
  `0d286968203f186b`: `build/purist-no-reroll-build_packet_dispatch.log`.
- Rebuilt all 1,168 client C sources and linked the release executable:
  `build/purist-no-reroll-build.log`.
- Source/reference, unchanged healing logic, session-field scope, random seed,
  whitespace and unrelated-change checks passed:
  `build/purist-no-reroll-audit.log`.

Backup: `build/before-reroll-removal`; inventory: `build/reroll-files.txt`;
isolated diff: `build/purist-no-reroll.diff`; audit helper:
`build/audit_reroll.py`. This is a purist cleanup with no foundation patch.
No live healing/gameplay test, asset edit, commit or push was performed.

## Unused table-initialization counter (START-116)

Decision: **remove**, approved 2026-11-01.

`session_init_table` increments the global `g_initTableRuns` before loading
sound names and other session tables. A source/test search found only the
counter's definition and increment: it is never read, displayed or logged.

Source: `src/session/session_tables.c`.

Removed the counter definition and increment. All table-loading calls, their
order and other initialization behavior are unchanged.

Validation: rebuilt `session_tables.c` with the release flags and successfully
relinked the executable using the previously rebuilt current source objects:
`build/purist-no-init-counter-build.log`. Source comparison verifies that only
the definition and increment were removed; source/tests and the fresh link map
contain no counter reference. Whitespace checks passed:
`build/purist-no-init-counter-audit.log`. No new tests were needed for this
unused scalar removal.

Backup: `build/before-init-counter-removal`; isolated diff:
`build/purist-no-init-counter.diff`; build helper:
`build/relink_init_counter_removal.bat`. No foundation patch, live game change,
commit or push was made.

## Player hit-impact sounds (START-117)

Decision: **keep**, approved 2026-11-01.

`session_init_job_hit_wave_name` initializes three filenames:
`player_clothes.wav`, `player_wooden_male.wav` and `player_metal.wav`. When a
player character is struck, the actor's hit handler can choose among these using
a job-ID mapping and play the result at that actor's position. This simulates
different impact materials by class; it does not inspect the equipment's actual
material or calculate damage.

Other hit paths use the weapon-hit sound catalog or generic randomized hit
sounds. Those are separate from this initializer and can be reviewed later.

Sources: `src/session/job_sounds.c`, `src/session/job_sounds.h`,
`src/session/session_tables.c`, and `src/actor/actor_sendmsg.c`.

This combat sound feedback remains unchanged. This keep decision changes only
the review ledger; no additional build or tests were needed. The weapon-hit
catalog remains for its later review in `session_init_table`.

## Map background-music catalog (START-118)

Decision: **keep**, approved 2026-11-01.

`session_load_mp3_names` loads pairs from `mp3NameTable.txt` through the shared
resource-table reader and sorts them by map name. `session_get_mp3_name` looks
up the current map, with an adjustment for instance-style names, and returns
its configured local music path. These names are kept valid by the reader's
retained text blocks until session cleanup.

Game-mode initialization plays the selected map track. The NPC-music handoff
also uses this lookup to restore map music once an event track finishes; the
handoff correction was already handled under START-096. The shared audio
manager honors the music toggle and loops map music. No download is involved.

At this review point, maps without entries fell back to `bgm\01.mp3`, including
when catalog data was absent. That separate policy was subsequently replaced by
silence under START-119; the catalog and per-map selection remain.

Sources: `src/session/session_tables.c`, `src/session/session_names.c`,
`src/core/table_reader.c`, `src/game/mode_init.c`, `src/game/mode_run.c`, and
`src/contents/sound.c`.

The configurable map-music catalog and selection remain unchanged. This keep
decision changes only the review ledger; no additional build or tests were
needed. Review the missing-entry fallback under START-119.

## Missing map-music fallback (START-119)

Decision: **use silence for unmapped maps**, approved 2026-11-01.

Previously, if a map did not match `mp3NameTable.txt`, `session_get_mp3_name`
returned the hardcoded `bgm\01.mp3` path. The same happened when the catalog was
missing or unreadable. Both map entry and restoration after an NPC track use this lookup.
An entry that names a missing audio file does not trigger this fallback: the
audio backend simply cannot open that configured file.

Removed the hardcoded map fallback. The lookup now returns NULL for an unmapped
map. Map entry and NPC-track restoration pass that result directly to
`sound_play_bgm`, avoiding the old non-null string copies. A NULL or empty map
track now closes playback and frees the remembered stream/map names while
holding the audio lock. Clearing needs no allocation and works while muted or
with sound mode disabled. Resume/toggle calls therefore have no old map track
to restart. Configured tracks retain their loop and mute behavior; allocation
failure while selecting a nonempty track still preserves the previous selection.

NPC event music can play on a silent map, then return to silence when it finishes.
The separate login-screen request for `bgm\01.mp3` remains unchanged and can be
reviewed separately. The catalog loader, NPC packet handler and saved audio
settings were not changed.

Sources: `src/session/session_names.c`, `src/session/session_tables.c`,
`src/game/mode_init.c`, `src/game/mode_run.c`, and `src/contents/sound.c`.

Extended the existing NPC-music regression to execute the production catalog
lookup and map-entry helper as well as the packet handler, sound operations and
completion handoff. Its simulated backend opens no device/window/server. Before
the change, ten checks failed: unmapped maps selected track 01, and empty map
entries left the previous song active. After the change, it verifies missing
entries, an empty catalog, explicit empty tracks, stopping/resuming, muted map
entry, NPC completion on a silent map and returning to a configured map track.
The sound-manager regression also checks clearing during allocation failure,
empty-path clearing, ownership cleanup and resuming configured music.

Validation:

- Pre-change policy regression: `build/purist-silent-maps-before.log`.
- Expanded map/NPC-music regression passed: `build/purist-silent-maps-npc.log`.
- Existing sound-manager suite plus silence/ownership cases passed:
  `build/purist-silent-maps-sound.log`.
- Recompiled the four changed C units and relinked the release executable:
  `build/purist-silent-maps-build.log`. Header changes only document contracts;
  no shared layout or function signature changed.
- Source-scope, unchanged login/NPC/catalog/settings code, whitespace and
  unrelated-change checks passed: `build/purist-silent-maps-audit.log`.

Backup: `build/before-silent-maps`; inventory: `build/silent-maps-files.txt`;
isolated diff: `build/purist-silent-maps.diff`; audit helper:
`build/audit_silent_maps.py`. This is a purist policy change, with no foundation
patch. No live audio/gameplay test, installed asset edit, commit or push was made.

## Indoor-map camera rules (START-120)

Decision: **keep**, approved 2026-11-01.

`session_load_indoor_rsws` loads map names from `indoorRswTable.txt`. The lookup
also handles instance-style map names. Listed maps use the indoor camera policy:
an initial fixed heading, normally limited left/right rotation around that
heading, a narrower pitch range and the indoor zoom limit. Indoor and outdoor
pitch/distance preferences are stored separately. The camera is constrained,
not completely immobile.

Maps absent from the list use the ordinary outdoor policy. Separate per-map
camera presets and existing administrator options can override parts of these
limits; those are outside this catalog decision. This table controls camera
behavior rather than graphics rendering or map geometry.

Sources: `src/session/session_tables.c`, `src/session/session_names.c`,
`src/game/camera.c`, `src/game/mouse_camera.c`, `src/game/mode_exit.c`,
`src/session/session_option_load.c`, and `src/session/session_option_save.c`.

The configurable indoor-camera behavior and saved preferences remain unchanged.
This keep decision changes only the review ledger; no additional build or tests
were needed. Review per-map camera presets separately under START-121.

## Per-map camera presets (START-121)

Decision: **keep**, approved 2026-11-01.

`session_load_view_points` reads `ViewPointTable.txt`: a map name followed by
nine fields describing camera distance, rotation and tilt. The lookup handles
instance-style map names. When a matching preset exists, map entry applies its
starting distance, heading and pitch. The camera still follows the player.

The mouse controls use the preset's distance minimum plus allowed distance
range, rotation minimum/maximum and pitch minimum/maximum. These replace the
usual indoor/outdoor bounds. Equal limits can constrain an axis to a fixed
view. The normal double-right-click reset is disabled while a preset is active.
Without a matching preset, the ordinary indoor/outdoor camera policy remains.

Sources: `src/session/session_tables.c`, `src/session/session_names.c`,
`src/session/session_lifetime.c`, `src/game/camera.c`, and
`src/game/mouse_camera.c`.

The preset catalog, initial camera view and adjustment limits remain unchanged.
This keep decision changes only the review ledger; no additional build or tests
were needed.

## Skill visual-effect mappings (START-122)

Decision: **keep**, approved 2026-11-01.

`session_init_skill_effect_id_maps` fills two session lookup trees from built-in
skill/effect pairs. One selects effects launched during skill use; the other
selects hit effects. A missing key or a stored -1 means no effect from that
lookup. This is one part of skill presentation alongside separate skill-specific
code and Lua effect definitions.

Actor skill handling looks up these IDs when creating effects and preparing
timed hit messages. Skill effect objects also use the hit lookup for selected
skills. The tables select visual effects; they do not define damage, costs or
server gameplay rules. Removing them would lose effects from existing skills.

Sources: `src/session/session_init.c`, `src/session/session_tables.c`,
`src/session/session_names.c`, `src/session/session_lifetime.c`,
`src/actor/skill_checks.c`, and `src/effects/skills/skill_msg.c`.

The built-in mappings and their consumers remain unchanged. Individual skills
remain for separate review. This keep decision changes only the review ledger;
no additional build or tests were needed.

## Shared interface text catalog (START-123)

Decision: **keep**, approved 2026-11-01.

`session_reload_texts` begins by loading `msgStringTable.txt`. The message
reader owns the parsed strings; numbered lookups supply interface labels,
dialog text, errors and notices throughout the client. For example, the login
window uses this catalog for its saved-ID label and credential-length errors,
and chat-room setup uses it for public/private labels and refusal notices.

Reloading releases the previous reader and cached charset-tagged strings before
reading the catalog again. Missing fields return `NO MSG`; IDs outside the
supported range return a diagnostic string through `msg_str`. This review is
about keeping the shared catalog, with regional text handling and the later
word-filter/map-name/item reload steps left for separate review.

Sources: `src/core/msg_str.c`, `src/core/msg_str.h`,
`src/session/session_tables.c`, `src/ui/character/login.c`, and
`src/ui/chat/chat_room_setup.c`.

The shared catalog and message lookups remain unchanged. This keep decision
changes only the review ledger; no additional build or tests were needed.

## Local word blacklist (START-124)

Decision: **remove**, approved 2026-11-01.

Previously, `session_reload_texts` loaded `manner.txt` into `g_nameFilter`. The
matcher lowercased ASCII letters in a temporary copy, walked text using the
configured code page, and compared listed terms at each character position.
It could match inside longer words and skipped recognized `<ITEML>...</ITEML>`
spans.

Callers either masked matches with asterisks or rejected the submission. Masking
was used in outgoing and incoming chat, whispers, displayed names and other
social text. Rejection applied to character/companion naming, some shop/chat-room
titles, guild notices, RODEX title/body, NPC text input and party-recruitment
text, among other callers. When masking a match, the implementation also
lowercased ASCII letters throughout the original text.

This is a local policy driven by the shipped word list. Other checks, including
length, control characters, markup restrictions and chat-repeat handling, are
separate. Server moderation and name acceptance are outside this client change.
Other hardcoded naming restrictions also remain for separate review.

Sources: `src/contents/manner_words.c`, `src/contents/manner_filter.c`,
`src/session/session_tables.c`, `src/game/msg_chat.c`, `src/game/chat.c`,
`src/game/name_balloon.c`, `src/game/msg_rodex.c`,
`src/ui/character/make_char_new.c`, `src/ui/quests/npc_input.c`, and
`src/ui/social/party_register.c`; additional callers found across chat,
guild/party and shop UI/packet handlers.

Removed the loader, matching/masking implementation, global state and lifecycle
calls, every consumer's blacklist branch, exclusive wrappers and unused message
constants. Name balloons no longer copy an entire name card just to hold a
censored name; their bounded text-formatting helpers remain. Character creation
still checks control characters, now using a clearly named `control_chars_ok`
result. Length, whitespace, service-specific character rules, markup checks,
chat-repeat handling, blocked senders and hardcoded reserved-name rules remain.

Deleted the three exclusive source/header files and the two exclusive filter
test files. Updated existing balloon tests to expect intact names and casing,
and the RODEX test to verify unchanged title/body bytes instead of a local
blacklist refusal. Remaining text-consumer checks retain encoding, ellipsis and
allocation-failure coverage. Removed retired test commands and filter mocks.

Validation:

- Recompiled all 1,166 remaining C sources and linked the release executable.
- Byte-line and balloon tests passed, including intact party/guild/rank labels,
  formatting, emblems, ownership and cleanup.
- RODEX send checks passed for packet fields, terminators, short/long text,
  unchanged case and allocation cleanup.
- ASCII/Korean text-consumer checks passed for ellipsis, static labels,
  conversion-failure fallback and cleanup.
- Source audit found no word-filter code/build references, preserved unrelated
  work and confirmed separate validation/repeat/blocked-sender checks remain.
  Whitespace checks passed. The complete change covers 53 files, five deleted.

Logs: `build/purist-no-manner-build.log`,
`build/purist-no-manner-build_byte_lines.log`,
`build/purist-no-manner-build_rodex_send.log`,
`build/purist-no-manner-check_manner_text_consumers.log`, and
`build/purist-no-manner-audit.log`. Backup: `build/before-manner-removal`;
inventory: `build/manner-files.txt`; isolated diff: `build/purist-no-manner.diff`.
This is a purist policy change, with no foundation patch. No live gameplay test,
installed asset/server edit, commit or push was made.

## Pet dialogue (START-125)

Decision: **remove**, approved 2026-11-01.

The startup function previously loaded three separate XML catalogs. The first,
`PetTalkTable.xml`, supplied pet speech. `session_get_pet_speak` chose a random
matching line by pet type, hunger category and activity, such as feeding,
hunting, danger, death, standing, performance or the owner's level-up.

Local events requested encoded pet actions through the game connection. The
send path normally required intimacy of at least 900 for speech, with an
administrator exception. On a received speech action, the client looked up the
line, formatted the speaker name, showed the pet's speech balloon and added chat
text unless chat was hidden. Emotion-icon actions used a separate branch of the
same packet handler. The catalog did not define pet hunger, loyalty, feeding
or combat behavior.

Sources: `src/session/session_init.c`, `src/session/talk_tables.c`,
`src/session/pet.c`, `src/game/msg_misc.c`, `src/game/packets_pet.c`,
`src/game/packets_act.c`, `src/game/par_change.c`, and `src/actor/actor_state.c`.

Removed the pet XML load/ownership, random-line lookup, exclusive pet-type and
activity keys, encoded speech-ID builder, and Korean gender-word substitutions
used only by pet speech. The shared formatter retains its existing behavior for
monster speech, which always disabled those substitutions.

Removed speech requests paired with feeding, combat, low-HP, death, level-up,
idle and performance events. Performance animations and emotion requests remain.
The remaining probability/cooldown logic is unchanged; timers/helpers were
renamed to describe emotions. The session no longer owns the pet XML document;
the two monster documents still initialize and release normally.

The pet-action sender now accepts emotion actions only. The existing incoming
packet still has its original 10-byte framing and dispatch entry; emotion
actions reach the actor, while speech is ignored without mutating the received
packet. Pet care/commands, hunger/intimacy classification, emotion tables,
performance animations and monster catalogs remain.

Validation:

- The new focused regression extracts production sender/receiver code and the
  existing emotion table: all 175 hunger/intimacy/activity combinations passed,
  along with disabled emotions, missing actors, ignored speech/other action
  kinds and unchanged incoming packet bytes.
- Existing attack-record, session-field and emblem-cache checks passed.
- Source-scope audit confirms unrelated work is preserved, speech references
  are removed, and pet care, emotion selection and timing/probability remain.
- Recompiled all 1,166 C sources and linked the release executable after the
  session layout change. Whitespace checks passed; the change covers 24 files,
  including three new test files.

Logs: `build/purist-no-pet-dialogue-actions.log`,
`build/purist-no-pet-dialogue-build_attack_records.log`,
`build/purist-no-pet-dialogue-build_session_fields.log`,
`build/purist-no-pet-dialogue-build_emblem_cache.log`,
`build/purist-no-pet-dialogue-build.log`, and
`build/purist-no-pet-dialogue-audit.log`. Backup:
`build/before-pet-dialogue-removal`; inventory: `build/pet-dialogue-files.txt`;
isolated diff: `build/purist-no-pet-dialogue.diff`.
This is a purist policy change, with no foundation patch. No live gameplay test,
installed asset/server edit, commit or push was made.

## Monster skill animation catalog (START-126)

Decision: **keep**, approved 2026-11-01.

`MonsterSkillInfo.xml` maps a lowercased monster resource name and skill ID to
`action` and `loopType`. During monster casting and skill use, callers select
the corresponding sprite action and pass the looping choice to the renderer.
If no matching record is found, the lookup returns action -1, leaving callers
to their existing normal behavior.

This is animation configuration, separate from `MonsterTalkTable.xml` speech.
It does not control which skills a monster knows, when its AI chooses them,
damage or server outcomes. Some other call sites only look up the values; the
casting/use and targeted-skill paths actively apply them.

Sources: `src/session/session_init.c`, `src/session/talk_tables.c`,
`src/game/skill_cast.c`, `src/game/skill_use.c`, `src/game/skill_notify.c`, and
`src/game/ground_skills.c`.

The catalog, loader and animation consumers remain unchanged. This keep
decision changes only the review ledger; no additional build or tests were
needed.

## Local monster speech (START-127)

Decision: **remove**, approved 2026-11-01.

`MonsterTalkTable.xml` supplied local lines keyed by monster resource name and
event. Its production caller was `attack_talk` in `packets_act.c`: for an actor
other than the local player, it selected a random attack line, prefixed the
actor's name and displayed it in a speech balloon plus chat. The caller ran
when an idle actor acquired a different attack target, excluding several action
types. Hidden chat suppressed the chat copy; the balloon was separate.

The lookup recognized additional casting, skill-use, skill-hit, low-HP and death
states, but no production caller supplied those states. The player's branch in
the same helper triggered the retained pet emotion icons.

This feature invented dialogue from local data in response to an attack event.
It was separate from server-supplied NPC chat (`HEADER_ZC_NPC_CHAT`), ordinary
NPC dialogs, combat handling and the retained monster skill animation catalog.

Sources: `src/session/session_init.c`, `src/session/talk_tables.c`,
`src/game/packets_act.c`, `src/session/session_lifetime.c`, and
`src/game/packet_dispatch.c`.

Removed the catalog loader, XML ownership, state-name/line lookups, formatter
and local attack speech. The remaining loader is named
`session_load_monster_skill_info`; it retains the approved animation catalog.
Its XML document now initializes and releases directly, without an array used
for the removed speech documents. The monster sprite-name helper also remains.

The attack helper is now `pet_attack_emotion`, preserving the entire player's
emotion branch and its existing trigger conditions. Attack messages, hit data,
monster skill animations and server-supplied speech handlers are unchanged.
The local speech path's random line selection is gone, so it no longer consumes
random draws.

Updated the existing attack-record checks to expect attack/speed/state/hit
events without local speech or chat. The ordinary attack/skill cases and hidden
chat case passed. Session-field checks passed. The source-scope audit verifies
that only the intended files changed, speech references are gone, the emotion
branch and combat packet paths remain, and retained monster lookups are
unchanged. All 1,166 C sources rebuilt and the release executable linked
successfully after the session layout change. Whitespace checks passed. This
change covers 12 files.

Logs: `build/purist-no-monster-speech-build_attack_records.log`,
`build/purist-no-monster-speech-build_session_fields.log`,
`build/purist-no-monster-speech-build.log`, and
`build/purist-no-monster-speech-audit.log`. Backup:
`build/before-monster-speech-removal`; inventory: `build/monster-speech-files.txt`;
isolated diff: `build/purist-no-monster-speech.diff`.
This is a purist policy change, with no foundation patch. No live gameplay test,
installed asset/server edit, commit or push was made.

## Readable map names (START-128)

Decision: **keep**, approved 2026-11-01.

The shared map-information manager stores display names keyed by map identifier.
`session_map_display_name` supplies them to the minimap title, character
selection, party member locations, quests, warp choices and world-map screens.
Its lookup also handles instance-style map names. An unknown map uses the
configured default name, initialized from message 0xbb.

At this review point, startup first imported map/name pairs from
`mapNameTable.txt`, then ran `system/mapInfo_true.lub`. START-129 subsequently
consolidates those sources into Lua. Lua can replace names for matching maps and also
configures separate map-entry banners. This review concerns the shared display
name support; the individual data sources and decorative banners remain for
separate decisions.

Sources: `src/session/session_tables.c`, `src/session/session_startup.c`,
`src/contents/map_info.c`, `src/session/session_names.c`,
`src/session/session_misc.c`, and callers in `src/ui/map/minimap_wnd.c`,
`src/ui/character/select_char_new.c`, `src/ui/social/mini_party_info.c`,
`src/game/quest_list.c`, and `src/ui/worldmap/route_info.c`.

The shared lookup and its consumers remain unchanged. This keep decision
changes only the review ledger; no additional build or tests were needed.

## Text map-name catalog (START-129)

Decision: **remove the text catalog; consolidate names into Lua**, approved
2026-11-01. The initial recommendation was to keep the text source because the
inspected loose Lua catalog was empty; Patrick chose consolidation instead.

Removed `mapNameTable.txt` loading from `session_reload_texts`, including its
now-unused reader pointer. `session_startup` still loads `System/mapInfo_true.lub`
after the other session tables. The shared map-name lookup, default name,
instance-name handling and UI consumers remain unchanged. The UAE/English text
reload no longer overwrites names or banner settings; names now come from the
single Lua catalog, without automatic language-folder selection.

Added `tools/migrate_map_names.py` and `docs/map-name-migration.md`. The converter
reads one extracted text catalog, keeps the last value for duplicate keys and
preserves original string bytes through Lua escapes. It rejects malformed
records and refuses to overwrite output files. An optional source-Lua base is
embedded unchanged, then its existing `main` runs after the converted names,
preserving original name-override and banner precedence. Compiled Lua bases and
repeated conversion of generated output are rejected. Output is self-contained;
there is no runtime text-file dependency.

Read-only archive inspection found 943 unique names in `data.grf` and 1,176 in
`revel.grf` (1,186 records, with duplicate keys). The loose
`client-files/data/mapnametable.txt` is byte-identical to the Revel entry.
Prepared separate conversions under `build/map-name-migration/System` and
`build/map-name-migration/revel/System`, both named `mapInfo_true.lub`. They embed
the existing empty loose Lua file. Source-entry and output hashes are recorded
in `build/map-name-migration/sources.json`.

The source client opens `data.grf`, defaults to archive-first reads and does not
open `revel.grf`; no `event.grf` was present. Thus the data.grf conversion is the
default prepared variant. The Revel conversion also serves users selecting the
loose table with `<readfolder>`. No live runtime selection was verified. These
catalogs are staged in the ignored build directory, not installed. The populated
Lua catalog must accompany the purist executable; keeping the old empty Lua file
would leave map names at their default fallback. README links the migration guide.

Sources: `src/session/session_tables.c`, `src/session/session_startup.c`,
`src/contents/map_info.c`, `src/contents/lua_data.c`, `src/core/table_reader.c`,
`src/login/login_send_msg.c`, `src/core/winmain.c`, `src/fileio/file_mgr.c`, and
`src/platform/clientinfo.c`.

Validation: recompiled the changed C unit and linked the release executable.
Existing map-manager ownership, lookup, instance and banner checks passed.
Five converter tests cover duplicate keys, empty display names, byte escaping,
malformed input, base preservation and safe file output; they now run with
`tests/build_map_info.bat`. Both actual generated catalogs were executed through
the production map manager and real Lua callbacks: every name matched its
source bytes. A separate fixture verified quote/backslash/non-ASCII escapes,
empty names and existing Lua name/banner overrides. Scope and whitespace checks
passed; unrelated work is preserved.

Logs: `build/purist-map-names-lua-build.log`,
`build/purist-map-names-lua-tests.log`,
`build/purist-map-names-lua-catalogs.log`, and
`build/purist-map-names-lua-audit.log`. Backup:
`build/before-map-name-consolidation`; isolated diff:
`build/purist-map-names-lua.diff`. Seven source/documentation/test files changed.
This is a purist policy change, with no foundation patch, installed asset/server
edit, live gameplay test, commit or push.

## Map-entry banners (START-130)

Decision: **remove**, approved 2026-11-01.

The map-entry banner combined two title lines and a PNG ornament near the upper
part of the screen, held for five seconds and faded out over roughly a
quarter-second. It depended on the map's Lua notification flag and the saved
`bShowMapName` graphics preference. Ordinary readable map names were separate.

Removed `map_name_display.c` and its exclusive `map_info.h` type header, the
banner pointer and ornament/title/notification fields in the map manager,
metadata setters, creation/render/cleanup helpers, map-entry trigger and
per-frame draw call. Removed the graphics checkbox and its handler/placement,
the session field, default, option read and option write. Existing saved files
are not edited; the old option is no longer read or emitted. Remaining settings
and their placement are unchanged.

`AddMapDisplayName` now reads its first two Lua arguments; old third arguments
are ignored. `AddMapSignName` and `AddMapBackgroundBmp` remain only as aliases
to one inert success reply. They allocate no map records, retain no metadata,
load no resources and draw nothing. This small compatibility adapter lets old
Lua catalogs continue past their decoration directives to later map names.
The C banner API and implementation are gone.

Updated the migration tool to generate two-argument name calls and regenerated
both staged catalogs. Their names remain byte-identical to their sources.
Existing Lua name overrides remain effective; their banner directives are now
ignored. The migration guide and README describe this behavior. No installed
asset files or archives were modified.

The existing map-owner checks retain storage, allocation-failure, stable-name,
instance-lookup and Lua validation coverage. They now check two-argument and
legacy three-argument name calls, and verify that decoration calls create no
map entry or prevent later names from loading. Deleted the exclusive banner
canvas test and its build invocation; remaining canvas tests still pass.
Session-field checks passed. Both actual Lua catalogs (943 and 1,176 names) and
a byte-escaping/override fixture passed through the real Lua callbacks and map
manager. All 1,165 C sources rebuilt and the release executable linked after
the shared layout change. Source-scope and whitespace checks passed.

Sources: `src/contents/map_info.c`, `src/contents/map_info_api.h`,
`src/game/mode_init.c`, `src/game/mode_util.c`, `src/game/mode_run.c`,
`src/ui/render/graphic_setting.c`, `src/ui/render/graphic_setting.h`, and session
state, lifetime and option load/save code.

Logs: `build/purist-no-map-banners-build.log`,
`build/purist-no-map-banners-map-info.log`,
`build/purist-no-map-banners-canvas.log`,
`build/purist-no-map-banners-session.log`,
`build/purist-no-map-banners-catalogs.log`, and
`build/purist-no-map-banners-audit.log`. Backup:
`build/before-map-banner-removal`; inventory: `build/map-banner-files.txt`;
isolated diff: `build/purist-no-map-banners.diff`. The change covers 23 files,
including three deletions. This is purist policy, with no foundation patch,
live gameplay test, installed asset/server edit, commit or push.

## Empty skill-info hook (START-131)

Decision: **remove**, approved 2026-11-01.

After reloading shared messages, monster skill animation data and item text
tables, `session_reload_texts` called `skill_info_nop`. Its body was empty:
it read no data, changed no state and returned nothing. This was its only caller.

Removed the call, empty definition and declaration. Actual skill-information
loading, lookups and all other code in the affected files remain unchanged.
Both changed C units compiled and the release executable linked successfully.
No new behavioral test is needed for deleting a function with no behavior.
The scope audit confirms the exact three removals, no remaining source/test
references and no changes to unrelated work. Whitespace checks passed.

Sources: `src/session/session_tables.c`, `src/skills/skill_info.c`, and
`src/skills/skill_info.h`.

Logs: `build/purist-no-empty-skill-hook-build.log` and
`build/purist-no-empty-skill-hook-audit.log`. Backup:
`build/before-empty-skill-hook-removal`; isolated diff:
`build/purist-no-empty-skill-hook.diff`. Four files changed including this ledger.
This is purist cleanup, with no foundation patch, installed asset/server edit,
live gameplay test, commit or push.

## Closing an open Setup window (START-132)

Decision: **remove**, approved 2026-11-01.

Immediately after text reload, `session_init_table` looked for a window titled
`Ragnarok Setup`. If found, it displayed message-table entry 0xc7a in an Error
dialog and synchronously sent `WM_CLOSE` to the matching window. The code did
not verify the process or executable behind the title. Initialization continued
afterward.

Removed the title search, error notice, close request and now-unused local
window handle. Saved/default settings and the in-game graphics settings remain
unchanged. This completes the reviewed cleanup of this separate Setup-related
behavior; the earlier dependency that launched `Setup.exe` was removed under
START-010.

The changed C unit compiled and the release executable linked. A source audit
checks the exact removal, preserved remainder of the initialization function,
absence of the title/check references, and unchanged unrelated work. Whitespace
checks passed. No live external window was opened or closed for validation.

Sources: `src/session/session_tables.c::session_init_table`; the retained generic
error dialog helper is `src/world/mode_draw.c::error_msg`.

Logs: `build/purist-no-setup-window-build.log` and
`build/purist-no-setup-window-audit.log`. Backup:
`build/before-setup-window-removal`; isolated diff:
`build/purist-no-setup-window.diff`. Two files changed including this ledger.
This is purist policy, with no foundation patch, installed asset/server edit,
live gameplay test, commit or push.

## Per-map fog (START-133)

Decision: **keep**, approved 2026-11-01.

`session_load_fog_parameters` loads `fogParameterTable.txt`. Each map record
supplies four values: fog start distance, end distance, color and density.
`session_find_fog_parameters` looks up the current map and normalizes
instance-style identifiers. The shared named-table loader also serves camera
presets, which were kept separately.

On map entry, `game_mode_apply_fog` applies the current map's values if the
player's Fog option is enabled. The in-game graphics checkbox and fog command
can toggle it. If the option is off or the map has no matching entry, the code
disables fog instead. Renderer code consumes the parameters to blend scenery
into the fog color with distance, using its existing rendering paths.

Sources: `src/session/session_tables.c`, `src/session/session_names.c`,
`src/game/mode_init.c`, `src/game/talk_commands.c`,
`src/ui/render/graphic_setting.c`, `src/render/crenderer.c`, and
`src/render/rp_pool.c`.

The fog catalog, lookup, renderer support and player toggle remain unchanged.
This keep decision updates only the review ledger; no additional build or tests
were needed.

## Shared session Lua runtime (START-134)

Decision: **keep**, approved 2026-11-01.

`session_init_lua_files` creates the session's `ClientLua` owner if needed,
opens its Lua interpreter, then executes a sequence of catalogs and helper
scripts. Their tables and functions remain available for later client lookups.
The session destructor closes the interpreter and frees its owner.

Current callers use it for skill information, NPC/job and equipment appearance
names, status-icon artwork, hotkey definitions, chat/options and navigation,
among other features. These are active dependencies of retained client systems.
The separate one-shot Lua map-name loader reviewed earlier does not replace
this shared session state.

The loader uses the client file layer for resource catalogs and disk-only
reads for saved settings. It tries `.lub` before `.lua`, executes the selected
file, reports Lua errors through the existing dialog helper and restores the
Lua stack after loading. The shared call helpers let C query the loaded data.

This review concerns the runtime/ownership and shared loading mechanism.
Individual script loads, fallback policies, saved-setting imports, regional
variants and optional features in the initializer are separate decisions.
The first load at this review point was `Lua Files/DataInfo/ChangeDirectoryList`;
its automatic file migration is removed under START-135.

Sources: `src/session/session_lua.c`, `src/contents/client_lua.c`,
`src/contents/client_lua_globals.c`, `src/session/session_lifetime.c`,
`src/skills/skill_info.c`, `src/session/session_efst_image.c`, and
`src/session/hotkeys.c`.

The shared runtime, lifecycle and loading/call helpers remain unchanged.
Individual loads will be reviewed in execution order. This keep decision
changes only the review ledger; no additional build or tests were needed.

## Automatic file relocation (START-135)

Decision: **remove**, approved 2026-11-01.

The first script in `session_init_lua_files` was
`Lua Files/DataInfo/ChangeDirectoryList`. Its C callback took a filename, source
directory and destination directory. When the destination was absent and the
source existed, it created destination directories and attempted `rename`.
The callback did not report whether the move succeeded.

The inspected loose script listed 13 `.eot` fonts to move from `data` into
`System/Font`, and four older settings files to move into `SaveData`:
`OptionInfo.lua`, `ChatWndInfo_U.lua`, `UserKeys.lua` and `UserKeys_s.lua`.
This changed files on disk, rather than redirecting resource names.

Removed the script load, `c_ChangeDirectoryOfDatas` registration, C callback and
its private path-joining helper. Its sole directory-creation helper,
`make_directories`, is now removed with its declaration and exclusive includes.
The remaining shared file utility still provides directory listing unchanged.
Other Lua callbacks and loads are unchanged. Ordinary font loading and reads
from `SaveData` remain. Old loose files must be placed in their intended folders
by packaging or a deliberate migration; startup no longer relocates them.

All three affected C units compiled and the release executable linked. Existing
Lua-call and file-loading/ownership boundary checks passed. The scope audit
confirms that other startup loads and callbacks, shared directory listing and
unrelated work remain unchanged. Migration references are gone from source and
tests, and whitespace checks passed. No migration script was executed and no
installed file was moved or deleted.

Sources: `src/session/session_lua.c`, `src/contents/client_lua_funcs.c`,
`src/core/file_util.c`, and `src/core/file_util.h`.

Logs: `build/purist-no-file-relocation-build.log`,
`build/purist-no-file-relocation-lua.log`, and
`build/purist-no-file-relocation-audit.log`. Backup:
`build/before-file-relocation-removal`; inventory:
`build/file-relocation-files.txt`; isolated diff:
`build/purist-no-file-relocation.diff`. Five files changed including this ledger.
This is purist policy, with no foundation patch, installed asset/server edit,
live gameplay test, commit or push.

## Lua UI text catalog (START-136)

Decision: **keep**, approved 2026-11-01.

`session_init_lua_files` now starts by loading `Lua Files/MsgString_KR` through
the shared Lua loader. The inspected local source defines a `MsgStrID` table
with named UI strings. Later files reference those names, including `HotKey`
for key-binding labels and `OptionInfo/CmdInfo` for command descriptions/help.
The command-list window gets this help through `l_GetCmdHelpMsg`; the hotkey
settings window gets labels through the Lua hotkey queries.

This is separate from the numeric `msgStringTable.txt` catalog kept under
START-123. Despite the `_KR` filename, strings can be translated: the inspected
local copy contains English text, including hotkey descriptions and Fog help.
The loader does not enforce a language based on that filename.

Retaining this text source does not retain every optional feature whose strings
happen to be present. Those consumers remain separate decisions.

Sources: `src/session/session_lua.c`, `src/contents/client_lua.c`,
`src/session/hotkeys.c`, `src/ui/hotkeys/hotkey_settings.c`,
`src/ui/chat/command_list_wnd.c`, and read-only local assets under
`client-files/data/luafiles514/lua files`: `msgstring_kr.lua`, `HotKey.lua`,
`OptionInfo/CmdInfo.lua` and `OptionInfo/OptionInfo_F.lua`.

The text catalog load and its consumers remain unchanged. This keep decision
updates only the review ledger; no additional build or tests were needed.

## Common Lua helpers (START-137)

Decision: **keep**, approved 2026-11-01.

The next load is `Lua Files/Common`. The inspected loose source defines five
helpers: `toInt`, `toStr`, `getTableSize`, `GetTableSizeForC` and `getHelpMsg`.
The first pair supplies default values for absent hotkey fields; the table-size
helpers count entries with `pairs`; the final helper returns a help line or an
empty string when the table/entry is absent. These are shared support functions,
not a separate player-facing feature.

The currently loaded HotKey script calls the value helpers, the emotion helper
script counts its order list, and SkillInfoz helpers count skill/requirement
lists. C code calls `GetTableSizeForC` when registering hat-effect factories and
when printing command help. `talk_cmd_help` obtains each help line through
`getHelpMsg`. These active users make the script necessary under the current
data layout.

Startup also queries the hat-effect count into a local `hatEffects` value that
is not subsequently read. That individual query remains for separate review
when reaching `HatEffectInfo`; the effect manager's separate count query is
actively used. Keeping Common does not decide whether every optional caller or
catalog should stay.

Sources: `src/session/session_lua.c`, `src/effects/effect_registry.c`,
`src/game/talk_commands.c`, and read-only local assets under
`client-files/data/luafiles514/lua files`: `common.lua`, `hotkey.lua`,
`emotion/emotionlist_f.lua` and `skillInfoz/skillinfo_f.lua`. The unprefixed
local Common source contains the same helper definitions. No scripts were
executed and no installed assets were changed.

The Common load and shared helpers remain unchanged. Optional features and
unused calls will be reviewed individually. This keep decision updates only
the ledger; no additional build or tests were needed.

## Lua window defaults (START-138)

Decision: **keep**, approved 2026-11-01.

The next script load is `Lua Files/wndInfo`. The inspected local source maps
window IDs to Lua tables containing width, height and initial x/y positions.
`GetWindowInfo(windowID)` returns those four values.

`make_lua_window` calls that function before creating a window. Its current
constructors include hotkey settings (0x9c), quick slots (0x9f), shop search
(0xb5) and the basic button panel (0xb6). Initial coordinates scale from a
1024-by-768 reference to the current screen size. The creation path also checks
previously closed positions, and quick slots receive their separate saved
window information. Keeping these defaults is separate from the already-kept
persistence of window layouts.

If the Lua query fails, this creation helper returns without making the
window. Removing the script load alone would therefore break its active users;
replacing it would require supplying equivalent defaults in C.

The Lua source also contains entries for older or removed windows and a
`GetSysButtonInfo` helper for base/close-button flags. No C caller of that helper
was found. The current helper's outer dispatch also still lists legacy IDs
beyond its four constructors; those routes and their callers need separate
review. Retaining the shared configuration does not retain or validate every
legacy window or table field.

Sources: `src/session/session_lua.c`, `src/ui/wndmgr/make_window.c`,
`src/ui/hotkeys/hotkey_settings.c`, `src/ui/hotkeys/quick_slot.c`,
`src/ui/shops/store_search.c`, `src/ui/character/basic_buttons.c`, and the
read-only local `client-files/data/luafiles514/lua files/wndInfo.lua` plus its
unprefixed counterpart. No scripts were executed or installed assets changed.

The Lua defaults and their creation path remain unchanged. Individual windows
and obsolete entries will be reviewed separately. This keep decision changes
only the review ledger; no additional build or tests were needed.

## Lua hotkey definitions and matching (START-139)

Decision: **keep**, approved 2026-11-01.

`Lua Files/HotKey` defines default key groups and their action labels, names for
key codes, and helpers for formatting combinations, querying default/user
bindings, matching a key press to an action, and changing bindings. The inspected
local source contains `HOTKEY_1` through `HOTKEY_4`, uses the retained MsgStrID
text catalog and Common helpers, and reads `USERKEY_*` overrides.

The window manager's keyboard path calls `GetBehaviorOfHotKey2` to identify an
action. Session and hotkey-settings code query the same definitions and user
bindings. Menu/button hover text uses `GetBeHaviorOfWindow` to display the
current shortcut. The C user-hotkey manager can apply per-account bindings into
the four Lua user tables; the local storage decision under START-038 is already
implemented and is not being reopened here.

This review concerns shared definitions, matching and remapping support.
Specific hotkey actions remain separate policies to review. The legacy
`SaveData/UserKeys` seed import was subsequently removed under START-141 and
forced `ResetTheHotKey` startup calls under START-142. Removing
the HotKey load alone would leave active input/settings queries without their
functions; replacing this mechanism would require equivalent built-in bindings
and matching support.

Sources: `src/session/session_lua.c`, `src/session/hotkeys.c`,
`src/session/session_hotkeys.c`, `src/contents/user_hotkeys.c`,
`src/contents/user_hotkey_apply.c`, `src/ui/wndmgr/keyboard.c`,
`src/ui/hotkeys/hotkey_settings.c`, `src/ui/character/menu_icon.c`,
`src/ui/controls/button.c`, and the read-only local
`client-files/data/luafiles514/lua files/HotKey.lua`.

The shared Lua hotkey definitions, matching and remapping helpers remain
unchanged. Individual actions and reset/seed policies will be reviewed
separately. This keep decision updates only the review ledger; no additional
build or tests were needed.

## Automatic empty data folder creation (START-140)

Decision: **remove**, approved 2026-11-01.

Immediately after loading HotKey, `session_init_lua_files` previously checked
for a local entry named `data` using `FindFirstFileA`. If none was found, it
called `CreateDirectoryA("data", NULL)` and ignored the result. This only
attempted to create an empty folder; it did not extract GRFs, populate
resources or create saved preferences.

`File` resolves ordinary resource names with a `data` prefix, but the file
manager uses that name as an archive key independently of a physical folder.
It still tries GRFs and loose files in the configured order. Existing loose
resource folders would remain usable without this creation call. Local
preferences use `SaveData` and the account-settings paths with their own
storage setup.

The inspected loose OptionInfo script also has a legacy branch that can write
`data/OptionInfo.lua`, but the only current C caller of
`SaveToFileCmdOnOffValueEx` passes zero and therefore writes to `SaveData`.
The loose StateIcon script defines `MakeBuffIconInfoFile`, an export helper
that writes `data/StateIconInfo_n.lua`; no caller was found in current C/tests
or the inspected loose Lua sources. These definitions do not establish a
normal startup dependency on an empty `data` folder. Custom scripts that
explicitly write there would need the folder supplied separately.

Sources: `src/session/session_lua.c`, `src/fileio/file.c`,
`src/fileio/file_mgr.c`, `src/session/session_option_save.c`,
`src/contents/settings_file.c`, and read-only local Lua sources
`OptionInfo/OptionInfo_F.lua` and `StateIcon/stateiconinfo_f.lua` under
`client-files/data`. No scripts were executed or installed assets changed.

Removed the five-line lookup/create/close block and its exclusive
`WIN32_FIND_DATAA` and `HANDLE` locals. The remainder of `session_lua.c` is
byte-for-byte unchanged. GRF reads, existing loose-resource loading and
saved-settings directory setup are retained. No existing folders were deleted.

Validation: the changed C unit compiled and the release executable linked.
An isolated audit verifies the exact source deletion, unchanged unrelated
working files and clean whitespace. No runtime smoke test was performed; this
is a directory-creation policy removal, with no new test needed.

Artifacts: `build/before-data-folder-removal`,
`build/purist-no-data-folder.diff`, `build/purist-no-data-folder-build.log` and
`build/purist-no-data-folder-audit.log`. No foundation patch, commit or push.

## Legacy shared hotkey seed import (START-141)

Decision: **remove**, approved 2026-11-01.

The next startup statement previously loaded `SaveData/UserKeys` through
`run_saved`. The generic Lua loader looks for `.lub` first, then `.lua`,
directly on disk. This executed any saved `USERKEY_*` tables before the forced
hotkey resets.
The inspected local asset folder contains both extensions as zero-byte files,
so neither supplies bindings there. Other installations can contain real
legacy overrides.

The retained HotKey script queries a user override first, then falls back to
its `HOTKEY_*` default when no override exists. The per-account settings owner
captures initial preferences before applying the first account file and
restores that baseline when switching accounts. A new or missing account file
therefore previously inherited any shared legacy bindings; a valid existing
account file replaces the user-key tables with its own saved bindings. This
explains why the old import was originally retained as a seed under START-038.

This was the only UserKeys reference in C, with no SaveUserHotKeys caller.
Normal edits already save to the separate local account JSON file. Removing
the seed import leaves Lua default definitions, the remapping window,
per-account persistence and account-switch isolation intact. It stops adopting
overrides from old shared UserKeys files. The forced reset calls were
subsequently removed under START-142; registry-sourced macro defaults remain
a separate decision.

Sources: `src/session/session_lua.c`, `src/contents/client_lua.c`,
`src/contents/account_settings.c`, `src/contents/account_settings_json.c`,
`src/contents/user_hotkeys.c`, `src/contents/user_hotkey_apply.c`,
`src/login/login_states.c`, and read-only local `client-files/SaveData/UserKeys`
files plus `client-files/data/lua files/HotKey.lua`. No scripts were executed
and no installed assets were changed.

Removed exactly the `run_saved(self, "SaveData\\UserKeys")` startup statement.
The remainder of `session_lua.c` is byte-for-byte unchanged. Existing UserKeys
files and installed assets were not modified or deleted. README and the
START-038 description now describe the retained defaults and account storage.

Validation: the changed C unit compiled and the release executable linked.
`tests/build_local_account.bat` passed its actual Lua/JSON/file round trips,
account/server/port/world isolation, initial preferences, empty-state
persistence and corrupt-file/write-failure handling. This is a regression check
of the retained account settings, not a live-client startup test. The isolated
audit verifies the one-line source deletion, no remaining source UserKeys
references, unchanged unrelated working files and clean whitespace.

Artifacts: `build/before-userkeys-import-removal`,
`build/purist-no-userkeys-import.diff`, and
`build/purist-no-userkeys-import-{build,account,audit}.log`. No foundation
patch, commit or push.

## Forced startup hotkey resets (START-142)

Decision: **remove**, approved 2026-11-01.

The next statement called `session_reset_the_hot_keys`, which invoked Lua
`ResetTheHotKey` twice: `(1, 0x84, 0xc0, 0x12)` and
`(1, 0x87, 0x24, 0x12)`. The key pairs are Alt+OEM_3 (the backtick/tilde key on
US layouts) and Alt+Home. In the inspected Lua defaults those combinations
belong to the information-panel button toggle and ground-cursor toggle.

The Lua helper checks a user binding in group two, then conditionally searches
user overrides for the supplied key pair and deletes an entry from `USERKEY_2`.
It does not change a key's default or update the local account JSON. These
calls ran during session table initialization, before the normal login path
loads per-account settings. They were adjacent to the shared UserKeys import
removed under START-141. Removing the calls leaves the default actions,
remapping and account storage intact.

The inspected local HotKey source also exposes a mismatch: C passes action
IDs 132/135, but Lua subtracts the size of group one (36), yielding group-two
indices 96/99, beyond that catalog's entries 0..60. The search helper returns
only an index, even when a match belongs to another group, while the caller
always deletes from group two. These are source observations for the inspected
loose asset; no running-client failure or archive-script equivalence has been
verified. They are reasons not to treat the old cleanup as a reliable part of
normal remapping. A fresh load of that HotKey source defines no USERKEY tables,
so the helper simply returns until such a table exists.

Sources: `src/session/session_lua.c`, `src/session/hotkeys.c`,
`src/session/session_options.h`, `src/login/login_states.c`, and read-only local
`client-files/data/luafiles514/lua files/HotKey.lua` plus its identical
unprefixed copy and `msgstring_kr.lua`. Both inspected HotKey copies have SHA-256
`8a7a43f851de84807997810cb0c7b02108fe4c81223c5898294c376b81d70502`.
No asset scripts were executed or changed.

Removed the startup call from `session_lua.c`, the two-call helper from
`hotkeys.c`, and its declaration from `session_options.h`. Every other byte
in those source files is preserved. The external HotKey script is unchanged;
its now-unused reset function is not part of this source checkout. Ordinary
defaults, toggle actions, remapping and per-account settings remain.

Validation: both changed C units compiled and the release executable linked.
The isolated audit checks the exact deletions, absence of remaining C/test
reset references, unrelated working-file hashes and whitespace. No new tests
or live-client smoke test were needed for these isolated deletions; the
account-settings regression suite passed in the immediately preceding step.

Artifacts: `build/before-hotkey-reset-removal`,
`build/purist-no-hotkey-resets.diff`, `build/purist-no-hotkey-resets-build.log`
and `build/purist-no-hotkey-resets-audit.log`. No foundation patch, commit or
push; the Lua mismatch above remains an explicitly scoped source observation.

## Shared Lua emotion definitions (START-143)

Decision: **keep**, approved 2026-11-01.

The next two script loads are `Emotion/EmotionList_F` followed by
`Emotion/EmotionList`. The first supplies lookup, registration and request
helpers; the second defines emotion IDs, animation action numbers, text-command
aliases and the order used by the emotion picker. Some command labels come
from the retained numeric message catalog, and extra aliases are listed in Lua.

This is shared support, not just the picker window. Received emotion packets
identify an actor and emotion type; the actor's emotion handler reaches
`game_mode_make_emotion_effect`, which queries `GetEmotionAction` to select an
action from `emotion.spr`/`emotion.act`. The same presentation path is also
used by actor emotion messages such as the chat-prohibition marker.

For outgoing text, `session_is_emotion_text` lowercases the first word and
asks `ReqEmotionByMsgStr` whether it is an emote command. The Lua helper invokes
`c_ReqEmotion` when recognized. Chat, whisper, chat-room and macro-key paths
use this shared recognition. The picker separately queries its ordering,
preview animation and command text; selecting an entry can populate chat or a
macro field and request an emotion through `ReqEmotionByIndex`.

Keeping this catalog/helper layer retains classic emote commands and
correct emotion animations. It does not decide every picker, alias or special
request policy. The C request callback also randomizes the dice and yut-style
emotion families; those optional behaviors can be reviewed separately without
removing the shared catalog. Removing only the script loads would leave these
active callers without their definitions.

Sources: `src/session/session_lua.c`, `src/session/session_lookups.c`,
`src/contents/client_lua_funcs.c`, `src/game/packets_actor.c`,
`src/actor/actor_sendmsg.c`, `src/game/actor_marks.c`, `src/ui/chat/emotion.c`,
and read-only local `client-files/data/luafiles514/lua files/Emotion/EmotionList`
and `EmotionList_F` Lua sources. No asset scripts were executed or changed.

The shared emotion definitions and helpers remain unchanged. The picker and
optional request behaviors will be reviewed separately. This keep decision
updates only the review ledger; no additional build or tests were needed.

## Emotion picker window (START-144)

Decision: **keep**, approved 2026-11-01.

The emotion picker is a separate window with sprite previews and Previous/Next
page controls, showing up to 30 entries per page in the order supplied by the
retained Lua catalog. The inspected default hotkey is Alt+L: Lua group-two
entry 11 maps to action 0x6f, and `key_down.c` toggles window 0x57. The macro
editor's View button also opens or closes this picker. Saved window position
and size use the existing window-layout support.

A normal click obtains the selected entry's command text through
`GetEmotionMsg`. When the macro editor is closed, it inserts that text into
the chat input; when the editor is open, it attempts to insert into the
currently selected macro field. Double-clicking requests the emote through
`ReqEmotionByIndex`. Although that handler is named `on_l_btn_up`, the window's
operations table explicitly binds it to double-click; ordinary button release
uses the base frame handler.

This window lets players find emotes without memorizing slash commands and
helps assign them to chat macros. Removing it would not require removing
shared emote commands or received animations, already kept under START-143.
Macro storage is already local per account; this review concerns the picker
window and its entry points, not that persistence policy or special dice/yut
request behavior.

Sources: `src/ui/chat/emotion.c`, `src/ui/chat/chat.h`,
`src/ui/wndmgr/key_down.c`, `src/ui/wndmgr/make_window_hud.c`, and read-only local
`client-files/data/luafiles514/lua files/HotKey.lua`, `MsgString_KR.lua`, and the
EmotionList scripts. No scripts were executed or installed assets changed.

The picker, page controls, default hotkey, macro-editor entry point and saved
layout support remain unchanged. This keep decision updates only the review
ledger; no additional build or tests were needed.

## Dice emote (START-145)

Decision: **keep**, approved 2026-11-01.

The retained local EmotionList script maps `/dice` and `/e19` to `ET_DICE1`
(emotion ID 58). When the shared Lua request callback receives that ID,
`lua_req_emotion` selects `rand() % 6` and sends one of emotion IDs 58..63.
The older C command table also recognizes those commands (plus its message-table
alias); its dispatch case 0x9e performs the same six-way selection. Both paths
therefore choose the face on the client.

The selected ID travels through the ordinary emotion request path as packet
0x00bf, subject to its existing pending-request and timing checks. Received
emotion IDs use the retained actor emotion renderer and Lua animation mapping.
This code requests a dice-face animation; it does not award items, change stats
or resolve a gameplay contest. No matching server implementation or live
server behavior was inspected for this review.

The dice animation family remains useful for a familiar social command. Its
local random selection is a distinct policy from the separate yut-style
seven-result randomization, the general emotion request limits, and received
emotion rendering. This decision concerns allowing players to request the
dice emote, not deleting received dice artwork from the shared catalog.

Sources: `src/contents/client_lua_funcs.c`, `src/session/session_talk.c`,
`src/game/talk_type.c`, `src/game/mode_msg.c`, `src/game/msg_misc.c`,
`src/game/packets_actor.c`, and read-only local
`client-files/data/luafiles514/lua files/Emotion/EmotionList.lua` and
`EmotionList_F.lua`. No scripts were executed or installed assets changed.

The dice request paths, local random selection and shared rendering remain
unchanged. The yut-style random request variant was subsequently removed under
START-146. This keep decision updates only the review ledger; no additional
build or tests were needed.

## Yut-style random emote (START-146)

Decision: **remove**, approved 2026-11-01.

Alongside the six-face dice branch, `lua_req_emotion` previously had a separate
branch for emotion ID 0x51 (81). It selected `rand() % 7`, yielding emotion IDs
81..87, then used the ordinary emotion request path. The external catalog names these
entries `ET_YUT1` through `ET_YUT7` and maps them to animation actions 86..92.
This is an emotion variant; the inspected code does not implement a board,
turns, rewards or gameplay scoring.

The inspected local EmotionList source registers `/dbc` and a Korean alias
encoded as slash plus bytes 192/183 (CP949 U+C737) to the first Yut entry.
`ReqEmotionByMsgStr` invokes the same shared C callback used by other emotes.
The family is absent from that source's `EMOTION_ORDERLIST`, so it is not
shown in the default picker. The older C command table and dispatch inspected
here do not add a separate Yut request path; the similarly numbered talk-type
cases refer to other commands and must not be removed by numeric coincidence.

This decision concerns local Yut requests and their seven-way randomization.
The ordinary dice emote is kept under START-145. Received emotion rendering
and the external animation catalog remain compatible after the local request
variant is removed. Runtime Lua assets are outside this checkout and
have only been inspected; no installed catalog has been modified.

Sources: `src/contents/client_lua_funcs.c`, `src/session/session_talk.c`,
`src/game/talk_type.c`, `src/game/msg_misc.c`, and read-only local
`client-files/data/luafiles514/lua files/Emotion/EmotionList.lua` and
`EmotionList_F.lua`. No asset scripts were executed and no live server was
queried.

Removed the seven-way randomization branch from `lua_req_emotion`. The Lua
callback now rejects every Yut ID (81..87), returns -1 and sends no mode
message. The central outgoing `game_mode_msg_emotion` also rejects that range
before touching the timestamp, pending state or packet sender, covering direct
C callers as well as Lua. The six-way dice branch is unchanged.

Existing external Lua catalogs can still recognize their old Yut aliases;
those calls are now inert. The inspected Lua text helper returns its own
recognized ID regardless of the callback result, so the alias is consumed
without becoming an ordinary chat line. No installed catalogs were edited.
Received Yut IDs still use the original shared actor/animation path.

Validation: both changed C units compiled and the release executable linked.
An isolated audit verifies the exact callback and outgoing-guard changes,
unchanged dice logic, byte-identical received-emotion sources and untouched
remaining timing/pending logic, along with unrelated working files and
whitespace. No live-client or server test was performed and no new test was
added for this small request-policy removal.

Artifacts: `build/before-yut-request-removal`,
`build/purist-no-yut-requests.diff`, `build/purist-no-yut-requests-build.log` and
`build/purist-no-yut-requests-audit.log`. No foundation patch, commit or push.

## Emote cooldown (START-147)

Decision: **keep**, approved 2026-11-01.

`game_mode_msg_emotion` rejects a request while the unsigned elapsed time since
`emotion_time` is at or below 2500 ms. On a request that passes, it records the
current tick value and sends the ordinary emotion packet.
The result is a minimum interval of slightly more than 2.5 seconds between
sent requests. Rejected requests are not queued for later playback.

`game_mode_initialize` also initializes `emotion_time` from the clock, so the
same initial delay applies after that initialization. The rate limit is shared
by ordinary emotes, picker requests and dice. This step does not alter received
animations or define a server-side limit.

A separate pending flag originally kept requests blocked beyond the time
interval until a completion notification or refusal. It was reviewed and
removed under START-148; the cooldown remains independent of received
animations.

The previous comparison added 2500 to the last tick value. Near 32-bit
rollover, that deadline could wrap below the current time and permit a request
too early. The correction reads the clock once and compares the unsigned
elapsed difference, preserving the strict boundary: 2500 ms is still blocked
and 2501 ms passes the time check.

Sources: `src/game/msg_misc.c`, `src/game/mode_initialize.c`,
`src/game/mode_msg.c`, `src/game/skill_fail.c`, and
`src/effects/skills/msg_effect_process.c`.

The 2.5-second policy is retained. This correction changed only its clock
calculation; invalid-ID checks, purist Yut rejection, packet type and payload
were preserved. The pending wait was separately removed under START-148. `tests/build_emote_timing.bat` extracts the actual production
function with its constants and substitutes only clock/transport boundaries.
Its 49 normal/rollover interval cases plus pending-state, packet and retry
checks fail against the original comparison and pass with the fix. No live
client/server run was performed. The changed C unit compiled and release linked.

Foundation fix: [emote-cooldown-rollover.patch](upstream/emote-cooldown-rollover.patch)
contains only the clock correction and the original regression checks, based on
`54ca05e3785e1ab2771362b150f8050c2a832b9a`. The full patch applied cleanly with
whitespace checking to pinned files extracted under `build/foundation-emote-timing`.
The original foundation function fails the regression; the patched function
passes. The patch retains foundation Yut behavior and does not contain the
purist request filter. The existing foundation checkout was read only. The
patch also retains the foundation pending lock; subsequent purist removal under
START-148 adapts the working-tree test but leaves this standalone patch intact.

Artifacts: `build/before-emote-timing-fix`, `build/purist-emote-timing.diff`,
`build/emote-timing-{before,after,build,audit}.log`, and
`build/foundation-emote-timing-{before,after}.log`. No commit or push.

## Pending-emote animation lock (START-148)

Decision: **remove**, approved 2026-11-01.

Besides the retained cooldown, `game_mode_msg_emotion` previously checked
`emotion_pending`. Sending an emotion set it to one; every later request was
refused while it remained set, regardless of elapsed time. The rate limiter
and this completion lock were independent conditions.

Normal completion comes from `step_play_once` in the message-effect renderer:
when a style-1 effect for the local player's actor finishes its animation, it
sends `MODE_MSG_EMOTION_ANSWERED`, which clears the flag. The basic-skill emotion
refusal path also clears it. The actor handler sends the same clear
notification for an out-of-range received emotion ID. Game-mode initialization
resets it. Receiving an ordinary emotion packet alone does not clear it.

No independent timeout for this flag was found. If no matching local animation
completes, the source provides no timed recovery through the already-kept
cooldown. This is a code-path observation, not a reproduced live-client stall.
The lock can also extend the interval beyond 2.5 seconds for longer animations.

Removing this lock lets the approved cooldown govern request spacing, with
each allowed request still going through the normal game connection. Shared
emote rendering, dice and the picker remain. This decision concerns
the flag, its check/set/reset sites and its exclusive completion notification;
it does not change the server's handling of emotion requests.

Sources: `src/game/msg_misc.c`, `src/game/gamemode.h`,
`src/game/mode_initialize.c`, `src/game/mode_msg.c`, `src/game/mode_msg.h`,
`src/game/skill_fail.c`, `src/game/packets_actor.c`,
`src/actor/actor_sendmsg.c`, and `src/effects/skills/msg_effect_process.c`.

Removed the GameMode flag, its sender check/set, initialization reset,
basic-skill-refusal reset, mode-message enum/handler, animation-completion
notification and out-of-range actor-notification fallback. The refusal notice
and animation playback/cleanup are preserved. Two renderer includes used only
by that notification were removed; `mode_callbacks.h` remains necessary for
the existing `g_isSkipFrame` check. No message IDs were renumbered.

The existing timing regression now checks that later requests succeed without
an animation-completion callback, that 2500 ms remains blocked and 2501 ms
passes across rollover, and that all seven rejected Yut IDs leave the timestamp
unchanged. The separate foundation clock patch is byte-identical to its
previously verified version and keeps foundation behavior.

Validation: all 1,165 C units were rebuilt for the changed GameMode layout.
The first compile caught a still-needed renderer header; restoring it and
recompiling that unit completed the release link. Timing regression passed,
including 49 normal/rollover cases, repeated requests and Yut rejection.
The existing game-mode layout/lifecycle regression passed with 6,562 trace
words, size 1208 and unchanged digest `274efe64`. The isolated audit checks
exact source changes, no remaining lock/notification references, unchanged
remaining animation code, foundation-patch preservation and unrelated files.
No live client/server test was performed.

Artifacts: `build/before-emote-lock-removal`,
`build/purist-no-emote-lock.diff`, and
`build/purist-no-emote-lock-{build,repair,timing,layout,audit}.log`. No new
foundation patch, commit or push for this pruning decision.

## NPC/monster identity table (START-149)

Decision: **keep**, approved 2026-11-01.

The next startup load is `Lua Files/DataInfo/NPCIdentity`. The inspected local
file defines `jobtbl`, mapping symbolic names to numeric type/appearance IDs,
for example `JT_PORING = 1002`, `JT_WARPNPC = 45` and
`JT_4_F_KAFRA1 = 117`. Despite the word job in the table name, these entries
include monsters and NPC appearances. The file itself is an identifier table;
it contains no monster statistics, combat decisions or AI.

Subsequent `jobName` data uses those constants as table keys. For example,
`jobtbl.JT_PORING` maps to the resource name `Poring`, and the Kafra entry maps
to `4_F_KAFRA1`. `jobName_F` exposes `ReqJobName`, which the C session table
loader queries to populate its appearance/resource-name lookup. The client
uses that lookup when resolving actor resources, including monster `.act`
files. PetInfo also keys pet names, illustrations, egg mappings and food data
with the same named IDs. Those individual consumers remain separate reviews.

Removing only NPCIdentity would leave the following Lua catalogs without the
`jobtbl` keys they depend on. Keeping the catalog supports existing and custom
appearances; it does not require keeping every optional pet feature or every
individual appearance entry forever.

Sources: `src/session/session_lua.c`,
`src/session/session_job_name_table.c`, `src/session/session_names.c`,
`src/session/session_lookups.c`, and read-only local files under
`client-files/data/luafiles514/lua files/DataInfo`: `NPCIdentity.lua`,
`jobName.lua`, `jobName_F.lua` and `PetInfo.lua`. No asset scripts were executed
or installed assets changed.

The shared identity table and its load remain unchanged. Appearance helpers
and pet-specific data will be reviewed separately. This keep decision updates
only the review ledger; no additional build or tests were needed.

## Actor appearance-name catalog and lookup (START-150)

Decision: **keep**, approved 2026-11-01.

The next script loads are `DataInfo/jobName_F` and `DataInfo/jobName`.
`ReqJobName(jobID)` looks up a resource name in `JobNameTable`, returning an
empty string for an absent entry. The catalog uses the retained NPCIdentity
constants as keys: Poring maps to `Poring`, Kafra type 117 to `4_F_KAFRA1`,
and the warp-NPC type to `1_ETC_01` in the inspected local copy.

`session_init_job_name_tables` queries this Lua helper over several NPC/monster
ID ranges and stores the results in its shared name table. `session_get_job_name`
returns those names to consumers. In `npc_set_spr_job_`, the client chooses the
appropriate resource folder and appends `.spr` and `.act` to load actor artwork
and animation. Navigation previews also use these names. The shared lookup
supplies keys for configured monster skill animations and existing Granny
model/animation loading, so removing the Lua catalog alone would affect more
than ordinary sprite drawing.

This decision concerns the appearance catalog and `ReqJobName` lookup. The
helper file also contains pet-specific lookups used with the next `PetInfo`
load; those will be reviewed separately. Its `GetJobString` reverse lookup
has no C caller found in the current checkout. Keeping the shared load does
not decide every optional helper, individual appearance or legacy C table row.

Sources: `src/session/session_lua.c`,
`src/session/session_job_name_table.c`, `src/session/session_names.c`,
`src/actor/npc.c`, `src/navi/navi_info.c`, `src/game/skill_notify.c`,
`src/resources/granny_res.c`, and read-only local
`client-files/data/luafiles514/lua files/DataInfo/jobName_F.lua` and
`jobName.lua`. No asset scripts were executed or installed assets changed.

The configurable appearance catalog, lookup and active consumers remain
unchanged. Pet-specific support will be reviewed separately. This keep decision
updates only the review ledger; no additional build or tests were needed.

## Unused pet-food precheck (START-151)

Decision: **remove**, approved 2026-11-01.

The next Lua load, `PetInfo`, supplies several independent pet tables. Its
`PetFoodTable` maps a pet type to a food item ID, exposed by `GetPetFood` in
`jobName_F` and, before this removal, `session_get_pet_food` in C. Its sole C
consumer was a conditional branch in
`game_mode_msg_command_pet(command, feeding)`.

When `feeding == 1`, that branch looks for the configured food in the inventory.
If none is found, it formats a missing-food notice and returns without sending
the pet command. The mode-message dispatcher passes argument `c` as this flag.
All current callers traced through the pet information window, popup menu and
`game_mode_confirm_feed_pet` pass zero for it. Both manual feed confirmation
paths therefore already bypass this branch. No independent direct caller of
`game_mode_msg_command_pet` was found.

Normal feeding sends command 1 in packet 0x01a1. The existing 0x01a3 response
handler takes the food ID and success flag from the server, displays success
or missing-food feedback, and triggers the retained pet emotion on success.
That response handling does not depend on the local PetFoodTable lookup. No
server checkout or live behavior was changed or verified for this review.

Removing the inactive branch also removes its `feeding` parameter, exclusive
C food-query wrapper/declaration and private notice color.
Ordinary feed requests, confirmation dialogs, trade/mail restrictions and
server-result messages remain separate existing behavior. The PetInfo load
must remain while its portrait, accessory, egg/species and evolution-related
consumers are reviewed; no installed Lua tables have been edited.

The inspected PetInfo also contains PetNameTable/GetPetName, for which no
current C caller or loose-Lua caller beyond the definition was found. This
external unused table does not justify removing the shared PetInfo load.

Sources: `src/session/session_lua.c`, `src/session/pet.c`,
`src/session/session_talk.h`, `src/game/msg_misc.c`, `src/game/mode_msg.c`,
`src/game/mode_msg.h`, `src/game/mode_util.c`, `src/game/msg_popup_menu.c`,
`src/game/packets_pet.c`, `src/ui/items/pet_info.c`, and read-only local
`client-files/data/luafiles514/lua files/DataInfo/PetInfo.lua` and
`jobName_F.lua`. No asset scripts were executed or changed.

Removed the inactive inventory-check branch and its locals, `feeding`
parameter, `CHAT_COLOR_PET_FOOD`, the C food-query helper/declaration and an
obsolete abort-only test stub. The mode dispatcher now passes only the command.
The sender still calls the same `send_u8(CZ_COMMAND_PET, command)` operation.
Both confirmation routes, UI restrictions, the server reply handler and all
other pet Lua lookups are unchanged. No installed Lua file was edited.

Validation: the three changed C units compiled and the release executable
linked. The existing shortcut-packet regression, which includes the changed
source and boundary stubs, passed its 72 bar slots, extra slots, flags and
fallback checks. This is compile/integration coverage of the touched fixture;
feeding preservation is verified by exact source and packet-path audit, not
a live server test. The audit also confirms no remaining C/test references to
the removed food query and no unrelated working-file changes. No new test was
added for this inactive branch removal.

Artifacts: `build/before-pet-food-check-removal`,
`build/purist-no-pet-food-check.diff`, and
`build/purist-no-pet-food-check-{build,shortcuts,audit}.log`. No foundation patch,
commit or push for this policy cleanup.

## Static pet portraits (START-152)

The retained PetInfo load supplies `PetIllustNameTable_Eng`, mapping pet type
IDs to BMP filenames such as `PET_PORING.bmp`. The current C path calls
`GetPetIllustName_V3`, which prefixes `UserInterface/illust/` and uses
`PET_NOIMAGE.bmp` for entries absent from that table. Older portrait helper
variants are present in the external Lua file but are not called by current C.

The pet status window draws the chosen illustration beside its pet information.
The evolution window also uses it to show the current and target pet; that
window additionally tries the no-image bitmap if loading the selected file
fails. This is static interface artwork. The pet's in-world sprite and accessory
animation are separate paths. Keeping portraits does not decide whether the
pet evolution feature should remain; that is a later review.

The shared C helper originally used `sprintf(out, name)` for the Lua-provided
filename and accepted no output capacity. A filename containing format tokens
was therefore interpreted as formatting input, and a long filename was copied
without a size bound. The status-window caller has a `MAX_PATH + 4` buffer;
the evolution caller has a 0x104-byte buffer. This is a separate correctness
finding suitable for a small foundation patch, independent of pruning policy.

Sources: `src/session/pet.c`, `src/session/session_talk.h`,
`src/ui/items/pet_info.c`, `src/ui/character/pet_evolution_wnd.c`, and read-only
local `client-files/data/luafiles514/lua files/DataInfo/PetInfo.lua` and
`jobName_F.lua`. No asset scripts were executed or installed files changed.

Patrick chose to keep portraits and fix filename handling. The helper now
accepts the destination capacity, copies literal filename bytes and always
terminates successful output. Both callers pass `sizeof` their actual buffers.
A failed Lua call, null result or filename too long for that buffer selects the
existing `UserInterface/illust/PET_NOIMAGE.bmp` fallback. If even that fallback
cannot fit, the output is empty; capacity zero causes no write or Lua call.
Existing valid filenames, including percent characters and non-ASCII bytes,
are preserved without interpreting format tokens or truncating paths. The
portrait rendering and all other pet helpers remain unchanged.

Validation: the three changed C units compiled and the release executable
linked. `tests/build_pet_portrait.bat` compiles the production helper with a
controlled Lua boundary and its real public declaration. All 18 cases passed:
normal/empty/percent/non-ASCII names, failed/null Lua results, exact fits and
oversized names for both 260- and 264-byte callers, fallback boundaries, tiny
buffers and zero capacity. Guard bytes confirm no writes beyond the output.
The original helper, identical in purist and the pinned foundation, failed a
separate literal-name reproduction: `PET_100%%.bmp` became `PET_100%.bmp`.

Foundation patch: `docs/upstream/pet-portrait-filename.patch`, prepared against
`54ca05e3785e1ab2771362b150f8050c2a832b9a`. It contains only the four source/header
changes and three focused test files. It applies cleanly with whitespace
checking to extracted foundation files; all 18 cases pass there too. Exact
source comparison confirms that the patch retains foundation pet dialogue,
food checks and every other unrelated behavior. The foundation checkout was
not edited, and nothing was committed or pushed. No installed assets changed.

The scope audit verifies exact edits, all production callers, unrelated-file
hashes, source line endings and whitespace. Artifacts:
`build/before-pet-portrait-fix`, `build/purist-pet-portrait.diff`,
`build/purist-pet-portrait-{build,test,audit}.log`,
`build/pet-portrait-before.log`, and `build/foundation-pet-portrait-test.log`.
This validation covers filename handling and compilation, not a live in-game
portrait rendering session. Evolution remains a separate review decision.

## Pet accessories (START-153)

PetInfo's `PetAccIDs` identifies pet accessory items; `IsPetAccessory` allows
those items through the inventory's equip action even when ordinary equipment
conditions do not apply. Equip requests still use the game connection and the
server decides the result. This local classification does not define bonuses
or override server eligibility.

`PetAccActNameTable` and `GetPetAccActName` choose the accessory appearance's
ACT resource. Actor-entry packets and pet accessory-state updates load that
appearance. Removing the accessory restores the pet's ordinary appearance.
The pet status window displays whether an accessory is equipped; its menu and
the pet popup can send the existing unequip command (command 4 in 0x01a1).

Current C uses the original `GetPetAccActName`, not the external Lua V2 helper
or English-name table. Lua and C contain a Chonchon gas-mask ACT fallback for
unknown/unavailable mappings. Retaining accessory support need not settle
that fallback policy, which should be reviewed separately.

Sources: `src/session/lua_tables.c`, `src/session/pet.c`,
`src/ui/items/item_wnd.c`, `src/ui/items/pet_info.c`,
`src/game/msg_popup_menu.c`, `src/game/packets_entry.c`,
`src/game/packets_entry_hp.c`, `src/game/packets_pet.c`, and read-only local
`client-files/data/luafiles514/lua files/DataInfo/PetInfo.lua` and
`jobName_F.lua`. No asset scripts were executed or installed files changed.

Patrick chose to keep pet accessory support and review the fallback separately.
The item classification, equip/unequip requests, server-state handling, UI and
configured animation selection remain unchanged. This is a ledger-only keep
decision; no source/assets changed and no build or tests were rerun.

## Pet accessory appearance fallback (START-154)

The read-only local `jobName_F.lua` initializes `GetPetAccActName` to
`chocho_방독면.act` under the monster resource directory, then substitutes a
configured `PetAccActNameTable` entry if one exists. Therefore an unknown
accessory already appears to C as the Chonchon gas-mask path. Independently,
`session_get_pet_accessory_act` originally returned that same hardcoded path
if the Lua call failed or `res_mgr_is_available` could not find the selected
resource. That final C fallback was not checked for file availability.

Four call sites in actor-entry and pet-state handling load the result into
the pet's ACT slot. `render_object_load_act_res` replaces the current ACT;
it does not choose a species-specific fallback. Consequently, an unavailable
accessory can select an unrelated animation even for a different pet. This
source trace does not establish exactly how a mismatched ACT looks in-game.
The availability check tests cached/file presence, not complete ACT validity.

Recommendation: remove both implicit substitutions and restore the pet's
normal appearance when the accessory has no usable mapping/file. Preserve the
server-provided accessory ID, the equipped-status display and unequip request;
this is a presentation fallback, not an equipment-state change. Valid mapped
accessories, including a deliberately configured Chonchon gas-mask animation,
should continue to work. Merely deleting the final C return is insufficient
because the Lua helper also supplies that fallback; implementation must
separate missing mappings from deliberate entries and restore normal animation
rather than leaving a stale previously equipped animation.

Sources: `src/session/pet.c`, `src/actor/game_object.c`, `src/actor/npc.c`,
`src/game/packets_entry.c`, `src/game/packets_entry_hp.c`,
`src/game/packets_pet.c`, `src/fileio/res_cache.c`, and read-only local
`client-files/data/luafiles514/lua files/DataInfo/jobName_F.lua` and
`PetInfo.lua`. No installed files changed or installed Lua scripts executed.

Patrick chose the normal pet appearance. The C lookup now first requires an
explicit, nonempty string in `PetAccActNameTable`, using a protected raw Lua
table lookup with stack restoration. Missing/invalid mappings never call the
legacy helper, so its implicit gas-mask default cannot take effect. For valid
entries the existing `GetPetAccActName` path construction remains in use.
Failed/null/empty results or unavailable files return NULL; the hardcoded C
gas-mask fallback is removed. A deliberate gas-mask table entry still works.
Installed Lua files stay unchanged.

The shared `game_actor_set_pet_accessory` applies a valid ACT, or calls the
existing normal sprite/job setter when none is available. All four existing
accessory application sites use it, clearing stale accessory animation when
switching to a missing mapping/file. Existing packet guards, ownership checks,
accessory IDs, equipped-status UI, equip/unequip requests and normal unequip
handling are preserved. No session/actor layout changes were made.

Validation: all five changed C units compiled and the release executable
linked. `tests/build_pet_accessory.bat` compiles the production lookup and
appearance helper plus extracted actor-entry helpers/packet branches against
real Lua 5.1.4 and the production Lua-call bridge. Controlled resource/actor
boundaries verify all four packet paths through valid -> unknown -> deliberate
gas-mask -> missing-file transitions, normal unequip, ownership/guild guards,
preserved equipped IDs, stale animation reset, invalid/empty/missing tables,
raw lookup without metatable defaults, nil/empty/error results, absent Lua
states and stack preservation. The test executes its own fixtures only.
No live server/rendering session or corrupt-ACT decoding was tested.

The scope audit verifies exact seven-source/header edits and all callers,
preserves every unrelated working file, and checks source line endings and
whitespace. Artifacts: `build/before-pet-accessory-fallback`,
`build/purist-pet-accessory.diff`, and
`build/purist-pet-accessory-{build,test,audit}.log`.
No foundation patch for this chosen presentation policy; the earlier portrait
correctness patch is unchanged. Nothing committed or pushed.

## Pet evolution (START-155)

Before removal, the pet manager loaded `System/PetEvolutionCln_true.lub`, whose
`InsertEvolutionRecipeLGU` calls connect a pet egg to target eggs and material
requirements. The pet menu adds available evolution targets for the currently
hatched egg. `PetEggItemID_PetJobID` and `PetStringTable` in PetInfo provide
species/name lookups for menu labels and the evolution window; the portrait
helper retained under START-152 displays both current and target artwork.

Selecting evolution requires intimacy greater than 900 in the local menu.
The window lists materials, checks the inventory for the required quantities,
and refuses submission while an accessory is equipped. Confirmation sends
target egg and material index/count pairs in packet 0x09fb; the server decides
the result. A reply handler prints the supplied result notice. These local
checks are not the server's evolution implementation or recipe authority.

This manager also holds a separate automatic-feeding eligibility list via
`InsertPetAutoFeeding`; the pet menu uses it for its auto-feed toggle. Removing
evolution alone must retain that list/loader support until the separate
automatic-feeding decision. Ordinary pets, portraits, accessories, feeding and
returning pets to eggs do not depend on keeping the evolution window.

Sources: `src/contents/pet_evolution.c`, `src/session/session_lifetime.c`,
`src/session/pet.c`, `src/ui/items/pet_info.c`,
`src/ui/character/pet_evolution_wnd.c`, `src/game/packets_pet.c`, and read-only
local PetInfo/jobName_F catalogs. These paths describe the reviewed feature;
the evolution source/header/window files are now deleted.

Patrick chose removal. Deleted the evolution window and construction/type
registration, menu target enumeration/choice, recipe tree and associated data
types/queries/validation, exclusive egg-to-species and pet-name C lookups,
0x09fb request implementation, 0x09fc reply/dispatch/header/table entries and
the obsolete test sink. Numeric IDs of unrelated packets and windows were not
renumbered. Pet portraits in the ordinary status window remain supported.

Replaced the combined manager with `src/contents/pet_auto_feed.c/.h`, which owns
only the automatic-feeding eligibility vector. Its startup/shutdown and the
three existing eligibility callers are retained under the new names. While
automatic feeding is pending review, it continues reading the same combined
`System/PetEvolutionCln_true.lub` file. `InsertEvolutionRecipeLGU` is now only a
compatibility callback returning success without storing or processing recipes,
so existing catalog scripts can reach `InsertPetAutoFeeding`. No installed
catalog was changed. A missing auto-feed manager safely reports ineligible.

Validation: all 1,164 current C units rebuilt and the release executable linked.
The shared Lua-data manager suite passed real loader/population/failure/cleanup
checks, including retained eligibility, duplicate suppression, mixed legacy
recipe calls and exact list contents. Its complete trace equals the passing
pre-removal trace with only recipe outputs replaced by the retained list IDs
(new digest `756fb6ed`). No unrelated manager output changed.

The dispatch suite passed all 1,179,800 cases. A separate before/after trace
comparison confirms only twelve cases changed: six data patterns for 0x09fc
through each dispatcher entry point, now taking the existing unknown-packet
path. The expectation is updated to `0e20d9f11f3d4d93`, with an explicit assertion
that the removed reply is unhandled. The type oracle now rejects the retired
evolution-window identity and passes 3,375 comparisons against the archived
runtime, 250 accepted casts, 409 names, 388 window names and 56 retired types.
Its old minimum-success assertion became stale after this removal and was
replaced by the verified exact totals. Window-reference cleanup checks passed.
Archived type-reference fixtures remain as historical test data.

The scope audit checks 28 files, including three deletions and two additions,
exact edits, removed source/link references, preserved unrelated working files,
foundation patches, line endings and whitespace. Artifacts:
`build/before-pet-evolution-removal`, `build/purist-no-pet-evolution.diff`,
`build/purist-no-pet-evolution-{build,managers,dispatch,types,audit}.log`, and
`build/pet-evolution-dispatch-comparison.log`. No live client/server session was
run. No foundation patch for this policy removal; nothing committed or pushed.

## Pet automatic feeding (START-156)

The retained `InsertPetAutoFeeding` list identifies eligible pet egg items.
Their pet menu includes an automatic-feeding toggle. Selecting it sends packet
0x02d8 with configuration kind 2 and the requested on/off value. The client
does not implement a hunger threshold, food-consumption loop or timed feeding
job here: it asks the server to manage feeding.

Server configuration notifications/acknowledgments update `petAutoFeed` and
display enabled/disabled chat feedback. The initial local state is off. When
hatched-egg inventory/equipment updates identify an ineligible pet while auto
feeding is on, the client requests that it be disabled. Ordinary manual feeding
and server feedback are separate retained behavior. Homunculus auto-feeding
uses kind 3 through shared packet machinery and remains a separate decision.

Sources: `src/contents/pet_auto_feed.c`, `src/ui/items/pet_info.c`,
`src/session/session_auto_feed.c`, `src/session/session_init_state.c`,
`src/game/user_config.c`, `src/game/equip_items.c`, and
`src/game/inventory_lists.c`. Server feeding behavior has not been tested live.

Patrick chose to keep pet automatic feeding. The eligibility catalog, menu
control, request packet, server-confirmed state and feedback remain. The
combined legacy catalog filename/no-op recipe callback retained under START-155
are still needed to load this eligibility list without changing installed
assets. This keep decision changes only the ledger; no source or asset edits,
build or test reruns.

## Homunculus automatic feeding (START-157)

The homunculus status window creates an automatic-feeding checkbox, initialized
from `homunAutoFeed`. Clicking it sends the shared 0x02d8 configuration request
with kind 3 and an on/off value. Unlike the pet menu, this control does not use
the pet egg eligibility catalog or a local species allowlist.

Server configuration notifications and kind-3 replies update `homunAutoFeed`
and print enabled/disabled chat feedback. The local initial state is off.
As with pets, this client path requests a server policy rather than running a
local feeding timer or consuming food itself. Actual thresholds, eligibility
and consumption are server behavior not verified live here. Manual feeding,
homunculus AI and skills are separate support.

Sources: `src/ui/character/companion_info.c`,
`src/session/session_auto_feed.c`, `src/session/session_init_state.c`,
`src/game/user_config.c`, and `src/session/state.h`.

Patrick chose to keep homunculus automatic feeding. The status-window checkbox,
configuration request, server-state handling and chat feedback remain unchanged.
This is a ledger-only keep decision; no source/assets changed and no build or
tests were rerun. Review resumes at the next Lua startup catalog.

## Headgear appearance catalogs (START-158)

After PetInfo, startup loads `AccessoryId`, `accName_F` and `accName`.
`ACCESSORY_IDs` gives readable constants to numeric appearance IDs (for example,
Goggles = 1 and Catty Hairband = 2 in the inspected local catalog).
`AccNameTable` assigns filename suffixes to these IDs; `ReqAccName` retrieves
those suffixes. They identify headgear artwork rather than inventory item IDs
or equipment statistics. The external English-name V2 helper is not used by
the current C path.

`ui_window_mgr_render_menu`, despite its name, populates session appearance
name tables, including `accessoryNames`. `session_get_accessory_name` retrieves
a suffix; `resource_path_accessory` combines it with the character sex/race
folders and the requested SPR/ACT extension. The character accessory setters
load these resources into three headgear layers. This is shared support for
ordinary and custom headgear artwork, separate from the pet accessories kept
under START-153 and from item names/icons in the item-information catalog.

The initializer currently queries IDs 1 through 2999, with ID 0 empty.
Out-of-range lookups and failed initialization lookups have a hardcoded chick-hat
suffix; absent entries successfully returned by local `ReqAccName` are empty.
Doram resource selection also has a separate fallback to the standard folder.
Keeping the shared catalog does not approve these fallback or limit policies;
review them individually next. No changes proposed to those policies yet.

Sources: `src/session/session_lua.c`, `src/session/session_job_names.c`,
`src/session/sprite_names.c`, `src/session/session_names.c`,
`src/ui/resource/resource_paths.c`, `src/actor/pc_setters.c`, and read-only local
`client-files/data/luafiles514/lua files/DataInfo/AccessoryId.lua`,
`accName_F.lua` and `accName.lua`. No installed Lua was executed or changed.

Patrick chose to keep the headgear catalogs and review fallback/limits
separately. The Lua loads, name table, resource path construction and headgear
layer consumers remain unchanged. This keep decision changes only the ledger;
no source/assets changed and no build or tests were rerun.

## Hardcoded headgear substitute (START-159)

Before removal, the appearance-name vector initialized new slots with `_병아리모자`
(Chick Hat), then empties ID 0 and calls `ReqAccName` for IDs 1 through 2999.
Successful calls replace the initial value, including the empty string that
the inspected Lua helper returns for absent entries. A failed lookup can leave
the initial Chick Hat value. Independently, `session_get_accessory_name` returns
that same suffix for negative IDs or IDs outside the vector.

The suffix becomes a sex-specific SPR/ACT path. The code therefore tries the
Chick Hat artwork for some unresolved IDs, including any positive ID >= 3000,
regardless of the intended headgear. This is not a fallback for every missing
SPR/ACT file: a valid catalog name with absent artwork follows ordinary resource
loading instead. The separately retained catalog and supported range should
not be conflated with this substitution policy.

Recommendation: initialize missing names as empty and return an empty name
for unresolved/out-of-range IDs. `resource_path_accessory` already leaves an
empty path when the name is empty; the existing headgear setters release the
previous layer resources and load no replacement, as with no headgear. The
appearance ID stored on the character and server equipment state remain;
this change affects presentation only. Deliberate Chick Hat mappings must
continue to work. Review the current 2999 upper supported ID and the separate
Doram-to-standard-folder fallback afterward.

Sources: `src/session/session_job_names.c`, `src/session/sprite_names.c`,
`src/ui/resource/resource_paths.c`, `src/actor/pc_setters.c`,
`src/fileio/res_get.c`, and read-only local `accName_F.lua`.
Patrick chose no headgear for unresolved IDs. The initial name-slot fill is
now empty, and the out-of-range lookup returns its already-initialized empty
String instead of assigning a Chick Hat suffix. Successful Lua mappings,
including deliberate Chick Hat artwork, remain unchanged. Appearance IDs and
equipment state still follow the existing setters; no item/stat logic changed.
The 3000-slot limit remains for the next decision.

Validation: the two changed C units compiled and the release executable linked.
The existing resource-path suite passed, including empty accessory names,
ordinary/Doram headgear paths, fallback paths, CP949 strings and ownership.
That suite substitutes the name lookup; the new default and out-of-range
behavior are verified by exact source audit rather than a new mirrored test.
The unchanged actor setters store the appearance ID, release old resources and
load the resulting path; an empty path clears the layer through the existing
no-headgear path. No live in-game rendering test was run.

The audit confirms exactly two source edits plus this ledger, no remaining
hardcoded Chick Hat suffix in either lookup source, preserved unrelated files
and clean line endings/whitespace. Artifacts:
`build/before-headgear-fallback-removal`, `build/purist-no-headgear-fallback.diff`,
and `build/purist-no-headgear-fallback-{build,paths,audit}.log`.
No installed assets changed. This is a purist display policy, with no foundation
patch, commit or push.

## Headgear catalog ID limit (START-160)

`ui_window_mgr_render_menu` sizes `accessoryNames` to 0xbb8 (3000) pointers and
calls `ReqAccName` only for IDs 1-2999. ID 0 means no headgear. An otherwise
valid Lua entry at 3000 or higher never enters this cache. Following START-159,
the getter returns an empty name for those IDs rather than the former Chick
Hat substitute. This is a fixed client catalog limit, independent of whether
the matching SPR/ACT files exist.

The inspected loose `AccessoryId.lua` contains 2,133 numeric assignments and
its highest value is 2153, so this particular catalog fits. This does not
establish what other GRFs or a future custom catalog contain. Character
accessory state uses int fields, while the inspected actor-entry packets use
signed 16-bit accessory fields; removing the 2999 catalog cap does not by
itself widen the protocol or establish support for every possible integer ID.

Recommendation: remove the arbitrary fixed catalog cap so configured custom
appearance IDs can resolve, preserving existing packet formats. Resolve/load
actual configured IDs rather than blindly adding another large fixed bound;
keep zero/invalid/unmapped IDs empty. The separate protocol widths and Doram
resource fallback remain separate review topics.

Sources: `src/session/session_job_names.c`, `src/session/sprite_names.c`,
`src/actor/types.h`, `src/game/packets_entry.h`, and read-only local
`client-files/data/luafiles514/lua files/DataInfo/AccessoryId.lua` and
`accName_F.lua`. No installed scripts were executed or changed.

Patrick chose to remove the fixed catalog cap. `session_get_accessory_name`
now calls the existing `ReqAccName` helper for the requested positive appearance
ID and immediately copies a successful non-null result into the caller-owned
String. Zero/negative IDs, missing entries, absent Lua and failed/null results
leave the name empty, retaining START-159's no-substitute policy. Custom Lua
lookup logic is preserved rather than bypassed with a raw table query.

The 3000-pointer cache, its startup sweep, Session field and lifetime allocation
hooks are removed. Resource paths, headgear setters, appearance/equipment state,
Lua assets and packet formats are unchanged. These lookups happen when resolving
artwork paths, not on every rendered frame. Name resolution no longer retains
Lua-owned pointers in this cache, and changes to the Lua mapping are seen on
subsequent lookups. Resource loading still uses the existing resource manager.

Validation: `tests/build_headgear_names.bat` passes 19 lookup cases plus
an owned-string lifetime case, using the extracted production lookup, real Lua
5.1.4, production Lua bridge and production String implementation. Coverage
includes IDs 2999/3000/10000/32767, an INT_MAX lookup (not a protocol claim),
zero/negative/missing/empty names, CP949 bytes, changed/missing catalogs, Lua
errors/nil/missing functions, absent Lua, stack preservation, and a computed
filename surviving Lua GC and shutdown. Allocation guards and cleanup pass.
The existing session-field and resource-path suites also pass. The latter
substitutes name lookup and checks the unchanged path construction separately.
All 1,164 C units rebuilt and the release executable linked successfully,
covering the changed shared Session layout.
No live in-game rendering test was run.

Artifacts: `build/before-headgear-cap-removal`,
`build/purist-headgear-cap-{build,test,fields,paths,audit}.log`, and
`build/purist-headgear-cap.diff`. The exact scope audit verifies four source/header
files, three focused test files and this ledger; unrelated work and existing
foundation patches are preserved, with clean whitespace and line endings.
This is the approved purist catalog policy/capability change; no standalone
foundation patch, installed-asset change, commit or push.

## Doram headgear resource fallback (START-161)

For a Doram character, `resource_path_accessory` first constructs the configured
headgear's path in the sex-specific `_doram` folder. If the resource manager
reports that file available, it uses it. Otherwise it constructs the same
headgear name in the standard sex-specific folder. ACT and SPR requests each
perform this selection independently. This does not substitute a different
appearance ID or a different named hat. Empty catalog names remain empty.

Recommendation: keep this reuse of standard headgear artwork while Doram
support remains. Removing it would require a separate Doram asset for every
hat to retain the current appearance. Race-specific files still take priority.
The broader decision about Doram characters is separate and not yet approved.
Source: `src/ui/resource/resource_paths.c`; existing path tests exercise both
Doram-specific availability and standard-folder fallback.

Patrick chose to keep this fallback. Doram-specific files retain priority, and
the same configured headgear name is used from the standard folder if the
race-specific file is absent. This keep decision changes only the ledger; no
source/assets changed, and no build or tests were rerun. The ledger-only scope
check and whitespace check passed. Artifact:
`build/purist-keep-doram-headgear-fallback.diff`.


## Optional xofyldas startup script (START-162, removed)

Immediately after `accName`, `session_init_lua_files` calls
`run_lua(self, "xofyldas")`. The normal Lua loader searches for
`data/LuaFiles514/xofyldas.lub`, then `.lua`, through the ordinary file manager.
If present, the script executes in the shared session Lua state; its return
values are discarded. If both files are missing, loading returns false quietly,
and this caller ignores that result and continues with chat-window settings.
A present script could modify Lua globals or call registered functions, so
its effect cannot be inferred merely from this filename or discarded result.

The sole source reference is this startup call. No matching loose file was
found in `client-files`; all filename records in its two GRFs were inspected
read-only: `data.grf` (160,696 entries) and `revel.grf` (44,053 entries), with
no case-insensitive xofyldas matches. The script's original purpose remains
unverified; this is not evidence that it is empty in all client distributions.
No installed Lua was executed or changed. Inspection record:
`build/xofyldas-archive-inspection.json`.

Recommendation: remove this unexplained optional startup load from the purist
base, keeping the explicitly named catalogs and settings loads.
Sources: `src/session/session_lua.c`,
`src/contents/client_lua.c`, `src/fileio/file.c`, `src/fileio/file_mgr.c`, and
`src/fileio/grf_pak.c`.

Patrick approved removal. The only source change deletes
`run_lua(self, "xofyldas");` from session startup. The shared Lua loader, all
other catalog/settings loads and installed assets remain unchanged. The changed
C unit compiled and the release executable linked successfully. No new tests
were added for this single optional-call removal; an exact byte-level scope
check verifies the source delta and confirms no remaining xofyldas source/test
references. Unrelated work and foundation patches are preserved, with clean
whitespace and CRLF. Artifacts: `build/before-xofyldas-removal`,
`build/purist-no-xofyldas.diff`, and
`build/purist-no-xofyldas-{build,audit}.log`.
This purist policy change has no foundation patch, commit or push.

## Chat tabs and detached-window settings (START-163, kept)

Startup first tries the local `SaveData/ChatWndInfo_U.lub` or `.lua` through
`run_saved`. If loading fails or `ChatSubWnd_1` is not a global table, it loads
the default `Lua Files/ChatWndInfo/ChatWndInfo` catalog instead. It then loads
`ChatWndInfo_F` and reads `CanDragWnd` into the session's layout-editing flag.

The registered Lua callbacks populate the main chat tabs and separate chat
windows: names, detached positions/sizes and 22 message-category visibility
flags. The Lua import supports ten entries in total. The UI constructs the
tabs and detached windows from these records, can move tabs between the two
forms, and provides a layout lock. When the main chat window is destroyed,
its settings are written to `SaveData/ChatWndInfo_U.lua` for the next launch.
This file is shared by this client installation rather than keyed by account.
These records are layout/filter preferences, not saved chat-message history.

Recommendation: keep this customizable chat support and its local persistence,
then review individual options, limits and save behavior separately. This is
separate from the general registry window layouts kept under START-055.
Sources: `src/session/session_lua.c`, `src/session/chat_tabs.h`,
`src/session/chat_tabs_lua.c`, `src/session/chat_tabs_layout.c`,
`src/contents/client_lua_funcs.c`, `src/ui/chat/chat_window.c`, and
`src/ui/chat/chat_window_msg.c`.

Patrick chose to keep chat layouts and review policies separately. Tabs,
detached windows, names, filters, geometry, layout lock and local persistence
remain unchanged. This is a ledger-only keep decision; no source/assets changed
and no build or tests were rerun. The scope and whitespace checks passed.
Artifact: `build/purist-keep-chat-layouts.diff`.

Separate correctness follow-up for the retained writer: tab names are inserted
verbatim into either Lua long brackets or quoted strings, and file open/write
results are ignored. Verify delimiter/backslash handling and failed saves when
reviewing persistence; prepare any confirmed general correction independently
for the foundation. No fix or failure reproduction was attempted in this keep
step.


## Combined chat-panel limit (START-164, kept)

The main chat window initializes `max_tabs` from `CHAT_MAX_TABS` (10).
`new_tab` sums main tabs and detached windows; when this equals the limit, it
shows the too-many-tabs notice and returns. If capacity remains but the tab
would not fit across the main window, it creates a detached window instead.
Moving an existing panel between docked and detached forms does not add to the
total. The Lua import has the corresponding `CHAT_TAB_LIMIT` of ten and stops
adding entries once the combined count reaches it. These are currently two
separate constants with the same value.

Recommendation: keep the existing ten-panel capacity for now. This is a UI
capacity policy, separate from chat history length, message filters and network
traffic. Raising/removing it should be a deliberate change that checks both
creation and saved-layout import, including the external Lua helper; it is not
necessary to keep the chat support already approved.

Sources: `src/ui/chat/chat_window.h`, `src/ui/chat/chat_window.c`,
`src/ui/chat/chat_window_msg.c`, `src/session/chat_tabs.h`, and
`src/session/chat_tabs_lua.c`. The internal tab storage itself is dynamic;
the ten-panel bound comes from these import/UI policies.


Patrick chose to keep the ten-panel limit. Both UI creation and Lua import
retain their current capacity policy; docked and detached panels still count
toward the same total. This changes only the review ledger. No source/assets
changed and no build or tests were rerun. The ledger-only scope and whitespace
checks passed. Artifact: `build/purist-keep-chat-panel-limit.diff`.

## Chat layout lock (START-165, kept)

The expanded main chat window draws a padlock icon using `canDragWnd`; clicking
it toggles the flag and refreshes both the main and detached chat controls.
When locked, the main chat drag handler does not move its frame, and the tab
transfer handlers refuse detaching or reattaching tabs. Detached windows block
drag initiation, double-click renaming and resize-control messages. Their
close and docking buttons are hidden; the main panel also hides its layout
editing buttons. Message-filter buttons remain available.

The flag is loaded from `CanDragWnd` in the chat settings and saved to
`SaveData/ChatWndInfo_U.lua` when the main chat window is destroyed. This is a
player-controlled layout preference, not a chat-message or network restriction.
It is not a blanket prohibition on every UI operation; the specific guards
above control the affected interactions.

Recommendation: keep the toggle and its saved state to prevent accidental
rearrangement. Sources: `src/ui/chat/chat_window.c`,
`src/ui/chat/chat_window_msg.c`, `src/ui/chat/sub_chat.c`,
`src/ui/chat/chat.h`, `src/session/session_lua.c`, and
`src/session/chat_tabs_layout.c`.

Patrick chose to keep the chat layout lock. The padlock toggle, interaction
guards, button visibility and saved preference remain unchanged. This keep
decision changes only the ledger; no source/assets changed, and no build or
tests were rerun. Ledger-only scope and whitespace checks passed. Artifact:
`build/purist-keep-chat-layout-lock.diff`.


## Chat-layout save scope (START-166, shared layout kept)

The startup loader uses the fixed `SaveData/ChatWndInfo_U` basename, and the
main chat window saves to the fixed `SaveData/ChatWndInfo_U.lua` path. Neither
path includes server, world, account or character identity. Consequently,
accounts sharing a client folder inherit the same last-saved layout: tab names,
message filters, detached-window positions/sizes and the layout lock.

The separate local account preferences introduced under START-038 already
have server/world/account identity and save/load lifecycle support, but their
current JSON capture/apply functions only include hotkeys, chat/emotion macros
and blocked-whisper names. Chat layout is not part of those account records.
Their filename key includes server address, port, world and account ID. Character
settings are separately scoped and need not change for this choice.

Recommendation: save chat layouts locally per server/account, using the existing
shared layout as the initial default for accounts without a saved layout. This
would let each account retain its chosen filters and arrangement, consistent
with the earlier local preference policy. Preserve the ten-panel limit and
player-controlled lock. Review/test account switching and save/load order as
part of any implementation; do not silently transfer the previous account's
layout to the next. Patrick subsequently chose to keep the shared file instead.

Sources: `src/session/session_lua.c`, `src/ui/chat/chat_window.c`,
`src/session/chat_tabs_layout.c`, `src/contents/account_settings.c`,
`src/contents/account_settings_json.c`, `src/contents/settings_file.c`, and
`src/login/login_states.c`. General save-string escaping and failed-save
handling remain correctness follow-ups for the foundation as well as purist,
independent of the chosen storage scope.


Patrick chose one shared chat layout per client folder. The fixed layout path,
load/save lifecycle and sharing between accounts remain unchanged; no account
or character settings were added. The subsequent literal-name correction below
preserves this choice. Shared-layout policy itself required only this ledger
update.

## Literal chat-layout names (START-167, correctness fix)

The retained writer previously inserted docked names directly inside Lua long
brackets and detached names directly inside double quotes. Both UI rename paths
accept nonempty names within their display-width limits without escaping these
Lua delimiters. For example, `A]]B` in a docked tab or `A"B` in a detached tab
can make the generated layout fail to parse. Detached `A\nB` is restored with
a newline instead of a literal backslash followed by n. Long-bracket strings
can also normalize line endings. This affects the foundation writer too.

The existing production-writer/Lua round-trip test was expanded before the fix.
It failed on quotes, backslashes, closing brackets, line breaks and byte/long-name
cases, confirming this is a serialization defect rather than a desired policy.
The writer now uses quoted Lua strings for both panel types, escaping quotes,
backslashes and non-printable/non-ASCII bytes as exactly three decimal digits.
It processes the stored name length, preserving every byte. Fixed-size output
chunks avoid per-character file writes or allocations. Plain ASCII names remain
readable in the file, while the restored displayed name is unchanged.

The shared filename, ten-panel limit, layout lock, geometry, message flags,
loading policy and write-error behavior are unchanged. Existing Lua layouts
remain readable. General file-write failure handling remains a separate queued
correctness follow-up; this patch only fixes name serialization.

Validation: the expanded suite checks 18 name inputs in both docked and detached
layouts against real Lua 5.1, including quotes, backslashes, Lua delimiters,
percent signs, Korean bytes, all 256 byte values, and long names with escapes
at buffer boundaries followed by decimal digits. It checks that delimiter-like
text does not create extra Lua fields. Existing geometry/options, ordering,
4096-byte name, Lua import/limit, allocation-rollback and 2000 edit-sequence
checks also pass. The changed C unit compiled and the release executable linked.
No live UI test was run.

Standalone foundation patch: `docs/upstream/chat-layout-names.patch`, two files
(the writer and its existing test). It applies cleanly against
`54ca05e3785e1ab2771362b150f8050c2a832b9a`, preserves the foundation's historical
Lua test hook and unrelated features, and passes the chat suites in an isolated
extracted tree. The actual foundation checkout was not modified. No commit or
push was made.

Artifacts: `build/before-chat-layout-name-fix`, `build/chat-layout-names-before.log`,
`build/purist-chat-layout-names-{build,test,audit}.log`,
`build/foundation-chat-layout-names-test.log`, and
`build/purist-chat-layout-names.diff`. Scope checks preserve all unrelated work
and existing foundation patches; changed source/test/ledger retain CRLF.

## Compiled chat-layout priority (START-168, removed)

`run_saved(self, "SaveData/ChatWndInfo_U")` uses the shared loader's extension
search: `.lub` first, then `.lua` only if the `.lub` does not exist. The writer
always saves `SaveData/ChatWndInfo_U.lua`. Therefore an older `.lub` left beside
the saved `.lua` keeps winning at startup, even after players change and save
their layout. A malformed `.lub` fails loading and triggers the packaged-default
path rather than retrying the user's `.lua`. This is a code-path finding; no
claim is made that such a compiled file exists in Patrick's runtime folder.

Recommendation: remove compiled-layout import specifically for this writable
user-settings file and load its `.lua` directly. Continue using packaged chat
defaults when the saved Lua layout is absent or invalid. Other compiled asset
catalogs and saved-setting policies remain separate. Sources:
`src/session/session_lua.c`, `src/contents/client_lua.c`, and
`src/ui/chat/chat_window.c`.


Patrick chose to remove `.lub` import for chat layouts. Startup now calls
`client_lua_load_local_file` with the exact `SaveData/ChatWndInfo_U.lua` filename.
This entry point checks/reads only the named local file: no automatic extension
search, resource prefix or archive lookup. The existing condition still falls
back to packaged chat defaults on load failure or a missing `ChatSubWnd_1` table.
The shared file policy, save writer, ten-panel bound and layout lock are unchanged.
Existing `.lub` files are left on disk and ignored by this chat-layout path.

The read/compile/execute/cleanup body is shared with the ordinary catalog loader.
All existing `client_lua_load_file` callers retain their prefix and `.lub`-first
extension search. The other `run_saved` settings also retain their current
policy. This is an exact-filename policy, not a new restriction on the Lua chunk
format stored inside a `.lua` file.

Validation: extended the existing production-loader/Lua boundary test for an
explicit saved path when both extensions exist, missing `.lua` with `.lub`
present, failed file reads, syntax/runtime errors, empty scripts, absent Lua,
stack/ownership cleanup and preserved compiled packaged-catalog selection.
The allocation-failure sweep now covers the exact local path as well as the
legacy loader: 419 initialization and 78 query/load allocation-failure positions
passed. The Lua call suite also passed its existing value/error/ownership cases.
Both changed C units compiled and the release executable linked. The unchanged
startup fallback condition is checked by exact source comparison; the full
interactive startup flow was not run.

The scope audit confirms only the loader, its declaration, the single startup
call, its existing boundary test and this ledger changed. Prior foundation
patches and unrelated work are preserved. Artifacts:
`build/before-chat-layout-lua-only`, `build/purist-chat-layout-lua-only.diff`, and
`build/purist-chat-layout-lua-only-{build,test,audit}.log`. No installed settings
or assets changed. This approved purist import policy has no foundation patch,
commit or push.

## Chat-layout save failures (START-169, corrected)

`chat_tabs_save_layout` opens the shared layout through `file_open` with a write
mode, which uses Windows `CREATE_ALWAYS`. The existing target is therefore
truncated before the new layout has finished writing. The writer ignores both
open and write return values, has no success/failure result for its caller, and
the file abstraction does not report flush/close failures. A disk/write failure
can leave an incomplete layout. An open failure already raises a generic
filename message, but failed writes after opening have no clear user notice.
This finding follows the production file/write paths; injected filesystem
failure reproduction remains part of the correction work.

The general correction should write a temporary sibling, verify write/flush/
close results, and replace the old layout only after a complete save. Preserve
the prior file on failure and remove temporary leftovers. Keep the same shared
layout path and the START-167 name serialization. Prepare the correctness change
separately for the foundation as well as purist.

Feedback preference to review: one clear visible warning per failed save, or
only a diagnostic log entry. Recommend the warning so players know their latest
layout changes were not saved. This question concerns presentation; preserving
the previous valid layout is part of fixing the retained save functionality.
Sources: `src/session/chat_tabs_layout.c`, `src/fileio/file.c`,
`src/session/chat_tabs_layout.h`, and `src/ui/chat/chat_window.c`.


Patrick chose one clear warning. The writer now returns success/failure, creates
a temporary sibling file and checks every write for both API failure and a short
byte count. Failure is sticky: no further writes or final replacement occur.
It checks flush and close before replacing the target with `MoveFileExA` on the
same volume, without the copy-across-volumes flag. The destination is not opened
for truncation. Failure triggers a best-effort temporary-file deletion; if the
OS also denies deletion, the previous saved layout is still preserved, though
a temporary file can remain.

The chat-window destructor finishes its normal vector/frame/object cleanup,
then shows one task-modal Windows warning on failure: "Could not save your chat
layout. Your latest layout changes will not be restored next time." This avoids
creating an in-game dialog during window teardown. Success shows no warning.
The shared filename, save timing, ten-panel limit, lock, `.lua` load policy,
names, geometry and filter values are unchanged. The general File abstraction
and other save features are unchanged.

Validation: the existing layout tests now save/read real files before loading
them through Lua, preserving all 18 literal-name cases, geometry, options,
ordering and long-name round trips. A new focused test runs the actual writer
against real temporary files with injected Windows API failures. It verifies
all 63 serializer write positions as both failed and short writes (126 cases),
plus path resolution/length, temporary creation, opening, flush, close and
replacement failures. Real locked/read-only targets, missing directories,
relative paths, first-save failure/success and simulated temporary-delete denial
are covered. The previous file is byte-for-byte unchanged in failure cases;
handles are closed and normal failure paths remove their temporary files.
The close-failure injector closes the real test handle, then reports failure to
verify replacement is withheld. It does not emulate every possible OS handle
failure state.

The actual production destructor is extracted into the focused test; only its
UI cleanup and MessageBox boundaries are substituted. It verifies full cleanup
before exactly one warning on each failure, and no warning on success. The
remaining chat storage/Lua/limit/allocation tests pass. Both changed C units
compiled and the release executable linked. No live game-window test or
power-loss test was run.

Foundation patch: `docs/upstream/chat-layout-safe-save.patch`. Apply it **after**
`docs/upstream/chat-layout-names.patch` on foundation commit
`54ca05e3785e1ab2771362b150f8050c2a832b9a`. The seven-file patch contains only
save/warning support and tests; it preserves foundation-only Lua test hooks,
curl include paths and unrelated chat behavior. Both patches applied cleanly
in a fresh isolated tree, and the complete chat suite passed there. The actual
foundation checkout was not changed. No commit or push was made.

Artifacts: `build/before-chat-layout-safe-save`,
`build/purist-chat-layout-safe-save-{build,test,audit}.log`,
`build/foundation-chat-layout-safe-save-test.log`, and
`build/purist-chat-layout-safe-save.diff`. The scoped audit covers the three
source/header files, two existing test/build files, two added test helpers,
this ledger and the new patch; prior patches/unrelated work remain unchanged.

## Job inheritance for the skill window (START-170, kept)

After chat-window settings, startup loads `Lua Files/SkillInfoz/jobinheritlist`.
`session_init_skill_tree_view` asks Lua's `GetInheritJob` about the current job,
then resolves the returned entries through `JobInheritTable` and optional
`JOB_INHERIT_LIST2` remapping. It calls `InitSkillTreeView` for those jobs,
assigning stage groups used by the skill window. Refreshing the skill tree
rebuilds these view records and refreshes the visible skill window. This is
shared support for showing inherited skills from earlier job stages.

Recommendation: keep this catalog and its lookup support, then review the skill
IDs, names/descriptions, individual class lists and presentation policies in
order. Keeping this lookup does not independently choose which jobs a server
should offer. Sources: `src/session/session_lua.c`, `src/session/lua_tables.c`,
`src/session/session_redraws.c`, and the `src/ui/character/skill_tree*.c`
consumers. The inspected loose `data/lua files/newskillinfo/jobinheritlist.lua`
is in a different path from the active SkillInfoz load, so it is not treated
as proof of the active runtime catalog.

Patrick chose to keep job-inheritance support. The startup load, lookup helpers
and skill-window consumers remain unchanged. This is a ledger-only decision;
no source, runtime assets or foundation patches changed, and no build or tests
were needed. The scoped diff is `build/purist-keep-job-inheritance.diff`.

## Skill-ID definitions (START-171, kept)

The next startup load is `Lua Files/SkillInfoz/SkillID`, before `SkillInfoList`,
`SkillDescript`, `SkillInfo_F` and `skilltreeview`. The inspected loose SkillInfoz
catalog defines an `SKID` table of readable symbols and numeric IDs, for example
`NV_BASIC = 1`, `SM_BASH = 5` and `MG_FIREBOLT = 19`. Its companion skill-info
catalog indexes records by these symbols, and the skill-tree catalog uses them
for the skills placed in each class's grid. This gives the data files a shared
vocabulary for skill IDs; the definitions alone do not calculate damage or
grant skills to a character.

Recommend keeping this shared lookup, then reviewing the skill metadata,
descriptions and tree layouts separately. Sources: the startup order in
`src/session/session_lua.c`, the resource prefix in `src/contents/client_lua.c`,
and read-only inspection of `client-files/data/lua files/skillInfoz/skillid.lua`,
`skillinfolist.lua` and `skilltreeview.lua`. These loose files illustrate the
catalog contract; the loader prefixes resource requests with `LuaFiles514`,
so this inspection does not establish which catalog the packaged runtime uses.

Patrick chose to keep the skill-ID definitions. The startup load remains
unchanged. This is a ledger-only decision; no source, runtime assets or
foundation patches changed, and no build or tests were needed. The scoped
diff is `build/purist-keep-skill-ids.diff`.

## Skill-information catalog (START-172, kept)

Startup next loads `Lua Files/SkillInfoz/SkillInfoList`. The inspected loose
catalog defines `SKILL_INFO_LIST`, indexed by skill ID. Entries supply readable
names, icon/resource identifiers, maximum levels, general or job-specific
prerequisites, per-level SP/AP costs and flags for choosing a casting level.
Some entries also supply per-level attack range and warning-area dimensions.
The companion `SkillInfo_F` functions expose these fields to the client.

C consumers use this data for skill names and icons, skill-tree records and
requirements, cost displays and casting-level selection. `skill_attack_range`
combines the Lua range for the selected level with the difference between the
known skill's range and its Lua baseline. Warning-plane effects query the
configured area dimensions. This catalog therefore influences client actions
as well as presentation; it is not merely a text-description file. It should
match the server's skills. Keeping it does not require keeping every existing
class or skill as a gameplay policy.

Recommend keeping this shared metadata support, then reviewing the separate
`SkillDescript` load, helpers, tree layouts and individual policies in order.
Sources: `src/session/session_lua.c`, `src/skills/skill_info.c`,
`src/contents/client_lua_funcs.c`, `src/game/msg_skills.c`,
`src/game/effect_notify.c`, `src/ui/character/skill_dialogs.c`, and skill-window/
hotkey consumers. Read-only loose asset inspection covered
`client-files/data/lua files/skillInfoz/skillinfolist.lua` and `skillinfo_f.lua`;
as with START-171, their path is not proof of the active prefixed runtime data.

Patrick chose to keep the skill-information catalog. Its startup load and
consumers remain unchanged. This is a ledger-only decision; no source, runtime
assets or foundation patches changed, and no build or tests were needed. The
scoped diff is `build/purist-keep-skill-information.diff`.

## Skill-description text (START-173, kept)

Startup next loads `Lua Files/SkillInfoz/SkillDescript`. The inspected loose
catalog contains `SKILL_DESCRIPT`, arrays of text lines keyed by skill ID,
including explanations, level-by-level effects, requirements and color markup.
These are local descriptions that can be translated or customized to describe
the server's skills. The text does not calculate the described effects.

The description window asks Lua's `GetSkillDescript` for the current job and
skill, copies the returned text and sends it to its text viewer. The inspected
Lua helper joins lines and handles job-specific requirement text and optional
requirement highlighting. The window also gets the skill name, icon and costs
through the separately retained metadata/session support. Skill-tree hover
panels use the description window when the saved description toggle is enabled;
other skill lists and shortcut controls can open it as well.

Recommend keeping this shared, customizable description catalog, with display
policies reviewed separately. Sources: `src/session/session_lua.c`,
`src/ui/character/skill_dialogs.c`, `src/ui/character/skill_tree.c`,
`src/ui/character/skill_list_wnd.c`, `src/ui/hotkeys/shortcut_bar.c` and
`src/ui/hotkeys/quick_slot.c`. Read-only loose asset inspection covered
`client-files/data/lua files/skillInfoz/skilldescript.lua` and `skillinfo_f.lua`;
these illustrate the contract, not proof of the active prefixed runtime data.

Patrick chose to keep skill descriptions. The catalog load and display support
remain unchanged. This is a ledger-only decision; no source, runtime assets or
foundation patches changed, and no build or tests were needed. The scoped diff
is `build/purist-keep-skill-descriptions.diff`.

## Skill Lua helper functions (START-174, kept)

After the three skill catalogs, startup loads `Lua Files/SkillInfoz/SkillInfo_F`.
This provides the functions the C client calls to use the retained data:
inherited jobs, skill names and icon identifiers, SP/AP costs, casting-level
selection, range and warning-area dimensions, descriptions and class-tab names.
The inspected loose helper also walks skill prerequisites and combines a job's
skill-tree layout with the metadata catalog, calling `c_AddSkillList` and
`c_AddNeedSkillList` to populate the client's tree records.

The C callbacks store these records and requirements in the player's skill
bundle. Session refresh calls `GetInheritJob` and `InitSkillTreeView`; skill
information, targeting, warning effects and the skill window call the individual
lookup functions. Keeping only the data tables without these functions would
leave those C consumers without their expected Lua interface. Recommend keeping
this shared support and reviewing the next tree-layout catalog and specific
policies separately.

Sources: `src/session/session_lua.c`, `src/session/lua_tables.c`,
`src/contents/client_lua_funcs.c`, `src/skills/skill_info.c`,
`src/game/msg_skills.c`, `src/game/effect_notify.c`,
`src/ui/character/skill_dialogs.c` and `src/ui/character/skill_tree.c`.
Read-only inspection of `client-files/data/lua files/skillInfoz/skillinfo_f.lua`
illustrates the helper implementation, not proof of the active prefixed runtime
asset. Optional functions found only in that loose asset (including `TestFile`)
are not being treated as live client features or edited in this decision.

Patrick chose to keep the skill Lua helpers. Their startup load and callers
remain unchanged. This is a ledger-only decision; no source, runtime assets or
foundation patches changed, and no build or tests were needed. The scoped diff
is `build/purist-keep-skill-lua-helpers.diff`.

## Per-job skill-tree layouts (START-175, kept)

The next startup load is `Lua Files/SkillInfoz/skilltreeview`. The inspected
loose catalog defines `SKILL_TREEVIEW_FOR_JOB`, mapping job IDs to numbered
cells containing skill IDs. For example, its Novice entries place Basic Skill,
First Aid and Play Dead at cells 0, 7 and 14. `InitSkillTreeView` combines these
entries with the retained skill metadata and prerequisites, then supplies them
to `c_AddSkillList` with the cell number. The C callback stores that number as
`SkillInfo.cell_index` in the corresponding class-stage tree.

The skill window uses these positions in its seven-column grid and handles
inherited class trees through the support already kept. The catalog provides
which skills to show in those trees and where they appear, including unlearned
entries; server skill updates supply the character's learned levels and other
current skill state. Keeping the configurable layout support does not grant
skills or freeze the catalog to the official classes. The local catalog should
be maintained alongside the server's available skills.

Recommend keeping this support for customizable skill trees, then reviewing
individual skill-window display policies separately. Sources:
`src/session/session_lua.c`, `src/session/lua_tables.c`,
`src/contents/client_lua_funcs.c`, `src/session/session_skills.c`,
`src/ui/character/skill_tree.c` and `src/ui/character/skill_tree_draw.c`.
Read-only loose asset inspection covered
`client-files/data/lua files/skillInfoz/skilltreeview.lua` and `skillinfo_f.lua`;
these illustrate the contract, not proof of the active prefixed runtime data.

Patrick chose to keep configurable skill-tree layouts. The catalog load and
layout consumers remain unchanged. This is a ledger-only decision; no source,
runtime assets or foundation patches changed, and no build or tests were
needed. The scoped diff is `build/purist-keep-skill-tree-layouts.diff`.

## Compact skill-list view (START-176, kept)

The skill window can switch between its seven-column tree grid and a narrower
one-column list. The command currently identified as `UI_ID_HELP` toggles
`simpleSkillWnd` and resizes/reflows the same window. In compact mode, skills
are addressed by their sequential row instead of the catalog's grid cell;
scrolling and vertical resizing adjust the visible rows. The existing class
tabs, point controls and casting-level controls remain available, with their
positions adapted to the list.

`session_load_options` reads `Simplicity_SkillList` with a built-in fallback of
zero (tree view), and option saving writes the player's selection. The shared
skill-tree catalogs retained under START-175 are still used; this is an optional
presentation of that data, not an alternate source of skill rules.

Recommend keeping the compact view so players can choose a smaller list or the
full tree, then reviewing hover descriptions and other window policies
separately. Sources: `src/ui/character/skill_tree.c`,
`src/ui/character/skill_tree_draw.c`, `src/ui/character/skill_tree_points.c`,
`src/session/session_option_load.c` and `src/session/session_option_save.c`.
Patrick chose to keep both views. The tree/list switch, compact layout,
controls and saved preference remain unchanged. This is a ledger-only decision;
no source, runtime assets or foundation patches changed, and no build or tests
were needed. The scoped diff is `build/purist-keep-compact-skill-list.diff`.

## Skill hover descriptions (START-177, kept)

The skill window has a description checkbox initialized from
`showSkillDescript`. When enabled, moving over a skill opens the shared
skill-description panel beside the cursor, with placement adjusted near screen
edges. Hovering outside the eligible skill area or over an empty cell hides
that panel. The behavior supports both the tree and compact list; in compact
mode the eligible hover area is restricted to the left part of each row.

The checkbox toggles the session preference. Option loading reads
`Show_SkillDescript` with a built-in fallback of zero; option saving writes the
current choice. Session construction initially sets it to one, so the effective
startup value depends on the loaded options. This review retains existing
default handling pending any separate policy decision.

This toggle controls automatic hover panels. Right-click description windows
from the shortcut and quick-slot bars use the same retained description support
without checking this preference. Recommend keeping the player-controlled
hover behavior so players can read skills while browsing and switch the panels
off when they obscure the tree.

Sources: `src/ui/character/skill_tree.c`, `src/ui/hotkeys/shortcut_bar.c`,
`src/ui/hotkeys/quick_slot.c`, `src/session/session_init_state.c`,
`src/session/session_option_load.c` and `src/session/session_option_save.c`.
Patrick chose to keep hover panels and their toggle. The hover handling,
checkbox and saved preference remain unchanged. This is a ledger-only decision;
no source, runtime assets or foundation patches changed, and no build or tests
were needed. The scoped diff is `build/purist-keep-skill-hover-descriptions.diff`.

## Skill prerequisite highlighting (START-178, kept)

Moving between skills updates the required-skill list while prerequisite
pinning is inactive. `skill_tree_mark_required` gathers the hovered skill and
its catalog-derived prerequisites. The grid renderer gives the corresponding
icons colored backgrounds/tints and prints each prerequisite's required level.
This is independent of the description-panel checkbox kept under START-177.

The helper also colors other class tabs using hardcoded skill-ID ranges to
choose a tab; it does not locate each prerequisite in the actual configured
class-tree lists. That assumption deserves separate review for custom skills.
Right-clicking can pin the required-skill selection, which also prevents the
point-planning helper from raising skills while pinned. Pinning and point
planning will be reviewed separately from the basic visual guidance.

The same required-skill list feeds the current tree point planner, so any
future removal of visual highlighting must preserve required data used by
retained planning behavior. This review is about keeping the visible guidance,
not changing prerequisite rules or sending skill-upgrade requests.

Recommend keeping prerequisite highlights and required-level labels so players
can understand how to reach a skill. Sources: `src/ui/character/skill_tree.c`,
`src/ui/character/skill_tree_points.c` and `src/ui/character/skill_tree_draw.c`.
Patrick chose to keep prerequisite highlighting. The icon highlighting,
required-level labels and underlying prerequisite data remain unchanged. This
is a ledger-only decision; no source, runtime assets or foundation patches
changed, and no build or tests were needed. The scoped diff is
`build/purist-keep-skill-prerequisite-highlighting.diff`.

## Right-click prerequisite pinning (START-179, kept)

Right-clicking a skill cell can set `show_required` and remember its numeric
cell in `required_cell`. While pinned, subsequent hovering does not rebuild
the required-skill list. Changing class tabs does not clear the pin, allowing
the same prerequisite selection to remain highlighted while browsing other
tabs. Right-clicking the remembered cell toggles the mode; clicking another
cell selects a new pin, and right-clicking outside eligible cells clears it.
The remembered identity is a cell number, not a skill ID or a tab/cell pair.

`skill_tree_raise_skill` immediately rejects attempts to add planned points
while `show_required` is set, in both tree and compact views. This does not
block the separate Apply action from submitting an already-existing plan.
The mode is initialized off when this window is constructed and is not a saved
option. There is no separate checkbox or explanatory message for the blocked
point-add operation in the inspected handler.

Recommend removing this hidden pinning mode and its associated point-add lock,
while preserving the ordinary hover highlights and prerequisites retained under
START-178. If removed, keep point-planning data and explicit server upgrade
requests intact; their policies are separate review items. The hardcoded
cross-tab highlighting heuristic also remains a separate concern.
Sources: `src/ui/character/skill_tree.c`, `src/ui/character/skill_tree.h` and
`src/ui/character/skill_tree_points.c`.
Patrick chose to keep prerequisite pinning. The right-click mode, highlighted
selection across tabs and existing block on adding planned points remain
unchanged. Do not remove the lock based on the earlier recommendation. This is
a ledger-only decision; no source, runtime assets or foundation patches changed,
and no build or tests were needed. The scoped diff is
`build/purist-keep-prerequisite-pinning.diff`.

## Prerequisite tab hints (START-180, corrected)

Patrick approved retaining the tab hints while replacing fixed skill-ID ranges
with lookup in the tabs' actual contents, and preparing the fix for the
foundation. Previously the helper mapped selected ranges to tab zero, IDs
2000 through 2532 to tab two, and all remaining IDs to tab one. This could mark
a tab without the prerequisite and could never select tab three.

`skill_tree_mark_required` now calls a small private helper that searches the
same lists displayed by the window: its class lists and the separate final
extra-skills list. Every other tab containing the prerequisite is highlighted;
missing skills produce no hint, and the selected tab remains excluded. The
existing colors, required-level labels, prerequisite collection, clearing of
stale colors and UI invalidation remain in place. Class-tab construction and
list mapping are unchanged. Right-click pinning, its point-add lock, planning,
Apply/Reset and network requests are unchanged.

Validation: a new focused suite uses the real window/session/skill structs,
production list storage and production functions extracted verbatim from the
current sources. Only allocation and redraw notification boundaries are
substituted; there is no copied implementation of the hint algorithm. The same
17 checks first reproduced 21 incorrect per-tab color results in the original
code, then passed with the fix. Cases cover custom/moved IDs, first through
fourth class tabs, the extra-skills tab as source and destination, duplicate
matches, missing and same-tab prerequisites, hidden backing lists, one/two-tab
windows, integer ID lookup, repeated requirements, stale hint clearing and
right-click pin/toggle/clear behavior. Required-level merging and unchanged
point-plan state are checked; no test allocations remain outstanding.

The changed C unit compiled and the release executable linked. No live game UI
test was run. New tests: `tests/build_skill_tree_hints.bat`,
`tests/test_skill_tree_hints.c` and `tests/extract_skill_tree_hints.py`.
The source change is confined to `src/ui/character/skill_tree_points.c`.

Foundation patch: `docs/upstream/skill-prerequisite-tab-hints.patch`. This is a
standalone four-file patch against `54ca05e3785e1ab2771362b150f8050c2a832b9a`;
it does not require earlier chat or other purist patches. The original source
matched the pinned foundation. The patch contains the same production change
and three test files, with the foundation's existing curl header include path
added to the isolated test runner. It applied and reverse-checked cleanly in
`build/foundation-skill-prerequisite-tab-hints`, and all 17 checks passed there.
The actual foundation checkout, runtime assets and servers were not changed.
No commit or push was made.

Artifacts: `build/before-skill-prerequisite-tab-hints`,
`build/skill-prerequisite-tab-hints-before.log`,
`build/purist-skill-prerequisite-tab-hints-{build,test,audit}.log`,
`build/foundation-skill-prerequisite-tab-hints-test.log`, and
`build/purist-skill-prerequisite-tab-hints.diff`. The scoped audit covers one
source file, three new test files, this ledger and the new foundation patch.

## Skill-point preview and Apply/Reset (START-181, removed)

The skill window stages level increases locally before sending upgrade
requests. The tree view keeps a planned-level list; the compact view changes
its displayed working copies. Both show the remaining available points and
pending level changes. Apply presents a confirmation and then submits individual
upgrade requests for the increases. Reset discards the unsubmitted plan and
rebuilds from current session skill state; it does not undo learned skills.

The game-mode handler for message `0x43` sends the ordinary `0x0112` upgrade
request, and server skill updates refresh the session levels. Recommend keeping
this preview and the explicit Apply/Reset controls so players can review choices
before submitting them. The tree's automatic inclusion of prerequisite levels,
request ordering and other allocation policies should be reviewed separately.
Sources: `src/ui/character/skill_tree.c`,
`src/ui/character/skill_tree_points.c`, `src/ui/character/skill_tree_draw.c`,
`src/game/mode_msg.c` and `src/game/skill_list.c`.
Patrick decided on 2026-11-02 to remove the preview and its Apply/Reset controls and to
raise skills immediately with one ordinary upgrade request:
[Skill-point preview removal](#skill-point-preview-removal-start-181-2026-11-02).


## Cash shop removal (2026-11-02)

Patrick approved removing the in-game item mall entirely, including the Gold PC
cafe mileage display. The Charge button and promotional banners were already
removed under START-082/083; this removes everything that remained.

What the client did before this removal:

- Three entry points sent mode message `0x143`, which requested the shop with
  `CZ_SE_CASHSHOP_OPEN_V2` (`0x0b6d`, tab number): the menu-icon "shop" button,
  the Lua-bindable hotkey behavior `0x89`, and a floating call button (window
  `0xbe`, opened at every map entry except `new_event.rsw`, shown or hidden with
  `/cashshop`). The replies `0x0845`, `0x0a2b` and `0x0b6e` opened the shop window
  (`0x13e`) with cash and free-point balances. If the window could not be made,
  the client answered with `0x0844`.
- On every map entry with empty goods lists, the client requested the goods
  schedule (`0x08c9`) and stored each `0x08ca` reply in one of eleven session
  lists. Opening or refreshing the shop also requested the account-limited sale
  list (`0x0b4c`, five-second throttle); `0x0b4d` filled the Sale tab.
- The shop had tabs, a name search across tabs, nine goods tiles per page (six
  taller ones with sale-period and stock details on the Sale tab), a cart of up
  to eight lines (100 per line) with drag-and-drop through the count-input
  window, optional free-point use and a confirmation prompt. Purchases sent `0x0848`; `0x0849` returned results and
  balances, `0x08d3` updated balances and closing the window sent `0x084a`. The
  selected tab was saved in the registry, and every map-server connection reset
  `HKCU\<client key>\NeoCash\CASH_CATEGORY` to 0.
- An older NCash item-mall window (`UInCashWnd`, window `0xb8`) was still built in,
  with its own cart, search, buy request and a bargain-sale count request
  (`0x09c3`). Nothing in this client opened window `0xb8`.
- A GM account-limited sale tool (`UIAccountLimitedToolWnd`, window `0x154`)
  managed the shop's Sale tab: `/계정한정` or `/accountlimitedtool` sent `0x0b4f`,
  the server opened it with `0x0b50`, and add/delete/search sent
  `0x0bac`/`0x0b4a`/`0x0b53` with results `0x0b49`/`0x0b4b`/`0x0b54`. Closing sent
  `0x0b51`; `0x0b52` closed it from the server.
- Gold PC cafe mileage (`UIMileageWnd`, window `0x10b`): `ZC_GOLDPCCAFE_POINT`
  (`0x0a15`) showed points (up to 300), an hourly countdown and a grade-specific
  `mileage_bg%d.bmp` background. The state was kept in the session and restored at
  each map entry; `/goldpc` toggled it. Clicking it sent
  `CZ_DYNAMICNPC_CREATE_REQUEST` (`0x0a16`, NPC name `GOLDPCCAFE`); `0x0a17`
  printed the creation result. This window was the only sender of `0x0a16`.

Removed:

- Ten files: `src/ui/shops/cash_shop.c`, `cash_shop.h`, `cash_shop_cart.c`,
  `cash_shop_goods.c`, `mileage.c`, `ncash_wnd.c`, `ncash_wnd.h`,
  `src/game/mode_cash_shop.c`, and the item mall's GM tool
  `src/ui/admin/account_limit.c` with its only header `src/ui/admin/admin.h`.
- Incoming handlers, dispatch cases, header IDs and packet-table rows for the
  fourteen previously handled messages: `0x0845`, `0x0849`, `0x08ca`, `0x08d3`,
  `0x0a15`, `0x0a17`, `0x0a2b`, `0x0b49`, `0x0b4b`, `0x0b4d`, `0x0b50`, `0x0b52`,
  `0x0b54`, `0x0b6e`.
- Every outgoing request: `0x0844`, `0x0848`, `0x084a`, `0x08c9`, `0x09c3`,
  `0x0a16`, `0x0b4a`, `0x0b4c`, `0x0b4f`, `0x0b51`, `0x0b53`, `0x0b6d`, `0x0bac`.
- The whole item-mall protocol family from the packet table: 54 length rows and
  54 names. Besides the IDs above, this includes the never-handled tab-list
  replies (`0x0846`, `0x0847`, `0x08c0`), the older simple cash shop and its button
  toggle (`0x035c`, `0x035d`, `0x0444`, `0x0445`, `0x044b`, `0x080c`, `0x080d`),
  the bargain-sale tool (`0x09ac`-`0x09b5`, `0x09bc`, `0x09bd`, `0x09c4`,
  `0x0a3d`), the Taiwan purchase result/balance (`0x0a2c`, `0x0a34`) and the
  remaining limited-sale request `0x0b48`.
- Session goods lists and their accessors, the limited-sale throttle, the PC-cafe
  mileage fields and both icon-visibility settings; window-manager slots, the
  NCash window rectangle and their teardown/forget/close/untracked/Escape entries;
  the menu-icon shop button (the keyboard, attendance and agency icons each move
  one grid position earlier), hotkey behavior `0x89`, `/cashshop`, `/goldpc` and
  `/accountlimitedtool` with their talk types and saved talk options; the count
  window's two cart/shop purposes; the `NeoCash` registry write; the shop
  constants in `shops.h`; and the guard that refused NPC cash-point shops while
  either removed shop window was open. With no plate windows, window ID `0x12d`
  used to fall back to the cash-shop window; it now returns NULL, as it did
  whenever that window was closed.
- Helpers left without callers, after checking every caller: the custom-skinned
  scroll bar (`UIScrollBar_Custom`, used only by the cart) with its sizing,
  arrow/thumb setters and bitmap helper; `ui_static_text_set_number`,
  `ui_window_draw_bitmap_scaled`, `ui_window_text_out_centred_outlined`,
  `drag_info_copy`/`drag_info_assign`, `session_clear_shop_buy_list` and the
  talk-type `toggle_window_option` helper. A whole-source reachability audit from
  `WinMain` (registry references excluded) finds no other newly orphaned names.
- Nine UI type identities are retired: `UIAccountLimitedToolWnd`, `UICashShopWnd`,
  `UICashShopWnd_Cart`, `UICashShopWnd_CartItem`, `UICashShopWnd_Item`,
  `UIMileageWnd`, `UInCashWnd`, `UInCash_CallWnd` and `UIScrollBar_Custom`.
  Historical type fixtures keep their names solely to verify that they are rejected.

Kept on purpose:

- NPC cash-point shops (`0x0287`/`0x0288`/`0x0289`): the ordinary NPC shop and
  NPC market windows sell for cash or free points (server `cashshop`-type NPCs).
  Their balances, purchase request and messages remain. Rental-item timers and
  expiry (`0x0298`/`0x0299`), the cash item type (barter windows still refuse it),
  the cash-item open confirmation and the `ASPDCASH` status icon are generic item
  behavior.
- Cash-password rows `0x02aa`-`0x02ac` and `0x0a8b` stay in the packet table. No
  handler or sender exists, and the name alone does not show that they belong only
  to the item mall.
- Drag sources `DRAG_CASH_CART` (4) and `DRAG_CASH_GOODS` (6) keep their misleading
  names; the NPC market and storage windows use those values. `ItemInfo`'s sale
  fields stay because the shared item record still copies and hashes them.
- `drag_info_destroy`, `ui_bmp_radio_btn_set_tool_tip` and `dc_bitmap_copy_scaled`
  no longer have production callers, but retained barter-record, radio-button and
  canvas tests use or exercise them, so they remain part of those APIs.

Server implications: servers must stop sending the fourteen incoming messages.
They now take the existing unknown-packet path, which resets connection queues.
The shop, schedule, sale list and limited-sale tool replies were request-driven,
so ordinary servers send nothing once the client stops asking; script- or
GM-triggered shop openings must be disabled. Gold PC points are pushed unsolicited
when enabled: keep Hercules `feature.goldpc.enable` false (the default in the local
checkout's `conf/map/battle/feature.conf`). Never-handled item-mall rows already
reset the queues when received; they are now also framed as unknown packets. NPC
cash-point shops keep working. No server checkout was modified.

Validation:

- Release build passed without compiler or linker warnings:
  `build/cash-shop-build.log` (baseline `build/cash-shop-baseline-build.log`).
  Every modified unit compiled at `/W4` before and after with no new diagnostics:
  `build/cash-shop-w4-compare.log`.
- All 1,179,800 packet-dispatch cases passed, digest `6165394e1e38d7ff`. An
  independently derived trace replaces only the 168 cases for the fourteen retired
  incoming packets with the existing unknown-packet result; the actual trace
  matches it byte for byte (SHA-256 `699e4058...2c912e06`). The test asserts the
  retired IDs take the unknown path: `build/cash-shop-dispatch-trace.log`,
  `build/derive_cash_shop_dispatch.py`.
- Exactly 54 length rows and 54 names were removed; all 1,312 surviving length rows
  and 1,380 names keep their IDs, sizes, minimum sizes, names and order:
  `build/cash-shop-packet-table-audit.log`.
- Type checks passed for 546 classes and 65 retired identities, 3,303 prior-runtime
  comparisons and 245 accepted casts: `build/cash-shop-object-type.log`.
- Content managers passed with the unchanged digest `f0fa1aba`; the trace is
  byte-identical to the pre-removal trace: `build/cash-shop-content-managers.log`.
- Talk options now also check that the former `/cashshop` and `/goldpc` options are
  unbound. UI drop (cart and goods owners removed; their shared shop stubs moved to
  the NPC-market owner), forget-window, emblem-cache, shortcut-packet, scroll,
  animation-owner and barter-record checks passed: `build/cash-shop-*.log`.
- Full suite `tests\build.bat` passed to the final screenshot check:
  `build/cash-shop-full-tests.log`. The COM, emote-timing, headgear-name, NPC-music,
  pet-accessory, pet-portrait and skill-tab suites outside it also passed:
  `build/cash-shop-extra-*.log`. The agency-server check needs a patched Hercules
  checkout and was not run.
- Function audit: 20,750 surviving functions are byte-identical. The 26 changed
  production functions only lose removed cases, calls, state and guards; the six
  changed test functions update expectations or add the unbound-option checks. No
  function was added: `build/cash-shop-function-audit.log`. Reachability:
  `build/cash-shop-reachability-audit.log`.
- No item-mall, mileage, limited-sale or retired-helper references remain in
  application sources, build inputs or the final link map; `git diff --check`
  passed.

Foundation candidates (not changed here):

- `enter_map` in `src/login/login_states.c` never assigns the 4-byte field after
  `authCode` in the `0x0436` map-enter packet, so stack bytes are sent. The same
  code is on `main`. Fixed on `main` in `f21e5cb13b` (`purist` `d477f7b91b`).
- On `main`, `ui_window_mgr_get_window(0x12d)` returns the open cash-shop window
  when no name plate exists, which looks like a decompiled fall-through; a lookup
  for the plate owner could then reach the shop. The original executable has the
  same fall-through. Fixed on `main` in `ad2c6075c8`.

The pre-removal tree is in `build/before-cash-shop-removal`; the full diff is
`build/cash-shop.diff`. No live client/server smoke test was run.


## Party Booking and Seek Party removal (2026-11-02)

Patrick approved removing the two older party-recruitment systems on 2026-11-02.
The Adventurer's Agency (START-041) and ordinary party features remain.

- Removed Party Booking state and logic: the session's listing rows, pending
  queue, filters, alarm flags and answer tick; row formatting, search decoding,
  insert/update/delete notifications and the new-listing alarm; sorted row and
  region/map storage; and the `queryRegionInfo`/`queryMapInfo` region catalog with
  startup loading of `lua files\seekparty\party_booking_config` and
  `party_booking_function` (which also provided `GetPartyBookingHelp`). Session
  construction/destruction and the login reset no longer own it.
- Removed the five Booking windows and identities: list `0xa9`
  (`UISeekPartyListWnd`), registration form `0xa8` (`UISeekPartyMBWnd`), own-listing
  status `0xa4` (`UISeekPartyWnd`), wanted-job selector `0xaa` (`UIJobListWnd`) and
  Lua help `0xad` (`UIPartyBookingHelpWnd`). Removed their factories, manager slots,
  close/forget/teardown paths, the `0xa4` untracked-ID entry, four registry window
  rectangles (`BOOKINGLISTWNDINFO`, `BOOKINGMBWNDINFO`, `BOOKINGWNDINFO`,
  `BOOKINGMBCHECKWNDINFO`) and their defaults, map-entry reopening of the status
  window and the job-selector startup map. Fifteen source/header files were deleted.
- Removed the entry points: the Basic window's live Booking button (later buttons
  flow one slot earlier; its layout assertions were updated), the already-skipped
  Booking menu icon, mode messages `0x105`/`0x10d` and talk types `0xc2`/`0xc6`. No
  chat command produced those talk types; this client has no `/booking` command.
- Removed incoming handlers, IDs, dispatch cases and packet-table rows for the
  booking replies/notifications `0x0803`, `0x0805`, `0x0807`, `0x0809`, `0x080a`,
  `0x080b` and for `0x07dd` (`ZC_SEEK_PARTY`), plus the rows of unhandled `0x07df`.
  Removed outgoing register/search/delete/job-update `0x0802`/`0x0804`/`0x0806`/
  `0x0808` and the rows of never-sent `0x07dc`/`0x07de`. The seven incoming messages
  now take the existing unknown-packet path, which resets connection queues.
- Removed two newly orphaned helpers after checking every caller: the `social.h`
  inline `ui_send_packet` and `ui_combo_box_def_wnd_get_label_text`. Removed the
  dedicated storage/catalog tests and their suite. Historical type fixtures retain
  all five retired UI names solely for rejection checks.
- No longer referenced: `party_alarm.wav`, `basic_interface\booking*.bmp`,
  `basic_interface\seekparty\` artwork, `btn_rewrite*.bmp` and the `seekparty` Lua
  files. Stale registry values written by earlier runs are simply ignored.

Kept:

- The Agency board, registration, application, information and result windows,
  `adventurer_agency.c`, `agency_transport.c`, `0x0bf0`/`0x0bf1` and the server
  patch. The Agency still uses `AdvenPartyInfo`, `PartyBookingList` and their owned
  text helpers in `src/contents/party_booking_data.{c,h}` (covered by
  `tests/test_owned_booking.c`), plus `src/ui/social/party_booking.h` (Agency
  information/application windows, role icons and `party_booking_send_name`). Their
  historical names were kept to avoid churn in shared files.
- Normal party invitations, joins, master-to-join requests, settings, the party and
  member windows and mini party information. All Agency and ordinary party files are
  byte-identical to the backup; `packets_party.c` lost only the seven removed
  handlers and two includes.
- Party Recruit (`0x08e5`-`0x0971`: name rows plus `0x096c`/`0x0971` length rows) is
  a separate, unimplemented family outside this decision and remains for review.
  `tests/test_service_json.c` still prints a stale "Party booking JSON" line left by
  the Agency's former JSON decoder; it is unrelated to this removal.

Server implications: Hercules built for `PACKETVER=20220406` keeps Party Booking
enabled (`PARTY_RECRUIT` applies only to 2012-04 RE clients) and maps `0x0802` to
its booking-register handler. This client no longer sends booking requests, so the
direct replies stop. The server still broadcasts `0x0809`/`0x080a`/`0x080b` to all
clients whenever any session registers, updates or deletes an advertisement,
including the deletion sent when a player with an advertisement logs out. Another
client build or a crafted request would therefore reset every purist client's
queues; the server fork should disable the four booking request handlers. Hercules
neither sends `0x07dd` nor handles `0x07dc`/`0x07de`. No server checkout was
modified.

Foundation-fix candidate: `rag_connection_get_packet` frames variable-length
packets and IDs without a length row by the 16-bit field at offset 2, accepting
values below the header size. With 0 it consumes nothing, the game dispatcher
ignores packets shorter than two bytes, and `game_mode_poll_network_msg` repeats the
same zero-length read indefinitely, hanging the client; with 1 it consumes one byte
and desynchronizes framing. The login loop shares the framing, and `main` has the
same code. For the retired IDs, `0x0803`/`0x0807` success replies carry 0 there (they
only answer the removed requests) and the notifications carry the advertisement
index (a hang when its low 16 bits are zero, otherwise misframing before the reset).
Not changed here: the removal does not depend on it. Suggested fix: treat a framed
length below 4 for variable/unknown packets as a protocol error. Fixed on `main` in
`ebb1887196` (`purist` `e731da3cf1`).

Validation:

- Full release build passed without compiler/linker warnings:
  `build/party-booking-build.log` (pre-removal: `build/party-booking-baseline-build.log`).
- All 1,179,800 packet-dispatch cases passed, digest `59a0b17fc30022df` (previously
  `0e20d9f11f3d4d93`). The actual trace's SHA-256 matches an independently derived
  baseline in which only 78 cases change to the unknown-packet result: 12 each for
  the six booking replies (direct and forwarded) and 6 for `0x07dd`, whose direct
  second-dispatcher cases were already unknown. The test now asserts the
  unknown-packet trace length for all seven IDs. Logs:
  `build/party-booking-dispatch-tests.log`, `build/party-booking-dispatch-expectation.log`.
- All 1,352 retained packet-length rows and 1,420 name rows match the pre-removal
  table in order; only the 28 rows for the 14 IDs were removed:
  `build/party-booking-packet-table.log`.
- Type checks passed for 550 concrete classes and 61 retired identities, including
  the five Booking types, with 3,335 prior-runtime comparisons, 245 accepted casts,
  404 diagnostic names and 383 window names: `build/party-booking-object-type-tests.log`.
- Content managers (digest `f0fa1aba`), game-mode layout (digest `274efe64`), owned
  booking text, Agency packets/UI, the patched-Hercules Agency handler fixture and
  forget-window checks passed: `build/party-booking-*-tests.log`.
- The complete behavioral suite `tests/build.bat` passed through its final step:
  `build/party-booking-full-tests.log`.
- No Booking/Seek Party code, build inputs or link-map symbols remain. The map diff
  removes 133 symbols from the eight deleted objects plus the seven handlers, the
  orphaned combo-box helper and feature strings only:
  `build/party-booking-map-diff.log`, audit `build/party-booking-audit.log`.
  `git diff --check` passed.

The pre-removal tree is in `build/before-party-booking-removal`; logs use
`build/party-booking-*.log`. No live client/server smoke test was run.


## 4th-class removal (2026-11-02)

Patrick approved removing 4th classes; his server will not offer them. Job IDs and
skill ranges were taken from the client's own tables and predicates.

- **Jobs.** Removed every C entry for the client's 4th-class range
  (`session_is_fourth_job`: `0x109c`–`0x10cb` = 4252–4299 and `0x10ce`–`0x10e9` =
  4302–4329). Named IDs:
  - 4252–4264: Dragon Knight, Meister, Shadow Cross, Arch Mage, Cardinal, Windhawk,
    Imperial Guard, Biolo, Abyss Chaser, Elemental Master, Inquisitor, Troubadour
    and Trouvere.
  - 4265–4277: their riding variants.
  - 4278–4281: Windhawk wolf, Meister madogear, Dragon Knight chicken and Imperial
    Guard chicken.
  - 4302–4308: Sky Emperor, Soul Ascetic, Shinkiro, Shiranui, Night Watch, Hyper
    Novice and Spirit Handler.
  - 4309–4315: their riding variants; 4316: Sky Emperor 2.

  Removed:
  - The predicate itself.
  - Name, sprite, IMF, body and sprite-path rows: four `.def` tables using the
    compact `job - 0xf6e` index, `pc_names.c` paths and their 44 now-unused string
    constants, and the write-only `jobIdNames` rows.
  - Mount and alternate-sprite mappings.
  - Membership in the Doram, any-Super-Novice and expanded-job predicates.
  - Weapon, dual-wield, equip-level and equip-value rules.
  - Skill-tree tab inheritance and tab kinds.
  - Hit sounds, spheres and servant weapons, palettes and costume suppression.
  - Max-level aura and level caps, Lua reads of `BaseLevel4th`/`JobLevel4th`/
    `MaxLevelEffect4th(_sub)`, the related session state, and the 4th-job
    basic-info/gauge layouts.

  Job-indexed rule tables now end before `0x109c`. Their dropped non-4th slots
  (`0x10cc`/`0x10cd`) already held fallback-equivalent values, so every removed ID
  takes each function's existing out-of-table fallback. For example, unknown names
  become "Poring" and unknown sprites the Novice sprite.
- **Skills.** Removed:
  - The six expanded-4th handler files and `skill_use_fourth.c`.
  - Every special case for player skills `0x1451`–`0x157a` (5201–5498). The
    expanded range `0x1519`–`0x157a` is the client's own check.
  - Elemental Master spirit skills `0x20fb`–`0x2109` and Meister ABR skills
    `0x2199`–`0x219d`.
  - These cases covered cast/use motions and effects, hit effects, cast looks,
    ground/unit looks, chat shouts, good-skill lists and targeting/turning.
  - Skill-unit handling for `0x10a`, `0x10b`, `0x10d`–`0x11c` and `0x11e`–`0x12a`.
    The `npc_fuuma` action at `0x122` was Shinkiro/Shiranui's Fuuma Shouaku; its
    only effect loads `SHINKIRO_SHIRANUI` resources.
  - Windhawk trap units `0x5174`–`0x5177`.
  - The skill and object halves of the skill-action manager. All 121 of their
    registrations were expanded-4th, so every lookup would always miss; each call
    site now takes its former not-found path. The status half and its five generic
    entries (size change and three show-effects) remain.
  - Newly orphaned helpers, including `skill_shuffled_indices`, the named-spawn
    helpers, `effect_screen_angle(_quadrant)`, `gs_motion`, the companion summon and
    ABR looks, and `MODE_MSG_REBUILD_STATUS_BARS`.
- **Traits and AP.** Removed:
  - The status window's trait section and expand toggle. The window is always the
    collapsed 0x8d layout; the former final toggle step is reproduced for that
    layout.
  - The AP bar, its layouts and draw paths, gauge sources and the `ap_colour`
    member. Five pinned gauge layouts shrink by 4 bytes.
  - The `UIBarGraphPlayerAP` type, AP cost text in seven places, and
    `SkillInfo.ap_amount` with `GetLevelUseApAmount`.
  - `/pow+`…`/crt+`, the trait state `statusValues`, `traitBonus` and `traitCost`,
    and the trait sub-ranges of `0x00b0` (params `0xdb`–`0xe9`), `0x00be` (sp
    247–252) and `0x0141` (sp 219–224). Those fields are now ignored; wire sizes are
    unchanged.
  - Incoming `0x0b25` and outgoing `0x0b24` (definitions, dispatch, handler, sink,
    packet-table rows).
- **Effects.** Removed 18 effect classes whose only client triggers were removed
  4th-class code, together with their 27 registry IDs: Storm Cannon, Crimson Arrow,
  Violent Quake, Astral Strike, Floral Flare Road, Savage Impact, Petitio, Eternal
  Slash attack, Pneumaticus Procella, Crescive Bolt, Hawk Boomerang, Gale Storm,
  random hit, cylinder, Acidified throw, Fuuma Shouaku, Talisman mark and Jupitel
  Thunderstorm.

Kept, and why:

- **Server-driven status visuals.** Visual handlers in `actor_efst.c`,
  `player_states.c` and `state_messages.c` that still spawn 4th-class effects stay.
  The status IDs come from the server, are unnamed in C and are mixed with monster
  statuses. Status icons come from Lua `GetEFSTImgFileName`; the C fallbacks stop
  at `0x461`.
- **Effect classes those handlers spawn.** These are servant weapon, Charging
  Pierce, Climax, A-Machine, Dancing Knife, Eternal Slash count, From the Abyss,
  Shadow Exceed, full-screen and Curse of Cube. Their unused fire/attack controls
  stay with them.
- **Shared data with 4th-class branches.** This covers the `severe_rain_storm`
  branches for `0x6d8`, `0x7ea`, `0x875`, `0x92c`–`0x92f` and `0x932`, the generic
  STR/sound/duration rows and the RagEffect per-ID processes. Their client triggers
  are gone, but packet `0x01f3` can still name them.
- **ABR/Bionic summons.** Packets `0x0b6b`/`0x0b6c`, ABR HP state and object types
  13/14 stay. They are woven into the shared summon ownership, targeting and
  HP-bar code. This is a clean boundary and a follow-up candidate.
- **Shared limits and catalogs.** Skill capacity bounds `0x157b`/`0x157c` (local
  settings format), Lua catalogs and the reference-only `ACTOR_HELPER_REFERENCE`
  test branch stay.

Server implications:

- Do not offer 4th-class jobs or skills. Removed job and skill IDs take the client's
  generic fallbacks.
- `0x0b25` now uses the existing unknown-packet path, which resets connection
  queues.
- Trait and AP fields in `0x00b0`, `0x00be` and `0x0141` are ignored.
- `0x01f3` special effects naming the 27 removed effect IDs fall back to the legacy
  RagEffect launch.
- The 4th-class keys in `MaxLevelTable` and `MaxLevelAuraTable` are no longer
  required.
- Saved 4th-job basic-info heights (`0x47`/`0x95`) reset to the default size.

Validation:

- **Release build.** Passed without compiler or linker warnings:
  `build/fourth-class-build.log`. The final link map has no removed symbols.
  Remaining hits are retained `severe_rain_storm`, RagEffect and STR data strings.
- **Packet dispatch.** All 1,179,800 cases passed, digest `48e08ef549dd6cdf`. An
  independently derived trace (`build/derive_fourth_class_dispatch.py`) replaces
  only the 12 cases for `0x0b25` with unknown-packet handling. Its SHA-256 matches
  the new trace. All 1,364 retained size rows and 1,432 retained name rows are
  unchanged. Logs: `build/fourth-class-packet-dispatch.log` and
  `build/fourth-class-dispatch-expectation.log`.
- **Effect factories.** Digest `274deeba`, which matches an independently derived
  trace (`build/derive_fourth_class_effect_factories.py`). The derivation drops 80
  nodes for the 27 IDs across three cases and renumbers fixture indices. Its
  deliberate failure-case collision moved from removed `0x93e` to the next row,
  `0x943`. Log: `build/fourth-class-effect-factories.log`.
- **Type checks.** Passed for 554 classes and 57 retired identities, including
  `UIBarGraphPlayerAP`: `build/fourth-class-object-type.log`.
- **Skill-action registry.** Rewritten for the status half; passed with 5 startup
  bindings: `build/fourth-class-skill-actions.log`.
- **Full suite.** All commands in `tests\build.bat` passed. The run resumed from the
  first failure after correcting two stale 4th-class expectations:
  - The appearance test listed Shadow Cross as a dual-wield job.
  - The resource-path test's removed 4th-job case had also reset its sprite stub
    for later cases; the reset is kept.

  Logs: `build/fourth-class-tests.log` (to line 310),
  `build/fourth-class-tests-resume-311.log` and
  `build/fourth-class-tests-resume-512.log` (to the end). Content managers passed
  with digest `f0fa1aba`, unchanged.
- **Adjusted tests.** The dedicated `test_skill_effect_calls` test and its
  unreached-symbol header were removed. Gauge, session-field, map-node, resource-path,
  appearance, hit-sound fixture and helper-owner tests drop only their 4th-class/AP
  cases.
- **Diff check.** `git diff --check` passed.

Foundation-fix candidates (not changed):

- `jobIdNames` is filled at startup but never read.
- `UI_MSG_SET_INFO` for the status window minimizes a saved title-only window and
  then resizes it to full height. As a result, the minimized state is never
  restored. This behavior is preserved. Fixed on `main` in `5da401b8f2` (`purist`
  `fb6d501f97`).

The pre-removal tree is in `build/before-fourth-class-removal`. No live
client/server smoke test was run.


## Lapine removal (2026-11-02)

Patrick approved removing Lapine synthesis and Lapine upgrade.

- Removed both Lapine windows (synthesis box and upgrade box), their ingredient
  lists, rows and scroll bar, seven UI identities, the `LapineDdukDDakBox` and
  `LapineUpgradeBox` Lua catalog loading, the `add_ingredient` and
  `add_ingredient_lapine_upgrade_box` Lua callbacks, the `0x122`/`0x12e`
  window-factory cases, the open and result handlers, and the drop-refusal check.
  Seven source/header files were deleted, including `contents_vtables.c`, which
  held only the two Lapine window tables.
- Removed incoming `0x0a4e`/`0x0a50` (synthesis open/result) and `0x0ab4`/`0x0ab7`
  (upgrade open/result), and outgoing `0x0a4f`/`0x0a70` (synthesis
  request/close) and `0x0ab5`/`0x0ab6` (upgrade close/request). This covers
  dispatch cases, handlers, IDs and packet-table size and name rows. Servers must
  stop sending the four incoming messages: they now take the existing
  unknown-packet path, which resets connection queues. No server checkout was
  modified.
- `game_mode_is_blocking_window_open` checked only the two Lapine windows.
  START-049 already dropped the enchanting windows from it, so it could only
  return 0. Removed it and its 19 dead checks in 10 files: item use, wear, take
  off and throw; walking; mail item add/take; skills; trade, storage and cart
  moves; NPC clicks; and the bank. The bank's `bank_busy` wrapper and its unused
  busy notice went with it.
- Removed newly orphaned helpers after checking every caller:
  `session_inventory_indices`, `ui_scroll_wnd_place_child`, and two unused
  macros in `contents/ui_window.h`. The repair-list and refining-material
  handlers that share `item_upgrade.c` remain.
- Kept the GM-only `ZC_DDUKDDAK_BOX_TEST` (`0x0bb3`) report writer. It records
  box-roll statistics to `DdukddakBoxTest` and uses neither the Lapine windows nor
  their catalogs. It is a separate decision if Patrick wants it gone.

Foundation fix candidate for `main`: on a failed upgrade (`0x0ab7`, result ≠ 0),
`game_mode_zc_ack_random_upgrade_item` looks up the synthesis window `0x122`
instead of the upgrade window `0x12e`, then calls `lapine_upgrade_set_waiting`
on it. With only the upgrade box open, which is the normal case, it never leaves
its waiting state. If a synthesis box were open, a `LapineBox` would be written
through the `LapineUpgradeBox` layout. Purist no longer has the code. Fixed on
`main` in `3e9d6ce8fe`.

Validation:

- Full release build passed without compiler/linker warnings:
  `build/no-lapine-build.log`.
- All 1,179,800 packet-dispatch cases passed, digest `aa19d1111ec36dc7`. The actual
  trace's SHA-256 matches an independently derived trace with only the 48 cases
  for the four retired incoming packets changed to unknown-packet handling
  (`build/no-lapine-dispatch-expectation.log`).
- Type checks passed for 548 classes and 63 retired identities
  (`build/no-lapine-object-type.log`). The content-manager digest is unchanged at
  `f0fa1aba`. Item use, RoDEX, shortcut, barter, local-shop, list, scroll, tag
  viewer and UI-drop suites passed: `build/no-lapine-build_*.log`.

## Item collection review (2026-11-02, not removed)

The approved "2021 collection book" does not exist in this client. The named files
are retained core item features, so nothing was removed under this item.

- `item_collection*.c` (`UIItemCollectionWnd`, window `0xc`) is the right-click
  item description window: picture, description, card slots, illustration
  button, equipment preview button, pet-egg details and random-option lines. It
  is opened from inventory, storage, shop and list right-clicks, chat item links,
  mail and item updates. `UICombinedCardItemCollectionWnd` (`0x52`) shows a
  slotted card's description. The two `_ComparisonWnd` types (`0xea`/`0xfa`)
  compare against worn equipment.
- `collection_slot.c` (`CUICollectionSystemWnd`, window `0xc4`) is the slot-machine
  animation of the monster-capture "collection" flow: `ZC_START_COLLECTION`
  `0x08b4`, `CZ_TRYCOLLECTION` `0x08b5` and `ZC_TRYCOLLECTION` `0x08b6`.
- No collection-book packets, catalogs, windows, menu entries or hotkeys exist.
  Grade-only drawing was removed from the description window under the grade
  removal below.

## Grade enchanting and item-grade removal (2026-11-02)

Patrick approved removing 2021 item grades. Grades can no longer exist, so grade
display was removed everywhere along with the enchanting window.

- Removed the grade-enchant window, its three button types (four UI identities),
  material list and result handling, success/failure notices, server broadcast
  notice, and the window-factory case `0x157`. Also removed `ZC_UI_OPEN_V2`
  type 8 (now ignored like other unknown types), the drag source `0x21` and
  double-click forwarding, the drop refusal while open, and the three
  mail-button blocks that refused mail while it was open. Deleted
  `grade_enchant.c` and `game/enchant.c`.
- Removed incoming `0x0b5a` (material list), `0x0b5d` (enchant result) and
  `0x0b5e` (broadcast), and outgoing `0x0b59` (select equipment), `0x0b5b`
  (request) and `0x0b5c` (close). Servers must stop sending the three incoming
  messages: they now take the existing unknown-packet path.
- Removed the `grade` member from `ItemInfo`, `DragInfo` and the guild-storage
  log entry, its 35 assignments, two helper parameters and the redraw key.
  Removed grade icons from inventory, equipment, quick slot, shortcut bar, item
  lists, identify, refine, both mail windows, merchant log/purchase/shop/setup,
  NPC shop/purchase, para shop, storage, store search and trade. Also removed the
  graded background in the item description window.
- Retained packets keep their exact wire layouts. Grade bytes in item lists,
  ground items, pickups, storage/cart adds, RoDEX, exchange
  (`ZC_ADD_EXCHANGE_ITEM5`), store search, vending entries,
  `ZC_CHANGE_ITEM_OPTION3`, the guild-storage log and the repair list are now
  ignored. No struct size or offset assertion changed. The exchange handler keeps
  its packet-version name.
- Item links keep their format. Both parsers still read past the grade field at
  its position but no longer store it. The encoder still writes the grade field,
  always as `0`, so links stay byte-identical for every item that can now exist.
- Removed the orphaned helper `ui_window_draw_text_aligned`.
- Kept STR effects `0x647`–`0x649` (`grade_enchant\new_*`), which START-048 and
  START-049 kept for grade enchanting. They no longer have a client-side
  spawner, but `ZC_NOTIFY_EFFECT2`/`3`/`4` can still request any effect ID from
  the server. START-049 kept its window's effects in the shared catalog on the
  same basis. Removing them would change the retained server-effect behavior, so
  it is left for Patrick to decide.
- Tests: the UI-drop harness lost its grade owner and the stubs only that owner
  needed (link-verified). Item-info, item-link and item-tag tests now expect no
  stored grade and an encoded grade of `'00`.

Validation:

- Full release build passed: `build/no-grade-build.log`.
- All 1,179,800 packet-dispatch cases passed, digest `9850f5fcccf5423f`. The actual
  trace matches the derived trace with only the 84 cases for the seven retired
  incoming packets changed. Retained handlers that stopped storing grades are
  outside the dispatcher (the test uses sinks), so no other case changed.
- The type-check harness (count assertions printed instead of asserted) reported
  544 classes and 67 retired identities with every other check passing; the
  real test ran in the final suite.
- Item links, item tags, item names, item use, UI drops, button drawing, canvas,
  RoDEX, shortcut, inventory match, merchant backup, barter, readable links, tag
  viewer and quest suites passed: `build/no-grade-build_*.log`.

## Swap equipment removal (2026-11-02)

Patrick approved removing the second (swap) equipment set.

- Removed the swap panel and button from the equipment window (tab handling,
  height adjustment and redraw keys), dragging from either set, the Ctrl+double-
  click shortcut, the inventory "in set" overlay, hotkey action `0x98`, mode
  messages `0x12e`–`0x131`, and the session sets, swap arrow, open flag and
  cooldown. Also removed the login reset, the refining and buying-store
  exclusions for set items, the saved `SWAPEQUIPMENTWNDINFO.X/Y` registry
  values, the default and the extra equipment-window height, and two UI
  identities.
- Deleted `swap_equipment.c`, `equip_sets.c`, `packets_equip_switch.c` and
  `msg_swap_equip.c`. That last file also held the unrelated limited-sale list
  request, so it moved unchanged to the new `msg_limited_sale.c` (its wait-notice
  colour constant was renamed, value unchanged). In the merge the cash-shop removal
  already deleted that request, so `msg_limited_sale.c` was not added.
- Removed incoming `0x0a98`, `0x0a9a`, `0x0a9b`, `0x0a9d` and `0x0ac8` and their
  handlers, plus the table rows of unhandled `0x0a9e`/`0x0a9f`. Removed outgoing
  `0x0a97`, `0x0a99`, `0x0a9c` and `0x0ace`. Servers must stop sending the
  swap messages. A server with equipment switching enabled may send the set list
  (`0x0a9b`) when a character loads; that must be disabled server-side,
  otherwise the client resets connection queues.
- Removed newly orphaned helpers: `session_set_arrow_status` (which existed only
  for the swap set's arrow), the write-only `hideui_skip_count`,
  `player_has_cast_gauge` and its test assertions, the inventory texture
  helper, and an unused command parameter.

Foundation fix candidate for `main`: `swap_equipment_wnd_info_make_default` writes
a 16-byte `WindowInfo` through `(struct WindowInfo *)&swap_equipment_pos`, a
`POINT`. When no saved UI rect info exists (first run), it overwrites the
following `plate_width`/`plate_height` (`0x97`/`0x25`) with `0x118`/`0xa7`. That
corrupts friend-list drag hit-testing in `messenger_group_wnd.c`. On `main` it
should set only `x`/`y`. Purist no longer has the code. Fixed on `main` in
`685ebfa786`.

Validation:

- Full release build passed without compiler/linker warnings:
  `build/item-systems-build.log`.
- All 1,179,800 packet-dispatch cases passed, digest `add09bb5a3f3a577`. The actual
  trace's SHA-256 matches the derived trace with exactly 144 cases changed for the
  12 retired incoming packets across all three removals
  (`build/item-systems-dispatch-expectation.log`). All 1,341 retained size rows and
  1,409 retained name rows are identical and in order
  (`build/item-systems-packet-table.log`).
- Type checks passed for 542 classes and 69 retired identities (396 named, 375
  windows; `build/item-systems-object-type.log`). Content managers: digest
  `f0fa1aba`, unchanged from the baseline.
- The full behavioral suite `tests/build.bat` passed end to end in 6 m 45 s:
  `build/item-systems-full-suite.log`.
- COM, emote timing, headgear names, NPC music, pet accessory/portrait and skill-tab
  suites passed. The agency-server check needs a patched Hercules checkout and
  was not run.
- `/W4` builds of all 78 modified implementation files show the same 92
  unused-variable/parameter/function warnings before and after
  (`build/item-systems-w4-*.log`). `git diff --check` passed. The final link map
  contains none of the removed symbols. Only the kept effect paths and
  limited-sale functions match the search terms.

The pre-removal tree is in `build/before-item-systems-removal`. No live
client/server smoke test was run.


## Macro detector and captcha removal (2026-11-02)

Patrick approved removing the server-driven anti-bot captcha challenge, the GM
macro reporter and its reported-player list. The CAPTCHA registration/upload tool
was already removed (see "CAPTCHA registration/upload removal validation").

- Removed the challenge window (`UIMacroDetectorWnd`, window `0x11d`) with its
  image control, answer box, countdown/retry display and answer request, the GM
  captcha preview window (`UICaptchaPreviewWnd`, `0x120`), chunked image assembly,
  zlib unpacking and BMP validation, and their window-manager slots, edit focus,
  paste block, ESC-menu block and close-all exception.
- Removed the macro reporter (`UIMacroReporterWnd`, `0x11e`) and its user-info
  popup (`UIMacroUserInfoWnd`, `0x11f`): reported-player rows and list storage,
  saved position/pick mode/players (`MacroReporterInfo`), restore on entering the
  map, actor picking and ground-area selection in `world_pick.c`, and opening by
  `ZC_UI_OPEN` (`0x0a38`) type 3, which is now ignored like other unknown types.
- Removed the chat commands `/매크로등록`/`/macro_register`/`/mr`,
  `/매크로확인`/`/macro_preview`/`/mp` and `/매크로신고`/`/macro_detector`/`/md`.
  `/mr` was a leftover of the registration removal: it still sent `CZ_UI_OPEN`
  type 2, whose reply had already been ignored.
- Removed eight incoming packets with their definitions, dispatch cases, handlers
  and table rows: `0x0a57` ACK_APPLY, `0x0a58` APPLY, `0x0a59` APPLY_CAPTCHA,
  `0x0a5b` REQ_ANSWER, `0x0a5d` CLOSE, `0x0a6a` ACK_PREVIEW,
  `0x0a6b` PREVIEW_CAPTCHA (all `*_MACRO_DETECTOR*`) and `0x0a6d`
  ZC_ACK_PLAYER_AID_IN_RANGE. Removed five outgoing packets and rows: `0x0a56`
  report, `0x0a5a` image received, `0x0a5c` answer, `0x0a69` preview request and
  `0x0a6c` players-in-range request. The incoming IDs now take the existing
  unknown-packet path, which resets connection queues.
- Deleted 15 source/header files (`captcha_packets`, `macro_detector`,
  `packets_macro_detector`, `ui/chat/captcha`, `captcha_data`, `macro_report.c`,
  `macro_reporter.h`, `reported_players`, `reporter_controls.h`) and the dedicated
  tests (`build_captcha.bat`, `build_reported_players.bat`, three captcha tests,
  the reported-players test and `captcha_test_bitmap.h`). Orphaned local
  definitions (`PacketInt`, `CZ_UI_OPEN` in `talk_type.c`, the reporter-only
  `ui/chat/chat.h` includes) went with them; the caller audit found no orphaned
  shared helpers. Historical type fixtures retain the five retired class names
  solely to verify that their identities are rejected.

Kept on purpose:

- The `ZC_REQ_AU_BOT` (`0x0a29`) automatic reply `CZ_ACK_AU_BOT` (`0x0a2a`). It
  lived in `macro_detector.c` but has no UI and is not part of the captcha
  challenge; removing it would turn a harmless echo into a queue reset. Its code
  moved unchanged to `src/game/au_bot.c`; the dispatch trace is unchanged.
  Neither checked server sends it. Decide separately whether to retire it.
- `CZ_UI_OPEN` (`0x0a68`), still used by the attendance button.
- Account-server captcha rows `0x0a71`/`0x0a72` and the `0x0a7a` name: a separate
  login captcha that this client never implemented.
- `language_paste()` refused to paste when the focused edit equalled the captcha
  answer box. With no challenge window both are null, so this also suppressed
  Ctrl+V/Shift+Insert while no edit has focus. The check is now
  `focus edit == NULL`, preserving that behavior exactly.

Server implications: servers must not start the macro detector for this client.
rAthena triggers it through the `macro_detector` script command and GM reports;
Hercules through the macro reporter. GMs on other clients can still report a
purist player. The challenge packets would then reset this client's queues, and
the server's unanswered-challenge timeout applies its configured penalty.
Disable the feature server-side. The client no longer sends report, preview or
answer packets. No server checkout was modified.

Validation:

- Release build passed without compiler/linker warnings:
  `build/antibot-pin-no-antibot-build.log`.
- All 1,179,800 packet-dispatch cases passed, digest `d85abd95e90ee207`
  (before: `0e20d9f11f3d4d93`). An independently derived trace replaces only the
  96 cases for the eight retired incoming packets with the existing unknown-packet
  result; its SHA-256 matches the new executable's trace byte for byte
  (`build/derive_antibot_dispatch.py`,
  `build/antibot-pin-no-antibot-dispatch-expectation.log`,
  `build/antibot-pin-no-antibot-dispatch.log`).
- Object-type checks passed for 550 classes and 61 retired identities
  (`build/antibot-pin-no-antibot-object-type.log`). Content-manager traces are
  byte-identical to the baseline, digest `f0fa1aba`. Scheduler, window
  forgetting, language input, talk options, whisper blocks, object operations,
  named dispatch and login OTP suites passed (`build/antibot-pin-no-antibot-*.log`).
  No scheduler job belonged to the challenge; the upload job was removed earlier.

## Second password removal (2026-11-02)

Patrick approved removing second-password (PIN code) support.

- Deleted `src/ui/character/second_password.c`: PIN entry, creation, change and
  deletion windows (`UISecondPasswordInputWnd`, `UISecondPasswordEditWnd`;
  windows `0xc7`, `0xc8`/`0x167`) and their storage-password base class
  `UIStorePasswordInputWnd`. That class was only constructed as the PIN windows'
  base; the storage-password window has no factory or packet handler on either
  branch.
- Removed PIN state (`pinState`, `pinAid`, `pinRequest`, `pinChangeOnly`,
  `pinResultValue`, `pin_failures`), the window slots, the character-selection
  PIN window switch, the hidden character-select PIN make/delete buttons
  (`0x198`/`0x199`), the "enable PIN" command (`0x19a`), and the input block while
  PIN windows were open. The unused button show/hide helpers went with them.
- Removed five incoming result packets with their handlers and table rows:
  `0x08bb` make, `0x08bd` delete, `0x08bf` edit, `0x08c4` not-available,
  `0x08c6` available. Removed six outgoing PIN packets and rows: `0x08b8` entry,
  `0x08ba` make, `0x08bc` delete, `0x08be` edit, `0x08c3` disable, `0x08c5`
  enable. The incoming IDs take the existing unknown-packet path.
- Kept the login-state notices `HC_SECOND_PASSWD_LOGIN` (`0x08b9`, 12 bytes) and
  `HC_SECOND_PASSWD_LOGIN2` (`0x0ae9`, 13 bytes) with their rows and dispatch;
  both now use one handler. Kept `password_prompts.c` unchanged (generic password
  window and character-deletion e-mail window).
- Kept the never-implemented name/length rows `0x08b7` HC_SECOND_PASSWD_REQ,
  `0x02ad` AC_REQUEST_SECOND_PASSWORD and storage-password
  `0x023a`-`0x023c`/`0x0a8e`; removing them would be unrelated cleanup.

PIN-state behavior after the character list:

| State | Meaning (Hercules / rAthena) | Before | Now |
|---|---|---|---|
| 0 | OK or PIN disabled | Character selection | Character selection, unchanged |
| 7 | Unused; pre-2018 rAthena "passed" | Character selection | Character selection, unchanged |
| 1-4 | Ask, not set, expired, create | PIN window over selection | Disconnect, message, login screen |
| 5, 6, 8-11 | Rejected or wrong PIN, errors | PIN messages; third failure quit the client | Disconnect, message, login screen |
| > 11 | Undefined | Ignored | Ignored |

The message is "PIN codes are not supported by this client. Please ask the server
administrator to disable PIN codes." The client then returns to the login screen
the same way as other refusals: disconnect, modal notice, then sub-mode 3.

Server implications: servers must disable PIN codes. Hercules
`conf/char/char-server.conf` `pincode: { enabled: false }` and rAthena
`char_athena.conf` `pincode_enabled: no` both send state 0. With PIN codes enabled
the player cannot reach character selection. No server checkout was modified.

Validation:

- Release build passed without compiler/linker warnings:
  `build/antibot-pin-no-pin-build.log`; final rebuild `build/antibot-pin-build.log`.
- New `tests/build_login_pin_state.bat` drives the real character-server packet
  loop and PIN handler. It checks all 65,536 states for both notices, the
  disconnect-before-message order, message text, login sub-mode and pending-AID
  reset. Session and window-manager state stay byte-identical. The retired PIN IDs
  reset queues. The same harness on the pre-removal sources confirms states 0 and
  7 already selected characters
  (`build/antibot-pin-no-pin-login_pin_state.log`,
  `build/antibot-pin-before-login-pin-harness.log`).
- Packet dispatch digest stays `d85abd95e90ee207`; the trace is byte-identical
  (`build/antibot-pin-no-pin-dispatch.log`). The game dispatcher never handled PIN
  packets.
- Object-type checks passed for 547 classes and 64 retired identities, including
  the three PIN/storage-password types (`build/antibot-pin-no-pin-object-type.log`).
  The packet-table audit confirms 1,366 -> 1,342 length rows: exactly the 13
  removal-1 and 11 PIN IDs disappeared; all retained rows keep IDs, sizes,
  minimum sizes and order (`build/audit_antibot_pin_packet_table.py`,
  `build/antibot-pin-packet-table-audit.log`).
- Focused suites passed again (`build/antibot-pin-no-pin-*.log`). The full `tests/build.bat`
  suite passed in 6 min 51 s (`build/antibot-pin-full-suite.log`).
- Source and final link-map audits found no macro-detector, captcha-challenge,
  reporter, PIN-window or PIN-state code; only the retained rows above, the AU-bot
  reply and `login_mode_on_pin_state` remain (`build/antibot-pin-source-audit.log`).
  `git diff --check` passed.

The pre-removal tree is in `build/before-antibot-pin`. No live client/server smoke
test was run.

## Dead-code sweep (2026-11-02)

Patrick approved removing dead code paths: production code that no runtime path in
the shipped executable can reach. The sweep started from d725b49 plus the day's
validated removals (cash shop, Party Booking, 4th classes, Lapine, grade enchant,
swap equipment, macro detector/captcha, PIN codes).

Removed:

- **Unreachable windows.** Every window-factory ID was traced to its openers:
  literal and named `ui_window_open`/`ui_make_window`/`ui_window_mgr_make_window`/
  `show_option_window` calls, helper wrappers (toggle, invite, family-request,
  market-refresh helpers), the hotkey action table, basic-info/menu-icon/ESC/storage
  tab tables, `ZC_UI_OPEN` mappings, talk types and mode messages. No opener exists
  for these IDs, so their factory cases, slots, `get_window`/untracked/`post_quit1`
  cases, forget-list and teardown entries, saved-rect registry keys, type
  registrations and code were removed:

  | ID | Class | Notes |
  |---|---|---|
  | `0x4` | `UIMakeCharWnd` | Pre-renewal character creation; login opens only `0x116`. `make_char.c` deleted. |
  | `0x9` | `UIToolTipWnd` | `tooltip_wnd.c` deleted. |
  | `0x13`, `0x30`, `0x81` | `UIRestartWnd`, `UIQuitWnd`, `UIRebirthWnd` | Death and ESC already use the ESC option window `0x9b`. The `quit_wnd` Enter-key branch went with them. |
  | `0x34` | `UICompositionWnd` | IME composition box; the candidate window `0x33` stays. |
  | `0x51` | `UIMakeTargetResultWnd` | Make-target list/process windows stay. |
  | `0x5d` | `UINoticeWnd` | `notice_patch.txt` patch-notice reader; `patch_notice.c` deleted. The character-select "raise `0x5d`" branch went with it. |
  | `0x82`, `0x83` | (slots of `UISubChatHisWnd`) | Fixed "ST"/"BT" history slots and their `SUBCHATWNDST`/`SUBCHATWNDBT` saved rects. The class stays for detached chat tabs. |
  | `0x85` | `UISubChatMiniWnd` | Its toggles were the only readers of `ONSTSUBCHAT`/`ONBTSUBCHAT`; that write-only state and its registry keys went too. |
  | `0xa3` | `UIMixAcceptWnd` | `UIYourItemWnd` in the same file stays. |
  | `0xb6` | `UIButtonsOfBasicWnd` | `basic_buttons.c` deleted, plus the render-loop special case, the hotkey-`0x97` second command and five basic-info "window below" branches. |
  | `0x123` | `UITransMenuWnd` | `trans_menu.c` deleted. |
  | `0x125` | `UIRenderTargetWnd` | Debug character render target; `render_target.c` deleted. |
  | `0xc9` | `UIMonsterDisplayWnd` | Its only opener was in `UIString` (below); `monster_display.c` deleted. |

  Factory labels with no constructor were also removed: Lua-window IDs `0x9e`,
  `0xa5`, `0xa6`, `0xa7`, `0xb7` and `0xbf` (only opener: the removed basic
  buttons), and the duplicate star-place label `0x166`.
- **Classes nothing constructs.** A second relink with the type registry emptied
  (below) showed 30 classes whose operation tables were referenced only by the
  type-identity registry, which never constructs objects. They and their files were
  removed: `CGrannyPc` (three files and its header), `TableBox`, `UIBasicItemCtrl`,
  `UIBasicSkillCtrl`, `UIBitmapButton2`, `UIBitmapTab`, `UIChatHisBox3`,
  `UIComboBoxV2Wnd`, `UIEditCtrl_OldList`, `UIImgMenu`, `UIListBox2`,
  `UIMessageBoxAutoreturn`, `UIMiniBtnWnd`, `UINavigationWnd`, `UINewComboBoxCtrl`,
  `UIScrollBar4`, `UIShowAndEditTimeCtrl`, `UISlotBitmap`, `UIStaticItem`,
  `UIStaticItem_Illust`, `UIStaticText3`, `UIString`, `UITableBox`, `UITagButton`,
  `UITagListBox`, `UITextViewEX`, `UITreeView`, `UIZeroQuestScrollBar`,
  `UITVBitmapButton` and `UITVTextButton`. Shared tag-prototype loading
  (`tag_parser.c`) and `UINaviLinkButton` stay.
- **Unreferenced functions and data.** 508 file-scope definitions in all: 375
  external and 86 static functions, 44 global data objects (all of them operation tables) and 3
  static data objects. Besides the classes above, the probe map omitted 38
  external functions and six statics. 36 were dead before the sweep, and the window
  removals orphaned `ui_sub_chat_his_wnd_new` and `character_render_collect_quads`.
  Examples: `byte_queue_prepend`, `connection_unread`,
  `dc_bitmap_copy_scaled`/`dc_bitmap_alpha_from_brightness`, `texture_draw`,
  `mouse_center`, `own_states_set_value`, `drag_info_destroy`, seven `json_value_*`
  accessors, `game_copy_view_matrix_at`/`view_get_view_matrix_at`,
  `world_cell_to_world`, `ui_window_mgr_modeless_message_box`,
  `ez2_str_effect_mgr_get_effect_id`, `wm_half_x`/`wm_half_y`, and the servant-weapon
  fire controls
  (`servant_weapon_effect_fire`/`_shoot`/`_sign_shot`/`_phantom_shot`/`_count_armed`
  and four statics) that the 4th-class removal had kept with their class. 333 stale
  prototypes and extern declarations were removed with them, together with the
  macros, struct definitions, forward declarations and layout asserts they orphaned.
- **Write-only state and dead branches.** `Session.jobIdNames` (filled at startup,
  never read) and its table; the sub-chat state and rect keys above; two
  unreferenced locals (`packets_guild_roster.c`, `make_target.c`). Checks of the
  retired windows' slots or IDs that could no longer succeed were removed with them,
  as were `post_quit1` calls for those IDs, which had become no-ops.
- **Files and directories.** 26 source files deleted (23 `.c`, 3 headers:
  `granny_pc.h`, `ui_string.h`, `navi_wnd.h`); four files that used `granny_pc.h`
  and `wndmgr_keys.c` (`navi_wnd.h`) now include only the headers they actually
  needed from it. The empty directories `src/browser`, `src/replay`,
  `src/security`, `src/ui/media`, `src/ui/net` and `src/ui/tips` were deleted.
  Net change of this sweep (working tree against the staged base): src 12,464
  lines deleted / 28 added in 160 files; tests 1,073 deleted / 125 added in 40 files.

Kept, and why:

- **Real indirect references.** Everything reachable through live operation tables,
  Lua C-function registration, packet/effect dispatch tables or saved settings.
  Effect classes the removed code spawned (e.g. the render-target effect `0x442`)
  stay because server effect packets can name their IDs.
- **Packets.** No packet handling changed; all size/name rows are untouched.
- **`OBJECT_BASIC_BUTTONS_WINDOW`.** No registered type carries this query bit any
  more. The enumerator stays because the historical 17-query cast fixture is indexed
  by it.
- **Speculative branches.** Mode-message and talk-type cases without a visible
  sender were not chased. Many senders are wrappers or computed (`type + 0x3e`,
  stat-command tables), and a before/after comparison found no mode message that
  this sweep left without a sender. Residue IDs in the untracked/`post_quit1` lists
  that no factory creates (`0x32`, `0x43`, `0xab`, …) were left as found.
- **Option-window request `0xd`.** `make_basic_info` still requests nonexistent
  window `0xd` when the saved `OPTIONWNDINFO.SHOW` is set. The call is reachable
  from saved settings and touches the ESC window-id list, so it is reported below
  rather than removed. Fixed on `main` in `07135fd6d1` (`purist` `7bde71e3b3`).
- **Tests and fixtures.** Retired type names stay in `type_query_fixture.h`;
  `test_object_type.c` lists the 44 new retired identities.

Method:

1. Window openers were traced with a script that resolves every call's ID
   argument (literals, `#define`/enum constants, ternaries) and follows `int`
   parameters back through helper functions to their callers. Each remaining
   variable source (storage tab table, guild tab `0x3c + tab`, hotkeys, menu
   tables) was checked by hand. The trace was rerun after each removal round;
   removing `UIString` orphaned window `0xc9`.
2. **Probe build** (`build/deadcode/deadcode.bat`): all of `src` with the release
   flags plus `/Ob0 /Gy /Gw /W4`, linked with `/OPT:REF /MAP` into
   `build/deadcode/`. `dumpsyms.bat` lists defined externals and `analyze.py`
   reports those absent from the map. MSVC does not emit C4505 for C (verified with
   a one-file test), so static functions and data were checked against the map's
   static-symbol section instead.
3. **Registry-less relink** (`noreg.bat`): the same objects linked against a copy of
   `object_type.c` with an empty type table; `mapdiff.py` lists what disappears.
4. Removal used `remove_defs.py` (definitions and same-file prototypes by name),
   `stale_decls.py` (prototypes whose definitions the sweep removed) and
   `leftovers.py`/`remove_names.py` (orphaned macros/structs). Steps 1–4 were
   repeated until nothing new appeared. The final round reports only CRT symbols,
   folded `static const` scalars, the stdcall-decorated `sound_fade_thread@4`, and an
   empty registry-only difference.

Tests changed:

- Removed with their code: the UI string suite (`build_ui_string.bat`,
  `test_ui_string.c`, `ui_string_owner.c`, `ui_string_test.h`,
  `ui_string_unreached.h`), the tag-button/list-box parts of `test_link_buttons.c`,
  tree-label cases in `test_text_buttons.c`, bitmap-tab and button-2 drop owners
  (and the blocked-exchange case that used button 2) in `test_ui_drop.c`, the
  render-target callback in `test_named_client_dispatch.c`, granny-PC parts of the
  actor action/appearance/render/granny-model tests, the own-view pass of
  `test_character_model.c`, and the single assertions/calls for the removed
  helpers. Stub headers were pruned to what each test still links.
- Retained tests rewired instead of keeping dead production helpers: the barter
  test frees its drag strings directly instead of `drag_info_destroy`; the service and
  merchant-store JSON tests use test-local number/null readers instead of the removed
  `json_value_*` accessors; the radio test assigns the tooltip directly instead of
  `ui_bmp_radio_btn_set_tool_tip`; the entry-appearance case uses a sprite PC
  (`pc_set_spr_act`) instead of `CGrannyPc`. The granny pose case now poses a
  model state directly (time 0.25) to keep its `granny_model_prepare_mesh`
  coverage. `test_forget_window.c` uses `skill_describe_wnd` as its sample slot.
- Object-type expectations: 475 classes (519 − 44), 136 retired identities, 329
  named, 309 windows, 2,741 comparisons, 208 accepted casts.

Validation:

- **Release build.** Passed without compiler or linker warnings:
  `build/deadcode-build.log`. None of the 419 removed external definitions and none
  of the 44 retired type names remain in `build/main_rebuild.map` or `src`
  (`build/deadcode/map_audit.py`).
- **Probe convergence.** The final probe round reports only CRT data, folded
  `static const` scalars and the stdcall thread entry (`build/deadcode/report-final.txt`).
  The registry-less relink drops nothing (`build/deadcode/noreg-final.txt`). The
  opener trace leaves only the table-driven guild tabs (`0x3c + tab`) and storage
  tabs (`s_tabIds`) without a literal opener. No stale prototypes or orphaned
  macros/structs remain, apart from the intentional historical fixture names.
- **Packet dispatch.** All 1,179,800 cases passed. The digest stays
  `a3fd444e45d2becf`, and the regenerated trace is byte-identical (SHA-256
  `44404caa…25123d0`) to the pre-sweep copy `build/packet-dispatch-before-deadcode.bin`
  (`build/deadcode-packet-dispatch.log`). No packet handling changed, so the
  expectation is unchanged.
- **Type checks.** Passed for 475 classes and 136 retired identities, with 2,741
  comparisons and 208 accepted casts.
- **Full suite.** All commands in `tests\build.bat` passed in 5 min 57 s
  (`build/deadcode-full-tests.log`). Digests are unchanged: content managers
  `f0fa1aba`, effect factories `274deeba`, EZ2STR `817beee3`, effect operations
  `a5ab8449`, particle pools `eca892c6`.
- **Standalone suites.** COM lifecycle, emote timing, headgear names, NPC music, pet
  accessory, pet portrait and skill-tree hints passed (`build/deadcode-com_lifecycle.log`,
  `build/deadcode-emote_timing.log`, `build/deadcode-headgear_names.log`,
  `build/deadcode-npc_bgm.log`, `build/deadcode-pet_accessory.log`,
  `build/deadcode-pet_portrait.log`, `build/deadcode-skill_tree_hints.log`).
  `build_agency_server.bat` was not run (it needs the patched Hercules checkout).
- **Diff check.** `git diff --check -- src tests` passed.

Bugs and foundation candidates found (not changed unless noted):

- `make_lua_window` dereferenced a NULL window for factory IDs `0x9e`, `0xa5`,
  `0xa6`, `0xa7`, `0xb7` and `0xbf` (no constructor case). Nothing reachable opens
  them now; the labels were removed. Before the sweep `0xbf` was reachable only from
  the never-created basic buttons window.
- If the saved `OPTIONWNDINFO.SHOW` is set, `make_basic_info` requests window `0xd`,
  which no factory creates. The client never sets it to 1; it only round-trips the saved value. The
  request leaves `0xd` in the ESC window-id list, so the list is never empty. ESC
  then tries to close `0xd`, fails and opens the ESC menu instead of cancelling
  active navigation guidance (`guideState == 2`). Fixed on `main` in `07135fd6d1`
  (`purist` `7bde71e3b3`); requests that create no window are no longer kept in
  the list either (`main` `1150faabf3`, `purist` `8cc259bdc5`).
- ESC menu button `ESC_ID_1C1` toggles window `0xec`, which no factory creates.
  Dead code: no ESC menu button carries that ID. Removed by the
  [leftover cleanup](#leftover-cleanup-2026-11-03).
- Misleading names: `keyboard.c` `MODE_MSG_TOGGLE_WINDOW` (0x1f) is
  `MODE_MSG_EMOTION`, and its `key_windows` entries are emotion IDs;
  `ui_window_mgr_render_menu` in `session_job_names.c` actually builds the job,
  sprite and palette name tables. Renamed by the
  [leftover cleanup](#leftover-cleanup-2026-11-03).
- `test_service_json.c` still prints a stale "Party booking JSON" line.

Open risks:

- Registry reads share one `size` variable across `RegQueryValueExA` calls. The
  removed `SUBCHATWND*`/`ONSTSUBCHAT`/`ONBTSUBCHAT` reads could only have affected
  later reads if those values had been stored with a non-DWORD size.
- `WM_AT_NONE` (only used by the removed `0x85` case) was removed, so the
  placement enum values shift by one. Every use is symbolic.
- No live client/server smoke test was run.

The pre-sweep tree is in `build/before-deadcode-sweep`; probe tools, maps and reports
are in `build/deadcode`.

Independent rerun on the final tree: the release build (no warnings), the full
suite (dispatch digest `a3fd444e45d2becf`; 475 classes, 136 retired identities)
and the seven standalone suites passed: `build/final-build.log`,
`build/final-full-tests.log`, `build/final-<suite>.log`.

## AU-bot reply, DdukDdak box test and Party Recruit removal (2026-11-02)

Patrick decided to remove three follow-up items left pending by the size-ranked
review: the AU-bot auto-reply, the DdukDdak box test and the Party Recruit packet
rows. The stale "Party booking JSON" line in `tests/test_service_json.c` went in
the same change.

What the client did before this removal:

- `ZC_REQ_AU_BOT` (`0x0a29`, 6 bytes: AID) made the second game dispatcher answer
  immediately with `CZ_ACK_AU_BOT` (`0x0a2a`) carrying the same AID. There was no UI,
  state or log. The code lived in `src/game/au_bot.c`, where the macro-detector
  removal had moved it unchanged.
- `ZC_DDUKDDAK_BOX_TEST` (`0x0bb3`, variable, minimum 8 bytes: box item ID and
  `{item ID, count}` records) is a GM statistics report. The client merged the
  records by item ID, sorted them by count, resolved item names and wrote
  `DdukddakBoxTest\<date>_(<box name>).txt` under the client directory with
  per-item percentages. It never touched the Lapine windows or catalogs.
- Party Recruit (the 2012-04 RE recruitment UI) had 28 packet-name rows
  (`0x08e5`-`0x08fb`, `0x0909`, `0x090b`, `0x090c`, `0x096c`, `0x0971`) and two
  fixed 6-byte length rows (`0x096c`, `0x0971`). No handler, sender, ID constant or
  UI existed, and the name table is only filled in, never read.

Removed:

- AU bot: the dispatch case, `PACKET_ZC_REQ_AU_BOT`, `HEADER_ZC_REQ_AU_BOT`,
  `src/game/au_bot.c` and `au_bot.h`, and the length and name rows of `0x0a29` and
  `0x0a2a`.
- DdukDdak box test: the dispatch case, `HEADER_ZC_DDUKDDAK_BOX_TEST`, the handler
  and its declaration, its private sort (introsort, heap and insertion sort over the
  merged records) and the `0x0bb3` length row (it had no name row). The handler was the
  only user of the report writer `src/game/box_report.{c,h}` and of
  `src/core/unique_file.{c,h}` (`file_open_unique`, the numbered "name(1).txt"
  opener), so both pairs were deleted along with the includes only the handler used.
  `tests/test_c_output.c`, which tested only those two helpers, and its four
  `tests/build.bat` lines went too. The neighbouring package-item test report
  (`0x0a31`) in `src/game/packets_item_test.c` is unchanged.
- Party Recruit: the 28 name rows and 2 length rows.
- `tests/test_service_json.c` no longer prints "Party booking JSON: ...", a leftover of
  the Agency's former JSON decoder. The service-JSON line remains.

Kept: the `0x0a31` package-item test report, and every other row of the packet table.
Two includes in `packets_item_test.c` (`ui/items/styling_shop.h`, `ui/ui_std.h`) were
already unnecessary before this change; they were left as found.

Server implications:

- `0x0a29` and `0x0bb3` now take the existing unknown-packet path, which resets
  connection queues. Neither Hercules nor rAthena sends them. Hercules has only a
  `packetLen` row for `0x0bb3` and accepts `0x0a2a` with `clif->pDull`, so nothing is
  lost when the client stops answering. rAthena defines neither.
- Hercules sends Party Recruit packets only when `PARTY_RECRUIT` is defined, which
  `mmo.h` limits to 2012-04-10/2012-04-18 RE clients. A `PACKETVER=20220406` build
  never does. `0x096c`/`0x0971` already reached the unknown-packet reset before; they
  are now framed as unknown packets too.
- No server checkout was modified.

Tests changed: the packet-dispatch test lists `0x0a29` and `0x0bb3` among the IDs that
must take the 12-entry unknown-packet trace, its sink file loses the two handler
sinks, and the digest moves to the derived value. `test_c_output` was removed with its
helpers.

Validation:

- Release build passed without compiler or linker warnings:
  `build/packet-only-build.log`. The link map and `src`/`tests` contain no AU-bot,
  DdukDdak, box-report, unique-file or Party Recruit names; the only `recruit` hits are
  the Agency's `adventurer_agency_is_recruiting` and its board bitmap.
- Packet dispatch: all 1,179,800 cases passed, digest `45cac67ceea7fb03` (before:
  `a3fd444e45d2becf`). `build/derive_packet_only_dispatch.py` derives the expectation
  from the regenerated baseline trace (`build/packet-dispatch-before-packet-only.bin`,
  SHA-256 `44404caa...25123d0`). It changes exactly 24 cases: 12 each for `0x0a29` and
  `0x0bb3`, the full-size non-null calls in all six packet patterns, direct and
  forwarded from the first dispatcher. Each formerly called its handler sink
  (`GameMode_OnReqAuBot`, `GameMode_Zc_Ddukddak_Box_Test`) and now resets the queues.
  The actual trace matches byte for byte (SHA-256 `84adb52d...4ab35686`):
  `build/packet-only-dispatch.log`, `build/packet-only-dispatch-expectation.log`.
- Packet table: 1,247 -> 1,242 length rows and 1,315 -> 1,285 name rows; every retained
  row keeps its ID, size, minimum size, name and order:
  `build/audit_packet_only_packet_table.py`, `build/packet-only-packet-table-audit.log`.
- Full suite `tests\build.bat` passed: `build/packet-only-full-tests.log`.
- `git diff --check` passed.

The pre-removal tree is in `build/before-packet-only-removals`. No live client/server
smoke test was run.

## Remaining 4th-class content and grade-effect removal (2026-11-02)

Patrick decided to remove the content the 4th-class and grade removals had kept: the
grade-enchant effects `0x647`-`0x649`, the 4th-class status visuals, the ABR/Bionic
summons and the effect data shared only with them. A server request for a removed
effect or status is ignored.

What remained before this removal:

- **Status visuals.** `actor_effect_state` (`actor_efst.c`), the map-entry refresh
  `player_refresh_status_effects` (`player_states.c`) and the begin/end handlers in
  `state_messages.c`/`state_messages_timed.c` spawned, removed and tinted effects and
  printed begin/end lines for 4th-class statuses the server can still send. These
  included Climax, Charging Pierce, Servant Weapon, Dragonic Aura, Vigor, Dancing Knife,
  Eternal Slash, Shadow Exceed, Competentia, the Inquisitor brands and faiths, A/D Machine,
  Abyss Slayer and From the Abyss, Windhawk signs, the Troubadour songs, Spell Enchanting,
  Elemental Veil and the twelve handicap states (lassitude, misfortune and dot marks,
  colour tints and instant-variable flags).
- **Effect data.** The 4th-class removal deleted the client triggers but kept the data
  that packet `0x01f3` can still name: effect-factory rows, `ez2str` STR files and their
  sound/list/height tables, RagEffect per-ID processes (spell circles, ABR circles,
  trait-stat icons), the `severe_rain_storm` branches for `0x6d8`, `0x7ea`, `0x875`,
  `0x92c`-`0x92f` and `0x932`, and ten effect classes spawned only by the status
  visuals. Grade enchanting kept `0x647`-`0x649` on the same basis.
- **ABR/Bionic summons.** `ZC_ABR_INIT` (`0x0b6b`) and `ZC_ABR_PAR_CHANGE` (`0x0b6c`)
  filled two session HP trees that made the summon an own unit, gave object types 13/14
  an owner, an HP-only player gauge and an entry HP message. The types were also treated
  as summons for picking, siege targeting, the skill-used notice, the action-7 motion and
  the homunculus/mercenary AI `IsMonster`. The Biolo Hell Tree (job `0x5173`) fired a
  `SUMMON_HELLTREE_FRUIT` missile.

How statuses were attributed: a status counts as 4th-class when rAthena's
`db/re/status.yml`, `db/re/skill_db.yml` and its skill sources apply it only from
4th-class job skills (DK, MT, SHC, AG, CD, WH, IG, BO, ABC, EM, IQ, TR), or when it belongs
to the 4th-class handicap-state family. 67 status IDs qualify: `0x480`, `0x488`-`0x48a`,
`0x494`-`0x498`, `0x49a`, `0x4a0`, `0x4a6`, `0x4a8`-`0x4c1`, `0x4c4`-`0x4c6`, `0x4cb`, `0x4cc`,
`0x4ce`-`0x4d2`, `0x4d4`, `0x4d5`, `0x4dc`, `0x4dd`, `0x4df`-`0x4e2`, `0x4e4`, `0x4e8`-`0x4ef`,
`0x4f7` and `0x515`.

Removed:

- **Status handling.** All cases for those 67 IDs in the four status files: 51 labels in
  `actor_effect_state`, 38 in the map-entry refresh, 39 end cases in `state_messages.c`
  and 35 begin cases in `state_messages_timed.c`, plus their entries in the quiet-status
  lists and the own-state value capture for `0x480`/`0x4ac`/`0x4dc`. The statuses are now
  unknown to the client: the generic status icon from Lua `GetEFSTImgFileName` remains,
  nothing else is drawn or printed. Removed with them: `ef_charging_pierce`,
  `ef_spawn_or_remove`, `ef_spawn_once_or_end`, `ps_spawn_once_here`, `say_n`, `say_s`
  and `STATE_COLOR_ALERT`; the handicap marks (actor messages `0x98`-`0x9d`, fields
  `lassitude_mark`, `misfortune_mark`, `handicap_dots_mark`); and
  `game_actor_set_instant_var`, whose only writes were the handicap flags `0xc`-`0x10`,
  together with the always-true checks of those flags in `attr_state.c`.
- **Effect data.** 566 effect IDs (`build/fourth-class-leftovers-effect-ids.json`):
  - The 553 found by tracing: effects referenced only by the removed status cases;
    effects whose only triggers were deleted by the 4th-class removal (found by diffing
    `build/before-fourth-class-removal` against `src`, including computed IDs such as
    `0x6f9+i` and `0x796+dir`); the severe-rainstorm IDs; and grade `0x647`-`0x649`.
  - 13 untriggered assets named for 4th-class skills or trait stats: `0x6b2`, `0x6b3`,
    `0x6b7` (servant phantom/demolition), `0x6e3` (violent quake), `0x6eb` (mystery
    illusion), `0x72c` (reparatio), `0x790` (ABR battle warrior), and the other six trait
    icons `0x8f1`-`0x8f4`, `0x8f7`, `0x8fb`.
  - This removed 534 factory registrations (1,186 -> 652), 515 STR file rows, their
    entries in five `ez2str` type tables, 219 `ez2str` case labels with their sound,
    list and angle code, 32 RagEffect per-ID cases, and the eight `severe_rain_storm`
    branches (only Severe Rainstorm `0x4fc` stays).
  - Ten effect classes were deleted with their declarations: `servant_weapon_effect`,
    `charging_pierce_effect`, `climax_effect`, `a_machine_effect`, `dancing_knife_effect`,
    `eternal_slash_count_effect`, `from_the_abyss_effect`, `shadow_exceed`,
    `full_screen_effect` and `competentia`.
  - Other special cases went too: the Wind Sign render exemption (`0x76c`), the Deft Stab
    sound (`0x854`) and the Shield Shooting branch of the axe boomerang (`0x887`).
  - Newly orphaned helpers went with them: `rag_status_icon`, `rag_status_arrow`, the
    casting-circle styles `0x2a`-`0x32`, `effect_screen_heading` (with its dedicated
    test), `tree_less_int64` and `struct Int64Pair`.
- **Clean ignore.** `game_actor_launch_effect` returns NULL for the 593 retired effect
  IDs (these 566 plus the 27 the 4th-class removal deleted), merged into 20 ranges.
  Without it, `0x01f3`/`0x0284`/`0x0b69`, show-effect statuses and Lua lists naming a
  retired ID would fall through to an inert RagEffect, as the 27 earlier IDs did: it is
  allocated and recorded, asks Lua `IsForceRenderEffect`, lives for
  `g_effectDurations[id]` steps (about the ID value) and draws nothing. Now nothing is
  created.
- **ABR/Bionic summons.** Incoming `0x0b6b`/`0x0b6c`: handlers, IDs, declarations,
  dispatch cases and both packet-table rows. `session/abr.h` and the `abrHp`/`abrMaxHp`
  session trees with their lifetime, login and vanish handling. The ABR term of
  `session_is_own_unit_aid`; the type 13/14 gauge source, layouts and entry HP message
  (dropping the now unused `id_0x22` parameter of `entry_hp_tail`); the owner assignment;
  `OBJECT_TYPE_ABR`/`OBJECT_TYPE_BIONIC`; and the 13/14 terms in siege targeting,
  `sm_kind_has_owner`, the pick helper (renamed `is_merc_or_elemental`), the action-7
  motion, the skill-used notice and `IsMonster`. Also the Hell Tree missile mapping.

Kept, and why:

- **Statuses shared with monsters or 3rd classes:** `0x47e` (Deadly Projection and
  `NPC_DEADLYCURSE`), `0x48d` (Genetic Hell's Plant), `0x48e` (`NPC_RELIEVE_ON`),
  `0x48f` (`NPC_LOCKON_LASER`), `0x53b` (`NPC_KILLING_AURA`), and the Curse of Red/Blue
  Cube statuses `0x539`/`0x53a`, which no checked server applies and which are not
  attributable to a 4th class. Their effects stay too: the `curse_of_cube_effect` class
  (`0x901`/`0x902`), `0x6cd` for Deadly Projection, and `0x63f`/`0x641`/`0x64a` for
  `0x48d`-`0x48f`. Every other effect that kept code still triggers stays, for example
  the item-use effects `0x63b`-`0x63f` and the ground-item drops `0x74d`-`0x74f`.
- **Effect IDs with no 4th-class link:** `0x642`-`0x646` (skill unit `0x10c`),
  `0x907`-`0x90f` (enchanting-window effects kept by START-049), `0x67a`, `0x67b`,
  `0x92a`, `0x92b`, `0x934`-`0x93d` and `0x943`/`0x944`, none of which had a client
  trigger; and the level-150 aura pair `0x8e3`/`0x8e4`, which the Lua aura tables may
  still name. The locally available `skilleffectinfo` Lua data names effect IDs only up
  to 1242, below every retired ID.
- **`durations.c`.** It is the original executable's dense positional table, not
  per-effect rows. With the factories, STR rows, per-ID cases and the launch guard gone,
  its retired slots are never read, so it stays byte-identical (the durations test still
  asserts the original digest).
- **Shared summon code.** Homunculus, mercenary, elemental and pet ownership, gauges,
  `session_is_own_unit_aid`, picking, targeting and AI handling are unchanged apart from
  dropping the 13/14 terms.

Server implications:

- `0x0b6b`/`0x0b6c` take the existing unknown-packet path, which resets connection
  queues. rAthena sends them only for summons with `AI_ABR`/`AI_BIONIC`, which only
  4th-class skills create; Hercules does not send them.
- An actor of object type 13/14 is now an ordinary monster-type unit: no owner, owner
  name colour or player gauge, no entry HP message, and siege checks judge the unit
  itself.
- The 67 statuses are unknown to the client: icons still come from Lua, but no effect,
  tint or chat line appears and no own-state value is kept for them.
- Effect requests for the 593 retired IDs show nothing and allocate nothing.
- No server checkout was modified.

Tests changed:

- The packet-dispatch test adds `0x0b6b`/`0x0b6c` to the unknown-packet IDs and loses
  their sinks.
- The effect-factory fixture loses the ten classes; its digest moves to the derived value.
- `test_effect_launch` checks that six retired IDs, at range edges and inside ranges,
  allocate and record nothing.
- `test_gauge_sources` drops its layout-13 case; the 64-bit source checks remain.
- `test_map_nodes` keeps its session instant-variable checks without the deleted actor
  setter.
- `test_effect_heading` was removed with `effect_screen_heading`.
- Object-type counts are unchanged (475 / 136 / 329 / 309 / 2,741 / 208): none of the
  deleted effect classes was in the object-type registry, whose only effect class is
  `CTetraBallEffect`.

Validation:

- Release build passed without compiler or linker warnings:
  `build/fourth-class-leftovers-build.log`.
- Packet dispatch: all 1,179,800 cases passed, digest `679cf1c915cfe60f` (before:
  `45cac67ceea7fb03`). `build/derive_fourth_class_leftovers_dispatch.py` changes exactly
  24 cases of the regenerated baseline (`build/packet-dispatch-before-fourth-class-leftovers.bin`):
  12 each for `0x0b6b` and `0x0b6c`, full-size non-null calls in all six patterns, direct
  and forwarded. Each formerly called `game_mode_zc_abr_init`/`_par_change` and now
  resets the queues. The actual trace matches byte for byte (SHA-256 `2edd6f23...3d5ea14`):
  `build/fourth-class-leftovers-dispatch.log`,
  `build/fourth-class-leftovers-dispatch-expectation.log`.
- Effect factories: digest `616ea281` (before: `274deeba`).
  `build/derive_fourth_class_leftovers_effect_factories.py` drops the 1,602 registered
  nodes of the 534 retired IDs across the three registry cases and renumbers the
  fixture for the ten deleted classes; the deliberate failure case still collides on
  `0x943`. The real trace (`build/effect-factories-after-fourth-class-leftovers.bin`)
  matches the derivation byte for byte:
  `build/fourth-class-leftovers-effect-factories-expectation.log`.
- Edits were made by scripts that verify every non-retired ID keeps its factory, STR
  row, table membership and case body (`build/fourth-class-leftovers-effect-data.log`,
  `-status-cases.log`, `-status-lists.log`). `build/audit_fourth_class_leftovers.py`
  confirms that only the two ABR rows left the packet table (1,242 -> 1,240 length rows,
  1,285 -> 1,283 name rows); that no retired ID remains in the registry, STR rows,
  `ez2str` cases or RagEffect cases; and that none of 32 removed functions remains in
  the link map: `build/fourth-class-leftovers-audit.log`.
- Full suite `tests\build.bat` passed in 5 min 40 s:
  `build/fourth-class-leftovers-full-tests.log`. Unchanged digests: content managers
  `f0fa1aba`, effect operations `a5ab8449`, EZ2STR `817beee3`, particle pools `eca892c6`,
  skill values `f620423f`; object types 475 classes and 136 retired identities; effect
  durations unchanged.
- The seven standalone suites passed: `build/fourth-class-leftovers-<suite>.log`.
  `build_agency_server.bat` was not run (it needs the patched Hercules checkout).
- `git diff --check` passed.

Bugs found (not fixed):

- `rag_effect_init` reads `g_effectDurations[effect_id]` for any ID up to 60000, but the
  table has 2,385 entries, so a server effect ID from 2385 to 60000 without a factory
  reads past its end (`rag_str_anim.c`). Foundation candidate. Fixed on `main` in
  `e26afaf764` (`purist` `6095736a2c`).
- Removed with the feature: the ABR handlers keyed the HP trees with a sign-extended AID
  while vanish and the readers used the zero-extended one, so AIDs of `0x80000000` and
  above never matched. The trees were also cleared only on vanish and at the login
  screen, not on map change.
- Dead branches left as found: `ui_player_gage_on_draw`'s `case 9`/`case 10` (no code
  sets those layouts; mercenary and elemental use the default layout), and the
  non-own-unit `pc_gage` block of `npc_send_msg` message `0x22`, which no sender reaches.
  Both removed by the [leftover cleanup](#leftover-cleanup-2026-11-03).

The pre-removal tree is in `build/before-fourth-class-leftovers`. The edit and derivation
scripts are `build/edit_fourth_class_effect_data.py`,
`build/edit_fourth_class_status_cases.py`, `build/edit_fourth_class_status_lists.py`,
`build/derive_fourth_class_leftovers_dispatch.py` and
`build/derive_fourth_class_leftovers_effect_factories.py`. No live client/server smoke
test was run.

## Skill-point preview removal (START-181, 2026-11-02)

Patrick decided to remove the skill-point preview and its Apply/Reset controls and to
restore the classic behavior: activating a skill's raise control sends one ordinary
upgrade request at once.

What the window did before this removal (`UINewSkillListWnd`, window `0x25`):

- Every refresh (`MSG_REFRESH`, sent on skill lists, skill updates and skill-point
  changes) copied the session's class-tab skill lists into four window-owned working
  lists (`tab_skills`), reset a local `points_left` from `g_session1.skillPoints` and
  cleared the plan. The class tabs displayed those copies; the extra-skills tab showed
  the session list directly.
- Tree view: left-clicking a skill cell planned levels in a `planned` list instead of
  sending anything. It walked the hovered skill's prerequisite list and also planned
  every missing prerequisite level (the tree's automatic prerequisite inclusion),
  showing "cannot raise %s" (`0xdcb`) for a prerequisite with no level limit.
- Compact view: the up buttons incremented the level in the working copy.
- Pending changes showed as a `+N` label on tree icons, purple/blue planned levels,
  the "left(spent) / total" points text, hidden level-selection arrows and blocked
  skill dragging. Apply (`0x107`) asked for confirmation (`0x561`) and then sent the
  planned increases as `0x0112` requests through mode message `0x43`; Reset (`0x163`)
  discarded the plan.

Now:

- `skill_tree_raise_skill` sends one `0x0112` request (mode message `0x43`) for the
  activated skill when it can be raised now: the session has skill points left, the
  skill is on the displayed tab, below its maximum level, and every prerequisite in its
  Lua requirement list already has at least the required level in the session's skill
  trees (a prerequisite missing from the session counts as level 0). Levels are the
  session's displayed `level` values; the removed tree planner had used the separate
  `level2` field, which a `0x010e` skill update leaves stale. Otherwise nothing is
  sent. Nothing changes locally; levels and points move only when the server's skill
  and point updates arrive, which refresh the window as before.
- The existing controls are unchanged: the tree view raises on a left click of a cell
  with a level limit, the compact view through the up buttons of upgradable skills.
  Repeated activations before the server answers send one request each.
- START-179 pinning stays. Its lock now refuses the immediate raise while prerequisites
  are pinned (`show_required`) instead of refusing planned points.
- START-180 tab hints stay. They now search the session's class-tab lists, which are
  exactly what the window displays. `skill_tree_mark_required` keeps the merged
  required level in the window's own list instead of writing it back into the skill's
  requirement entries, which are now session data rather than a window copy; the
  highlighted levels are unchanged.
- Displayed levels and remaining points come only from session state: the class tabs
  show `player_skill_bundle_tree_tab`, both points texts show `g_session1.skillPoints`,
  and the tree's "can be learned" highlight and the compact up buttons use it too.

Removed:

- The `planned`, `tab_skills`, `points_left` and `pending` members, the Apply and Reset
  buttons (`btn_apply*`/`btn_reset*` bitmaps, IDs `0x107`/`0x163`), the confirmation
  (`0x561`), the "cannot raise" notice (`0xdcb`), `skill_tree_planned_level`,
  `skill_tree_reset_points`, `skill_tree_apply_points` and the automatic prerequisite
  planning.
- The pending-change display: the `+N` labels, planned-level colors, the
  "left(spent) / total" points format, and the `pending` gates on the level-selection
  arrows and skill dragging.
- `skill_tree_skill_at`'s session/copy switch (both lookups now read the same session
  list) and the comparisons against the session level that only highlighted planned
  differences.
- `player_skill_bundle_flag_all_level_changes`, whose only production caller was the
  refresh copy; it set flags nothing reads. `tests/test_map_nodes.c` no longer calls it.
- Two includes left unused (`ui/chat/chat_window_parts.h` for the post-Apply chat focus,
  `core/msg_str.h` in the points file) and the redundant `core/thread_text.h` include
  of the draw file.
- `UINewSkillListWnd` shrinks from `0x2b0` to `0x278` bytes; its layout assertions
  follow the new offsets.

No saved setting belonged to the removed controls: the compact-view
(`Simplicity_SkillList`) and hover-description (`Show_SkillDescript`) options keep
their meaning, and no plan was ever stored. The window-position record is unchanged.

Server implications: none. The client sends the same `0x0112` request as before, one
per activation and only for skills it believes can be raised; the server remains the
authority and replies with the usual skill and point updates. Raising a skill whose
prerequisites are missing now needs the prerequisites raised first.

Bug found (removed with the feature, not fixed separately): on the compact view's
extra-skills tab, the up button incremented the session's own extra-skill level, since
that tab showed the session list rather than a copy. Apply compared the list with
itself and sent nothing, and Reset did not restore it, so the displayed level stayed
wrong until the next server update.

Observation: `GameMode.skill_upgrade_pending` is set by mode message
`0x43` and cleared by skill updates but never read. Removed by the
[leftover cleanup](#leftover-cleanup-2026-11-03).

Tests changed: `tests/test_skill_tree_hints.c` now builds its tabs as session lists (the
session's tree-tab lookup, tree-skill level lookup and the mode-message sender are
fixture boundaries) and adds 11 checks of the production `skill_tree_raise_skill`,
extracted verbatim with its prerequisite helper and message constant: one request when
prerequisites are met, one per activation without local changes, nothing for a
prerequisite one level short or missing, at maximum level, for a skill on another tab,
an unknown skill, no points or pinned prerequisites, and a raise from the extra tab.
The duplicate-requirement case now also checks that requirement entries are not
rewritten. 28 checks in all.

Validation:

- Release build passed without compiler or linker warnings (the `UINewSkillListWnd`
  layout assertions included): `build/skill-point-preview-build.log`.
- `tests\build_skill_tree_hints.bat`: 28 checks, 0 failures, no outstanding allocations
  (`build/skill-point-preview-skill_tree_hints.log`). The 17 earlier START-179/180 checks
  are unchanged apart from reading the session lists. A mutation run that dropped the
  prerequisite check and the pin lock from a scratch copy of `skill_tree_points.c`
  failed at the "prerequisite one level short" check, so the new checks detect the
  removed automatic prerequisite inclusion.
- Full suite `tests\build.bat` passed in 6 min 5 s: `build/skill-point-preview-full-tests.log`.
  Packet dispatch stays `679cf1c915cfe60f`; skill values (`test_map_nodes`) digest
  `f620423f` is unchanged.
- The seven standalone suites passed: `build/skill-point-preview-<suite>.log`.
  `build_agency_server.bat` was not run (it needs the patched Hercules checkout).
- `git diff --check` passed.

The pre-removal tree is in `build/before-skill-point-preview-removal`. No live
client/server smoke test was run.

## Leftover cleanup (2026-11-03)

Patrick approved removing the small leftovers reported by the 2026-11-02 removals and
renaming the misleading names they found.

Removed:

- `GameMode.skill_upgrade_pending`: set by mode message `0x43` (`0x0112` upgrade
  request), cleared by three skill-list updates, never read.
- `ui_player_gage_on_draw` layouts 9 and 10. Only layouts 7 (pet) and 8
  (homunculus) are ever set; mercenary and elemental gauges use the default layout.
- The NPC HP gauge (`pc_gage`) of `npc_send_msg` message `0x22`. All four senders
  target the player's own units: `npc_on_process` for the own pet
  (`Session.aid` holds the pet AID), `npc_run_ai` after loading homunculus or
  mercenary AI, and `elemental_tell` for the own elemental. Every one of them takes
  the companion-gauge branch, so the non-own branch never ran and `pc_gage` stayed
  `NULL` on every NPC. The field, its placement in `npc_on_process` and its cleanup
  in `npc_destruct`/`npc_construct` went with it; the message now returns early for
  non-own units. The `UIPcGage` class stays, because party, messenger and PC code use it.
- ESC menu command `ESC_ID_1C1`, which toggled nonexistent window `0xec`. No ESC
  menu button carries that ID, here or in the original executable.

Renamed (no behavior change):

- `keyboard.c`: `MODE_MSG_TOGGLE_WINDOW` → the shared `MODE_MSG_EMOTION`,
  `key_windows` → `flag_emotions`, `WM_KEY_WINDOW_FIRST` →
  `WM_KEY_FLAG_EMOTE_FIRST` and `ui_window_mgr_window_hotkey` →
  `ui_window_mgr_flag_emote_hotkey`. These hunks are taken unchanged from upstream
  `996806c` ("Name ids, messages and functions for what their values are") on
  `origin/main`, so a later sync applies cleanly.
- `ui_window_mgr_render_menu` → `session_init_sprite_name_tables`, and the
  game-mode `PacketEnter.client_tick` (offset 14 of `0x0436`, always zero; the tick
  is the following `time` field) → `reserved`, matching the login-mode packet. Both
  renames are also committed on `main`.

Not changed: `Session.aid` and `session_is_own_aid` also have misleading names, since
they hold and test the pet AID. They are used widely, so renaming them is left for a
coordinated rename with upstream.

Validation: the release build had no warnings; the full `tests/build.bat`
(`build/leftovers-full-tests.log`) and the seven standalone suites
(`build/leftovers-<suite>.log`) passed; the packet dispatch digest is unchanged at
`679cf1c915cfe60f`; `git diff --check` passed. No live client/server smoke test
was run.

## Play-time warning, billing and Taren removal (2026-11-03)

Decisions: Patrick approved this removal on 2026-11-03 after a search for leftover
billing, play-time and regional login code. It continues the earlier decisions to
drop official billing and PC-cafe behavior: the unpaid-account redirect
([START-080](#account-payment-disconnect-redirect-start-080-removed)) and the
launcher PC-cafe mode (START-006).

What the client did before this removal:

- **Hourly play-time warning.** Every network poll in game mode compared
  `timeGetTime()` with `Session.loginTick` (set when the login server accepted the
  account). Once per elapsed hour, for every service type, it put a two-line balloon
  above the screen: "%d 시간이 경과했습니다." and "과도한 게임이용은 정상적인 일상생활에 지장을 줄
  수 있습니다." (`notice_play_time`, `g_playTimeHoursNoticed`).
- **Billing and remaining time.** On map entry with service type 0, the client sent
  `CZ_REQ_REMAINTIME` (`0x01c0`, mode message `0xa1`). `ZC_REPLY_REMAINTIME` (`0x01c1`)
  printed the account's billing plan and remaining time in chat (Korean PC-bang tiers,
  premium plans, mixed/flat/metered plans, China days/hours).
  `ZC_INFO_REMAINTIME` (`0x01c2`) printed flat/metered/PC-bang minutes left (Korea) or
  the voucher minutes (Indonesia). `ZC_PCBANG_EFFECT` (`0x027b`) showed the PC-cafe
  EXP/death/drop factors in the same balloon. All three lived in `src/game/billing.c`.
- **Login billing.** The login and character-list packets carry a 20-byte
  `LoginBillingInfo`. The client byte-swapped it into `LoginMode.billing`
  (`read_billing`, `billing_swap`). A hidden "Notice" button (message `0xd10`, ID
  `0x116`, never shown) on the character-select window asked mode message `0x2724` for
  `billing_text`, which formatted the plan text. `SC_BILLING_INFO` (`0x01b5`) showed a
  play-time/billing message box during login (`login_mode_on_play_time`).
- **Character-slot labels.** Slots beyond the normal count were labelled
  "Premium Service" or "Billing Service" (`draw_slot_kind`).
- **Taren refusal.** `AC_LOGIN_TAREN_REFUSE` (`0x0aca`) was the Chinese anti-addiction
  login refusal: it mapped codes 1-9 to messages `0xc7c`-`0xc82`, `0xc8e` and `0xf47`,
  disconnected, showed "text(code)" and returned to the login screen.

Removed: `notice_play_time`, `g_playTimeHoursNoticed`, `Session.loginTick` and its
write; `src/game/billing.c` with the three handlers, their dispatch cases, IDs,
declarations and test sinks; the `0x01c0` request and mode message `0xa1`; the stored
`LoginMode.billing`, `read_billing`, `billing_swap`, `billing_text`, mode message
`0x2724`, the hidden Notice button with its command case and IDs; `0x01b5`'s handler,
case and declaration; the slot labels; `on_refuse_taren` with its text table and
packet struct; and the length and name rows of `0x01b5`, `0x01c0`-`0x01c2`, `0x027b` and
`0x0aca` (the rows went with their code, as for earlier removals). Two includes only
the removed code used went too (`ui/chat/balloon.h` in `net_poll.c`, `winsock2.h` in
`char_list.c`).

Kept:

- `LoginBillingInfo` stays inside `PACKET_HC_ACCEPT_ENTER`, the neo-union header and
  `PACKET_HC_ACCEPT_ENTER2`, so every field offset and size assertion is unchanged; it is
  just no longer read.
- The slot counts `charListCounts[1]`-`[4]`/`[6]` and the slot layout are unchanged.
- `GameMode.notice_wnd`/`notice_shown_at` and their close logic in `balloon_frame.c`:
  server broadcasts (`broadcasts.c`) still use them.
- The `MODE_MSG_CHAT_ROOM_MSG 0xe` call on map entry for service type 0.

Server implications:

- The client no longer sends `0x01c0`. Hercules (`clif_parse_reqRemainTime`) and rAthena
  (`packet(0x01c0,2)`) both accepted and ignored it.
- `0x01c1`, `0x01c2` and `0x027b` now take the game dispatcher's unknown-packet path, and
  `0x01b5` and `0x0aca` the login loop's, which reset connection queues. Neither
  stock Hercules nor stock rAthena sends any of them.
- No server checkout was modified.

Tests changed: the packet-dispatch test adds `0x01c1`, `0x01c2` and `0x027b` to the
unknown-packet IDs and loses their sinks. The login-loop harness
(`test_login_pin_state`, which drives the real `login_dispatch.c`) now also checks
that `0x01b5` and `0x0aca` reach the unknown-packet path, and loses the play-time stub.

Validation:

- Release build passed without compiler or linker warnings:
  `build/play-time-billing-build.log`.
- Packet dispatch: all 1,179,800 cases passed, digest `f57257980132c31b` (before:
  `679cf1c915cfe60f`). `build/derive_round2_dispatch.py` changes exactly 18 cases of the
  regenerated baseline (`build/packet-dispatch-before-play-time-billing.bin`): 6 each for
  `0x01c1`, `0x01c2` and `0x027b`, the full-size non-null first-dispatcher calls in all
  six packet patterns. These were first-dispatcher handlers; each case formerly called
  its sink and now forwards to the second dispatcher's unknown-packet reset. Their
  direct second-dispatcher cases were already unknown, which the script asserts. The
  actual trace matches byte for byte (SHA-256 `96e76cc5...af7db77e9a1`):
  `build/play-time-billing-dispatch.log`, `build/play-time-billing-dispatch-expectation.log`.
- Packet table: 1,240 -> 1,234 length rows and 1,283 -> 1,277 name rows; only the six
  IDs above lost rows, all retained rows unchanged (`build/audit_round2_packet_table.py`,
  `build/play-time-billing-packet-table-audit.log`).
- Login loop: `build/play-time-billing-login_pin_state.log`.
- Full suite `tests\build.bat` passed in 5 min 57 s: `build/play-time-billing-full-tests.log`.
- `git diff --check` passed.

The pre-removal tree is in `build/before-play-time-billing-removal`. No live
client/server smoke test was run.

## OTP login removal (2026-11-03)

Decisions: Patrick approved this removal on 2026-11-03 after a leftover search. It
follows the [second password removal](#second-password-removal-2026-11-02): an
unsupported login challenge ends the attempt with a clear message instead of a prompt.
The MOTP homepage notice for result 900 stays as decided under
[START-079](#authentication-result-gnjoy-redirect-start-079-removed).

What the client did before this removal:

- **Login step `0x0acf`.** The login window always enters sub-mode `0x26`
  (`login_acf`), which connects to the account server and sends `0x0acf`
  (`CT_AUTH`/`CA_LOGIN_OTP`: user ID, hashed password, account class). The reply
  `0x0ae3` (or `0x0ad1`) carries a result code. 0 hands over a token and continues to
  the ordinary `0x0064` login (sub-mode 4). Codes 500-900 are failures with messages
  (500 "OTP information is unavailable", 600 wrong ID/password, 700 maintenance, 800
  delayed, 900 MOTP homepage).
- **OTP prompt.** Code 400 opened the OTP window (`UILoginOTPWnd`, window `0xb4`): a
  masked 8-digit code with a 60-second countdown. It also re-opened after a 500
  failure. Sub-mode `0x26` then waited up to 60 seconds and showed "MOTP Input time
  exceeded" (`0xd16`). The window sent the code as `CT_MOTP` (`0x0ad0`, mode messages
  `0x2728` and `0x27`, `LoginMode.otp_code`).
- **Mobile OTP.** `AC_REQ_MOBILE_OTP` (`0x09a2`) stored `LoginMode.mobile_otp_id`, entered
  sub-mode `0x18` and opened the same window. `login_send_mobile_otp` (`0x09a3`, mode
  message `0x17`) existed, but the window only ever sent `0x0ad0`, so nothing called it.

Removed: the OTP window with its class, factory case, window-manager slot, lookup,
close and forget entries and declarations; `src/login/otp.c`/`otp.h`
(`login_set_otp_code`, `login_send_mobile_otp`, `login_send_otp` and the window-only
`login_state_tick`); mode messages `0x17`, `0x27` and `0x2728`; sub-mode `0x18`; the
`LoginMode` fields `otp_code`, `mobile_otp_id`, `auth_waiting` and `auth_timed_out` with
the 60-second OTP wait; the 500 re-prompt; `PACKET_AC_REQ_MOBILE_OTP`; and the "Login OTP"
test (`tests/test_login_auth.c`, `build_login_auth.bat`).

New behavior:

- `0x09a2` disconnects, shows "OTP codes are not supported by this client. Please ask
  the server administrator to disable OTP." and returns to the login screen, the same
  way the PIN removal answers PIN states (`OTP_UNSUPPORTED` in `loginmode.h`).
- A `0x0ae3`/`0x0ad1` result 400 shows the same text through the ordinary result path
  ("text(400) [server text]"), disconnects and returns to the login screen.
- Result 500 shows its message and returns to the login screen instead of reopening a
  prompt.

Kept:

- Sub-mode `0x26` and `0x0acf` are the normal first login step, not an OTP feature:
  stock Hercules (`login_client_login_otp`) and rAthena (`logclif_otp_result`) answer
  every `0x0acf` with a fake token and login flag 0, after which the client sends
  `0x0064`. The 80-second "connection delayed" timeout for an unanswered `0x0acf` stays.
- The `0x09a2` length and name rows stay, because the packet is now handled (refused).
  The rows of the client-sent `0x09a3`/`0x0ad0`, now without senders, go with the
  other OTP rows in the next removal.
- Results 0 and 500-900 and the MOTP notice for 900 (START-079).

Server implications: OTP must be off. Stock Hercules never calls its `0x09a2` sender
(`login_client_login_mobile_otp_request`), and neither stock server asks for an OTP
code, so ordinary logins are unaffected. A server that requests OTP (`0x09a2`, or
result 400 to `0x0acf`) now gets a refused login with the message above. No server
checkout was modified.

Tests changed: the "Login OTP" test was removed with `otp.c`. The login-loop harness
(`test_login_pin_state`) checks that `0x09a2` of every length from 6 to 16 bytes
disconnects, shows the OTP message and returns to the login screen with no other state
change. The object-type test retires `UILoginOTPWnd` (reference index 400): 474 classes,
137 retired identities, 328 named, 308 windows, 2,733 comparisons, 207 accepted casts.

Validation:

- Release build passed without compiler or linker warnings: `build/otp-build.log`. The
  link map has no OTP window or `otp.c` symbol.
- Packet dispatch: all 1,179,800 cases passed with the digest unchanged at
  `f57257980132c31b`; the regenerated trace is byte-identical to the pre-removal copy
  `build/packet-dispatch-before-otp.bin`. The game dispatcher never handled an OTP packet,
  so no case changes: `build/otp-dispatch.log`.
- Login loop with the `0x09a2` refusal checks: `build/otp-login_pin_state.log`.
- Object types: 474 classes, 137 retired identities, 328 named, 308 windows, 2,733
  comparisons, 207 accepted casts (counts measured with a print-only copy of the test,
  then asserted).
- Full suite `tests\build.bat` passed in 6 min 0 s: `build/otp-full-tests.log`.
- `git diff --check` passed.

The pre-removal tree is in `build/before-otp-removal`. No live client/server smoke
test was run.

## Protection leftovers and dead packet rows removal (2026-11-03)

Decisions: Patrick approved this removal on 2026-11-03 after a leftover search. It
completes earlier protection removals: GameGuard/nProtect (START-007,
[GameGuard removal validation](#gameguard-removal-validation)), packet-key
obfuscation (START-014, [packet-key removal validation](#packet-key-removal-validation)),
the CAPTCHA registration tool (START-023) and the
[macro detector and captcha](#macro-detector-and-captcha-removal-2026-11-02).

What remained before this removal:

- **Encryption popup.** `SC_ACK_ENCRYPTION` (`0x01c7`) showed the native error box "No
  Packet Encryption !!!" in both the game dispatcher and the login loop. Packet
  encryption was removed earlier, and nothing sends the request `0x01c6`.
- **SRPACKETR2 lengths.** `rag_connection_get_packet_length` returned hard-coded lengths
  12 and 4 for `0x02bf`/`0x02c0` before consulting the packet table. Nothing sends or
  handles either packet. The foundation fix in that function (the short-length
  rejection in `rag_connection_get_packet`) is unchanged.
- **Ignored protection packets.** The second game dispatcher explicitly ignored GPK
  (`0x0851`/`0x0853`), SecretScan (`0x09cc`), EAC (`0x0a7b`), the Chinese play-time notice
  (`0x0ab1`) and the premium-campaign notice (`0x090d`). The login loop ignored SecretScan
  (`0x09c2`), Google OTP (`0x0a73`) and EAC (`0x0a7b`), and passed GPK `0x0851` to the
  empty `game_mode_ignore_packet`.
- **Rows with no code.** The packet table still named and sized protection, login and
  billing packets that no code sends or handles.

Removed:

- The `0x01c7` cases in both dispatchers and `HEADER_SC_ACK_ENCRYPTION`.
- The `0x02bf`/`0x02c0` special lengths. Both IDs now get the table's unknown-packet length.
- The eight ignore entries above, their IDs, the login case `0x0851`, and
  `game_mode_ignore_packet` with its file `src/game/packets_unused.c`.
- 49 IDs' packet-table rows (40 length rows, 49 name rows):
  - encryption `0x01c6`/`0x01c7`;
  - SRPACKET `0x02ae`/`0x02af` and SRPACKETR2 `0x02bf`/`0x02c0`;
  - HackShield `0x02a6`-`0x02a9`, `0x0449`; XIGNCODE `0x0b74`/`0x0b75`; CheatDefender
    `0x0abc`;
  - GPK `0x0851`-`0x0854`; SecretScan `0x09c2`/`0x09cc`; EAC `0x0a7b`/`0x0a7c`;
  - bot check `0x02e6`; login captcha `0x0a71`/`0x0a72`/`0x0a7a`;
  - SSO/Lyto `0x0825`/`0x0826`/`0x0a42`/`0x0a75`;
  - OTP `0x0821`-`0x0823`, `0x09a3`, `0x09c8`, `0x0a73`, `0x0a74`, `0x0ad0`;
  - cash password `0x02aa`-`0x02ac`, `0x0a8b`;
  - PC cafe `0x0277`/`0x0278`/`0x02bc`;
  - China fatigue/Taren `0x0982`, `0x0ab1`, `0x0aac`; premium campaign `0x090d`.
- The name search found five rows beyond the requested list, all without code: the
  encryption request `0x01c6`, GPK `0x0852`/`0x0854`, EAC `0x0a7c` and the Taren login
  request `0x0aac`. Every other numeric match in the sources for these IDs is a message,
  skill, effect or window ID.

Kept:

- The billing rows `0x01b5`, `0x01c0`-`0x01c2`, `0x027b` and Taren `0x0aca` were already
  removed with their code in the play-time/billing removal.
- `0x09a2` keeps its rows: the OTP removal answers it with a refusal.
- Rows whose names match but have code or a different meaning: `0x0ac9` and `0x0acd`
  (handled accept/refuse-login variants), the PIN rows (`0x08b7`, `0x08b9`, `0x0ae9`), and
  `0x0839`/`0x083e`/`0x0842`/`0x0843` (guild ban, SSO-refusal handling and GM recall
  commands, not SSO login).
- The China rows without a requested keyword (`0x0981`, `0x0987`, `0x09a4`, `0x0a3c`) are
  left for review.
- Banking, RoDEX, world-store, cell-attribute, ping-live and `0x0855` stay in the ignore
  lists. The other login-loop ignore entries are untouched.
- Also kept: the NPC cash-point shop (`0x0287`-`0x0289`), the rental-item timers
  (`0x0298`/`0x0299`), the MD5 login hash (`0x01be`/`0x01dc`) and the PIN-state handlers.

Server implications:

- These incoming packets now take the unknown-packet path, which resets connection
  queues: game `0x01c7`, `0x0851`, `0x0853`, `0x090d`, `0x09cc`, `0x0a7b`, `0x0ab1`;
  login `0x01c7`, `0x0851`, `0x09c2`, `0x0a73`, `0x0a7b`.
- None of them is sent by stock Hercules or stock rAthena (local checkouts). Their
  numeric matches there are only message-table IDs, and neither defines a GPK, SecretScan,
  EAC, Google-OTP, CHN play-time or premium-campaign sender.
- No server checkout was modified.

Tests changed: the packet-dispatch test adds the seven game IDs to the unknown-packet
IDs and drops its now unused `error_msg` stub. The login-loop harness checks the five
login IDs and loses two stubs. `test_packets` now expects the unknown length 4 for
`0x02bf`/`0x02c0`.

Validation:

- Release build passed without compiler or linker warnings:
  `build/protection-leftovers-build.log`; `game_mode_ignore_packet` is gone from the link map.
- Packet dispatch: all 1,179,800 cases passed, digest `d8b534ea02a5d31f` (before:
  `f57257980132c31b`). `build/derive_round2_dispatch.py` changes exactly 78 cases of the
  pre-removal trace (`build/packet-dispatch-before-protection-leftovers.bin`):
  - 6 for `0x01c7`, a first-dispatcher handler (formerly the `ErrorMsg` sink; its direct
    second-dispatcher cases were already unknown);
  - 12 each for `0x0851`, `0x0853`, `0x090d`, `0x09cc`, `0x0a7b` and `0x0ab1`, which the second
    dispatcher ignored both directly and when forwarded.
  Each now reaches the unknown-packet reset. The actual trace matches byte for byte
  (SHA-256 `23e1746c...ec95a5e51`): `build/protection-leftovers-dispatch.log`,
  `build/protection-leftovers-dispatch-expectation.log`.
- Packet table: 1,234 -> 1,194 length rows and 1,277 -> 1,228 name rows; only the 49 IDs
  above lost rows and every retained row is unchanged
  (`build/protection-leftovers-packet-table-audit.log`).
- Login loop: `build/protection-leftovers-login_pin_state.log`.
- Full suite `tests\build.bat` passed in 6 min 35 s (`test_packets` included):
  `build/protection-leftovers-full-tests.log`.
- The seven standalone suites passed: `build/protection-leftovers-<suite>.log`.
  `build_agency_server.bat` was not run (it needs the patched Hercules checkout).
- `git diff --check` passed.

The pre-removal tree is in `build/before-protection-leftovers-removal`. No live
client/server smoke test was run.

## Langtype 20 lock (2026-11-03)

Decision: Patrick approved fixing the client at langtype 20 on 2026-11-03, as the first
step towards UTF-8 text. The client must keep behaving as it does with Revel RO's
`clientinfo.xml` (`client-files/data/clientinfo.xml`, not modified), which has
`<servicetype>korea</servicetype>` on the root and `<langtype>20</langtype>` in its only
`<connection>`. Text behavior stays exactly as before: code page 0, the Arial/ANSI default
face and no charset tags.

How the service type was set before this change:

- `clientinfo_load` read the root options. `<servicetype>korea</servicetype>` set service
  type 0, so the default loading-screen count was Korea's 7 and `init_client_info_` took
  the Korean path: it selected connection 0 and formatted `g_accountServerAddr`
  (`address:port`, used by the `0x0acf` login step). Reading that connection set the
  service type to 20 from `<langtype>`.
- `init_language_` then chose language index 1, code page 0, the default language object
  and no charset tag. It also built the Traditional Chinese, Thai and Arabic objects on
  every start; the Thai one opened `..\ftwrk.lex`.
- `window_mgr_construct` runs in `client_init`, before `clientinfo.xml` is read, so it saw
  service type 0 and kept the Korean login-logo offset (`logo_y = -25`).
- After `clientinfo_load` every reader saw 20. Nothing else changed it: other connections
  are not in the file, and only service types 5 and 7 listed subconnections.

What changed:

- `clientinfo.c` no longer reads `<servicetype>` or `<langtype>`. Removed: the service-name
  table, the "ServiceType Error", "No ServiceType", "Unknown ServiceType" and "no language
  setting" errors, `g_serviceType`, `g_languageIndex`, `g_showNameA`/`g_showNameB`,
  `g_nameFontHeight` and `session_set_text_type` (a no-op under 20). `clientinfo_load` reads
  the root options and `etcinfo.txt`, selects connection 0, formats the account address
  and builds the one language object. The default loading-screen count is 7; the logo
  offset stays -25. `CLIENT_LANGTYPE` (20) remains only where the number was data: the
  shortcut registry value names (`SITEM<slot>20<char>`, `S_SKILL_USE_LEVEL<slot>20<char>`)
  and the server-select window's default before its saved value is read.
- Language objects: removed the Thai object and its lexicon loader (`thai_lexicon.c`), the
  Traditional Chinese Zhuyin object (`language_tc.c`, reading keys, the `VK_PROCESSKEY`
  hook in the message pump), the Arabic object, the per-object code page and the
  keyboard-layout code-page switch (every caller now uses `g_codePage`, still 0), the
  language-folder lookup (`language_language_file`, `language_set_folder`, folder strings;
  it returned the name unchanged under 20) and the Zhuyin key-stroke window
  (`UIKeyStrokeWnd`, window `0x55`, its slot, factory case and placement calls).
- Every other `g_serviceType` branch (about 160 reads in 80 files) collapsed to its
  langtype-20 path, and code that only those branches reached went with them:
  - Login: client type `0x24` (0 with server type `pk`); the hash login always sends
    `0x01dd`, so `0x01fa` (CA_LOGIN3), `0x027c` (CA_LOGIN4) and their packet-table rows
    went; character deletion always sends `0x0829` with the 6-byte e-mail field, so
    `0x01fb` (CH_DELETE_CHAR2) and its rows went; the plain `0x0064` login, so the service
    4 and 7 password encodings went (`login_password_encode` now only has the 32-byte
    `0x0acf` form); subconnection lists (sub-mode `0xd`, mode message `0x2726`,
    `select_client_info_`, `login_mode_load_sub_connections`); the 15-second login timeout;
    login mode always draws the login screen; login refusal 16 and disconnect reason 107
    show no message box, in-game ban 15 shows the generic message `0x718`, and delete
    refusal 0 uses message `0x12d`.
  - Display: no hourly game-grade badge (`renderer_draw_game_grade_badge`); no event-account
    rendering or second GM sprite set (`is_event_aid`, `pc_render_event_account`); no
    message-table mirror decoding (`text_decode`); siege emblems are shown; name balloons
    use `name (party)` and `guild [position]` with the 12-pixel regular face; the default
    face is Arial size+3 (Arial with charset 163 on Turkish Windows); English labels in the
    basic-info, skill and NPC-input windows; GM chat in `0xffff`; cash shops say
    "Point"/"P"; UI bitmaps always go through `ui_skin_path`, which now looks for the
    plain path and then `UI\`.
  - Gameplay text: `/where` also goes to the chat log, `/hi` friends text carries the (empty)
    charset tag, `MODE_MSG_USER_COUNT` always asks the server, map flags are cleared after
    each map-property packet, Slow Poison launches no effect, `/showname` only toggles its
    saved flag, scream/frost-joke lines come from `DC_scream.txt`/`BA_frostjoke.txt`.
  - Tables: the Korean body-palette names (`s_jobBodyNamesKorea`), Brazilian window names,
    Brazilian and Russian item-name orders and the service-specific chat-tab widths.
  - Unused afterwards: `embedded_fonts.c` and `t2embed_api.h` (the Rix fonts were loaded only
    for Korea), `code_page_from_lang`, `count_in_words`, `weather_pokjuk`,
    `session_is_ascii_name`, the Arabic modes of `chat_speaker_name` and the chat history,
    and the Japanese/Chinese candidate-window layouts.

Kept:

- `g_useCharsetTag` (never set: only service 10 and server type 3, which nothing assigns,
  turned it on), `Language.keyboard_lang` (read only by the charset tag) and the tag code;
  they go with the charset tags in the drawing step of the UTF-8 work.
- `Language.folder_lang`, because Turkish Windows selects face 6.
- `g_codePage` (0) and the font tables, including the Rix faces: their fonts are created
  by name and fall back to the GDI default, as before.
- Server-type branches (`g_serverType` 3 and 13 are never assigned; listed below).
- Selecting a connection still re-reads it, which appends its `<loading>` images again.

Behavior under langtype 20 that may be unexpected (all unchanged):

- Body palettes of second-class, transcendent and expanded jobs use first-class names
  (`s_jobBodyNamesOther`): a dyed Crusader loads `기사_남_N.pal`, Monk the Priest palette,
  Sage the Wizard one, Rogue the Assassin one, Alchemist the Blacksmith one, Bard and
  Dancer the Hunter one, Gunslinger and Ninja the Novice one.
- The IME candidate window draws only its frame: its layouts existed for language indices
  0, 2, 3 and 4.
- `/showname` changes nothing visible.
- With Revel's file the loading-screen list holds `loading00`-`06` twice (the Korean
  default and the connection's `<loading>` list).

Bugs found (not fixed):

- `homun_name` (`companion_packets.c`) passes the server-sent homunculus name to `sprintf` as
  the format; only services 10, 12 and 15 used `"%s"`.
- `session_get_monster_skill_info` (`talk_tables.c`) uses `monsterskillinfo.xml` text as a
  `sprintf` format into 8-byte buffers.
- `text_dc_select_font` indexes the 12-entry `faceCharsets` with faces 0x14-0x1d (Rix and
  SCDream); the language-0 guard that mapped them to entry 0 never applied under 20, so it
  reads `hexDigitFlags`.
- `g_serverType` is never 3 or 13, so its branches (repeated-chat check, guild and whisper
  checks, charset tag, Thai stage settings) are dead.
- `login_conn_entry_copy` copies the uninitialized `version` of `load_connections`' entry.

Tests changed:

- `test_language_input`: the Zhuyin mapping, trace and conversion-failure tests, the Thai
  lexicon and the folder lookup went; the IME test uses the default object and checks
  `WM_INPUTLANGCHANGE`; the per-object charset-tag test stays.
- `test_message_tables` and `test_item_restrictions`: no language-folder lookup, no UAE
  re-tagging.
- `test_balloon_text`: name cards only in the langtype-20 layout.
- `test_texture_quad`: the game-grade badge cases went.
- `test_sprite_atlas`: no Germany UV trim.
- `test_password_encode`: 4,096 `0x0acf` vectors (was 12,288 over three encodings).
- `test_text_parsing`: no Arabic speaker-name case. `test_item_names`: its archived
  reference block runs with service type 20.
- `test_embedded_fonts` removed. Stubs of `g_serviceType`, `text_decode`,
  `language_language_file` and the key-stroke placement went.
- Object types: `UIKeyStrokeWnd` (reference index 383) is retired: 473 classes, 138 retired
  identities, 327 named, 307 windows, 2,725 comparisons, 206 accepted casts (measured with
  a print-only copy of the test, then asserted).

Validation:

- Release build passed without compiler or linker warnings: `build/langtype20-build.log`.
- Packet dispatch: digest unchanged at `d8b534ea02a5d31f`; neither dispatcher was touched.
- Packet table: 1,194 -> 1,191 length rows and 1,228 -> 1,225 name rows; only `0x01fa`,
  `0x01fb` and `0x027c` lost rows, every retained row is unchanged
  (`build/langtype20-packet-table-audit.log`).
- Full suite `tests\build.bat` passed: `build/langtype20-full-tests.log`.
- The seven standalone suites passed: `build/langtype20-<suite>.log`.
- `git diff --check` passed.

The pre-change tree is in `build/before-langtype20-lock`. No live client/server smoke
test was run.

## UTF-8 text module (2026-11-03)

Decision: Patrick approved switching the client's text to UTF-8 on 2026-11-03, so that
players can type and read Korean, Russian and other scripts on any Windows locale. The
design follows the read-only survey of the same day: UTF-8 is the only in-memory text
encoding, text is converted to UTF-16 only at Win32 calls, and CP949 remains only for
resource names in the file layer. The work comes in steps after the
[langtype 20 lock](#langtype-20-lock-2026-11-03); this first one adds the shared helpers,
which nothing calls yet.

Why a module of its own: the Windows helpers the client uses do not handle UTF-8.
`CharNextExA`/`CharPrevExA` with code page 65001 step one byte, and
`IsDBCSLeadByteEx(65001, ...)` is always false, so every caret, wrap and truncation
would split characters.

Added `src/core/utf8.c` and `utf8.h`:

- A character is a well-formed sequence of 1-4 bytes (RFC 3629: no overlong forms,
  surrogates or values above U+10FFFF). Every other byte, including a truncated
  sequence's lead byte, is a one-byte character of its own. Stepping therefore never
  splits a valid character, never swallows the one after an invalid byte, and never reads
  past the end: bounded functions stop at `text + length`, `utf8_next` at the terminator.
- `utf8_decode`, `utf8_char_size`; `utf8_next` (NUL-terminated), `utf8_step` (bounded),
  `utf8_prev` (consistent with forward stepping); `utf8_valid`; `utf8_prefix`, the
  longest byte-limited prefix ending on a character boundary; `utf8_columns`, which
  counts East Asian wide and fullwidth characters as two columns (the CP949 rule counted
  every double-byte character as two); `utf8_encode`; `utf8_from_utf16`, which joins
  surrogate pairs, writes U+FFFD for unpaired surrogates and never splits a character at
  the capacity; and `utf16_feed`, which pairs surrogates arriving one `WM_CHAR` at a time.

Tests: `tests/test_utf8.c` (`tests/build_utf8.bat`, called from `tests\build.bat`) checks
1-4-byte characters (é, Ж, 한, €, 😀, U+10FFFF) and invalid bytes (stray continuation,
overlong, surrogate, above U+10FFFF, truncated at the end and before ASCII); agreement
with Windows' strict decoder (`MB_ERR_INVALID_CHARS`) for all one- and two-byte
sequences, three-byte sequences over every lead and second byte, and four-byte sequences
over every lead and second byte with eight tail values; an encode/decode round trip of
every code point; forward and backward boundaries agreeing on 20,000 generated strings
placed against a no-access page; maximal byte-limited prefixes; column widths;
validation (CP949 가 is not UTF-8); UTF-16 conversion at every capacity; and surrogate
pairing.

Validation:

- Release build passed without compiler or linker warnings: `build/utf8-module-build.log`.
- Full suite `tests\build.bat` passed: `build/utf8-module-full-tests.log`.
- `git diff --check` passed.

No live client/server smoke test was run.

## UTF-8 character stepping (2026-11-03)

Decision: the second step of the [UTF-8 switch](#utf-8-text-module-2026-11-03) approved by
Patrick on 2026-11-03. Text in memory is now UTF-8 (`g_codePage = CP_UTF8`), and every
place that stepped through text by code-page characters steps whole UTF-8 characters.
Drawing still decodes with the font's charset (the next step), and the window still
delivers ANSI `WM_CHAR` text (the input step).

What changed:

- `g_codePage` is `CP_UTF8`. Its remaining users convert between it and UTF-8: the local
  account and shop settings (now an identity conversion that still rejects invalid UTF-8)
  and the IME composition, result and candidate text, which now arrive as UTF-8.
- Stepping: the editor caret (Left/Right, Backspace and Delete through
  `language_caret_back`/`_forward`), edit scrolling (`ui_edit_ctrl_sync_from_ime_with`), the
  multiline caret column (`chars_to_x`), line breaking (`text_lines.c`, `window_lines.c`,
  `window_coloured_lines.c`, `window_wrap.c`), per-character drawing and measuring
  (`window_text.c`, `window_text_styles.c`), item-name runs, byte-wise shortening
  (`ui_window_shorten_text`), markup lines (`line_info.c`), colour-code scanning
  (`text_format.c`) and the underscore replacement in `session_remove_underbar` use
  `utf8_step`/`utf8_next`/`utf8_prev`. `language_char_next` and the per-call
  `CharNextExA`/`CharPrevExA` copies are gone, and so are the `code_page` parameters of
  the colour-code helpers.
- Byte limits stay, but truncation lands on a character boundary: edit fields
  (`ui_edit_ctrl_truncate_text`, still padding with spaces to the limit as before) and the
  quest summary use `utf8_prefix`; `text_prefix_length` went.
- `game_mode_format_string_msg` counts `%s` stepping UTF-8 characters instead of skipping
  a byte after every `IsDBCSLeadByte` byte. The old loop used the system ANSI code page
  and could step over the terminator when a lead byte ended the format.
- `_mbsstr` (40 calls in 17 files) became `strstr`: the needles are ASCII markup tags,
  `#`, `<`, `>`, `]` and player names, and UTF-8 never places an ASCII byte inside a
  character.
- Name columns (`text_within_columns`, used by the rename window) count with
  `utf8_columns`. `_setmbcp(949)`, which set the CRT's multibyte code page for the whole
  process on first use, went with `_mbsbtype`.
- Chat tokens are lowercased with the new ASCII-only `text_to_lower_ascii` (`/on`/`/off`
  arguments, talk-command names, emotion words); `text_to_lower` keeps its CP949-aware
  form for file and resource names (illustrations, `etcinfo.txt` weather maps and
  effects).
- EZ2STR effect files, sounds and names convert from UTF-8 to `RESOURCE_NAME_CODE_PAGE`
  (949) instead of `g_codePage`. Under code page 0 they used the ANSI code page, which
  broke the Korean names on non-Korean Windows.

Kept:

- The charset-tag code and `g_useCharsetTag`, still never set (removed with drawing).
- CP949 resource names and data: the message table, Lua data and XML still hold CP949
  text, which now steps as one-byte characters until the data is converted.

Tests changed: CP949 assumptions became UTF-8 expectations with 2-, 3- and 4-byte
characters and invalid bytes, using `tests/utf8_text.h`:

- `test_edit_text`: limits on character boundaries (Korean, Cyrillic, emoji, invalid
  bytes) and the space padding.
- `test_language_input`: caret stepping, Backspace and Delete over mixed characters,
  IME candidates as UTF-8.
- `test_text_format`: colour codes after every two-byte prefix are found (replacing the
  455,175 SDK code-page pairs), UTF-8 and invalid text around colour codes.
- `test_byte_lines`, `test_plain_lines`, `test_multiline_lines`, `test_multiline_draw`,
  `test_shorten_text`, `test_line_layout`: UTF-8 widths and boundaries; the CP949
  `language_char_next` stubs went.

Validation:

- Release build passed without compiler or linker warnings: `build/utf8-stepping-build.log`.
- Full suite `tests\build.bat` passed: `build/utf8-stepping-full-tests.log`.
- `git diff --check` passed.

The pre-change tree is in `build/before-utf8-stepping`. No live client/server smoke test
was run.

## UTF-8 drawing (2026-11-03)

Decision: the third step of the [UTF-8 switch](#utf-8-text-module-2026-11-03), approved by
Patrick on 2026-11-03. Text was drawn with `TextOutW` and measured with
`GetTextExtentPoint32W`, but the bytes were decoded with the code page of the font's
charset (`get_code_page_from_charset`), not `g_codePage`. The langtype-20 default face is
Arial with `ANSI_CHARSET`, so every string was decoded as Windows-1252.

What changed:

- `text_dc_text_out` and the measurement in `ctextdc.c` decode UTF-8; the charset now only
  selects the font. `get_code_page_from_charset` and `platform/charset.c` went.
- Ellipsis and fitting (`ui_window_ellipsize_text`, `ui_window_fit_text`,
  `ui_window_fit_plain_text`) drop whole UTF-8 characters from the end with `utf8_prev`
  instead of converting through `CP_THREAD_ACP` and dropping two UTF-16 units whenever both
  had bit 0x80 set (an imitation of a CP949 double-byte character, which also cut Hangul
  pairs).
- Navigation search and the navigation text lines (`navi_search.c`, `navi_info.c`) and
  the guild job-name sort (`guild_containers.c`) convert from `CP_UTF8` instead of the
  ANSI code page.
- Charset tags are gone. A tag (`|` and two hex digits) switched the font charset inside
  a string; only service type 10 and server type 3 turned them on, and nothing assigns
  either under langtype 20, so `g_useCharsetTag` was always 0. Removed: `g_useCharsetTag`,
  `language_charset_tag` with `Language.charset_tag` and `keyboard_lang`,
  `get_charset_from_lang`, `text_find_charset_tag`, the tagged message-table copies in
  `msg_str`, the tag prefixes of chat, battleground chat, whispers, guild leave/ban
  reasons and notices, `/hi`, skill descriptions, edit fields, item-collection lines and
  expiry text, the tag skipping in item names and window text, the line-wrap tag carry
  (`carry_colour`), the login-mode save/restore of the flag (`LoginMode.savedFlag`) and the
  run splitter in `textdc.c`. A font fallback per script (phase B) will need its own
  splitter by Unicode range, so the tag splitter was not kept. `WM_INPUTLANGCHANGE` is
  still consumed, as before.
- The EZ2STR resource names were already pinned to CP949 in the stepping step, where
  `g_codePage` changed.

After this step Latin, Cyrillic and Greek text decode and draw correctly with Arial. Korean
and other scripts that Arial lacks still draw as boxes until the font fallback.

Tests changed:

- `test_text_render`: one UTF-8 run in the context's charset (`|xx` is ordinary text),
  Korean, Cyrillic, emoji surrogate pairs, invalid bytes as U+FFFD, guarded endings.
- `test_fit_plain_text`, `test_text_consumers` (ellipsis), `test_readable_links`
  (fitting): whole UTF-8 characters with 2-, 3- and 4-byte characters and invalid bytes;
  the CP949 conversion stubs and their failure cases went with the conversions.
- `test_text_format`, `test_message_tables`, `test_language_input`, `test_byte_lines`,
  `test_item_names`, `test_balloon_text`, `test_multiline_draw`: the charset-tag cases
  and stubs went; tags are ordinary text.

Validation:

- Release build passed without compiler or linker warnings: `build/utf8-drawing-build.log`.
- Full suite `tests\build.bat` passed: `build/utf8-drawing-full-tests.log`.
- `git diff --check` passed.

The pre-change tree is in `build/before-utf8-drawing`. No live client/server smoke test
was run.

## UTF-8 input (2026-11-03)

Decision: the fourth step of the [UTF-8 switch](#utf-8-text-module-2026-11-03), approved by
Patrick on 2026-11-03, and the end of phase A. The main window was an ANSI window
(`RegisterClassA`, `CreateWindowExA`, `PeekMessageA`, `DispatchMessageA`,
`DefWindowProcA`), so `WM_CHAR` arrived in the system ANSI code page: typed text depended
on the Windows locale, and a character outside it could not be typed at all. The
clipboard used `CF_TEXT` the same way.

What changed:

- The window class, `CreateWindowExW`, the message pump and the default procedure are
  Unicode. `WM_CHAR` delivers UTF-16 units; `language_on_char` encodes them as UTF-8 and
  pairs surrogates that arrive in two messages (`utf16_feed`, pending unit in
  `Language.pending_unit`). An unpaired surrogate becomes U+FFFD; Backspace, Tab, Enter,
  Escape and other control characters drop a pending high surrogate. Before, the four
  bytes of the `WM_CHAR` integer were inserted as they came.
- Clipboard: paste reads `CF_UNICODETEXT` (bounded by `GlobalSize`) and converts it to
  UTF-8; copy, including the masked `*` copy, writes `CF_UNICODETEXT` through the new
  `language_copy_text`, which the character-code copies of the player menu and the
  whisper window now use as well (they put ASCII codes on the clipboard as `CF_TEXT`).
- IME: composition and result strings are read at their full length
  (`ImmGetCompositionStringW` sized first) instead of failing above 256 UTF-16 units,
  and the result is inserted whole. The composition and candidate buffers hold 256 units
  as UTF-8 (`LANGUAGE_TEXT_MAX`, 769 bytes) and truncate on a character boundary instead
  of dropping the text when it does not fit. `GCS_CURSORPOS` is read with the W call and
  converted from UTF-16 units to a byte offset in the composition, clamped to it and
  never inside a surrogate pair.
- The keyboard-layout code-page switch was already removed with the langtype lock, and
  the keyboard layout itself with the charset tags.
- Remaining A calls that receive text from the window or the system: none. `MessageBoxA`
  (outgoing text), `LoadIconA`/`LoadCursorA` (resource IDs), `SetWindowLongA(GWL_STYLE)`
  and the crash dialog's ASCII report copy do not carry window text.

After phase A, Latin, Cyrillic and Greek text types, pastes, displays and copies
correctly on any Windows locale. Korean types and is stored as UTF-8 but draws as boxes
until the font fallback.

Tests changed: `test_language_input` types Korean, Cyrillic and accented letters as
UTF-16 units, pairs a surrogate emoji, turns lone surrogates into U+FFFD, reads a
600-unit composition (truncated at 256 characters) and result (inserted whole), converts
cursor positions to byte offsets around a surrogate pair, and checks `CF_UNICODETEXT` for
Korean, emoji, invalid bytes and masked text.

Validation:

- Release build passed without compiler or linker warnings: `build/utf8-input-build.log`.
- Full suite `tests\build.bat` passed: `build/utf8-input-full-tests.log`.
- The seven standalone suites passed: `build/utf8-input-<suite>.log`.
- `git diff --check` passed.

The pre-change tree is in `build/before-utf8-input`. No live client/server smoke test was
run; typing with real IMEs (Korean, Japanese, Chinese) on Korean, Russian and Western
Windows is still to be checked by hand.

## UTF-8 data conversion and server patch (2026-11-03)

Patrick approved converting the client data and preparing a server patch for the
UTF-8 client, without touching `client-files` or the HampterHercules checkouts.

- `tools/utf8-data` converts the display text of the data the purist client loads
  from CP949 to UTF-8. It finds each file the way the client does (`data.grf` before
  loose files, since Revel's `clientinfo.xml` has no `<readfolder/>`; `.lub` before
  `.lua`; `System\` from disk), classifies every Lua string by field path and keeps
  names that become file paths in CP949. It also repairs `msgstringtable.txt`,
  `jobName.lua` and `stateiconinfo.lua`, which an editor had saved as UTF-8 with
  U+FFFD; 16 `stateiconinfo.lua` strings could not be recovered. The output went to
  `client-files-utf8`, next to `client-files`; see the tool's README for deployment.
  Deploy it only together with a UTF-8 client.
- [`docs/server/hercules-utf8-text.patch`](server/utf8-text.md) targets the
  HampterDev/Hercules fork (`73334c4`, also `feat/tournament`). It stops the rename
  trim from turning UTF-8 continuation bytes into spaces, validates names as UTF-8,
  truncates titles and messages on character boundaries (fixing a one-byte mail-body
  overflow), sets `latin1` on every database connection and switches seven name
  columns to `latin1_bin`. Character and party names become case-sensitive.

Validation: the tool's 41 unit tests and `validate.py` over the 24 output files
passed. The server patch was built with MSVC and GCC, its helpers passed 162 checks
under AddressSanitizer, and its upgrade and connection charset were checked on a
throwaway MariaDB 10.5 instance. Neither was tried with a live client or server.

## Font fallback by script (2026-11-03)

Decision: the first phase-B step of the [UTF-8 switch](#utf-8-text-module-2026-11-03),
approved on 2026-11-03. After phase A every string was decoded as UTF-8 but drawn in one
font. The langtype-20 default face is Arial (`ANSI_CHARSET`), which has no Hangul, and GDI
links Arial to no other font, so Hangul, Han, kana and most CP949 symbols drew as boxes.
The Korean client got them from Korean Windows' font association, which drew the
double-byte characters of an ANSI font in Gulim.

What changed:

- `render/text_runs.c` splits UTF-8 text into runs by Unicode range: Hangul (jamo,
  compatibility and halfwidth jamo, syllables), Han (radicals, CJK symbols and
  punctuation, enclosed and compatibility CJK, ideographs including the supplementary
  planes, fullwidth forms), kana, Thai, symbols (U+2100-2BFF, letterlike through
  dingbats and arrows, plus ‥ and ※) and emoji (U+1F000-1FAFF). Spaces, combining marks,
  zero-width joiners and variation selectors join the run around them. Invalid bytes and
  all other characters, Latin, Cyrillic, Greek and the rest of General Punctuation
  included, stay in the selected face.
- `text_dc_text_extent` and `text_dc_text_out` measure and draw run by run. When the
  selected face lacks a run's script, the run is drawn in the script's first installed
  Windows font; no font is bundled. Whether a face has a script is checked once per font
  with `GetGlyphIndicesW` on a sample character (한, 中, あ, ก, ★; emoji always fall back).
  Fallbacks, each tried under its English and its local name:
  - Hangul: Gulim, then Malgun Gothic.
  - Han: Gulim, MS Gothic, SimSun, Microsoft YaHei, Malgun Gothic.
  - Kana: MS PGothic, MS Gothic, Meiryo, Yu Gothic, Gulim, Malgun Gothic.
  - Thai: Tahoma, Leelawadee UI.
  - Symbols: Gulim, Malgun Gothic, Segoe UI Symbol.
  - Emoji: Segoe UI Emoji, Segoe UI Symbol.

  A fallback font has the selected size, weight and italic and `DEFAULT_CHARSET`, and is
  cached with the other fonts; the installed face is looked up once per script. A
  fallback run is moved down by the difference of the two ascents so that it sits on the
  selected face's baseline, and each run starts where the measured width of the runs
  before it ends. With no fallback installed, the run stays in the selected face.
- Italic widths, the sum of each character's A and B widths, now step per character: a
  surrogate pair is measured whole with `GetTextExtentPoint32W` instead of as two halves
  with `GetCharABCWidthsW`.
- Font calls are W calls (`CreateFontW`, `CreateFontIndirectW`, `EnumFontFamiliesExW`,
  `AddFontResourceExW`, `RemoveFontResourceExW`), and the face tables hold UTF-16 names.
  The localized names 굴림, ＭＳ Ｐゴシック, 宋体 and 細明體 were CP949, CP932, CP936 and
  CP950 bytes written as CP949 literals, so each matched only where the ANSI code page was
  its own. Installed faces are still found by enumerating the face's charset and
  comparing names without case. The SCDream family name from GDI+ goes into the
  `LOGFONTW` as it is instead of through `CP_THREAD_ACP`; a name longer than 31
  characters now leaves the face name empty instead of truncating it.
- The extent cache is keyed by the font handle, which already implies the charset that
  was part of the key. `text_dc_measure_in_charset` became `text_dc_measure_font`.
- `text_dc_select_font` no longer reads past the 12-entry `faceCharsets` (the bug listed
  under the [langtype lock](#langtype-20-lock-2026-11-03)). Faces past the table keep the
  charset that mattered: SCDream (0x1d) read 0x00010101 from `hexDigitFlags`, which
  `CreateFont` and `LOGFONT` truncate to `DEFAULT_CHARSET`, and now gets
  `DEFAULT_CHARSET`; faces 12-0x1c get `ANSI_CHARSET`. Faces 12-0x13 read 0, and the EOT
  faces 0x14-0x1c read 0, 0x101, 0x01010100 or 0x01010101 but put a fixed charset 1 in
  their `LOGFONT`, so their value was only part of the cache keys. `main` has the same
  read: its guard sends faces 0x14 and up to entry 0 only for language index 0, so faces
  12-0x13 always, and faces 0x14-0x1d for every other language index, read
  `hexDigitFlags`.

Kept:

- Coverage is decided per script from one sample, not per character. A symbol that Gulim
  lacks but Segoe UI Symbol has still draws as a box, and General Punctuation, currency
  signs and Latin-1 stay in Arial, although the Korean client on Korean Windows drew the
  CP949 ones in Gulim.
- GDI draws Segoe UI Emoji as monochrome outlines; there is no colour emoji.
- The fallback uses the selected size, which for the default face is Arial's size, three
  points above the requested one, as font association did.
- On Windows without Gulim (it ships only with Korean installs or the optional Korean
  supplemental fonts), Hangul draws in Malgun Gothic, which looks different from the
  Korean client.

Tests changed:

- New `test_text_runs`: the script table, spaces, marks and variation selectors joining
  runs, invalid bytes, guarded endings.
- `test_text_render`: draws and measures run by run against a model of the font choice
  (covered scripts, missing fallbacks, missing ascents), hand-computed positions and
  baselines, one face lookup per call, an italic surrogate pair measured whole; the 3000
  generated cases add symbols, kana, Han, Thai, spaced Hangul and combining marks.
- `test_font_cache`: W calls, script fallbacks (Gulim before Malgun Gothic, local names,
  matching without case, one lookup per script, nothing installed), glyph coverage and
  ascents cached per font with the DC's font restored, retry after allocation, DC,
  creation, selection, glyph and metric failures, and a too-long SCDream family name.
- `test_font_cache_system`: with real GDI, Arial lacks Hangul, the Hangul fallback is
  Gulim or Malgun Gothic and has it, and GDI objects stay balanced over 32 cycles.

Validation:

- Release build passed without compiler or linker warnings: `build/font-fallback-build.log`.
- Full suite `tests\build.bat` passed: `build/font-fallback-full-tests.log`.
- The seven standalone suites passed: `build/font-fallback-<suite>.log`.
- `git diff --check` passed.

The pre-change tree is in `build/before-font-fallback`. No live client was run; how
mixed Arial and Gulim or Malgun Gothic text looks in the game is still to be checked by
hand.

## UTF-8 display literals (2026-11-03)

Decision: the second phase-B step of the [UTF-8 switch](#utf-8-text-module-2026-11-03),
approved on 2026-11-03. The build compiles narrow literals to CP949
(`/execution-charset:.949`), so Korean text in the source stayed CP949 while everything
around it became UTF-8: the 메시지 title of every message box drew as mojibake, Korean
slash commands and the 그라비티 name check never matched typed text, and the Korean blue
broadcast tag never matched.

What changed:

- Literals that the client draws, compares with typed or received text, sends, or keeps
  as text are UTF-8 escapes, each followed by a comment with the text. The message-box
  title 메시지 is `UI_MSGBOX_TITLE` (`ui/window_messages.h`, 87 uses in 65 files); the
  one 메세지 title (login refusal 800) keeps its spelling.
  - Commands and checks: the Korean slash commands (74 in `session_talk.c`, 32 in
    `session_talk_type.c`), 그라비티 in character creation and renaming, the ideographic
    space in guild names, the 블루 broadcast tag.
  - Chat and message-box text: the Devotion overflow notice, the Hangame purchase
    refusal, the NPC shop's 10-item refusal, the chat room's enter and leave markers,
    the chat-prohibition log results and the monster skill info errors.
  - Window text: pet intimacy and hunger, the pet item lines, the default mercenary
    (용병) and character (잠이들었던거야) names, the cart-release menu item, the 닫기 and
    dye-removal tooltips, the sub-storage caption, the hotkey guide's ← ↑ → ↓ ▤ keys,
    the achievement ▶ and ● marks, the quest ▲ and ▼ level marks and the prohibition
    list's record line.
  - Text that is never drawn: the JSON loader's errors (never read) and the TraceAI log
    time stamp.
- Code that counted CP949 bytes:
  - The broadcast-prefix table held byte lengths (3, 5, 7, 0xb for /ㅠ, /ㅣㅠ, /파란말,
    /지역파란말); they now come from `sizeof`. `/차단`, `/해제`, `/gc` and `/guildchat`
    took their argument at fixed offsets (`text + 6`, `+ 4`, `+ 0xb`) and now take it
    after the command token and one blank.
  - The pet item lines were assigned with CP949 lengths (7, 9, 0xb, 0x11, 0x13) and are
    now assigned whole.
  - 블루 was compared within the four-byte ASCII tag; the six UTF-8 bytes are now matched
    as their own prefix.
  - The guild-name check compared two bytes of the name against "　", using the third
    byte of the unrelated second argument (always 0) as the terminator, and read one
    byte before a one-byte name. It now compares the three UTF-8 bytes at both ends.
- Format strings with UTF-8 text are arrays (TraceAI stamp, prohibition record, quest
  hunt levels), and text with no conversions is copied with `strcpy`. MSVC checks a
  literal format in the execution charset and warns (C4819) on UTF-8 bytes.

Kept in CP949, because they name files in `data.grf` or on disk, which keep CP949 names
(`tools/utf8-data` keeps them too): sprite, action, texture, model, sound and effect
paths and the fragments they are built from (이팩트, 몬스터, 유저인터페이스, 몸통,
머리통, 남/여, the shield, cart and mount names, the `.str` and wave names), the storage
tab bitmaps, the 사과 fallback item resource and the 총알이빨토니 bitmap magic.

Left for the next step, because they go to ANSI Win32 calls: the `MessageBoxA` and
`error_msg` texts (`navi_lua_tables.c`, `csurface.c`, `attr.c`, the model loaders,
`rma.c`, `session_lua.c`, `world_load.c`), the chat macro defaults (registry) and
사크라이 (a registry key name).

Tests changed:

- New `test_talk_type`: the UTF-8 broadcast prefixes start the message after their
  bytes and blanks, `/차단`, `/해제`, `/gc` and `/guildchat` take the text after the
  command, `/길드` takes its argument, and the CP949 spelling is no longer a command.
- `test_broadcast_text`: 블루 in UTF-8 is the blue tag; its CP949 bytes and a cut UTF-8
  tag are ordinary text.
- `test_navigation_events`: the message-box title is `UI_MSGBOX_TITLE`.

Validation:

- Release build passed without compiler or linker warnings: `build/utf8-literals-build.log`.
- Full suite `tests\build.bat` passed: `build/utf8-literals-full-tests.log`.
- The seven standalone suites passed: `build/utf8-literals-<suite>.log`.
- `git diff --check` passed.

The pre-change tree is in `build/before-utf8-literals`. No live client was run.

## UTF-8 message boxes and registry (2026-11-03)

Decision: the third phase-B step of the [UTF-8 switch](#utf-8-text-module-2026-11-03),
approved on 2026-11-03. The client called `MessageBoxA` and the ANSI registry functions,
which take text in the Windows code page. With UTF-8 strings every message box with
non-ASCII text showed mojibake, and registry strings were converted through the code
page: the registry stores UTF-16, so a chat macro the setup tool saved on Korean
Windows read back as `?` on another locale, and the client's UTF-8 strings were stored
as mojibake.

What changed:

- New `platform/win_utf8.c` with drop-in replacements that keep the ANSI signatures:
  `win_utf8_message_box`, `win_utf8_reg_open_key`, `win_utf8_reg_create_key`,
  `win_utf8_reg_query_value` and `win_utf8_reg_set_value`. They convert UTF-8 to UTF-16
  and call the W functions. Text that is not valid UTF-8 is decoded as CP949, so an
  error that names a resource file still shows its name. String values (`REG_SZ`,
  `REG_EXPAND_SZ`) are read as UTF-16 and returned as UTF-8; one that does not fit
  gives `ERROR_MORE_DATA` with the size it needs, as the A call does. A string written
  is the text up to its first NUL within the given size. Other value types pass
  through.
- All 32 `MessageBoxA` calls, `error_msg` among them, and all 45 registry calls in 28
  files use them: chat macros, the saved login ID, the shortcut names and their key
  path (which holds the server name), window rectangles, the selected server and slot,
  the name-balloon position and the display GUID. No ANSI message box or registry call
  is left.
- The Korean literals that went to these calls are UTF-8 escapes: the navigation
  tables' Lua read errors, the GFC, GAT attribute, model and action errors, the world
  loading errors, the transparent-item error and its 알림 caption, the chat macro
  emote defaults (/기쁨 to /돈) and 사크라이, part of the shortcut key path and compared
  with the server name. The formats among them are arrays, as in the
  [previous step](#utf-8-display-literals-2026-11-03).

Kept:

- The other ANSI calls carry ASCII or file names: the file APIs (`CreateFileA`,
  `FindFirstFileA`, `CreateDirectoryA`, `MoveFileExA` and the rest, under the client
  folder), `GetEnvironmentVariableA`, `OutputDebugStringA`, the performance overlay's
  `CreateFontA`/`TextOutA`, `renderer_text_out` (draws only the `XXXXXXX-…` placeholder),
  the crash dialog (ASCII report) and `ShellExecuteA` for http/https links.
- Values written before this change read back the same where the old writer's code page
  could hold them; the registry stored them as UTF-16 either way.

Tests changed:

- New `test_win_utf8` (the W calls mocked): message box text and caption as UTF-16,
  emoji as a surrogate pair, a CP949 resource name, empty and missing captions, the ANSI
  fallback when memory runs out; key paths with Korean server names; string values read
  as UTF-8 (sizes, `ERROR_MORE_DATA` leaving the buffer alone, size queries, values
  without a terminator or with an embedded NUL, empty values, missing values leaving the
  type alone) and written up to the first NUL, from UTF-8 or CP949; DWORD and binary
  values passing through; allocation failures.
- Tests that replaced `MessageBoxA` or the registry calls with stand-ins now replace the
  wrappers (`test_achievement_owner`, `test_chat_layout_save`, `test_com_lifecycle`,
  `test_headgear_names`, `test_item_definitions`, `test_local_character`,
  `test_lua_boundaries`, `test_lua_calls`, `test_lua_data_managers`, `test_pet_accessory`,
  `test_session_text`, `test_startup_settings`, `test_town_info`,
  `extract_item_restrictions.py`). `test_session_text`'s registry stand-in returns /기쁨
  in UTF-8, as the wrapper does, and `test_rma_owner` expects the duplicate-action error
  in UTF-8.
- Tests that build those sources without a stand-in link `win_utf8.c`
  (`test_message_tables`, `test_ai_context`, `test_navigation_objects`,
  `test_item_definitions` for `item_loader.c`, `test_local_account`,
  `test_surface_lifecycle`, `test_renderer_storage`, `test_game_mode_layout`).

Validation:

- Release build passed without compiler or linker warnings: `build/win-utf8-build.log`.
- Full suite `tests\build.bat` passed: `build/win-utf8-full-tests.log`.
- The seven standalone suites passed: `build/win-utf8-<suite>.log`.
- `git diff --check` passed.

The pre-change tree is in `build/before-win-utf8`. No live client was run; the message
boxes and saved settings are still to be checked by hand on a non-Korean Windows.

## CP949 lookups of UTF-8 file names (2026-11-03)

Decision: the fourth phase-B step of the [UTF-8 switch](#utf-8-text-module-2026-11-03),
approved on 2026-11-03. Resource names stay CP949: the GRF entries and the loose files
are named in CP949, the client's path literals are CP949, and `tools/utf8-data` keeps
every field that becomes a file path CP949 (sprite, texture, sound and illustration
names, `ResNameTable.txt`, `Num2CardIllustNameTable.txt`). A name that reaches the file
layer in UTF-8, for example an illustration or sound name a server script sends in
UTF-8, matched nothing.

What changed:

- `resource_name_to_cp949` (`fileio/resource_path.c`) gives the CP949 spelling of a
  UTF-8 name: only for a name with non-ASCII characters that is valid UTF-8 and whose
  characters CP949 all holds (no best-fit substitutes).
- `file_open` (reading) and `file_make_file_name` look a name up as it is first and,
  on a miss, in its CP949 spelling; the file's name is then the CP949 one. The name as
  it is goes first because short CP949 names can be valid UTF-8 as well (킴 is C5 B4,
  also U+0174), the same reason `tools/utf8-data` decides encodings per file. A CP949
  name never takes the second lookup unless it misses and happens to be valid,
  convertible UTF-8; then the lookup is of different bytes and misses too.
- `res_mgr_get` (`fileio/res_get.c`): a UTF-8 name without an alias of its own uses
  the alias of its CP949 spelling, since `ResNameTable.txt` keys are CP949. The cache
  key stays the path asked for, and its file is found through `file_open`.

Kept:

- Files the client writes keep the name they are given, and the file APIs stay ANSI
  (`CreateFileA` and the rest). The names written are ASCII (`Chat\Chat.txt`,
  `TraceAI.txt`, `savedata\…`, settings paths built from the server address and IDs);
  a client folder whose path the ANSI code page cannot hold still fails, as before.
  On non-Korean Windows the ANSI calls cannot open loose files with Korean names; the
  GRF lookup is by bytes and is unaffected.
- `resource_normalize_name` still lowercases every byte from A to Z, CP949 trail bytes
  included; GRF lookups lowercase both sides the same way.

Tests changed:

- `test_file_io`: `resource_name_to_cp949` for Hangul and Cyrillic, ASCII, CP949, 킴's
  bytes, an emoji, a cut sequence and buffer sizes; `file_open` and
  `file_make_file_name` look up a UTF-8 name and then its CP949 spelling, a CP949 name
  once, names without a CP949 spelling once, unresolved names in both forms, and keep
  the name that was found.
- `test_resource_cache`: a UTF-8 name uses its CP949 spelling's alias, keeps an alias of
  its own, and without either stays the path asked for.
- The 14 test builds that compile `file.c`, directly or through
  `renderer_texture_owner_test.h`, also compile `resource_path.c`.

Validation:

- Release build passed without compiler or linker warnings: `build/file-names-build.log`.
- Full suite `tests\build.bat` passed: `build/file-names-full-tests.log`.
- The seven standalone suites passed: `build/file-names-<suite>.log`.
- `git diff --check` passed.

The pre-change tree is in `build/before-file-names`. No live client was run.

## UTF-8 splits in navigation lines and packet fields (2026-11-03)

Decision: the fifth and last phase-B step of the
[UTF-8 switch](#utf-8-text-module-2026-11-03), approved on 2026-11-03. Three places cut
text at a fixed count that assumed CP949 or UTF-16 units and could split a character.

What changed:

- Navigation names (`navi_manager_add_text_lines`) are split by the new
  `navi/navi_text_lines.c`: lines of at most 11 UTF-16 units, as before, but a line that
  would end between the two halves of a surrogate pair ends before the pair, which
  starts the next line. The name is cut to its first 100 units by whole characters; the
  old conversion passed the byte count as the capacity of its 100-unit buffer and wrote
  past it for names longer than 100 units. The line count stays `len / 12 + 1`.
- Guild and whisper packet fields are copied with the new `utf8_copy_prefix`
  (`core/utf8.c`), a whole-character `strncpy` that always terminates and zero-fills the
  field: guild position titles, the disband key, the leave and expel reasons, and the
  notice subject and text (`msg_guild.c`); the whisper line kept for the repeat check and
  the whisper text (`msg_chat.c`). A whisper longer than the packet holds is still
  refused: the check now measures the line itself, because the whole-character copy can
  be shorter than the old byte cut. The guild-name fields were already length-checked
  first and are unchanged.
- The renewed-quest condition names (`renew_quest_text.c`) dropped their last two
  bytes, one Korean character or two ASCII ones in CP949, and cut a UTF-8 syllable in
  the middle. They now drop the characters those two bytes belong to, so a Korean
  syllable goes whole and two ASCII letters still go.

Not changed: the vending titles (`msg_vending.c`) are cut at 0x4f bytes the same way,
with no length check before; that is outside the guild and whisper fields this step
covers. Party and prison names (`msg_party.c`, `msg_misc.c`) are length-checked first or
come from the server, so their copies never cut.

Tests changed:

- New `test_navi_text_lines`: one line for short names, 11 units a line and the
  `len / 12 + 1` count, surrogate pairs kept whole at line ends, the 100-unit limit by
  whole characters, invalid bytes as U+FFFD.
- New `test_renew_quest_text`: a Korean syllable dropped whole, two ASCII letters,
  names kept whole, a syllable before an ASCII ending and an emoji ending, and the UTF-8
  level marks.
- `test_utf8`: `utf8_copy_prefix` stops before a character that does not fit, zero-fills
  the field, handles sizes 0 and 1, empty text, invalid bytes and emoji.

Bugs found (not fixed):

- `navi_manager_add_text_lines` makes `len / 12 + 1` lines of 11 units, so a name of 23
  units (or 35, 47 …) loses its last unit.

Validation:

- Release build passed without compiler or linker warnings: `build/utf8-splits-build.log`.
- Full suite `tests\build.bat` passed: `build/utf8-splits-full-tests.log`.
- The seven standalone suites passed: `build/utf8-splits-<suite>.log`.
- `git diff --check` passed.

The pre-change tree is in `build/before-utf8-splits`. No live client was run.

## IME candidate list (2026-11-03)

Patrick chose to let Windows draw the IME candidate list, the list Japanese and Chinese
input picks words from and Korean uses for Hanja.

Before: `WM_IME_SETCONTEXT` cleared every `ISC_SHOWUI*` flag, so Windows showed no IME
window. `language.c` consumed `WM_IME_NOTIFY`, read the candidates with
`ImmGetCandidateListW` and passed nine at a time to `UICandidateWnd` (window `0x33`),
which drew only its frame: its Korean, Japanese and Chinese layouts belonged to other
service types. Japanese and Chinese users saw an empty box.

Now:

- `WM_IME_SETCONTEXT` clears only `ISC_SHOWUICOMPOSITIONWINDOW`. The edit controls still
  draw the composition inline; the IME draws its own candidate window.
- `WM_IME_NOTIFY` is no longer handled, so it reaches `DefWindowProcW` and the IME's
  window.
- While composing, the single-line, custom and multiline edits call
  `language_place_candidates` with the composition's start and line in client
  coordinates. It sets `CFS_CANDIDATEPOS` below the line and `CFS_EXCLUDE` around it,
  because IMEs differ in which one they use. It skips unchanged positions and places the
  list again for each new composition. The single-line and custom edits used to pass
  the composition's x without the text offset; they now include it. The multiline edit
  uses the caret's line instead of the bottom of the whole control.

Removed with the old list: `UICandidateWnd` (`ime_wnd.c`), window `0x33` (factory, close,
forget and teardown slots), the window-manager messages 8, 9 and 10 (named `EMBLEM`,
`EMBLEM_2` and `CLOSE_EMBLEM`; nothing sends them, and they only opened and closed window
`0x33`), `CandidatePage`, the candidate fields of `struct Language` and the edit draw-key
inputs that tracked them.

Object types: `UICandidateWnd` (reference index 223) is retired. Counts are now 472
classes, 139 retired, 326 named, 306 windows, 2,717 comparisons and 205 accepted casts,
measured with a print-only copy of the test and then asserted.

Tests changed: `test_language_input` no longer covers candidate-list reading. It checks
that candidate notifications are left to the default window procedure, the two
`CANDIDATEFORM`s, that an unchanged position is not set again, that a new composition
sets it again, and a missing input context.

Validation:

- Release build passed without compiler or linker warnings: `build/ime-candidates-build.log`.
- Full suite `tests\build.bat` passed: `build/ime-candidates-full-tests.log`.
- The seven standalone suites passed: `build/ime-candidates-<suite>.log`.
- `git diff --check` passed.
- Live check on Windows 10 in a 1024x768 window, from a separate folder with a hard link
  to Revel's `data.grf`: with the Microsoft Japanese IME, typing `nihongo` and Space twice
  in the login ID field showed Windows' list (日本語, にほんご, ニホンゴ) directly below the
  field, starting at the composition. Exclusive fullscreen was not tested; IME windows
  cannot appear over an exclusive fullscreen Direct3D device. Chinese and Korean IMEs
  were not tried.

Observation from that run: with Revel's `data.grf` alone, the client finds neither
`LuaFiles514\Lua Files\MsgString_KR` nor `LuaFiles514\Lua Files\service_eu\ExternalSettings_eu`.
They exist only under the older `data\lua files\` layout (loose in `client-files` and
in `revel.grf`), and their absence shows about 35 Lua error boxes at startup. The test
folder used copies of the loose `.lua` files. `client-files` was not changed.

## Login Opening button removal (2026-11-03)

Patrick asked whether the login window's **Opening** button should go with the removed
opening videos. It should: the button never did anything in this client. It was a
`UIBmpTextButton` with the `bt_close_*` bitmaps and caption message `0xd67` ("Opening"),
command ID `0x115`, at (13, 104) in the login window. Clicking it sends `UI_MSG_COMMAND`
`0x115` to `ui_login_wnd_send_msg`, which has no case for it. That is so on `main` and in
the original source tree as well (`3d9f1a9`). Nothing else in the client plays an opening
video from the login window.

Removed: the button, the `add_text_button` helper it was the only user of, and its
`LOGIN_ID_CANCEL` and `MSG_LOGIN_CLOSE` defines (both misnamed). The `UIBmpTextButton`
class stays, because character creation and selection use it.

Validation:

- Release build passed without compiler or linker warnings: `build/opening-button-build.log`.
- Full suite `tests\build.bat` passed: `build/opening-button-full-tests.log`.
- The seven standalone suites passed: `build/opening-button-<suite>.log`.
- `git diff --check` passed.
- Live check in the same test folder as the IME candidate list: the login window shows
  the account combo box, both fields, Save ID and the Login and close buttons, with no
  Opening button.

## Dead server types 3 and 13 (2026-11-03)

Decision: the server-type branches listed as dead under the
[langtype 20 lock](#langtype-20-lock-2026-11-03), removed at Patrick's request on
2026-11-03. A purist removal; `main` keeps them.

`set_option` (`platform/clientinfo.c`) is the only code that assigns `g_serverType`:
`<servertype>` `primary` gives 0, `sakray` 1, `local` 2 and `pk` 5, starting from 0. For
`primary` it kept 13 (0xd) if the value already was 13, which nothing sets. So the value
is never 3 or 13, every `g_serverType == 3` branch is dead and every `!= 3` test is true.

What changed, with behavior unchanged:

- `set_option` sets 0 for `primary`.
- `game_mode_mute_self_for_flooding` (`game/chat.c`) skips only administrators, and
  `chat_refuse_repeated_line` lost its early return; `chat.c` no longer includes
  `platform/service.h`.
- `game_mode_msg_give_manner_point` (`game/msg_guild.c`) lost its early return.
- `session_init_talk_type_table_` (`session/session_talk.c`) always adds the shift
  commands.
- The add-friend entries of the chat-room menu (`fill_menu` in `ui/chat/chat_room_wnd.c`,
  owner and member), the whisper menu (`open_menu` in `ui/chat/whisper.c`) and the player
  popup menu (`player_menu` in `game/actor_popup_menu.c`) no longer test the server
  type; their other conditions are unchanged. `gm_rows` always adds the two prohibition
  rows and the prohibition count. `SERVER_TYPE_GUILD` is gone.

Kept: `g_serverType` itself. `login_mode_client_type` (`login/account.c`) reads 5, and
`session_load_navigation_lua` and `session_load_external_settings`
(`session/session_lua.c`) read 0 and 1.

No test changed: no test covered these branches, and the values that reach them are the
same.

Validation:

- Release build passed without compiler or linker warnings: `build/server-type-build.log`.
- Full suite `tests\build.bat` passed with this change and the ports of the
  [UTF-8-review fixes](#foundation-fixes-from-the-utf-8-review-2026-11-03):
  `build/utf8-review-fixes-full-tests.log`; the packet dispatch digest is still
  `d8b534ea02a5d31f`.
- The seven standalone suites passed: `build/utf8-review-fixes-<suite>.log`.
- `git diff --check` passed.

No live client was run.

## Foundation fixes from the UTF-8 review (2026-11-03)

Decision: Patrick approved fixing these bugs on `main` and porting them here on
2026-11-03. Four were listed under the [langtype 20 lock](#langtype-20-lock-2026-11-03):
the homunculus-name format, the monster-skill buffers, the face-charset read and the
uninitialized login-entry field. The other format-string sites were found by listing every
printf-family call whose format is not a string literal (about 450 on `main`, 336 here)
and checking where each format comes from. Each `main` commit has a focused regression
test; the patches and commits are listed in the
[upstream notes](upstream/README.md#foundation-fixes-from-the-utf-8-review-2026-11-03).

The bugs this section listed as found and not fixed were fixed the same way afterwards,
at Patrick's request on 2026-11-03, in two batches (`main` `0fc910c` to `6177cea`, then
`9443627` to `eec8116`) and one more fix (`ab24db8`); they are the eleven items after
the first four below. Two bugs found during the
[third-class removal](#third-class-removal-2026-11-03) were fixed the same way, also at
Patrick's request on 2026-11-03: the mercenary job range (`main` `6110626`, here
`a2e9070`) and the constant names (`278420d`, here `0436ca5`); they come next. Two bugs
found by the
[effects-as-data census](#effects-as-data-trace-harness-and-str-bindings-2026-11-03)
were fixed the same way, also at Patrick's request on 2026-11-03: the heal-effect types
(`b10f009`, here `8fe0de7`) and the meteor-storm wave frame (`d492102`, here `457d6dd`);
they come next. One found by the
[window-list test](#window-id-lists-characterization-test-25a5fe3) was fixed the same
way, at Patrick's request on 2026-11-03: the private-airplane forget entry (`3ac83b2`,
here `3df3cba`); it comes next. The RODEX destroy name, found with the window and
object-type tables, was changed the same way on 2026-11-04 (`5572da7`, here `9f11693`);
it comes next. The EZ2STR and map-sky allocation failures, found with the
[manager lifecycle table](#restructuring-manager-lifecycle-table-2026-11-04), were fixed
the same way on 2026-11-04 (`91ba2d9`, here `5b707c7`); it is the last item.

What each fix does:

- Names and other text are copied into fixed buffers as text (`main` `1b44264`, here
  `8016545`). Thirteen copies passed server, user or file text to `sprintf`,
  `_snprintf_s` or `fprintf` as the format, so a `%` read arguments that were never
  passed and `%n` stopped the client; packet fields were read up to the next NUL, past a
  full field. The copies: the homunculus name (`homun_name`), the pet name (`pet_named`,
  `game_mode_zc_property_pet`), the couple name (`game_mode_zc_couple_name`), the cut-in
  image name (`game_mode_update_cut_in`), the char-server names (`server_list`), the map
  name (`game_mode_move_to_server`), the default and typed homunculus and pet names
  (`companion_info.c`, `pet_info.c`), the saved styling-shop records
  (`ui_my_style_wnd_save_slots`, now `fputs`) and the line of state 0x27c (`change_began`),
  which was formatted a second time. Each now copies with `"%s"`, or with `"%.*s"` bounded
  by the packet field, through `_snprintf_s(..., _TRUNCATE, ...)` bounded by the buffer.
  Here `homun_name` had only the format branch; the `"%s"` branch of services 10, 12 and 15
  went with the langtype lock. The default names taken from job names are copied with
  `utf8_copy_prefix` instead, because a UTF-8 job name can be longer than the 32-byte
  buffers and the cut must keep characters whole, as in the
  [UTF-8 splits](#utf-8-splits-in-navigation-lines-and-packet-fields-2026-11-03) step. The
  other copies never cut valid text: the 24- and 20-byte fields go into larger buffers,
  and the typed names are refused at 24 bytes before the copy. A full 24-byte couple name,
  which needs a server that leaves out the NUL, is cut to 23 bytes and can end in part of
  a character.
- Monster skill values (`b071edc`, here `b5ef560`). `session_get_monster_skill_info`
  copied the `<action>` and `<loopType>` text of `monsterskillinfo.xml` into 8-byte
  buffers with the text as the format, and for players who are not administrators a
  missing node reached `xml_node_text(NULL)`. The values are now parsed with `atoi` from
  the node text, and a missing node counts as 0, the value the administrator path already
  set. The monster name is copied into `name[0x100]` with a bound instead of `strcpy`.
  Here there is no `text_decode`, so the values were already read as stored; on `main` the
  commit also stops passing them through it, which cannot change an `atoi` result.
- Face charsets (`e0db373`, `main` only). `main` now does what
  [font fallback by script](#font-fallback-by-script-2026-11-03) did here: faces past the
  12-entry `faceCharsets` get `DEFAULT_CHARSET` for SCDream and `ANSI_CHARSET` otherwise,
  without reading `hexDigitFlags`. `main` keeps its rule that faces 0x14 and up use the
  first entry with language index 0. Not needed here.
- Login connection entries (`d28b5ab`, here `d4940e6`). `login_conn_entry_copy` copied
  fields that nothing set. `main` zeroes `version` and `service_type` in
  `login_conn_entry_construct` and builds the entry of `login_mode_load_connections` with
  it; zeroing the constructor alone would have missed that function, which used five
  `string_init` calls. Here `service_type` was already gone and nothing reads `version`,
  so the field and its copy are removed; `login_mode_load_connections` is unchanged.
- Char-server list lines (`0fc910c`, here `f0c016a`). `server_list` copied each colored
  line back into `text[0x40]` with `strcpy_s`, which ends the client for a line of 64
  bytes or more, as no invalid-parameter handler is set; long busy or state messages
  make one. The new-server prefix and the name are now built in a String as well, the
  name still bounded by its 20-byte field, and `server_line` copies the finished line
  cut by whole characters: with `text_prefix_length` and `g_codePage` on `main`, with
  `utf8_copy_prefix` here. The prefix (msg 0x1e2) is copied as text, not used as a
  format.
- Pet window names (`e07bb12`, here `0af0722`). `pet_named` and
  `game_mode_zc_property_pet` bounded `petName` by the 24-byte field but passed the field
  itself to `ui_pet_info_set_name`, which sets the edit text with `strlen`;
  `ui_item_collection_set_pet` already stopped at 23 bytes. They now pass `petName` and a
  bounded copy of the field.
- Short map names (`f41fb0b`, here `01f20dc`). `game_mode_move_to_server` drops the
  `.gat` the servers append. For a name shorter than 4 bytes `name.size - 4` wrapped, and
  `string_resize` ended the client with "string too long". Such a name cannot carry the
  extension and is now kept; longer names lose their last four bytes as before.
- Format-string message arguments (`61939b5`, here `911f3e8`).
  `game_mode_format_string_msg` found the second and third `ZC_FORMATSTRING_MSG` argument
  with `strlen` and ignored the packet length. The argument bytes are now copied up to
  the packet length into a String, which ends them with a NUL, and `next_argument` steps
  through that copy with `memchr`: an argument that runs to the end of the packet ends
  there, and those after it are empty.
- Talk lines (`6177cea`, `main` only; the pet and monster talk tables are gone here).
  `session_get_pet_speak`, `session_get_monster_talk_info` and `session_format_chat_line`
  wrote talk-table text into fixed buffers with `strcpy`, `strcat` and `sprintf`, the
  last with the line as the format, and wrote "name : " back into the caller's text.
  They now take their output size, build the line in Strings and copy it cut by whole
  characters. The talk words are replaced as before, including the two bytes skipped
  after `형씨`, whose replacement is two bytes longer, but the rest is never taken from
  past the end of the line. On 108 lines that fit, the old and new
  `session_format_chat_line` gave the same text, except `%d`, which the old code
  formatted.
- One map-extension helper (`9443627`, here `199793a`). `game_mode_zc_npcack_mapmove`
  (`packets_npc.c`) and `login_mode_enter_zone` (`zone.c`) cut the extension with
  `size - 4` as `game_mode_move_to_server` did. `text_drop_map_extension`
  (`core/text_util.c`) now does it at all three places: a name of 4 bytes or more
  loses its last four, a shorter one is kept.
- Private-airplane map field (`1d31026`, here `89dc33c`).
  `game_mode_on_map_move_private_airplane` copied the 16-byte packet field with
  `strcpy_s`, which ends the client for a full field; it now copies field to field
  with `strncpy`, as a `ZC_NPCACK_MAPMOVE` from the server carries it.
  `game_mode_on_server_move_private_airplane` read the field with `string_assign_cstr`;
  it now takes at most its 16 bytes.
- Monster talk decoding (`378db2f`, `main` only). For the Korean service `text_decode`
  mirrors in place. `session_get_monster_talk_info` decoded the string literal from
  `session_get_state_name`, which the release build keeps read-only (`/O2` implies
  `/GF`): a scratch run of the previous code ended with an access violation
  (`0xC0000005`) at the first Korean lookup. Built with writable literals, it flipped
  the name back for the sibling search, so only the first line was found, and it
  decoded the table's line texts in place, so the second call returned the stored
  text. The state name is now decoded once in `stateKey`, the chosen line in a String;
  a job outside `reqJobNames` and an unknown state give the not-found result, an
  empty line.
- Colour codes at the server-list cut (`c7ba72d`, here `8559386`). `server_line` ends a
  cut line before a `^RRGGBB` code that the cut would split, found with
  `text_find_colour_code`, which steps by characters; a code that ends at the limit
  stays.
- The `%s` count of `game_mode_format_string_msg` (`eec8116`, `main` only; here it steps
  with `utf8_next`) took a lead byte's trail byte even when that was the NUL; it now
  stops there.
- Skin-folder files without an extension (`ab24db8`, `main` only; purist has no zip
  skins). `skin_mgr_scan` passed `str_find_last(fd.cFileName, '.')` to
  `string_assign_cstr`, which crashed on the NULL for a file without a `.`; such a file
  is now skipped like any other file that is not a zip skin. The new `test_skin_scan`
  runs the scan over a stubbed folder listing and crashes with an access violation on
  the previous code. On `main` the release build passed without warnings
  (`build/skin-scan-build.log`), the full suite passed with the digest still
  `32d81ae6406478e3` (`build/skin-scan-full-tests.log`), the five standalone suites there
  passed (`build/skin-scan-<suite>.log`) and `git diff --check` passed.
- The mercenary job range (`6110626`, here `a2e9070`). On `main`
  `session_is_mercenary_or_homun_s_job` took jobs 6018-6052, but the mercenaries are
  6017-6046 (`MER_ARCHER01`, Mina, to `MER_SWORDMAN10`, Wayne, in Hercules'
  `mercenary_db.txt`) and the Homunculus S 6048-6052, so Mina was left out; it now starts
  at 6017. Here `session_is_mercenary_job` took 6018-6047 and now takes exactly
  6017-6046; 6047 is no job. What 6017 missed: `game_actor_dir_of` shows a mercenary that
  stands, walks or is in action 0x10 in the direction rounded from its facing, and Mina got
  the facing cut down instead, one direction below the others for about half of all
  angles (40 degrees gave direction 0 instead of 1). `skill_cast_target_look` and the AI
  GetV type 12 also ask the predicate, but give the same result for 6017 either way.
- Constant names (`278420d`, here `0436ca5`), checked against `msgstringtable.txt`,
  `jobidentity.lua` and Hercules' `CZ_UNINSTALLATION` kinds (`REMOVE_MOUNT_*`); no
  behaviour change. In `rodex_compose.c`, `JOB_SUMMONER` and `JOB_SPIRIT_HANDLER` are
  `JOB_TAEKWON` (4046) and `JOB_BABY_TAEKWON` (4225), `MSG_DORAM` ("Taekwon") is
  `MSG_TAEKWON`, `MSG_NAME_TOO_LONG` ("Information of the recipient does not exist.") is
  `MSG_NO_SUCH_RECEIVER` and `MSG_BODY_NAVI_LINK` ("The message contains unrecognizable
  character") is `MSG_BODY_BAD_CHARACTER`. In `equip_wnd.c`, `MSG_RELEASE_HIDING`
  ("Release Falcon") is `MSG_RELEASE_FALCON`, `MSG_RELEASE_RUWACH` ("Dismount") is
  `MSG_DISMOUNT`, `MSG_RELEASE_BOTH` ("Mounting/Dismounting") is `MSG_MOUNT_DISMOUNT`,
  `MSG_CANNOT_NOW` ("Please close Shop.") is `MSG_CLOSE_SHOP`, and the release kinds
  `RELEASE_HIDING` 5 and `RELEASE_RUWACH` 4 are `RELEASE_FALCON` and `RELEASE_RIDING`.
  On `main` also the gryphon, Mado Gear and dragon texts and kinds, which purist removed
  with the third classes: `MSG_RELEASE_RUWACH_DORAM` is `MSG_DISMOUNT_GRYPHON` (picked by
  `gryphon_rider_job`, formerly `ruwach_doram_job`), `MSG_RELEASE_SANTA` is
  `MSG_DISMOUNT_MADO_GEAR`, `MSG_RELEASE_TRANSFORM` is `MSG_DISMOUNT_DRAGON` (picked by
  `riding_dragon`, formerly `transformed`), and `RELEASE_SANTA` 3 and `RELEASE_TRANSFORM`
  1 are `RELEASE_MADO_GEAR` and `RELEASE_DRAGON`.
- Heal-effect types (`b10f009`, here `8fe0de7`). `HEAL_TYPE_PURPLE`, `HEAL_TYPE_CYAN`
  and `HEAL_TYPE_YELLOW` were `0x59e`, `0x5bc` and `0x5bd`, each `0x145` too high; the
  heal factory is registered for `0x459`, `0x477` and `0x478`, so those effects matched no
  case and drew nothing. They are now `0x459`, `0x477` and `0x478`, the cases of the
  original's heal step (`0x9aabf0`), as the
  [census](#the-heal-effect-ids-checked-not-changed) found and the original's colour and
  texture pushes confirm.
- Meteor-storm wave frame (`d492102`, here `457d6dd`). `rag_meteor_storm_waves`
  (RagEffect 90) played its eighth `wizard_meteo.wav` at frame 170. The original's process
  switch (`0x6f874d`, table `0x6ffd94`, entry 90 = `0x6fe37d`) plays it at frames 20, 50,
  80, 100, 130, 140, 180 and 200; the unreferenced copy at `0x6eb310` and the cue list of
  effect `0x48a` (`0x996ad5`, already 180 in `ez2str_effect.c`) agree. Frame 0 falls
  through into frame 10 there too, so it plays both `wizard_meteo` and `hunter_blastmine`:
  that is original behaviour, kept, and now marked in the code.
- Private-airplane forget entry (`3ac83b2`, here `3df3cba`). `open_airship`
  (`navigation_cmd.c`) keeps the window in `private_airplane_wnd` for its modal loop and
  clears the slot only when the loop ends with 0xb4 (close button, Escape, teardown, the
  server's accept). A flight ends it with 0xb1, which the airplane map move sets in
  `game_mode_zc_npcack_mapmove`: the window was destroyed at the end of that frame and
  the slot pointed at it until the next map's mode init cleared all slots. Nothing
  dereferenced it in that time (`key_down` only tests it; the ack handler runs from the
  network poll at the start of a frame). The slot is now in `forget_window`'s list, so the
  base destructor clears it. `hot_key_wnd`, `quest_display_wnd` and `drag_plate` have no
  destroy path that skips their slot: they close through `post_quit1` (0x9c, 0xcc), the
  drag-plate functions or `clear_slots`, none closes itself or is a child of another
  window, and they are left out.
- RODEX destroy name (`5572da7`, here `9f11693`). `set_hwnd_to_game_mon`
  (`ui/mail/rodex_box.c`) destroys and clears `g_rodex`; it is `rodex_destroy_instance`
  now, next to `rodex_create_instance`. Its one caller is the shutdown sequence in
  `core/app.c`; here `test_startup_order`'s stub and expected step take the new name.
- EZ2STR and map-sky allocation failures (`91ba2d9`, here `5b707c7`). The two managers are
  allocated on first use by `ez2str_effect_mgr_instance` and `map_sky_mgr_instance`, which
  return NULL when `mem_try_alloc` fails, and startup passed that NULL to their loads
  (`map_sky_mgr_load` reads `self->lua`, the EZ2STR load writes `self->rows` once its file
  has loaded). The load is skipped now (on `main` in `app_init_managers`, here in the
  manager table's adapters, which return 0 to their ignore rows), so startup goes on
  without the manager, as when its file does not load. Its later users ask the instance
  again and get NULL while memory is short; they now take it as an unloaded manager:
  `map_sky_mgr_apply` sets the black background of a map without sky data, and the EZ2STR
  lookups (`get_effect_ids`, `find_by_effect_id`, and on `main` `get_effect_id`) find no rows.

Format-string calls left as they are:

- About 430 calls format a `msg_str` message, the client's message table, some through a
  variable (`system_messages.c`, `packets_party.c`, `packets_inventory.c`,
  `packets_entry.c`) or a wrapper (`ranking_line`, `thread_text_format`). That is their
  purpose; the server text they show is passed as an argument.
- Formats chosen from literals or tables in the code: sprite, effect and bitmap paths
  (`merc_render.c`, `npc.c`, `render_sprite.c`, `rag_str_anim.c`, `rag_skills_800_839.c`,
  `navi_info.c`, `session_names.c`, `tabs.c`, `window_chrome.c`, `hair_style.c`, the
  party job icons, `bitmap_tab.c`, `cash_shop.c`, `menu_icon_wnd.c`,
  `refining_children.c`), labels and lines (`basic_info_wnd.c`, `skill_list_wnd.c`,
  `npc_shop_purchase.c`, `status_wnd.c`, `select_char_new.c`, `info_overlays.c`,
  `achievement_upper.c`, `refining_draw.c`, `homun_value_line`), error texts
  (`achievement_lua.c`, `item_reform.c`, `lua_data.c`, the navigation and session Lua
  tables, `prohibit_list.c`, `merc_ai_trace.c`, `quest_panes.c`), and the wrappers that
  pass on their own format (`std/string.c`, `thread_text.c`, `ranking_output.c`,
  `lua_calls.c`, `crash_report.c`, `json_loader.c`, `item_info_mgr.c`).
- Client data meant to hold a conversion: the state-icon descriptions from Lua
  (`put_time_left` in `mode_tips.c`) and the option names from `GetVarOptionName`
  (`item_collection_text.c`).
- `render_argument` (`message_tags.c`) checks first that its format is one `%s`, `%d` or
  `%f` conversion.
- `inet_ntoa` as the format of the map-server address (`game_mode_move_to_server`): it
  holds only digits and dots.
- The message box keeps its text as a countdown format (`msgbox_start_count`,
  `message_box.c`), used only when the box has a duration; no caller passes one.
- `printf(text, "")` for state 0x162 (`change_began`, both branches) was meant as
  `sprintf(text, "")`, the empty line of `no_line`. The caller clears `text` and nothing
  writes it before, so the call prints nothing and the line stays empty: no visible
  effect, so it is left.

Found and not fixed: every item listed here before is fixed above. One note: on `main`,
`skin_mgr_scan` resets the skin names, paths and zip flags but not `extracted`, which it
appends to; the scan runs once, at startup, so the lists stay aligned.

Tests added:

- `test_name_text` runs the extracted production code of every copy: the small
  functions whole, and from the large ones the statement that fills the buffer. Every
  text holds `a%d%x%%b` or `%s%n%s%n` and must arrive unchanged; full packet fields
  followed by other bytes, a 319-byte image name, a 26-character map name and
  36-character typed names must be cut to the field or buffer, and the couple name must
  not reach the field after it. Here it also cuts an 11-syllable job name to 10 whole
  syllables. With the follow-up fixes it also builds char-server list lines with long
  busy and state messages and one whose limit falls inside a character (CP949 on `main`,
  UTF-8 here), records the names the pet window and the item collection window get, and
  strips the extension from map names of 0 to 30 bytes. The last batch adds the
  map-move and zone-entry names with short names, and colour codes at the cut. 68
  cases here, 75 on `main`.
- `test_monster_skill_info` runs the extracted function over a parsed
  `monsterskillinfo.xml`: plain, 12-character, trailing-text and `5%d%s%n` values, empty
  and missing nodes, unknown skills and monsters, a 767-byte name and the administrator
  path. 16 lookups here; on `main` the same 16 again with the tags mirrored as for the
  Korean service.
- `test_face_charset` (`main` only) runs `text_dc_select_font` with the production
  `g_3dDevice` tables for faces 0-0x1d, language indexes 0-12, five service types and two
  folder languages, and checks the old bytes past the table against the new values and
  their code pages. 3,900 selections.
- `test_login_conn_entry` loads the connection list (and on `main` both subconnection
  lists) from a parsed `clientinfo.xml` after filling the stack with a pattern. On `main`
  every entry must have both fields 0; here it checks the strings and asserts at compile
  time that an entry is its five strings.
- `test_format_string_msg` runs the extracted `game_mode_format_string_msg` on packets
  followed by `Z` bytes: terminated and unterminated arguments, empty and missing ones,
  no `%s`, four `%s` (not shown) and a 768-byte argument, which fills the line's 0x104
  bytes. On `main` also a message that ends in a CP949 lead byte, with lead bytes taken
  from code page 949 whatever the machine's. 15 packets here, 17 on `main`.
- `test_private_airplane` runs both private-airplane moves on 16-byte fields followed
  by other bytes, up to a full field. 5 names on both branches.
- `test_talk_lines` (`main` only) runs the extracted talk functions over parsed
  `PetTalkTable.xml` and `MonsterTalkTable.xml` into 256-byte outputs followed by a
  guard: plain, tabbed, `%s%n%d` and talk-word lines, the pet's own name, an unknown
  speaker, a 4-byte output, 300-byte lines, a cut inside a CP949 character, a 752-byte
  monster line and a 319-byte job name. The last batch adds jobs just outside the name
  table, and the Korean service with the real mirror over a mirrored table: the
  state-name literal is checked to be on a page that cannot be written, 64 lookups must
  find both lines and the table must keep its text. 58 checks. The monster-talk stub of
  `test_attack_records` takes the new size and checks it.
- `test_mercenary_job` runs the extracted predicate, found by its code because its name
  differs here, and `game_actor_dir_of`: jobs 5990-6070 must be mercenaries exactly for
  6017-6046 (on `main` also 6047-6052), and every mercenary at seven angles must show
  the direction of job 6018. 291 checks on both branches.
- The renames have no test, having no behaviour to test; they were checked by
  disassembling the release objects of both files before and after.
- `test_heal_colours` runs `heal_effect.c` whole for 100 frames with the particle
  functions recorded: `0x459`, `0x477` and `0x478` must each spawn 42 column particles
  with their texture and colour and 36 sparkles with their colour, and `0x138` the green
  heal.
- `test_meteor_storm_waves` runs the extracted `rag_meteor_storm_waves` for frames 0-300
  with the waves and quake messages recorded, against the original's schedule.
- `test_airplane_window_slot` runs the slot statements extracted from `open_airship`
  around the extracted modal loop with one frame that ends it, then destroys the posted
  window through the extracted `ui_window_destruct` and the real `forget_window.c`: ended
  by closing (0xb4) and by the flight's map move (0xb1), the slot must be NULL.
- `test_manager_alloc_failure` runs the startup loads (from `app_init_managers` on `main`,
  the table's adapters here), the two instances, the map-sky load and apply and the EZ2STR
  lookups extracted from production with an allocator that fails: no load may get NULL, and
  the users must answer as for unloaded managers; with memory, both loads must run. It fails
  with any one of the three changed files reverted, on both branches. Here
  `test_startup_order` also runs startup with each instance returning NULL: the sequence
  without that load, and success (its result line now ends "2 warning loads, 2 unallocated
  managers)").

Validation:

- `main`: release builds passed without compiler or linker warnings after each commit:
  `build/name-text-build.log`, `build/monster-skill-info-build.log`,
  `build/face-charset-build.log` and `build/login-conn-entry-build.log` in the `main`
  checkout. The full suite passed on the completed series:
  `build/utf8-review-fixes-full-tests.log`, with the packet dispatch digest still
  `32d81ae6406478e3`. Five of the seven standalone suites exist on `main` and passed:
  `build/utf8-review-fixes-<suite>.log`; `headgear_names` and `pet_accessory` exist only
  here. `git diff --check` passed.
- Here: release builds passed without warnings after each port:
  `build/name-text-build.log`, `build/monster-skill-info-build.log` and
  `build/login-conn-entry-build.log`. The full suite passed with the
  [server-type removal](#dead-server-types-3-and-13-2026-11-03):
  `build/utf8-review-fixes-full-tests.log`, with the digest still `d8b534ea02a5d31f`. The
  seven standalone suites passed: `build/utf8-review-fixes-<suite>.log`.
  `git diff --check` passed.
- Each new test fails on the code before its fix. `test_name_text` fails on both
  branches with any one of the eleven changed files reverted. `test_monster_skill_info`
  fails on the previous code and with only the `strcpy` restored. `test_face_charset`
  fails on `main`'s previous lookup. `test_login_conn_entry` fails on `main`'s previous
  code and with only the constructor changed; here its size assertion does not compile
  against the previous `loginmode.h`.
- Follow-up fixes, `main`: release builds passed without warnings after each commit:
  `build/server-line-build.log`, `build/pet-window-names-build.log`,
  `build/short-map-name-build.log`, `build/format-string-args-build.log` and
  `build/talk-lines-build.log`. The full suite passed on the completed series:
  `build/followup-fixes-full-tests.log`, digest still `32d81ae6406478e3`. The five
  standalone suites there passed: `build/followup-fixes-<suite>.log`. `git diff --check`
  passed.
- Follow-up fixes, here: release builds passed without warnings after each port:
  `build/server-line-build.log`, `build/pet-window-names-build.log`,
  `build/short-map-name-build.log` and `build/format-string-args-build.log`. The full
  suite passed: `build/followup-fixes-full-tests.log`, digest still `d8b534ea02a5d31f`.
  The seven standalone suites passed: `build/followup-fixes-<suite>.log`.
  `git diff --check` passed.
- Each follow-up test fails on the code before its fix, on both branches where the fix
  exists: the char-server lines end at `strcpy_s`, and a plain byte cut fails the
  character case; the pet window names fail on the previous code (on `main` with either
  file reverted); the short map names end in `fatal_error`; `test_format_string_msg`
  fails; `test_talk_lines` fails on the previous code run through adapters for the old
  signatures, also with its `%` cases and its 4-byte output left out.
- Last batch and the overlay, `main`: release builds passed without warnings after each
  commit: `build/map-extension-build.log`, `build/private-airplane-build.log`,
  `build/monster-talk-decode-build.log`, `build/colour-code-cut-build.log`,
  `build/lead-byte-count-build.log` and `build/perf-overlay-build.log`. The full suite
  passed: `build/remaining-fixes-full-tests.log`, digest still `32d81ae6406478e3`. The
  five standalone suites there passed: `build/remaining-fixes-<suite>.log`.
  `git diff --check` passed.
- Last batch and the overlay, here: release builds passed without warnings after each
  port: `build/map-extension-build.log`, `build/private-airplane-build.log`,
  `build/colour-code-cut-build.log` and `build/perf-overlay-build.log`. The full suite
  passed: `build/remaining-fixes-full-tests.log`, digest still `d8b534ea02a5d31f`. The
  seven standalone suites passed: `build/remaining-fixes-<suite>.log`.
  `git diff --check` passed.
- Each test of the last batch fails on the code before its fix: the short map names end
  in `fatal_error`, on `main` also with either new site reverted alone; the airplane
  map move ends at `strcpy_s`, and the server move fails reverted alone; the monster
  talk fails at the job bound, and the access violation and the flipped text were
  shown in scratch copies as described above; the colour-code cut and the lead-byte
  count fail.
- Mercenary range and names, `main`: release builds passed without warnings after each
  commit: `build/mercenary-job-build.log` and `build/name-fixes-build.log`. The full suite
  passed: `build/name-fixes-full-tests.log`, digest still `32d81ae6406478e3`. The five
  standalone suites there passed: `build/name-fixes-<suite>.log`. `git diff --check`
  passed.
- Mercenary range and names, here: release builds passed without warnings after each
  port: `build/mercenary-job-build.log` and `build/name-fixes-build.log`. The full suite
  passed: `build/name-fixes-full-tests.log`, digest `d77aaadbb5fc63b7`, the value since
  the [third-class removal](#third-class-removal-2026-11-03). The seven standalone
  suites passed: `build/name-fixes-<suite>.log`. `git diff --check` passed.
- `test_mercenary_job` fails on the previous predicate on both branches. With the two
  renamed static functions mapped back, `dumpbin /disasm` shows the same code for every
  function of `equip_wnd.obj` and `rodex_compose.obj` before and after the renames (69
  and 56 functions on `main`, 61 and 56 here).
- Heal types and meteor waves, `main`: release builds passed without warnings after each
  commit: `build/heal-colours-build.log` and `build/meteor-storm-build.log`. The full suite
  passed: `build/effect-fixes-full-tests.log`, digest still `32d81ae6406478e3`. The five
  standalone suites there passed: `build/effect-fixes-<suite>.log`. `git diff --check`
  passed.
- Heal types and meteor waves, here: both tests passed after each port; the release build
  of the completed series passed without warnings: `build/effect-fixes-build.log`. The
  full suite passed: `build/effect-fixes-full-tests.log`, digest `d77aaadbb5fc63b7`. The
  seven standalone suites passed: `build/effect-fixes-<suite>.log`. `git diff --check`
  passed. `test_effect_trace` kept `cbdaaf30ef3127d6` after each port
  (`build/effect-trace-heal.log`, `build/effect-trace-meteor.log`): it does not run
  `heal_effect.c`, and it traces effect 90's waves with their frames but its STR stub ends
  the effect after 36-41 frames in all five input sets, before frame 170 or 180.
- `test_heal_colours` fails on the previous constants ("Heal type 0x459 drew nothing") and
  `test_meteor_storm_waves` on the previous frame (170 instead of 180).
- Private-airplane forget entry, `main`: the release build passed without warnings:
  `build/airplane-slot-build.log`. The full suite passed:
  `build/window-list-full-tests.log`, digest still `32d81ae6406478e3`. The five standalone
  suites there passed: `build/window-list-<suite>.log`. `git diff --check` passed.
- Private-airplane forget entry, here: the release build passed without warnings:
  `build/airplane-slot-build.log`. The full suite passed:
  `build/window-list-full-tests.log`, packet dispatch `d77aaadbb5fc63b7`, effect trace
  `cbdaaf30ef3127d6`. The seven standalone suites passed: `build/window-list-<suite>.log`.
  `git diff --check` passed. `test_window_lists` now gives 34,733 lines and
  `39f36d8f0948c9bf` (was 34,731 and `778535d4c6391e4b`): its "forget field
  private_airplane_wnd[0]" case adds the line `  mgr.private_airplane_wnd: W0 -> 0` in
  each of its two environments (trace lines 15358 and 34566); nothing else changes.
- `test_airplane_window_slot` fails on the previous `forget_window.c` on both branches
  ("private_airplane_wnd still points at the destroyed window"); here the extractor takes
  `ui_window_mgr_run_modal`, the name of `run_modal` here.
- RODEX destroy name: no test, being a rename. `dumpbin /disasm` of `rodex_box.obj` and
  `app.obj` before and after, with the symbol mapped, shows the same code for every
  function (8 and 12 on `main`, 8 and 6 here). Release builds without warnings
  (`build/rodex-rename-build.log` on both branches); full suites
  (`build/rodex-rename-full-tests.log` on `main`, digest `32d81ae6406478e3`;
  `build/table-followups-full-tests.log` here) and standalone suites
  (`build/rodex-rename-<suite>.log`, `build/table-followups-<suite>.log`) passed;
  `git diff --check` passed.
- EZ2STR and map-sky allocation failures: release builds without warnings
  (`build/manager-alloc-build.log` on both branches, `build/manager-alloc-build-x64.log`
  here); full suites (`build/manager-alloc-full-tests.log`: `main` digest
  `32d81ae6406478e3`; here packet dispatch `d77aaadbb5fc63b7`, effect trace
  `cbdaaf30ef3127d6`, window lists `2232167ff470caa7`) and standalone suites
  (`build/manager-alloc-<suite>.log`, five on `main`, seven here) passed. Here CMake with MSVC
  x64 passes CTest 21 of 21 (`build/manager-alloc-cmake-x64-*.log`), and Linux (WSL, as for
  the merge) builds with 1,671 warnings and passes 26 of 26 with `SDL_VIDEO_DRIVER=offscreen`
  (`build/wsl-manager-alloc-*.log`). `git diff --check` passed.
- The four managers only `main` has (`0b21c23`, `main` only; purist removed them):
  `app_init_managers` loaded the reputation, item-name, item-reform and enchant managers the
  same way (`repute_mgr_load`, `item_db_to_id_load`, `item_reform_mgr_load`,
  `enchant_mgr_load`), each reaching its Lua script or tables through a NULL instance. Their
  loads are skipped the same way, and their 29 later users get the answers of an unloaded
  manager: empty reputation lists, the "changed" text and list refresh without a manager, the
  "no information" notice in the reputation window, item name 0 (already so), no reform
  items or reforms, `REFORM_MAX_MATERIALS` (6) material slots through the new
  `item_reform_max_materials`, no enchant list (the window closes as for an unknown table)
  and enchant setters that store nothing. `test_manager_alloc_failure` links the four
  managers' files whole, with their outside calls in `tests/manager_alloc_failure_stubs.c`,
  and runs every user check without memory and on new managers that have not loaded, with
  the same answers; it fails with `startup.c`, `repute_mgr.c`, `item_reform.c` or
  `enchant_mgr.c` reverted. Release build without warnings
  (`build/manager-alloc4-build.log`), full suite (`build/manager-alloc4-full-tests.log`,
  digest `32d81ae6406478e3`) and the five standalone suites (`build/manager-alloc4-<suite>.log`)
  passed on `main`; `git diff --check` passed.

No live client was run.

## Frame-time overlay opt-in (2026-11-03)

Decision: approved by Patrick on 2026-11-03 and made on `main` first (`53d23a1`, here
`a25052e`); the patch is listed with the
[UTF-8 review fixes](upstream/README.md#foundation-fixes-from-the-utf-8-review-2026-11-03).
The frame-time overlay (`diag/perf.c`) started shown when `RAGEXE_OVERLAY` was set, but
`perf_overlay_draw` also toggled it whenever Scroll Lock changed state, with or without
the variable. Scroll Lock is a screenshot key (START-066), so players who take
screenshots with it got the debug overlay on screen.

What changed:

- Without `RAGEXE_OVERLAY`, or with it set to 0, the overlay never appears and Scroll
  Lock has no effect on it. `perf_begin`, `perf_end`, `perf_frame_end` and
  `perf_overlay_draw` return at once, so nothing is timed; the `g_perf` counters that
  other files increment stay.
- With `RAGEXE_OVERLAY=1` the overlay starts shown as before, and Scroll Lock hides and
  shows it. The variable keeps its name and meaning; `docs/portability.md` on
  `purist-portable` uses it.
- Scroll Lock and Print Screen still take screenshots through `ui_window_mgr_handle_key`
  (`wndmgr_keys.c`), which is unchanged.

Tests added: `test_perf_overlay` compiles `diag/perf.c` with `GetEnvironmentVariableA`,
`GetKeyState` and the performance counter stubbed and the drawing calls counted. Without
the variable and with it set to 0, 40 frames with Scroll Lock toggled every five draw
nothing and read the clock no more after the first call; with it set to 1 the overlay
draws once its first half-second is in, and Scroll Lock hides it and shows it again. The
screenshot case of `ui_window_mgr_handle_key`, extracted from this branch's
`wndmgr_keys.c` and run with each setting, takes one screenshot per first press of
Scroll Lock or Print Screen. The test fails on the previous code: Scroll Lock shows the
overlay without the variable.

Validation: in the release builds, full suites, standalone suites and `git diff --check`
of the [last batch](#foundation-fixes-from-the-utf-8-review-2026-11-03) on both branches
(`build/perf-overlay-build.log`, `build/remaining-fixes-full-tests.log`). No live client
was run.

## Third-class removal (2026-11-03)

Patrick decided that Revel will not offer third classes. While the work was under way he
widened the removal to the expanded classes Expanded Super Novice, Kagerou/Oboro,
Rebellion, Summoner (and with it the Doram race), Soul Reaper and Star Emperor, each with
its baby variants. Kept jobs: Novice, the first, second and transcendent classes, Super
Novice, Taekwon, Star Gladiator, Soul Linker, Ninja, Gunslinger and the babies of all of
these. All monster and NPC behaviour stays. A purist removal; `main` keeps these classes.

How the IDs were chosen:

- **Jobs.** The client's own predicates. Third classes are `session_is_third_job`:
  `0xfd6`-`0x1010` (4054-4112: the 13 classes, their transcendent and baby forms, dragon,
  gryphon, warg and Madogear mounts and the five dragon colours) plus the 26 costume
  mounts that `session_get_mounted_job` maps them to: `0x1018`, `0x101f`, `0x1021`,
  `0x1024`/`0x1025`, `0x102c`, `0x102f`, `0x1034`, `0x1037`, `0x104d`-`0x1054`, `0x1056`,
  `0x1065`/`0x1066`, `0x1069`/`0x106a`, `0x106d`/`0x106e`, `0x1070`/`0x1071` (85 IDs).
  The expanded classes are `session_is_expanded_super_novice_job` (`0x105e`-`0x1061`),
  `session_is_doram_job` (`0x1079`-`0x107d`) and the Kagerou/Oboro, Rebellion, Star Emperor
  and Soul Reaper members of `session_is_expanded_job` (`0x1073`-`0x1078`, `0x107f`,
  `0x1080`, `0x1085`, `0x1087`, `0x1088`, `0x108d`, `0x108f`-`0x1098`), 31 IDs. Baby
  Ninja, Taekwon, Star Gladiator, Soul Linker and Gunslinger (`0x107e`, `0x1081`-`0x1084`,
  `0x1086`, `0x1089`-`0x108c`, `0x108e`) stay.
- **Skills.** The block markers of the client's kRO skill-ID list (`skillid.lub`):
  third-class 2001-2532 (`THIRDJOB_END`) except `SC_REPRODUCE` 2285, which Revel's
  `skill_tree.conf` gives to Rogue; level expansion 5001-5016 (with `ALL_FULL_THROTTLE`,
  learned only by removed jobs on Revel); the Arch Bishop skills `AB_VITUPERATUM`/
  `AB_CONVENIO` 5072/5073; Rebellion, Star Emperor and Soul Reaper 2551-2612; Kagerou/Oboro
  3001-3029; Doram 5017-5057. Homunculus S skills are the client's `MH_` IDs 8018-8042 and
  8045-8060. 8043/8044 stay: kRO numbers them `MH_VOLCANIC_ASH`/`MH_LAST`, but Revel reuses
  them for `HAMI_COVER` (Amistr) and `HLIF_MANA` (Lif). Elemental skills are 8401-8442.
- **Skill units.** The client's kRO unit numbering, `0xca`-`0x109`.

Server checks (HampterHercules, read only):

- `db/job_db.conf` and `db/skill_tree.conf` still define the removed classes; they must be
  disabled on the server.
- `db/mob_skill_db.conf` and `db/mob_skill_db2.conf` use no third-class or expanded-class
  skill: their 146 distinct skills are first-, second- and transcendent-class and `NPC_`
  skills. **No monster uses a removed skill.**
- `db/item_db.conf`/`item_db2.conf`: the 24 `itemskill` calls use only first- and
  second-class skills; no item names a removed skill.
- No NPC script calls `hommutate` or `morphembryo`, so Revel has no way to create a
  Homunculus S. Elementals come only from `SO_SUMMON_AGNI`/`AQUA`/`VENTUS`/`TERA`.

### Jobs and tables (`225876e`)

- **Predicates.** `session_is_third_job`, `session_is_expanded_job`,
  `session_is_expanded_super_novice_job`, `session_is_doram_job` and
  `session_is_any_super_novice_job` (now identical to `session_is_super_novice_job`) are
  gone; every caller takes its former false path. `session_is_baby_job` lost the removed
  babies; `session_get_baby_job_scale` now calls it instead of repeating the list.
- **Job-indexed rule tables** (`session_job_rules.c`: level, value, weapon rules; the
  skill-tree tab table in `ui/character/skill_tree.c`) give each removed ID the value that
  reproduces the function's existing out-of-table fallback and end after the last kept
  job, `0x108e`. A side-by-side harness compiled the old and new
  `session_job_rules.c` and checked 3,278,600 job/level/value/weapon combinations: kept
  jobs unchanged, removed jobs equal to the out-of-table probe `0x1700`. The tab table was
  checked the same way in Python.
- **Rows.** The four `.def` tables (compact `job - 0xf6e` index: 115/115/114/115 rows),
  `pc_names.c` (114 rows, 23 now unused path strings), the job-name table (83 rows) and
  the hit-sound cases (23 labels, with their fixture rows).
- **Special cases** in mount and drawn-job mapping (`session_get_mounted_job`,
  `session_get_drawn_job`, `game_actor_get_job`, `game_actor_state_job`,
  `game_actor_set_job`, `session_get_job_without_mount`, `ef_unmounted_job`), mount
  height and lift, weapon/dual-wield rules (Guillotine Cross), Bard/Dancer sex rules,
  ammo messages, glow layers, hit marks, level-up effects, the Star Emperor entries of the
  Star Gladiator camera and miracle notice, the third-class text of the job-message item
  and the Rune Knight make-target path.
- **Level caps and auras.** The Lua reads of `BaseLevel3rd`, `BaseLevelExtend2`,
  `BaseLevelUpperJob`, `JobLevel3rd`, `JobLevelExtend2`, `JobLevelUpperJob`,
  `BaseLevelDoram`, `JobLevelDoram`, `Default160LvAura(_sub)` and `Default185LvAura(_sub)`,
  their session fields, and the actor messages `0x92` (160 aura) and `0x96` (185 aura).
  The session destructor zeroed the level fields with an address-sized `memset`; it now
  takes the size from the layout.
- **2nd-costume styling tab.** It was enabled only for third jobs; it now always shows its
  disabled bitmaps and cannot be selected, which is what kept jobs always saw.

### Expanded classes and Doram

- Kagerou/Oboro, Rebellion, Star Emperor and Soul Reaper went through the same tables and
  special cases as the third classes: shadow and hit actions, attack chances, glow, zoom
  and miracle notices for Star Emperor, the Soul Linker/Soul Reaper spirit action, the
  Expanded Super Novice guardian-angel check, and the skill-tree tab kinds.
- **Doram race.** The Doram button of the character-creation window (`make_char_new.c`,
  `descript_race.c`: only the human race button stays), the Doram hair-style limit, hair
  names (`s_doramHairMale`/`Female`), `MakeableRace` Doram read, `도람족` body, head,
  palette and accessory paths, including the `_doram` headgear lookup and its fallback
  (START-161), the `OffsetItemPos_GetOffsetForDoram` offsets in five draw paths, Doram
  head heights, the Doram menu icon, styling-shop filters, the orc-face variant, Doram
  attack motions and the soul-attack shot (status `0x3d6`, `Pc.soul_attack`, the
  `su_soulattack` missile).

### Skills, units and effects (`7315268`)

- **Looks.** Every special case for the removed skills: cast and use motions and
  effects, cast looks, hit effects, ground looks, chat shouts, harm/good-skill and
  no-use-effect lists, hit-effect tables, skill-hit messages and targeting.
  `skill_use_third.c` and `skill_use_third2.c` are gone; the six looks that monster
  `NPC_` skills share (soul strike, water, glow, hallucination walk, Jack Frost ring) moved
  into `skill_use_extra.c`, and the `SC_REPRODUCE` look moved into `skill_use_late.c`.
- **Units.** `0xca`-`0x109` except the units monster skills also create: `0xda`
  (`NPC_REVERBERATION`), `0xdc`/`0xdd` (`NPC_FIREWALK`/`NPC_ELECTRICWALK`), `0xdf`
  (`NPC_PSYCHIC_WAVE`), `0xe0` (`NPC_CLOUD_KILL`), `0xfd` (`NPC_VENOMFOG`), `0xfe`
  (`NPC_ICEMINE`), `0xff` (`NPC_FLAMECROSS`), `0x100` (`NPC_HELLBURNING`) and `0x101`
  (`NPC_MAGMA_ERUPTION`). The Feint Bomb unit was the only caller of
  `actor_spawn_hallu_copy`, which went with `actor/hallucination.c` and its test.
- **Rolling Cutter.** The hold-to-repeat use, whose `CZ_USE_SKILL_START`/`END`
  (`0x0b10`/`0x0b11`) requests are no longer sent, its session one-shot counter and four
  input paths, and the `RollingCutterRagEffect` spin class. The generic actor counters, `rag_effect_set_duration`
  and `render_object_return_zero` that shared its file moved to `game_actor.c`,
  `rag_effect.c` and `game_object.c`.
- **SP-percent skills.** `session_is_space` (Ancilla and one Sorcerer skill) and its three
  `%` branches.
- **Effect classes** whose only triggers were removed code, with their registry IDs: Bind
  Trap (`0x42d`-`0x42f`), Cart Cannon (`0x54d`), Fallen Angel (`0x474`), Frigg Song
  (`0x423`), Creating Star (`jupitel_thunder_hit.c`, `0x4b0`), Meteor (`0x44b`, `0x452`,
  `0x457`, `0x458`), Nyanggrass (`0x465`), Powdering (`0x450`), Severe Rainstorm (`0x4fc`),
  Star Emperor (`0x4b2`/`0x4b3`), Stem Spear (`0x44c`), SVG Spirit (`0x468`) and Triangle
  Shot (`0x52c`). The 19 IDs joined `k_retiredEffects`, so a server request naming one
  launches nothing, as for the 4th-class IDs.
- Newly orphaned helpers went too, among them `play_wave_look`/`struct WaveLook`,
  `game_camera_target_heading`, `ui_short_cut_wnd_slot`, `GroundScope.skill_id` and the
  Poison Smoke and Comet ground textures.

### Companion systems and packets (`b7f2466`)

- **Elementals.** `ZC_EL_INIT`/`ZC_EL_PAR_CHANGE` handlers, the session elemental fields,
  the own-unit term, the type-10 owner, gauge, picking (`is_merc_or_elemental` is now
  `is_mercenary`), targeting, skill-used notice and AI `IsMonster` terms, the elemental
  half of the skill predicate (now `session_is_mercenary_skill`), elemental skill looks and
  units `0xed`-`0xf0`, and the Rect Up effect class (`0x440`).
- **Homunculus S.** Jobs `0x17a0`-`0x17a4` leave `session_is_homunculus_job` and
  `session_is_homun_or_merc_job` and their names; `session_is_homunculus_s_job` is gone and
  `session_is_mercenary_or_homun_s_job` is now `session_is_mercenary_job` (its range still
  started at `0x1782`, as found; fixed in `a2e9070`). The Eleanor action, the morph-order
  action, the aura exception, `MH_` skill looks and units `0xf5`-`0xf7` went with them.
- **Madogear.** Status `0x47d` (begin/end and value capture), `g_session1.madogear`, its
  map-entry copy, scaled equip/preview drawing and draw key, `마도기어_`/`마도아머_`
  sprite renaming, the Madogear glow, shadow and robe rules and the release button.
- **Dragons, wargs, gryphons.** The form-change handling of option bits 19 and 23-26
  (dragons), 21 (warg rider) and 22 (Madogear), the warg companion bit 20 with actor
  messages `0x89`/`0x8a`, and the dragon and Madogear release kinds and texts.
- **Spheres.** Rebellion coins (Rich's Coin class, `0x46f`), Soul Reaper soul energy (Soul
  Collect class, `0x4c9`) and the Kagerou/Oboro charm display `game_actor_show_orbs`.

Packet handlers that became no-ops: the second dispatcher keeps these case labels and
consumes the packets without a call, next to its existing ignore list:

- `0x081d` `ZC_EL_INIT` and `0x081e` `ZC_EL_PAR_CHANGE` (elementals);
- `0x08cf` `ZC_SPIRITS_ATTRIBUTE` (Kagerou/Oboro charms);
- `0x0b73` `ZC_SOULENERGY` (Soul Reaper).

No packet ID left the packet table or either dispatcher.

Kept, and why:

- **Monster versions.** No Revel monster casts a removed skill, but the `NPC_` skills that
  share looks, hit effects or units with third-class skills stay (`NPC_MILLENNIUMSHIELD`,
  `NPC_JACKFROST`, `NPC_CHEAL`, `NPC_SR_CURSEDCIRCLE`, `NPC_VENOMIMPRESS`,
  `NPC_IGNITIONBREAK`, `NPC_PHANTOMTHRUST`, `NPC_HALLUCINATIONWALK`, `NPC_ARROWSTORM`,
  `NPC_FATALMENACE`, `0x2db`, `NPC_PSYCHIC_WAVE`, `NPC_CLOUD_KILL`, `NPC_MAGMA_ERUPTION` and
  the units above), and so does `ZC_MILLENNIUMSHIELD` (`0x0440`), which
  `NPC_MILLENNIUMSHIELD` also produces.
- **Server-driven status visuals**, as the 4th-class removal's first pass did: status
  cases for third-class and expanded statuses in `actor_efst.c`, `player_states.c` and
  `state_messages(_timed).c`, and the effect classes they spawn, among them Root Twist,
  Tuna Party, Sprite Mable, Sun Stance, Gravity Control and Soul Unity. A follow-up
  candidate.
- **Shared effect data.** EZ2STR rows and RagEffect per-ID processes of removed skills,
  the charm-orb rows `0x2df`-`0x2e2` of the shared energy-orb class, `orb_kind`/`orb_count`
  and the `THROW_ORBS` branch of the shared throw-item class, and Light Sphere
  (`0x4ad`-`0x4af`) and New Cloud Kill (`0x4ac`), which had no client trigger before this
  removal either.
- **Generic plumbing.** The actor sprite-variant vtable slots, `Pc.madogear_type` and the
  variant/mount parameters of the body sprite and palette path functions: nothing sets the
  variant any more, so they always carry 0. The 2nd-costume window, the skill-select window
  (`ZC_SKILL_SELECT_REQUEST`) and the item-list window (`ZC_ITEMLISTWIN_OPEN`), which only
  removed skills open on Hercules. The positional warg and Mado-cart rows of the message
  effect kinds table. `body_style`, which the server sends for alternate outfits.
- **Unknown IDs.** `0x13cc` and `0x13cf` (5068, 5071) in `skill_use_late.c` have no name in
  the client's skill lists; their looks stay.
- `SC_REPRODUCE`, `ALL_ODINS_RECALL` 2533, the `ALL_`/`ECL_` item skills and the new
  second-class skills 2613-2625.

Server implications:

- Disable the removed jobs in `db/job_db.conf`/`db/skill_tree.conf`. A removed job ID takes
  the client's fallbacks: name "Poring", Novice sprite, human paths, no level-cap entry,
  out-of-table rule values, no special case.
- Removed skills draw no special look; removed skill units draw the generic unit, and the
  19 + 3 retired effect IDs launch nothing.
- `0x081d`, `0x081e`, `0x08cf` and `0x0b73` are consumed and ignored. Status `0x47d`
  (Madogear) and option bits 19-26 no longer change the drawn sprite; object type 10 is an
  ordinary unit.
- The client no longer sends `CZ_USE_SKILL_START`/`CZ_USE_SKILL_END` or the dragon and
  Madogear release kinds. The `MaxLevelTable` keys listed above are no longer read.
- No server checkout was modified.

Tests changed:

- Rewritten for the removal: the race-button factory (human only, 9 destructors), the
  resource paths (human costume paths instead of Doram ones; Madogear renaming gone),
  the actor dual-wield list, the hit-sound fixture, the effect launch (four retired IDs),
  the effect destruction factory case (now Square Range), the scalar packets (elemental
  part gone), the game context (target heading gone), the AI timed order (every homunculus
  job now plays action `0x38`; the Homunculus S range played `0x30`) and the AI-context and
  camera stub renames.
- `test_hallucination.c` was removed with `actor_spawn_hallu_copy`.
- Stale Doram stubs left the character draw/model tests and the draw-map unreached list.
- Object-type counts are unchanged (472 classes, 139 retired identities): no removed class
  was in the object-type registry.

Validation:

- Release builds passed without compiler or linker warnings before each commit (the
  log is grepped for `: warning ` and `LNK`): `build/no-third-jobs-build.log` for the
  first commit and `build/no-third-companions-build.log` for the final tree; the build of
  the skills commit was checked the same way, but later builds overwrote its log.
- Packet dispatch: all 1,179,800 cases passed, digest `d77aaadbb5fc63b7` (before:
  `d8b534ea02a5d31f`). `build/derive_no_third_dispatch.py` changes exactly 48 cases of the
  pre-change trace (`build/packet-dispatch-before-no-third.bin`): 12 each for `0x081d`,
  `0x081e`, `0x08cf` and `0x0b73`, full-size non-null calls in all six patterns, direct and
  forwarded, each losing its single sink call. The actual trace matches byte for byte
  (SHA-256 `aaa9247d...6537c6`): `build/no-third-dispatch-expectation.log`.
- Effect factories: digest `d3673c01` (before: `616ea281`), in two derived steps:
  `build/derive_no_third_effect_factories.py` drops the 57 nodes of the 19 skill-effect IDs
  (`f1707460`), and `build/derive_no_third_companion_effect_factories.py` drops the 9 nodes of
  `0x440`, `0x46f` and `0x4c9`; both renumber the fixture, and the failure case still collides
  on `0x943`. Both real traces match byte for byte:
  `build/no-third-effect-factories-expectation.log`,
  `build/no-third-companion-effect-factories-expectation.log`.
- Full suite `tests\build.bat` and the seven standalone suites passed for each commit:
  `build/no-third-jobs-tests.log`, `build/no-third-skills-tests.log` with
  `build/no-third-skills-tests-resume.log` (the effect destruction test needed five more
  unreached stubs; the run resumed there), `build/no-third-companions-tests.log` (one
  uninterrupted run after the two AI-context corrections), and
  `build/no-third-<jobs|skills|companions>-<suite>.log`.
- Edits were made by scripts that prune only the labels and table rows of the listed IDs,
  stop on a fallthrough into a removed group and report newly unreferenced static and
  external definitions. `git diff --check` passed.

Bugs found (not fixed):

- `ui/mail/rodex_compose.c` names `0xfce` and `0x1081` `JOB_SUMMONER` and
  `JOB_SPIRIT_HANDLER` and `0xd3c` `MSG_DORAM`; they are TaeKwon, Baby TaeKwon and the
  TaeKwon job name, which is what the code shows.
- `ui/character/equip_wnd.c` names the release texts after the wrong things:
  `MSG_RELEASE_HIDING` (`0x1b9`) is "Release Falcon" and `MSG_RELEASE_RUWACH` (`0x1ba`)
  "Dismount"; the removed `MSG_RELEASE_SANTA`, `MSG_RELEASE_TRANSFORM` and
  `MSG_RELEASE_RUWACH_DORAM` were "Dismount Mado Gear", "Dismount Dragon" and "Dismount from
  Gryphon".
- `session_is_mercenary_job` (formerly `..._or_homun_s_job`) starts at `0x1782`, so the
  first mercenary ID `0x1781` is not included; kept as found.

All three were fixed afterwards, on `main` first: the names in `0436ca5` and the range in
`a2e9070`; see the [UTF-8 review fixes](#foundation-fixes-from-the-utf-8-review-2026-11-03).

The pre-removal tree is in `build/before-no-third`. The edit and check scripts are in the
session scratch area; the derivations are in `build/`. No live client/server smoke test was
run.

## Restructuring: definitions and hooks (2026-11-03)

Patrick approved four restructuring steps: move misplaced definitions, replace upward
calls with hooks and registered handlers, turn the central feature lists into tables,
and extract feature clusters into modules or static libraries. This section records the
first two, on the branch `purist-restructure`. No behavior change.

### Measurement

The dependency map is built from the compiled objects: `dumpbin /symbols` on every
`build\obj\*.obj`, then the file graph of which external functions and data each `.c`
file defines and uses (`depmap.py`). `modules.py` leaves out the 23 registry and startup
files (`core/object_type.c`, the packet dispatchers, `make_window*.c`, `window_slots.c`,
`client_init.c`, `winmain.c` and the others in its `REGISTRY` set) and reports the
largest strongly connected component (the knot), the files below it that never reach
it (the library layer) and the files above it, grouped into clusters. The scripts and
every measurement are in `build/depmap/`; `whatif.py` there simulates dropped
references and moved definitions, which is how the cuts below were chosen and ordered
(single-edge ranking, then a leave-one-out run over every planned cut).

The baseline measurement of `purist` at `be1ec6e` reproduced the published numbers
exactly: 1,046 files and 289,359 lines; knot 473 files, 149,369 lines; library 205
files, 21,966 lines; above the knot 345 files, 109,360 lines in 164 clusters; registry
and startup files 23 files, 8,664 lines.

Two properties of the measurement matter when reading it:

- A global without an initializer (`int g_x;`) is a communal symbol in MSVC C. It has
  no defining object, so the map never draws an edge for it. `g_lightmap` and
  `g_depthPerPixelCot` were initialized and did form edges; `g_trilinearFilter`,
  `g_sprHalfWidth`, `g_displayWidth` and the other display options in
  `ui/render/graphic_setting.c` are not, so their use from `render/c3ddevice.c`,
  `resources/spr.c` and `ui/controls/frame_wnd.c` does not show. They create no link
  dependency either.
- The map counts only link-time references. Header includes are not measured; where a
  change removed the last use of an upper-layer header, the include went too.

### Step 1: moved definitions

Moved code is unchanged in its new place, apart from the renames noted.

- **`error_msg`** (`1617399`): from `world/mode_draw.c` to the new
  `platform/error_message.c`, next to its header. Every resource loader, the file layer,
  the renderer and the platform code call it. Its one display step, flipping a
  full-screen DirectDraw display back to GDI, is the hook `g_errorMsgPrepareDisplay`.
- **`screen_sprite_frame`** (`e90a23e`): from `world/mode_draw.c` to the new
  `render/sprite_draw.c`. It is declared in `render/sprite_draw.h` and called by
  `render/cframe.c` and `render/rp_hud.c`.
- **`g_lightmap`** (`74209d5`): from `ui/render/graphic_setting.c` to the new
  `render/render_options.c`, declared in the new `render/render_options.h` (included by
  `render/render_api.h`); initial value 1 as before. The renderer reads it every frame.
  It did not go into `crenderer.c` because the renderer storage test includes that file
  and defines its own zero-initialized `g_lightmap`.
- **`g_depthPerPixelCot`** (`a6c37be`): from `world/view_camera.c` to `render/rp_screen.c`
  beside `g_spriteDepthScale` and `g_spriteRowDepthStep`; declared in `render/renderer.h`
  instead of `world/world_imports.h`.
- **`borderless_rect`, `borderless_setting`, `monitor_rects`** (`d3b3051`): from
  `render/resize.c`, which also builds settings-window rows, re-lays out windows and
  restarts the status icons, to `render/c3ddevice.c`, whose `c3d_device_init` is their
  only device-side caller. `monitor_rects` was static and keeps its name.
- **`ui_frame_wnd_ops`** (`5935d78`): from `ui/character/companion_info.c` to
  `ui/controls/frame_wnd.c`, with the functions it lists.
- **`ui_edit_ctrl_var_id_ops`** (`f01d496`): from `ui/chat/guild_notice.c` to
  `ui/controls/edit_variants.c`, with the other edit-control tables. Not in the original
  list of ties; the single-edge ranking found it (3,916 lines on its own).
- **The skin folder** (`58341a1`): `ui_skin_path` in `core/skin_path.c` no longer reads
  `g_skinMgr` or calls `skin_mgr_get_current_path`. The skin manager hands the selected
  folder to the new `ui_skin_path_set_skin` at the end of `skin_mgr_scan` (also when no
  skin folder exists), in `skin_mgr_set_current_skin_num` and, with NULL, in
  `skin_mgr_destruct`. NULL means no skin, as an empty list or selection -1 did. The
  string stays the skin manager's and changes only with a new scan, which publishes
  again. `core/skin_path.h` no longer includes `ui/skin_manager.h`.
- **The item-link step** (`739eef5`): `ui_frame_wnd_send_msg`, the edit control's link
  handler (`edit_navi_links.c`) and the list box's `LIST_MSG_ITEM_LINK` case each opened
  the item collection window and, through `session_get_worn_counterpart`, the comparison
  window. The three copies were identical; the step is now `item_collection_show_link`
  in `ui/items/item_collection.c` (declared in the new `ui/items/item_collection.h`) and
  the widgets reach it through `g_uiHooks.show_item_link`.
- **The item tag button** (`630b00d`): `struct UIItemTagButton`, `ui_item_tag_button_ops`,
  its four methods and its constructor, from `ui/items/item_tags.c`/`.h` to the new
  `ui/controls/item_tag_button.c`/`.h`, which `ui/items/item_tags.h` includes. The
  multi-line edit control creates these buttons.

Window-manager functions that are not about windows. They are `UIWindowMgr` methods of
the original client, keep their names and bodies, and only game and feature code calls
them, so they moved to the owning module instead of getting a handler:

- `ui_window_mgr_is_friend` (`window_list.c`) and `ui_window_mgr_request_add_friend`
  (`messages.c`, the `CZ_ADD_FRIENDS` sender) to `ui/chat/friend_request.c`, declared in
  the new `ui/chat/friends.h` (`a6ef706`);
- `ui_window_mgr_open_sub_chat` (`window_list.c`) to `ui/chat/sub_chat.c` (`a6ef706`);
- `ui_window_mgr_navigation_help` (`window_list.c`) to `ui/quests/navigation_wnd.c` and
  `ui_window_mgr_run_email_address_box` (`modal.c`) to `ui/character/password_prompts.c`
  (`3fb79d0`); the modal loop the prompt uses, formerly the static `run_modal`, is
  `ui_window_mgr_run_modal` in `modal.c`;
- `ui_window_mgr_add_whisper_chat_to_whisper_wnd` and `g_whisperSound`
  (`whisper_map.c`) to `ui/chat/whisper.c` (`f8cb90c`).

### Step 2: hooks and handlers

Every hook and handler is set in one place: `client_install_hooks()` in
`core/client_init.c`, the first call of `client_init()`, which is the first call of
`WinMain`. Nothing that uses a hook runs earlier. Each is documented where it is
declared. A NULL member is checked at every call; the column says what happens then.

| Hook | Declared / defined in | Installed with | NULL |
|---|---|---|---|
| `g_errorMsgPrepareDisplay` | `platform/error_message.h` / `.c` | `device_flip_to_gdi` (`render/c3ddevice.c`) | the box is shown without the flip, as with no device |
| `g_uiHooks.show_tooltip` | `ui/controls/ui_hooks.h` / `ui_hooks.c` | `mode_show_current_tooltip` (`world/mode_ui_hooks.c`) | no help balloon |
| `g_uiHooks.set_cursor_action` | same | `mode_set_current_cursor_action` (`world/mode_ui_hooks.c`) | the cursor stays |
| `g_uiHooks.show_item_link` | same | `item_collection_show_link` (`ui/items/item_collection.c`) | the click opens nothing |
| `g_uiHooks.navi_navigate`, `navi_search`, `navi_show_map` | same | `navi_link_navigate`, `navi_link_search`, `navi_link_show_map` (`navi/navi_ui_hooks.c`) | the link does nothing |
| `g_uiHooks.open_quest` | same | `game_open_quest_link` (`game/game_ui_hooks.c`) | nothing opens |
| `g_uiHooks.may_move_ime_caret` | same | `ui_rodex_compose_allows_caret_move` (`ui/mail/rodex_ui_hooks.c`) | the caret may move |
| `g_windowMgrHandlers.chat_edit`, `chat_whisper_edit` | `ui/wndmgr/wndmgr_handlers.h` / `.c` | `ui_chat_window_edit`, `ui_chat_window_whisper_edit` (`ui/chat/chat_window.c`) | the chat window has no edit line: focusing it does nothing, battle mode clears nothing, no focused edit counts as the chat window's |
| `g_windowMgrHandlers.save_chat_log` | same | `ui_chat_wnd_write_chat_to_file` (`ui/chat/chat_window_log.c`) | `UIWM_MSG_SAVE_CHAT` writes nothing |
| `g_windowMgrHandlers.save_chat_room_talk` | same | `ui_chat_room_wnd_save_talk` (`ui/chat/chat_room.c`) | `UIWM_MSG_SAVE_CHAT_ROOM` writes nothing |
| `g_windowMgrHandlers.open_whisper` | same | `ui_whisper_wnd_open` (`ui/chat/whisper.c`) | `UIWM_MSG_WHISPER` opens nothing and returns 0 |
| `g_windowMgrHandlers.skill_hot_key_slot` | same | `session_skill_hot_key_slot` (`session/session_hotkeys.c`) | -1, no skill hotkey |
| `g_renderBeforeClearLists` | `render/rp_pool.h` / `rp_pool.c` | `effect_mgr_destroy_removed_effects` (`effects/effect_render_hooks.c`) | the render lists are cleared without it |
| `g_emblemRules.is_bad`, `mark_bad`, `max_size` | `resources/emblem.h` / `emblem_res.c` | `guild_emblem_is_bad`, `guild_emblem_mark_bad`, `guild_emblem_max_size` (`contents/guild_emblem_hooks.c`) | no emblem is known bad, nothing is recorded, the size limit is 0 so nothing loads |
| `g_grannyJobModelName` | `resources/granny_res.h` / `granny_res.c` | `session_granny_job_model` (`session/session_granny_hooks.c`) | no job has a model; its bone animation is not loaded |
| `g_clientInfoWeatherScript` | `platform/clientinfo.h` / `clientinfo.c` | `weather_add_etc_script` (`actor/weather.c`) | the etcinfo.txt weather line is ignored and its strings freed |

What changed at the call sites:

- **Widgets** (`e6fb6af`, `f39b5f7`, `739eef5`). The base window, button, check box, radio
  buttons, edit line, static text, resizer and scroll bar call `ui_show_tooltip` and
  `ui_set_cursor_action` (inline in `ui_hooks.h`) instead of `mode_show_tooltip` and
  `mode_set_cursor_action` on `client_current_mode()`; six of them no longer include a
  `world/` header. The navigation link button and the text viewer's navigation and quest
  links, and the edit control's IME-caret check, go through their `g_uiHooks` members.
  Each adapter makes exactly the former call; the adapters live in small
  `<module>_ui_hooks.c` files so the tests can include them.
- **Window manager** (`bd9c6ab`, `f8cb90c`, `2b65246`). `ui_window_mgr_edit_focus_to_chat_wnd`,
  `ui_window_mgr_set_text_at_battle_mode`, `ui_window_mgr_is_chat_edit_focused`,
  `UIWM_MSG_SAVE_CHAT`, `UIWM_MSG_SAVE_CHAT_ROOM` and `UIWM_MSG_WHISPER` use the handlers;
  with all of them installed the calls, their order and their short-circuits are as
  before. The skill-hotkey loop at the end of `ui_window_mgr_get_hot_key_behavior` moved
  unchanged into `session_skill_hot_key_slot`. `open_whisper` and its `place_whisper`
  moved unchanged into `ui/chat/whisper.c` as `ui_whisper_wnd_open`; its window ID
  constant is `WHISPER_WINDOW_ID` there because `whisper.c` already uses `WHISPER_WND_ID`
  for a control. `ui_window_mgr_on_enter` sends Enter to the character selection window
  through `ui_window_send_msg`: `new_select_char_wnd` is only ever set by
  `make_select_char` to a window whose ops' `send_msg` is
  `ui_new_select_char_wnd_send_msg`, so the same function runs.
- **Renderer and resources** (`3d66d4d`, `82b5baa`, `e179e00`, `359c0b4`). The render-list
  reset, the emblem loader, the Granny bone-animation cache and the etcinfo.txt reader
  call their hooks. `emblem.h` no longer includes `contents/guild_mgr.h`, `granny_res.c`
  no session header and `clientinfo.c`/`.h` no `actor/weather.h`.

### Dependency map after each commit

Each row is measured on that commit's own release build (`build/depmap/<label>.txt`).

| commit | change | knot files / lines | library files / lines | above the knot files / lines | clusters |
|---|---|---|---|---|---|
| `be1ec6e` | baseline (purist with the class removal) | 473 / 149,369 | 205 / 21,966 | 345 / 109,360 | 164 |
| `1617399` | error_msg to platform/error_message.c | 448 / 145,087 | 236 / 26,901 | 340 / 108,711 | 163 |
| `e90a23e` | screen_sprite_frame to render/sprite_draw.c | 449 / 145,092 | 236 / 26,901 | 340 / 108,711 | 163 |
| `74209d5` | g_lightmap to render/render_options.c | 444 / 143,265 | 237 / 26,905 | 345 / 110,539 | 155 |
| `a6c37be` | g_depthPerPixelCot to render/rp_screen.c | 444 / 143,265 | 237 / 26,905 | 345 / 110,539 | 155 |
| `d3b3051` | borderless_rect to render/c3ddevice.c | 442 / 142,884 | 237 / 26,905 | 347 / 110,921 | 152 |
| `5935d78` | ui_frame_wnd_ops to frame_wnd.c | 441 / 142,171 | 237 / 26,905 | 348 / 111,633 | 153 |
| `f01d496` | ui_edit_ctrl_var_id_ops to edit_variants.c | 431 / 138,305 | 237 / 26,905 | 358 / 115,497 | 153 |
| `58341a1` | skin manager publishes the skin folder | 431 / 138,322 | 237 / 26,905 | 358 / 115,499 | 153 |
| `739eef5` | g_uiHooks.show_item_link | 431 / 138,264 | 238 / 26,909 | 358 / 115,520 | 153 |
| `e6fb6af` | g_uiHooks tooltip and cursor | 431 / 138,262 | 238 / 26,909 | 359 / 115,534 | 154 |
| `f39b5f7` | g_uiHooks navigation, quest, caret | 430 / 138,052 | 238 / 26,909 | 363 / 115,783 | 156 |
| `a6ef706` | friend and sub-chat helpers to chat | 432 / 138,550 | 238 / 26,909 | 361 / 115,296 | 157 |
| `3fb79d0` | navigation help, e-mail prompt to their windows | 432 / 138,553 | 238 / 26,909 | 361 / 115,296 | 157 |
| `bd9c6ab` | g_windowMgrHandlers chat and hotkey | 431 / 138,478 | 238 / 26,738 | 363 / 115,560 | 153 |
| `f8cb90c` | g_windowMgrHandlers.open_whisper | 429 / 137,768 | 239 / 26,777 | 364 / 116,232 | 153 |
| `2b65246` | select-char Enter through ops | 420 / 133,695 | 239 / 26,777 | 373 / 120,304 | 148 |
| `3d66d4d` | g_renderBeforeClearLists | 420 / 133,697 | 239 / 26,777 | 374 / 120,312 | 149 |
| `82b5baa` | g_emblemRules | 420 / 133,699 | 239 / 26,777 | 375 / 120,330 | 150 |
| `e179e00` | g_grannyJobModelName | 266 / 96,096 | 454 / 77,895 | 315 / 106,824 | 122 |
| `630b00d` | item tag button to ui/controls | 261 / 93,923 | 461 / 80,142 | 314 / 106,754 | 121 |
| `359c0b4` | g_clientInfoWeatherScript | 259 / 93,568 | 468 / 81,457 | 309 / 105,805 | 118 |

The knot went from 473 files / 149,369 lines to 259 files / 93,568 lines (-214 files,
-55,801 lines); the library layer from 205 files / 21,966 lines to 468 files / 81,457
lines. Most cuts only pay off together. The renderer, resource and file layers leave the
knot only when `g_lightmap`, `g_depthPerPixelCot`, `borderless_rect`, the render-list
reset, the emblem rules and the Granny name are all cut, and the widget and
window-manager cuts made before that only take effect once the layers below them stop
reaching up. That is why the knot moves little until `e179e00` and then drops from 420
to 266 files. The knot grew by two files at `a6ef706` because `ui/chat/friend_request.c`,
which now holds the friend helpers, and its base `ui/social/accept_base.c` joined it;
both are feature code. The second component of 71 files that appears at `e179e00` is the
renderer, resource and widget foundation calling each other; it lies below the knot.

Composition of the knot, before and after:

| directory | baseline files / lines | after files / lines |
|---|---|---|
| `effects` | 80 / 40,005 | 77 / 38,715 |
| `ui/controls` | 51 / 13,783 | - |
| `actor` | 47 / 13,015 | 39 / 12,338 |
| `session` | 48 / 9,860 | 32 / 7,469 |
| `render` | 24 / 7,762 | - |
| `game` | 26 / 6,866 | 20 / 5,384 |
| `ui/chat` | 16 / 6,746 | 8 / 3,200 |
| `ui/social` | 18 / 6,570 | 13 / 3,689 |
| `ui/character` | 9 / 5,224 | 4 / 1,643 |
| `contents` | 28 / 4,727 | 18 / 2,982 |
| `world` | 20 / 4,572 | 12 / 2,598 |
| `ui/shops` | 9 / 4,202 | 8 / 3,859 |
| `resources` | 21 / 3,746 | - |
| `navi` | 16 / 3,733 | 8 / 2,416 |
| `ui/items` | 10 / 3,086 | 6 / 2,017 |
| `ui/quests` | 8 / 3,078 | 6 / 2,610 |
| `ui/render` | 3 / 2,081 | 1 / 1,005 |
| `ui/wndmgr` | 7 / 1,967 | 1 / 294 |
| `login` | 5 / 1,127 | 2 / 753 |
| `ui/worldmap` | 1 / 1,060 | 1 / 1,060 |
| `ui/mail` | 1 / 967 | 1 / 967 |
| `web` | 3 / 961 | - |
| `platform` | 2 / 870 | - |
| `fileio` | 4 / 758 | - |
| `core` | 7 / 688 | 1 / 208 |
| `iteminfo` | 2 / 459 | - |
| `skills` | 1 / 376 | - |
| `diag` | 1 / 368 | - |
| `ui/resource` | 1 / 361 | 1 / 361 |
| `ui (top level)` | 3 / 210 | - |
| `ui/misc` | 1 / 141 | - |

### Left in the knot, and why

- **actor, game, session, effects, world, navi** (as asked): the real core, untouched
  except for the trivial moves above.
- **`core/character_npc.c`**: an NPC actor class filed under `core/`. It calls about
  fifteen actor files and `ui/render/character_render.c`. Moving the file changes no
  edge; it belongs with the actor work.
- **`ui/wndmgr/plates.c`**: the party member plates (mini party windows, plate records in
  the session), a feature filed under the window manager. Only feature code reaches it.
- **`ui/render/character_render.c` and `ui/render/graphic_setting.c`**: a character
  preview renderer and the graphics settings window, features filed beside the widgets.
  `graphic_setting.c` left the knot once `g_lightmap` moved.
- **The feature windows** that call the game mode and the session and are called back
  by them (`ui/shops`, `ui/social`, `ui/chat`, `ui/quests`, `ui/items`, ...).

Upward calls left in the low layers that no longer hold the knot together, and so were
not converted: `core/drive_state.c` and `ui/controls/edit.c` calling
`ui_window_mgr_is_chat_edit_focused`, `platform/language.c` calling
`ui_window_mgr_get_focus_edit` and `fileio/resource_errors.c` calling
`ui_window_mgr_send_msg_` (the window manager is below the knot now); widgets calling
`client_current_mode` and `mode_send_message` (`world/mode_context.c` and
`world/mode_dispatch.c` only dispatch through the mode's table and are not in the knot);
`core/map_sky.c` and `diag/crash_report.c` calling game and session code;
`ui/controls/agit_message_box.c` using `ui_agit_message_box_ops` and
`ui_agit_message_box_create` from `ui/social/guild_manage.c` (a misplaced definition with
no effect on the knot); `ui/wndmgr/key_down.c` with `wndmgr_keys.c`, and `registry.c`,
which call into the chat window, navigation, world view, hotkeys and the session but are
reached only from `core/wndproc.c`, `core/winmain.c` and feature code, so they sit above
the knot.

### Tests changed

Only to follow a moved symbol; no assertion changed.

- Stubs that only the moved code needed were removed: the game-mode layout test
  (`device_flip_to_gdi`, `win_utf8.c`, the seven `screen_sprite_frame` dependencies), the
  edit navigation test (four item-link stubs and `g_session1`), the scroll and UI-drop
  tests (the cursor and mode stubs), the chat-navigation and URL-link tests (navi, quest
  and game-mode stubs, `g_navi`) and the particle pools test (`effect_mgr_destroy_removed`).
- The projection test dropped its own `g_depthPerPixelCot`, which `rp_screen.c` now
  defines.
- The skin manager test stubs `ui_skin_path_set_skin`, as it stubs `ui_skin_path`.
- Tests that link a widget, `rp_pool.c`, `emblem_res.c` or `granny_res.c` define the hook
  table where needed. The ones whose recording stubs the moved calls reached (button
  drawing, link buttons, edit navigation, renderer storage, bitmap owner, resource
  lifecycle) include the real adapter file (`world/mode_ui_hooks.c`,
  `navi/navi_ui_hooks.c`, `ui/mail/rodex_ui_hooks.c`, `effects/effect_render_hooks.c`,
  `contents/guild_emblem_hooks.c`, `session/session_granny_hooks.c`) and install it as
  `client_install_hooks()` does, so the same stubs are reached in the same order.

### Validation

Every commit was validated in its own export (`git archive` into `build/snap/<label>`)
with `build/depmap/validate.ps1`: a release build with the log grepped for `: warning `
and `LNK`, the full suite `tests\build.bat`, the seven standalone suites, a comparison
of every `digest`/`passed` line of the suite log with the baseline run, and the
dependency-map measurement. Logs: `build/restructure-<label>-build.log`,
`-tests.log` and `-<suite>.log`; summary in `build/restructure-validation.txt`; baseline
in `build/restructure-baseline-build.log`, `-tests.log` and `-digests.txt`.

- Every commit builds without compiler or linker warnings and passes the full suite and
  the seven standalone suites.
- All 256 digest and result lines of every commit's suite match the baseline, among them
  packet dispatch `d77aaadbb5fc63b7` and effect factories `d3673c01`.
- One printed line depends on the checkout path, not on the code: `test_lua_boundaries`
  prints how many allocations `client_lua_open` makes (it asserts no fixed count; it fails
  each one in turn and checks that nothing is left allocated). Lua 5.1's `setprogdir`
  puts the exe directory into the four `!` of the default `package.path` (53 + 4 x D
  bytes), and past the 512-byte `luaL_gsub` buffer the count rises by 3: 419 up to a
  114-character directory, 422 from 115. The first run of `e6fb6af` used a longer
  snapshot folder; its rerun with a 13-character label (`c10-cursortip`) matches. Not a
  bug: three allocations once per Lua state, all freed.
- Two commits failed their first validation and were amended in place: `f39b5f7` (the
  URL-link test, which extracts functions from `text_viewer_links.c`, needed
  `g_uiHooks`) and `a6ef706` (five more callers of `ui_window_mgr_is_friend` needed
  `ui/chat/friends.h`). The commits after them were rebased onto the fixed ones and
  validated again; the table rows are those of the final commits (the failed runs'
  logs, `c11-links`, `c12-chatmoves` and `c13-navemail`, are kept).
- `git diff --check` passed for every commit.

### Bugs found (not fixed)

- Withdrawn: `ui_agit_message_box_destroy` in `ui/social/guild_manage.c` was reported
  here as calling `mem_free(self)` twice. It calls it once on both branches and always
  has (`git log -L`), and a counting-allocator check of the whole destroy chain frees
  each block once.
- The navigation-link parsing (`navi_link`) is duplicated in
  `ui/controls/text_viewer_links.c` and `ui/quests/tag_text_viewer.c`, and the item-link
  step was copied three times (now one function); not bugs, but the copies can drift.
- The text-button, link-button and chat-navigation tests define a
  `mode_set_cursor_action` stub that only their archived reference build can reach.

No live client/server smoke test was run.

## Effects as data: trace harness and STR bindings (2026-11-03)

Decision: approved by Patrick on 2026-11-03, after the read-only census of all effect IDs:
build a trace harness first, then move the effects that only play an STR file, and the STR
file choices, from C into a data file. Behavior is unchanged. Commits `731155f` (harness)
and `558ceb4` (conversion), both on `purist-effects`.

### The harness (`tests/test_effect_trace.c`)

`tests\build_effect_trace.bat` compiles the real client files with the client's flags
(`/std:c11 /O2 /Oy- /MD /GS /Gd /we4013`, warnings as errors) and the harness sets
`_controlfp(0x20000, 0x30000)` as `winmain.c` does (START-073):

- the dispatch `rag_effect_process.c`, `rag_effect_process_491.c`, `rag_effect_process_800.c`;
- the launchers `rag_skills_*.c` (18 files), `rag_songs.c`, `rag_msg_sprites.c`,
  `rag_unreferenced.c`, `rag_unreferenced_prims.c`, `rag_process_moves.c`;
- `rag_prim_launch.c`, `effect_prim.c`, `rag_effect.c` (construct, messages, destruct),
  `rag_effect_init` and the STR player (`rag_str_anim.c`, `rag_str_playback.c`,
  `str_path.c`, `str_animation.c`), the launch entry `game_actor_launch_effect`
  (`actor/effect_launch.c`), and `ez2str_effect_files.c`;
- support code that computes values: `math3d.c`, `rp_project.c`, `position.c`,
  `effect_objects.c`, `effect_sound.c`, `durations.c`, `std/list.c`, `vec.c`, `string.c`;
- after the conversion also `effect_data.c`, `rag_str_bindings.c`, `resource_path.c` and
  the embedded data, built the way `build.bat` builds them.

Everything else is a stub in the test file: the world object list, the texture cache, the
resource manager (STR files get a synthetic animation of 1-4 layers and 1-4 keys whose
length and values come from a hash of the file name, so files differ in length and loops
and rewinds happen), sprites and ACT frame counts, sound (logs the wave, its position and
the effect's frame), `game_send_message` (quakes; message `0x87` logs the three floats its
pointers carry), mode messages, Lua calls, effect-manager spawns, after-image and plant
constructors, a master actor and a player at fixed positions, a fixed view matrix and
renderer projection, a ground height that is a linear function of x and z, and the session
fields the effects read (`map_rule_flags[0]`, `effectOn`, `ownAid`, `stateIconSlots`).

`tests/effect_trace_rand.h` is forced into every client file (`cl /FI`): it includes
`stdlib.h` and renames `rand` to `effect_trace_rand`, a copy of the MSVC CRT generator
(`holdrand * 214013 + 2531011`, bits 16-30) seeded per run and counting its calls. No
production file needed a seam.

Runs: every ID of the duration table (0-2384) and 2385, 10000 and 60000, each with five
input sets that vary the seed, the master's facing (0, 90, 225, 315, 180), the launch
offset and rotation, the hit count (none, 3, 51, 1, 0), a duration message (none, 37, 120),
`msg_params`, the min-effects flag (0, 1, 2), whether the master is a PC and whether it is
the player, the camera heading, `effectOn`, and the logic-step schedule. Each render frame
sets `master->drawn` and `g_isSkipFrame`; a schedule of 1 calls `rag_effect_on_process`
once per frame, the other schedules pick 0, 1, 2 or 3 steps per frame, and a frame of 0
steps calls the process once with `g_isSkipFrame` 0, so `rag_runs_every_frame` decides
whether anything happens. A run stops when the process returns 0, at frame 400, or after
1,200 calls, and ends with the real destructor.

After every call the harness hashes (64-bit FNV over words):

- the side-effect log as text lines: textures, resource loads, locks and unlocks, waves,
  messages, tints, motions, Lua calls, spawns, constructors, ground-height and actor
  queries;
- the words of the RagEffect (without its 512 triangles, which launchers never write) and
  of the master that changed since the previous call; the first call hashes them whole;
- every object new in the world list, in full: primitives with their textures, alpha
  keys and triangles, child RagEffects and other objects;
- the `rand()` call count.

Pointers are replaced by names: objects by kind and creation order (`E0`, `p12`),
textures and resources by file name, the STR animation by its file, other heap blocks by
allocation order and offset. Heap blocks are filled with `0xcd`, and the stack is painted
before each call, so that uninitialized bytes hash the same in every run. At the end of a
run every object is hashed again whole. Per-ID digests combine the five runs; the total
combines the IDs.

A second part calls `ez2_str_effect_open_for_type` for EZ2STR types 0-2450 with the
min-effects flag 0, 1 and 2 and two seeds (14,706 calls) and hashes what it loads.

Results: 2,388 IDs, 2,031,553 process calls, 36,986 primitives; effect trace digest
`cbdaaf30ef3127d6`, EZ2STR file choice digest `1b0232b840610959`. The executable runs in
about 12 seconds. Three runs gave the same per-ID digests; because ASLR moves the image,
heap and stack, a pointer that escaped the naming would differ between runs (the first
version leaked the stack addresses of message `0x87` that way, in IDs 79 and 619).
`--digests FILE` writes the digest of every ID and type, `--trace FILE` the readable log
with the changed words of every call, `--ids FIRST LAST` limits the run.

Detection proof (each change made, the harness run, the change reverted with
`git checkout`):

| Change | Effect trace | EZ2STR | IDs that changed |
|---|---|---|---|
| none | `cbdaaf30ef3127d6` | `1b0232b840610959` | |
| `rag_hit1`: `p->speed = 0.7f` to `0.71f` (`rag_skills_000_019.c`) | `116beead1b0d74b1` | same | 0 |
| `hit_spark`: the two angle `rnd()` draws swapped | `fff4211b079df558` | same | 2, 3 |
| `rag_effect_init` case 23: `StoneCurse.str` to `StoneCurse2.str` | `5c809a775b09dabe` | same | 23 |
| EZ2STR row 997: `setsudan.str` to `setsudan2.str` | same | `269269faf04d303c` | type 997 |
| after the conversion, `effects.json` ID 30: `magician_thunderstorm.wav` to `...2.wav` | `1bcd9c222710225a` | same | 30 |

### Data format, embedding and override

The data is `data/effects/effects.json`, documented in `data/effects/README.md`:

- `RagEffect` rows by effect ID: `Str` (`File`, `MinFile` used when `map_rule_flags[0]`
  is not 0, `Variants` for `1 + rand() % n` in a `%d`, `Loops` for `str_max_loops`) and
  `Play`, which replaces the dispatch case: a `Timeline` of cues (`At` a frame or every
  frame, `AfterStr`, `Do`: `Follow`, `Wave` with `Height` or `AtPlayer`, `Quake`,
  `Duration`), the step's `Offset` (default `[0, -80, 0]`), `OnStrEnd` (`Stop`,
  `Continue`, `Rewind`) and `StopFrame`.
- `Ez2StrFile` rows by EZ2STR type: `File`, `MinFile` (used when the flag is 1), `Open`
  (through `ez2_str_particle_open`), `FullOnly` and `Variants`.

`build.bat` compiles `tools/bin2c.c` (plain C99), which writes `build/gen/effect_data_embedded.c`
with the bytes as `g_effectData`; that file is compiled and linked like the sources. The
test scripts embed the file the same way. A build without `build.bat`, such as the port
on `purist-portable`, compiles and runs the same tool; nothing is Win32-specific.

`src/effects/effect_data.c` (596 lines) reads the file once, on the first lookup, with
the vendored yyjson in strict mode: no comments, trailing commas or BOM, a fixed `Format`
string, only known keys, each key and ID once, integers in range, floats only when a
32-bit float holds them exactly, `%` only as the one `%d` of `Variants`, names within the
63 and 255 bytes the C buffers hold, and names converted from UTF-8 to CP949 by
`resource_name_to_cp949` (rejected when CP949 lacks a character). Lookups index arrays by
ID.

`RAGEXE_EFFECT_DATA`, unset or empty by default, names a file to read instead. When that
file is missing or breaks a rule, `error_msg` shows "Effect data FILE: REASON. The client
uses its built-in effect data instead." with the place of the error (for example
`RagEffect 5 Play, cue 1, operation 1: Height and AtPlayer belong to a Wave`), and the
embedded data is used.

### What was converted

The converter `tools/convert_str_bindings.py` parsed the C code of `2239d84` and wrote
the data: the `rag_effect_init` switch (226 IDs), the dispatch case groups whose body only
steps the STR, with waves, quakes, following and a duration, inlining the helpers they
call (93 case groups, 195 IDs), and the EZ2STR table with its special cases 141, 1163 and
1244-1253 (510 types). It refuses any statement outside that vocabulary; the 195 IDs it
accepted are exactly the census's category D. With `--apply` it deleted the case groups.

- `rag_effect_init` (`rag_str_anim.c`) looks the ID up and picks `MinFile`, rolls the
  variant and sets `str_max_loops` as the switch did; the switch (226 cases, 228 lines) and
  `FULL_OR_MIN` are gone, and the names are formatted with `snprintf`.
- `rag_effect_on_process` checks for a `Play` row before its dispatch and runs
  `rag_str_play` (`rag_str_bindings.c`, 59 lines): cues before the step, `rag_str_step`,
  cues after the step, then stop, continue or rewind. 58 case groups left
  `rag_effect_process.c` (including all of 305, 306, 311, 371 and 372 in
  `rag_effect_process300`), 22 left `rag_effect_process_491.c` and 13
  `rag_effect_process_800.c`.
- 42 functions that only those cases called are gone with their declarations in
  `rag_effect.h` and `rag_process_moves.h`: the 33 `rag_*_wave(s)` functions and
  `rag_str_step_or_stop` (`rag_unreferenced.c`), `rag_lex_divina` and `rag_fire_pillar`,
  `rag_two_hand_quicken`, `rag_shield_charge` and `rag_magic_rod`, `rag_str_raised`,
  `rag_str_only` and `rag_str_or_rewind`. The static helpers `follow_wave_str`
  (`rag_effect_process.c`) and `wave_then_step` (`rag_unreferenced.c`) went too.
- `ez2str_effect_files.c` lost its 498-row table, the `LOAD`/`OPEN`/`FULL_ONLY`/
  `MINEFFECT` macros, the binary search and the cases 141, 1163 and 1244-1253; type 1412
  (its own name) and the headgear types from 2384 (Lua) stay in C.

Before and after (`git diff --stat 2239d84`, 30 files changed, 4338 insertions(+), 1537 deletions(-)):

```
    added  gone  file
       10     0  build.bat
      113     0  data/effects/README.md
      743     0  data/effects/effects.json
      596     0  src/effects/effect_data.c
       75     0  src/effects/effect_data.h
        2    41  src/effects/rag/rag_effect.h
        7   240  src/effects/rag/rag_effect_process.c
        0   115  src/effects/rag/rag_effect_process_491.c
        0    64  src/effects/rag/rag_effect_process_800.c
        0     6  src/effects/rag/rag_process_moves.c
        0     1  src/effects/rag/rag_process_moves.h
        0     5  src/effects/rag/rag_skills_020_039.c
        0    24  src/effects/rag/rag_skills_080_119.c
        0     9  src/effects/rag/rag_skills_120_219.c
        0    13  src/effects/rag/rag_skills_220_259.c
        0     5  src/effects/rag/rag_skills_920_2373.c
       11   232  src/effects/rag/rag_str_anim.c
       59     0  src/effects/rag/rag_str_bindings.c
        0   209  src/effects/rag/rag_unreferenced.c
       20   572  src/effects/skills/ez2str_effect_files.c
        4     0  tests/build.bat
       20     0  tests/build_effect_data.bat
        8     1  tests/build_effect_launch_duration.bat
       27     0  tests/build_effect_trace.bat
       13     0  tests/effect_trace_rand.h
      190     0  tests/test_effect_data.c
        8     0  tests/test_effect_launch_duration.c
     1634     0  tests/test_effect_trace.c
       61     0  tools/bin2c.c
      737     0  tools/convert_str_bindings.py
```

In the client's C code 1,536 lines are gone and 770 added: 40 in the existing files and 730 in the
new loader (`effect_data.c`/`.h`, 671) and STR player (`rag_str_bindings.c`, 59). The 743 lines of
`effects.json` replace the 226-case switch, 93 dispatch case groups, 42 helpers and the 498-row
EZ2STR table.

### Validation

- Effect trace before and after the deletion: the same `cbdaaf30ef3127d6` and
  `1b0232b840610959`, and the same digest for every one of the 2,388 IDs and 2,451 EZ2STR
  types (`build/effects-digests-base.txt` and `build/effects-digests-after.txt` compare
  equal).
- Effect factories: digest `d3673c01`. `effect_registry.c` is untouched, so the
  registrations are the same; the EZ2STR conversion changes which file a registered type
  loads, which the EZ2STR digest covers.
- Packet dispatch: digest `d77aaadbb5fc63b7`.
- `tests\build_effect_data.bat` (new): the embedded data loads, an override file replaces
  it, and a missing file and 40 bad files (syntax, BOM, format, keys, IDs, duplicates,
  names, `%`, CP949, ranges, floats, operations) are each reported once and fall back.
- `test_effect_launch_duration`, which runs the extracted `rag_effect_init`, now links the
  loader and the embedded data; its nine cases, among them ID 13 loading `SafetyWall.str`,
  pass unchanged.
- The converter run on a clean export of `2239d84` writes the committed `effects.json`
  byte for byte, and its `--apply` writes the committed `rag_effect_process_491.c` and
  `rag_effect_process_800.c`; `rag_effect_process.c` differs from its output only by the
  `rag_str_play` call and the removed `follow_wave_str`.
- Release build without compiler or linker warnings, full suite `tests\build.bat` and the
  seven standalone suites, for each commit: `build/effects-build*.log`,
  `build/effects-suite*.log`, `build/effects-standalone*-<suite>.log`. `git diff --check`
  passed.

No live client was run.

### The heal-effect IDs (checked, not changed)

The census noticed that `heal_effect` is registered for `0x459`, `0x477` and `0x478` but
handles `0x59e`, `0x5bc` and `0x5bd`, which are registered to the STR player. The original
executable (read with a disassembler, nothing written) settles it:

- Its registration function calls the register routine 1,206 times. The heal factory
  (`0x9618c0`, `CEffectAllocator<CHealEffect>` by RTTI) is used for exactly
  `0x138`, `0x139`, `0x140`, `0x145`, `0xfd`, `0x459`, `0x477` and `0x478`, as in the port.
  `0x59e`, `0x5bc` and `0x5bd` use the EZ2STR factory (`0x9612c0`), as in the port.
- The heal spawn step (`0x9aabf0`) compares the type: above `0x145` it jumps to
  `sub eax, 0x459` / `sub eax, 0x1e` / `sub eax, 1` on the unchanged type, so its cases are
  `0x459` (purple: `ring_white.tga`, color 115/25/115, sparkle kind 6), `0x477` (cyan,
  `alpha_down.tga`, kind 7) and `0x478` (yellow, kind 8). It never compares with `0x59e`,
  `0x5bc` or `0x5bd`.

So the port's registration is right and its switch is wrong: `HEAL_TYPE_PURPLE`,
`HEAL_TYPE_CYAN` and `HEAL_TYPE_YELLOW` in `heal_effect.c` should be `0x459`, `0x477` and
`0x478`. Each is off by exactly `0x145`, the constant of the comparison before the
subtraction. As it is, these three heal effects draw nothing. Not changed here; it is a
`main` fix. Fixed afterwards: `main` `b10f009`, here `8fe0de7`.

### Found, not fixed

- The heal-effect constants above (fixed afterwards, `8fe0de7`).
- `rag_earth_spike` (`rag_skills_040_079.c`) sends message `0x87` with the addresses of
  three stack floats cast to `int`; that truncates the pointers on a 64-bit build.
  Checked afterwards, not changed: the message goes through `game_send_message`, whose
  arguments are `int` on `main` and here, so a wider cast would be cut back to `int` at
  the call. `purist-portable` widened the mode-message arguments to `intptr_t` in
  `40bcdef`, and its `rag_earth_spike` already passes `(intptr_t)` pointers. It is the only
  sender of mode message `0x87` on all three branches.
- `rag_effect_construct` leaves `screen_pos` and `str_state[60..127]` uninitialized; the
  first STR step writes them before anything reads them.
- `rag_meteor_storm_waves` (`rag_unreferenced.c`) falls through from frame 0 to frame 10,
  so frame 0 plays both waves; not compared with the original. Compared afterwards: the
  original does the same (kept), but plays the eighth meteor wave at frame 180, not 170
  (fixed, `457d6dd`).

### Next batch

The census's next candidates launch primitives, so `Play` needs a `Launch` operation and a
generated `offsetof` table over `EffectPrim` and the record structs it sets. The
`rag_msg_sprite` family (74 constant styles in category A, then 9 parameterised ones) is
one launcher with a style switch and is the smallest step there; the constant ring
families (46 IDs) follow. The harness stays as it is and covers them already.

## Restructuring: feature tables (2026-11-03)

Step 3 of the restructuring Patrick approved: turn the central feature lists into tables
whose rows the features own, assembled at one explicit composition point. This section
covers the effect-type registry and the two game packet dispatchers, and the
characterization test of the window-ID lists that has to come before those lists change.
Branch `purist-tables`, from `purist` at `62aabf8`. No behavior change.

### Effect types (`21fcd02`)

Before, `effects/effect_registry.c` listed every effect class: three row arrays
(`k_effects_first`, `k_effects_middle`, `k_effects_last`, 194 rows, 630 types), the
`hatEffectTable` loop and the EZ2STR file loop, called by `effect_mgr_construct`.

- **Row type** (`effects/effect_types.h`, new): `struct EffectTypeRow { first, last, create }`;
  the types `first..last` are created by `create`. A class's rows are a
  `struct EffectTypeTable { rows, count }`, written with `EFFECT_TYPE(type, create)`,
  `EFFECT_TYPES(first, last, create)` and `EFFECT_TYPE_TABLE(rows)`.
- **Owners**: each effect class, in its own file under `effects/skills/`, right after its
  constructor: a static `<class>_type_rows[]` and the table `<class>_types`, for example
  `heal_effect_types` in `heal_effect.c`. 47 tables in 46 class files; `Ez2StrEffect` also
  has `ez2_str_effect_late_types` (0x943-0x944). The two kinds of types known only at
  startup are functions of their class: `ez2_str_effect_add_hat_types` (`ez2str_effect.c`,
  one type per row of the Lua `hatEffectTable` from 0x951, a duplicate stops the
  registration) and `ez2_str_effect_ex_add_file_types` (`ez2str_effect_ex.c`, the EZ2STR
  file's types from 10000, a duplicate is skipped). All are declared in
  `effects/effect_allocator.h` next to the constructors.
- **Composition point**: `g_effectTypes` in `effects/effect_registry.c` (declared in the new
  `effect_registry.h`), a list of `struct EffectTypeSource` entries, each a table or an
  `add` function: the 44 fixed tables, then the headgear types and the EZ2STR file types,
  then the late tables `ez2_str_effect_late_types`, `change_size_effect_types` and
  `show_effect_types`. `app_init_managers` (`core/startup.c`) calls
  `effect_mgr_create(&g_effectTypes)`; `effect_mgr_construct` takes the list and
  registers it with two generic loops (`register_types`, `register_table`). `effect_mgr.c`
  no longer calls into the registry or names a class. The file went from 259 to 81 lines.
- **Order**: within a table the rows keep their former order; the tables are registered
  in the list's order. A missing table or row means its types are not effect-manager
  types: `effect_mgr_spawn` creates nothing and the actor code that asks
  `effect_mgr_is_registered` takes its RagEffect path.

The registration order is kept where it decides something, not literally. Before, the
fixed rows of all classes were interleaved (first, hats, middle, file, last); now they go
class by class. The tree of factories is sorted by type, and a type that is already
registered is refused, so order matters only for clashes. The 626 fixed types before the
file are pairwise distinct and all below the headgear range (checked: no type is listed
twice, the largest fixed type is 0x947), so registering them in any order, with the
headgear range before or after them, gives the same tree and never stops. What the order
does decide is kept: the file types come after every fixed table except the late ones and
after the headgear range, so a file type never replaces one (it is skipped) and a type the
file lists twice keeps its first row; the late tables come after the file, so a file type
they list stops the registration at that row, in the same row order as before.
`effect_mgr_create` fails in exactly the same cases, and the tree on success is the same.
The partial tree after a failed registration (only `effect_mgr_construct` called directly
shows it) differs only for clashes among the fixed tables, which the data does not have.
The list's comment documents these rules.

`tests/test_effect_factories.c` included `effect_registry.c` and defines stub
constructors, so it cannot compile the class files. It now passes `&g_effectTypes` to
`effect_mgr_construct` and `effect_mgr_create` (through two macros, so its reference mode
still builds), and includes `effect_type_tables.inc`, which the new
`tests/extract_effect_types.py` writes from the class files: for every entry of the list,
the row array and table, or the `add` function with the `#define` lines it uses, each
found exactly once. Its digest stays `d3673c01`; no assertion changed.

### Game packets (`7d53cd7`)

Before, `game_mode_dispatch_packet` (`game/packet_dispatch.c`) and
`game_mode_dispatch_packet2` (`game/packet_dispatch2.c`) were two switches: 347 and 281
case labels, no ID in both, the first's default calling the second, the second's default
resetting the connection's queues.

- **Row type** (`game/packet_rows.h`, new): each dispatcher's packets are one row list,
  `GAME_PACKET_ROWS_1(PACKET, CONSUMED)` and `GAME_PACKET_ROWS_2`, with `PACKET(id, call)`
  for the 600 handled IDs (in `call`, `self`, `pkt` and `packet_size` are the
  dispatcher's arguments) and `CONSUMED(id)` for the 28 IDs read and dropped without effect.
  The list is expanded three times: `GAME_PACKET_ADAPTER` makes one function per row with
  the one handler signature `GamePacketHandler (struct GameMode *self, unsigned char *pkt,
  size_t packet_size)`; `GAME_PACKET_ENTRY` fills a const array indexed by packet ID
  (`k_game_packets_1`, 0x85a entries, and `k_game_packets_2`, 0xbf2 entries; CONSUMED rows
  get `game_packet_consumed`); `GAME_PACKET_CASE` makes every ID a case label of a switch
  that is never called (`game_packet_rows_n_are_unique`), so an ID listed twice is a
  compile error, as in the switch (checked: C2196 for `CONSUMED(HEADER_PING)` repeated and
  for `CONSUMED(0x0187)`, the same value under another name).
- **Shims**: 62 case bodies that unpacked the packet into arguments or checked its size
  (7 in the first table, 55 in the second) are small static functions next to the packet
  structs, called from their row; every other row keeps its former statement.
- **Owners and order**: the rows stay in the dispatcher files, in the switch's order, which
  means nothing for the lookup; each row calls the handler of its feature.
- **Composition point**: the two arrays. `game_mode_dispatch_packet` returns on a NULL
  packet or one shorter than 2 bytes, looks the ID up in `k_game_packets_1`
  (`GAME_PACKET_HANDLER` accepts any int; `game_mode_poll_network_msg` passes IDs from
  0x8000 on as negative numbers) and passes an ID without a row to
  `game_mode_dispatch_packet2`, which does the same with `k_game_packets_2` and calls
  `connection_reset_queues` for an ID without a row.

The packet-dispatch test was not changed; its 1,179,800 cases give `d77aaadbb5fc63b7` as
before.

`login/login_dispatch.c` was left as it is. Its switch sits inside the receive loop, two
cases (`0x73`, `0x2eb`) end the loop with `return`, others write the packet buffer, and no
test covers it beyond the PIN-state packets of `test_login_pin_state`. A table there needs
a handler result (continue or stop) and a characterization test of every ID first.

### Window-ID lists: characterization test (`25a5fe3`)

The window IDs have four views: the factory (`ui_window_mgr_make_window` and
`wm_make_character`, `_hud`, `_trade`, `_social` in `make_window*.c`), the slots
(`ui_window_mgr_get_window`, `ui_window_mgr_is_untracked_id` and
`ui_window_mgr_post_quit1` in `window_slots.c`), the forget list (`forget_window.c`) and
the teardown (`ui_window_mgr_clear_slots` and `ui_window_mgr_close_all_windows` in
`teardown.c`). No test covered them. `tests/test_window_lists.c` (new, in `tests\build.bat`
after the packet dispatch test) compiles these eight files for real against recording
stubs: `tests/window_lists_stubs.c` (205 functions), written by
`tests/make_window_lists_stubs.py` from the preprocessed sources and the list of symbols
the files use, and a few hand-written ones (`client_lua_call`, `ui_window_create`,
`ui_window_mgr_add_window`, `rag_connection_send_packet`, `msg_str`, `ui_skin_path`,
`g_resMgr`). Window constructors return fake windows, zeroed blocks whose ops record every
call; `_construct` stubs make the caller's block one.

For every window ID 0-0x23f, with the stubs first returning NULL and 0 and then a zeroed
block and 1, on a freshly built manager each time, it records: `make_window` twice,
`get_window` and `is_untracked_id`; `post_quit1` after `make_window` and on a fresh
manager; `forget_window` for every window the factory made; `clear_slots`; and
`close_all_windows`. Then, for every window field of `UIWindowMgr`, whether
`forget_window` and `clear_slots` clear it. It records every stub call and op with its
arguments, the manager fields that changed, by name (`WL_MGR_FIELDS` lists all 277 fields
with their kind, so a field that moves only needs a new place in that list), the changed
words of `g_session1`, `g_navi` and `g_snapMgr`, the words of every fake window, and return
values. Pointers are names (`W3`, `D0`, `H12`, `mgr.item_info`); an unnamed pointer stops the
test. Result: 34,731 lines, digest `778535d4c6391e4b`, the same over repeated runs.
`--trace FILE` writes the lines.

Detection proof (each change made, the test run, the file restored with `git checkout`):

| Change | Lines | Digest |
|---|---|---|
| none | 34,731 | `778535d4c6391e4b` |
| `forget_window.c`: `minimap_zoom_wnd` dropped from the slot list | 34,726 | `6e0af9412308818f` |
| `teardown.c`: `clear_slots` no longer clears `login_wnd` | 34,725 | `9e2d658b0ee2a2ab` |
| `make_window.c`: login window 0x12e wide instead of 0x12d | 34,731 | `724bf596e2acd1df` |
| `window_slots.c`: `get_window(0x1b)` reads `chat_room_wnd` | 34,727 | `d93f97a1889a9ee0` |

What the four views do not agree on (recorded, not changed):

- `clear_slots` leaves `spell_list_wnd` (0x63), `prohibit_list_wnd` (0x67),
  `change_name_wnd` (0x80), `disband_guild_wnd` (0xd7),
  `combined_card_item_collection_comparison_wnd` (0xfa), `para_result_wnd` (0x100),
  `round_box_tooltip_wnd` (0x114), `capture_root` and `focus_drawn` set. The base
  destructor `ui_window_destruct` (`ui/controls/window_base.c`) calls `forget_window`,
  which clears all of them except `focus_drawn`, so after `close_all_windows` destroyed
  the windows they are NULL anyway. Known and harmless; left for the window table, which
  will make it explicit.
- `forget_window` does not clear `hot_key_wnd`, `quest_display_wnd`, `drag_plate`,
  `private_airplane_wnd`, `orphan_char_wnd`, `orphan_guild_wnd` or `focus_drawn`.
  `post_quit1` (0x9c, 0xcc) and `clear_slots` clear the first two; a hotkey or quest
  display window destroyed another way would leave its slot pointing at freed memory.
  `focus_drawn` is only compared, never dereferenced. Checked afterwards: a flight left
  `private_airplane_wnd` pointing at the destroyed window, so it is in the list now
  (`3df3cba`; `test_window_lists` is 34,733 lines, `39f36d8f0948c9bf` since). The other
  three have no destroy path that skips their slot and are left out; see the
  [UTF-8 review fixes](#foundation-fixes-from-the-utf-8-review-2026-11-03).
- `orphan_char_wnd` and `orphan_guild_wnd` are never set; only `clear_slots` writes them.
  Known and harmless; left for the window table.
  `private_airplane_wnd` is set outside the four views, in `ui/quests/navigation_cmd.c`.
- `post_quit1` queues `login_wnd` (0x3), `loading_wnd` (0x7), `item_wnd` (via 0x127, the
  refining window, whose factory also opens the item window 0x8) and `rpimage_wnd2` (0x145)
  without clearing the slot; the destructor's `forget_window` clears it when the quit list
  is processed.
- A second `make_window(0x3)` creates a second login window and overwrites `login_wnd`.
  Checked afterwards, not changed: the only caller, `login_screen`
  (`login/login_states.c`), opens it only when `ui_window_find(3)` returns no login
  window, and `login_wnd` is set only there and cleared only by `clear_slots` and by the
  destructor's `forget_window`; no other opener can pass 3 (the variable-ID openers are
  toggles called with other IDs). The original's MakeWindow (switch at `0x634acf`, case
  3 alone at `0x634ead`) creates the 0xd0-byte login window, stores it at `+0x1b4` and
  creates it 0x12d by 0x84 without looking for an existing one, as the port does.

The lists were not unified; see "Left for later".

### Dependency map per commit

Measured on each commit's own release build in its export (`build/depmap/<label>.txt`),
with `modules.py` cutting the 23 files of its `REGISTRY` set. The last column runs
`modules_drop.py`, which keeps one registry file in the graph, to see whether that file
still ties features into the knot.

| commit | change | knot files / lines | library files / lines | above the knot files / lines | clusters | registry files / lines | knot with the file kept in the graph |
|---|---|---|---|---|---|---|---|
| `62aabf8` | baseline | 260 / 92,725 | 469 / 82,054 | 309 / 105,253 | 118 | 23 / 8,712 | `effect_registry.c`: 305 / 105,151; `packet_dispatch.c`, `packet_dispatch2.c`: 260 / 92,725 |
| `21fcd02` | effect-type tables | 260 / 92,781 | 469 / 82,054 | 309 / 105,732 | 118 | 23 / 8,535 | `effect_registry.c`: 260 / 92,781 |
| `7d53cd7` | packet tables | 260 / 92,781 | 469 / 82,054 | 309 / 105,732 | 118 | 23 / 8,713 | `packet_dispatch.c`, `packet_dispatch2.c`: 260 / 92,781 |
| `25a5fe3` | window-list test | 260 / 92,781 | 469 / 82,054 | 309 / 105,732 | 118 | 23 / 8,713 | `window_slots.c`: 432 / 136,616 |

The baseline already differs from the end of step 2 (259 / 93,568) because `purist` moved
on. The registry files are cut in the main measurement, so the main numbers barely move;
the knot grew by 56 lines because `effect_mgr.c` (+25, the generic registration) and four
class files in the knot (`curse_of_cube_effect.c`, `emitter_effect.c`,
`energy_orb_effect.c`, `tetra_ball_effect.c`) got their rows. The change shows in the last
column: kept in the graph, the old `effect_registry.c` tied 45 effect class files into the
knot, because `effect_mgr_construct` called it; the new one is only reached from
`core/startup.c` and ties nothing in. The packet dispatchers never tied features into the
knot; `window_slots.c` still does (172 files), as the window lists are not converted yet.

### Validation

Every commit was validated in its own export (`git archive` into `build/snap/<label>`) with
`build/depmap/validate.ps1`: release build with the log grepped for `: warning ` and
`LNK`, the full suite `tests\build.bat`, the seven standalone suites, a comparison of every
`digest`/`passed` line with the baseline run, `git diff --check`, and the dependency-map
measurement (`measure.ps1`). Logs: `build/tables-<label>-build.log`, `-tests.log`,
`-<suite>.log` and `-digests.txt`; summary in `build/tables-validation.txt`.

- Every commit builds without compiler or linker warnings and passes the full suite and
  the seven standalone suites.
- All 259 digest and result lines of the baseline are unchanged in every commit, among them
  packet dispatch `d77aaadbb5fc63b7`, effect factories `d3673c01`, effect trace
  `cbdaaf30ef3127d6` and EZ2STR file choice `1b0232b840610959`. From `25a5fe3` on there is
  one more line, the window-list digest `778535d4c6391e4b`.
- `git diff --check` passed for every commit.

No live client was run.

### Left for later

- **Window table.** The test is in place; the unification is not done. A window table
  would hold one row per window ID: the slot (an offset into `UIWindowMgr`, or an index
  once the named slots become an array), whether ESC tracks it (`is_untracked_id`), the
  factory (a `WM_OPEN` description where the case is one, constructor, size, placement and
  info, as in 60 of the 188 factory cases; a function for the others), and the close kind (`close_slot`,
  `close_slot_stored`, or a function for the cases with side effects). `get_window`,
  `is_untracked_id`, `post_quit1`'s plain cases, `forget_window` and `clear_slots` would
  then read the table. The disagreements above have to become explicit columns (or be
  fixed on `main` first), or the digest changes. It is about 2,700 lines of
  `ui/wndmgr`, and it should be its own change. Done afterwards: see
  [Restructuring: window and object-type tables](#restructuring-window-and-object-type-tables-2026-11-03).
- **`login/login_dispatch.c`**: see above.
- **Packet rows per feature.** The rows now are data in one place per dispatcher. Moving
  them next to their handlers means about 100 handler files; the packet test stubs those
  handlers, so the rows would go into separate row files the test can compile. That fits
  step 4, when the game clusters become libraries and each takes its rows.
- **`core/object_type.c`**: a list of 326 ops tables with their type bits and class names,
  used by `object_as_type` and the diagnostics. The type bits and name could move into
  each class's ops table (a common first member, or a small info struct next to the ops),
  so `object_as_type` reads them from the object; `test_object_type` checks the result.
  Done afterwards as per-directory row tables; the bits could not go into the operation
  tables because of how `test_object_type` works (see [Restructuring: window and object-type tables](#restructuring-window-and-object-type-tables-2026-11-03)).
- **Startup and shutdown** (`core/app.c`, `startup.c`, `client_init.c`, `winmain.c`,
  `session/session_lifetime.c`): `app_init_managers` (`startup.c`) and
  `app_destroy_singletons` (`app.c`) are sequences of create and destroy calls with
  failure messages. They could become one table of
  `{name, create, destroy}` rows run forward at startup and backward at shutdown, which
  makes the order explicit and the two directions symmetric. That needs a test recording
  the order of every create and destroy first, as `test_com_lifecycle` does for COM.
  The test came afterwards (`test_startup_order`); the table did not, because shutdown is
  not the reverse of startup (see [Restructuring: window and object-type tables](#restructuring-window-and-object-type-tables-2026-11-03)).
  Done afterwards with a destroy rank per row: see
  [Restructuring: manager lifecycle table](#restructuring-manager-lifecycle-table-2026-11-04).

## Restructuring: window and object-type tables (2026-11-03)

The rest of step 3: the window-ID lists and the object-type list become tables whose rows the
features own, assembled at one composition point each, and the startup and shutdown order
gets its characterization test. Branch `purist-windows`, from `purist` at `615505c`. No
behavior change.

### Window table (`b7f7bbb`)

Before, the window IDs were listed four times: the factory switches
(`ui_window_mgr_make_window` and `wm_make_character`, `_hud`, `_trade`, `_social`; 177 IDs),
the switches of `ui_window_mgr_get_window` (26 IDs), `ui_window_mgr_is_untracked_id` (97) and
`ui_window_mgr_post_quit1` (147, 47 of them with side effects) in `window_slots.c`, the forget
list (`forget_window.c`, 135 named slots and the eight storage pages) and
`ui_window_mgr_clear_slots` (`teardown.c`, 133 named slots and the storage pages).
`ui_window_mgr_post_quit2` and `ui_window_mgr_make_window` held two more per-ID facts: which
slots a queued window is cleared from, and that the navigation window 0xcb goes to the end of
the ESC list.

- **Row type** (`ui/wndmgr/window_table.h`, new): `struct WmWindowRow { id, slot, flags, make,
  close, get, open }`. `id` is the window ID, or `WM_NO_ID` for a slot that no window ID opens.
  `slot` is `WM_SLOT(field)`, the offset of the `UIWindowMgr` field that holds the window, or
  `WM_NO_SLOT`; the named fields stay, so `test_window_lists` names them as before. `flags`:
  `WM_UNTRACKED` and `WM_UNTRACKED_UNLESS_SHOWN` (`is_untracked_id`), `WM_GET_SLOT`
  (`get_window` reads the slot instead of `windows_by_id`), `WM_FORGOTTEN_BY_QUIT`
  (`post_quit2` clears the slot), `WM_ESC_LAST`, and the two quirk flags below. `make` is the
  factory; `make_window` registers what it returns, as the switches did after their `break`.
  `close` is what `post_quit1` does: `WM_CLOSE_WINDOW` (queue whatever `get_window` returns,
  the default), `WM_CLOSE_SLOT`, `WM_CLOSE_STORED`, `WM_CLOSE_UNCOUNTED`, or a function. `get`
  replaces `get_window`'s answer (only 0x12d, the first party plate). `open` describes the 58
  factories that were one `WM_OPEN` call (constructor, size, placement, info); their `make` is
  the generic `wm_open_row`. The rows are written with `WM_ROW`, `WM_ROW_OPEN`,
  `WM_ROW_OPEN_INFO`, `WM_ROW_OPEN_INFO1`, `WM_ROW_GET` and `WM_ROW_SLOT`.
- **Owners**: the cluster files, each with its rows at the end, after the factories and closes
  they name: `make_window_login.c` (new: the login, character-selection, change-name and
  Lua-described windows that `make_window.c`'s switch made; 14 rows), `make_window_character.c`
  (33), `make_window_hud.c` (43), `make_window_trade.c` (48) and `make_window_social.c` (44). A
  row goes with the code that creates the window or sets the slot: the whisper ID 0x43 and
  `private_airplane_wnd` are with the HUD rows, 0x12d and `drag_plate` with the social rows.
  The rows nobody owns are in `window_registry.c` (15): the twelve IDs of removed windows that
  the untracked list still named (0x32, 0x73, 0x7b, 0x7c, 0x88, 0x90, 0x91, 0x9a, 0xb4, 0xba,
  0xbd, 0x157), `UI_ID_NONE`, and `orphan_char_wnd` and `orphan_guild_wnd`. 197 rows: 193 IDs
  and 4 slots without an ID.
- **Factories and closes**: every factory case and every `post_quit1` case with side effects
  is now a static function next to its row, the case body moved unchanged (`break` became
  `return wnd` or `return closed`; the old `close_slot` and `close_slot_stored` are
  `wm_close_slot` and `wm_close_slot_stored` in `window_slots.c`). 93 factory and 38 close
  functions; cases shared by several IDs stay one function (the storage pages 0x92-0x98 and
  0x135, the guild tabs 0x3c-0x42, 0x15/0x8f, 0x35/0xcf, 0xd5/0xd7, 0x101/0x102, 0x132/0x13a,
  the Lua windows 0x9c/0x9f/0xb5). Six existing helpers are the `make` of their rows directly
  (`make_basic_info`, `make_select_char`, `make_make_char`, `make_store_page`,
  `make_refining_wnd`, `make_guild_tab`); the four that took only `self` gained the unused
  `window_id`. `close_drag_plate` and `send_packet_id` moved from `window_slots.c` to
  `make_window_social.c`, their only users.
- **Composition point**: `g_windowTable` in `ui/wndmgr/window_registry.c`, the list of the six
  tables. `client_install_hooks()` installs it with `ui_window_mgr_set_window_table`
  (`window_table.c`), which builds an index by ID and a list of all rows, and stops the client
  (`fatal_error`) on a second row for an ID, an ID outside 0..0x23f, slot flags without a slot,
  or two rows that give one slot different slot flags (0x35 and 0xcf share
  `party_setting_wnd`). `client_install_hooks()` is the first call of `client_init()`, which is
  the first call of `WinMain`, so no window is opened before the table is installed.
- **Order and missing rows**: lookups go by ID and the slot loops only compare and clear, so
  the order of the tables and rows decides nothing. A missing row means what the switches'
  `default` meant: `make_window` creates nothing, `get_window` looks only in `windows_by_id`,
  `is_untracked_id` returns 0, `post_quit1` queues whatever `get_window` returns, and no slot is
  forgotten or cleared for the ID. `window_table.h` documents the owners, the order and a
  missing row next to the row type; each cluster's row array points to it.
- **The window manager core** reads the rows: `get_window`, `is_untracked_id`,
  `touch_window_id`, `post_quit2` and `post_quit1` (`window_slots.c`), `forget_window`,
  `clear_slots` (`teardown.c`) and `make_window` (`make_window.c`, now only the generic part).
  The manager's own references (`capture`, `capture_root`, `focus_edit`, `modal_wnd`,
  `mouse_wnd`, `active_top_wnd`) stay explicit there. None of these files names a feature
  window or a window ID any more, except `close_all_but_chat` (below).

The disagreements between the lists are flags, recorded as found, not fixed:

| Flag or close | Rows | What it keeps |
|---|---|---|
| `WM_KEPT_BY_CLEAR` | 0x63, 0x67, 0x80, 0xd7, 0xfa, 0x100, 0x114 | `clear_slots` leaves the slot; the window's destructor forgets it |
| `WM_KEPT_BY_FORGET` | 0x9c (`hot_key_wnd`), 0xcc (`quest_display_wnd`), `drag_plate`, `orphan_char_wnd`, `orphan_guild_wnd` | `forget_window` leaves the slot |
| `WM_CLOSE_UNCOUNTED` | 0x4f, 0x50, 0x6b, 0x6f | `post_quit1` closes the slot but returns 0, so the ID stays in the ESC list |
| `WM_CLOSE_WINDOW` with a slot | 0x3, 0x7, 0x145 (and 0x7f, 0x115, cleared by `WM_FORGOTTEN_BY_QUIT`) | `post_quit1` queues the window and leaves the slot for the destructor |
| no factory, a close | 0xab | `post_quit1` still treats this removed ID like the Lua windows |
| `WM_UNTRACKED_UNLESS_SHOWN` | 0x91 | tracked only while a visible window has the ID, which none has |

`capture_root` and `focus_drawn` stay out of `clear_slots`, and `focus_drawn` out of
`forget_window`, as before; they are manager fields, commented where they are cleared. Left
as it is: `close_all_but_chat` (`teardown.c`) keeps the chat window, whisper windows (by their
ID 0x43) and chat tabs and saves the memorial-dungeon window's position. That is chat code and
needs no column; `window_table.h` says so. `make_guild_tab`, `make_store_page` and the
store-page closes still compute their slot from the ID as they did, instead of reading the
row's slot.

Tests changed, only to follow the moved code; no assertion changed:

- `test_window_lists` compiles `make_window_login.c`, `window_table.c` and `window_registry.c`
  too and installs `g_windowTable` as `client_install_hooks()` does. 34,733 lines, digest
  `39f36d8f0948c9bf`, unchanged; its stubs file did not change.
- `test_forget_window`, `test_airplane_window_slot` and `test_window_id_list` compiled one
  view each (the forget list, or the switches extracted from `window_slots.c`). They now read
  the real rows through `tests/extract_window_rows.py`, which copies every table of
  `g_windowTable`'s list into `window_rows.inc` (each found exactly once, as
  `extract_effect_types.py` does), compiled with `tests/window_rows_data.h`, whose row macros
  keep the ID, slot and flags and drop the functions. `test_window_id_list` makes every row's
  factory its controlled factory, which now returns the window for `make_window` to register;
  the stubs only the login factories needed left `window_id_list_unreached.c`.
- `test_basic_info_restore` extracts `make_basic_info` with its new signature.

Detection proof (each change made, `test_window_lists` run, the file restored):

| Change | Result |
|---|---|
| none | 34,733 lines, `39f36d8f0948c9bf` |
| 0x63 without `WM_KEPT_BY_CLEAR` | 34,739 lines, `3e75c648f8c91571` |
| 0x4f `WM_CLOSE_SLOT` instead of `WM_CLOSE_UNCOUNTED` | 34,733 lines, `6fcb11b0633e98c9` |
| `private_airplane_wnd` row dropped | 34,729 lines, `554593f302611a83` |
| 0x91 `WM_UNTRACKED` instead of `WM_UNTRACKED_UNLESS_SHOWN` | 34,724 lines, `440ef31b52ada1b2` |
| 0x157 listed twice | `fatal_error`: a window ID with two rows |
| 0xcf with `WM_KEPT_BY_CLEAR` (its slot is 0x35's) | `fatal_error`: two rows give one slot different slot flags |
| 0x1c without `WM_GET_SLOT` | not detected: 34,733 lines, `39f36d8f0948c9bf` |

The last row is a gap of the characterization test, not of the table: the test reads
`get_window` only right after `make_window`, when the slot and `windows_by_id` hold the same
window. So `WM_GET_SLOT` and the untracked flags were also checked once against the old code
(a scratch program, not in the suite): the old `get_window` and `is_untracked_id`, extracted
from `615505c`, and the new ones on the installed real rows, for IDs -2 to 0x2ff, with every
window field of the manager and every `windows_by_id` entry holding a different window, the
party plate list empty and not, and 0x91 absent, hidden and shown; and only 0xcb has
`WM_ESC_LAST`. All agree; with 0x1c's `WM_GET_SLOT` removed it reports `get_window(0x1c)`.

### Window-manager handlers (`fc6fa1a`)

Kept in the map, `make_window.c` and `forget_window.c` still tied 163 files each into the knot
after the window table: `ui_window_mgr_make_window` began by dropping a party plate that is
being dragged while a hotkey opens a window (`plates.c`, which reaches the session), and
`ui_window_mgr_forget_window` ended with `actors_forget_window`. Both are now members of
`g_windowMgrHandlers` (`ui/wndmgr/wndmgr_handlers.h`), installed by `client_install_hooks()`:

| Handler | Installed with | NULL |
|---|---|---|
| `g_windowMgrHandlers.drop_dragged_plate` | `ui_window_mgr_drop_plate_for_hot_key` (`ui/wndmgr/plate_hooks.c`, new) | the plate stays |
| `g_windowMgrHandlers.forget_window` | `actors_forget_window` (`actor/actor_process.c`) | the actors keep their references |

The plate step moved unchanged into `plate_hooks.c`, a file of its own so that
`test_window_lists` and `test_window_id_list` compile it and install it as startup does and
reach the same stubs in the same order; `test_forget_window` and `test_airplane_window_slot`
install their `actors_forget_window` stubs. All four tests also compile `wndmgr_handlers.c`.

### Object types (`139d17d`)

Before, `core/object_type.c` listed all 326 classes the type queries know: the operation
table, the `ObjectType` bits `object_as_type` answers (the original's `dynamic_cast`) and the
name `object_type_name` returns.

- **Row type** (`core/object_type.h`): `struct ObjectTypeRow { table, types, name }`, a
  directory's rows `struct ObjectTypeRows` (`OBJECT_TYPE_ROWS(rows)`), the installed list
  `struct ObjectTypeTable`, and `OBJECT_TYPE(kind)` for the bits (formerly the file's own
  `TYPE`).
- **Owners**: the 15 directories that define the classes, each in its `*_object_types.c`, the
  rows in their former order: `ui/controls` (57), `ui/items` (54), `ui/character` (46),
  `ui/quests` (39), `ui/social` (37), `ui/shops` (33), `ui/chat` (28), `actor` (11, with the
  NPC actor `CCharacterNpc` of `core/character_npc.c`), `resources` (6), `ui/mail` (5),
  `ui/hotkeys` (4), `effects/skills` (3), and one each in `ui/render`, `ui/map` and
  `ui/worldmap`. Basenames are unique because `build.bat` puts all objects in one folder.
- **Composition point**: `g_objectTypes` in `core/object_type_registry.c` (declared with the
  15 tables in `object_type_registry.h`), installed by `client_install_hooks()` with
  `object_types_install`, which stops the client on an operation table with two rows.
  `object_type.c` is only the lookup (51 lines, from 367).
- **Order and missing rows**: with every table once, the first match is the only match, so the
  order decides nothing. A table without a row gets NULL from `object_as_type` for every type
  and "?" from `object_type_name`, as the classes the client no longer has always did.

The bits did not go into the operation tables, the first option in the plan.
`test_object_type` compiles the list with `type_query_fixture.h` forced in, which defines
every operation-table name as a vtable copied from the original client, and compares
`object_as_type` with that runtime's `__RTDynamicCast`. That needs the bits keyed by table
identity; inside the C operation structs the fixture could not replace them. The test now
compiles the 15 row files and the registry under the same fixture into `object_type_test\`,
installs `g_objectTypes` and defines a `fatal_error` stub. Its counts are unchanged: 472
concrete classes, 139 retired identities, 2,717 prior-runtime comparisons, 205 accepted casts,
326 names, 306 window names. Detection: a row dropped (`CMergeItemWnd`) or a bit removed
(`CBlurPC` without `PC`) fails its cast comparison; `UIWindow` listed twice stops at install.

### Startup and shutdown: characterization test (`d5d292c`) and design

`tests/test_startup_order.c` (new, in `tests\build.bat` after the window-list test) extracts
`app_init_managers` and `init_failed` (`core/startup.c`) and `app_destroy_singletons`
(`core/app.c`) with `tests/extract_startup_order.py` and runs them against stubs that record
each step. It checks the 23 startup steps when everything succeeds; for each of the 14 checked
creates, that its failure ends the sequence right there, returns its own text and undoes
nothing; that a failure of the four ignored results (`ez2_str_effect_mgr_load`,
`town_info_create`, `champion_monster_create`, `map_sky_mgr_load`) changes nothing; that the two
loads that only warn (`town_info_load`, `champion_monster_load`) add their message box; and the
19 shutdown steps. A swapped pair of destroys fails it (checked).

The table was not built. Shutdown is not the reverse of startup, so one `{name, create,
destroy}` table run forwards and backwards would change the shutdown order:

| Startup (`app_init_managers`) | Shutdown (`app_destroy_singletons`) |
|---|---|
| EZ2STR effect manager (instance, load) | skill-action manager (created in `client_init`) |
| effect manager | EZ2STR effect manager |
| particle pools | map sky |
| time scheduler | guild manager |
| town info (create, load) | player names |
| icon sheet | whisper block |
| renewed quests (create, load) | account data |
| champion monsters (create, load) | character data |
| map properties | Rodex (`rodex_destroy_instance`) |
| Rodex | map properties |
| account data | champion monsters |
| character data | renewed quests |
| whisper block | icon sheet |
| player names | town info |
| guild manager | time scheduler |
| adventurer agency | particle pools |
| join-party manager | effect manager |
| map sky (instance, load) | adventurer agency |
| | join-party manager |

Run backwards, the startup list would destroy the join-party manager and the adventurer
agency first and character data before account data, and it has no skill-action manager. Also, when a create
fails, `WinMain` shows the text and returns without calling `app_destroy_singletons`, so the
managers created before it are not destroyed (the process ends anyway).

Design for the table, for Patrick to decide:

- One list `g_clientManagers` of `{name, create, destroy, destroy_rank, on_failure}` rows,
  where `on_failure` is stop with the message (the 14 checked creates), warn with a message
  box (the two loads) or ignore. Startup runs the list forwards; shutdown runs the rows in
  `destroy_rank` order, which reproduces today's order exactly. A later, separate change could
  make shutdown the exact reverse once someone has checked that no destroy depends on another
  manager still existing.
- Whether a failed startup should destroy what it created (in reverse) is a behavior change
  and stays out of the conversion.
- `client_init` already pairs each construction with an `atexit` release, which C runs in
  reverse; its rows could join the same list, but its order interleaves constructions,
  `atexit` registrations and table loads, and it would need its own characterization first.
  `WinMain`'s own shutdown steps (window manager, connection, renderer, sound) are another
  sequence with the same question.

Patrick approved the table. It is done in
[Restructuring: manager lifecycle table](#restructuring-manager-lifecycle-table-2026-11-04); the cleanup after a failed startup
and the `client_init` and `WinMain` sequences stay as they are.

### Dependency map per commit

Measured on each commit's own release build in its export (`build/depmap/<label>.txt`).
`modules.py` cuts the 23 files of its former `REGISTRY` set plus the new files that list a
whole feature set: `make_window_login.c` and `window_registry.c` from `b7f7bbb`, and
`object_type_registry.c` and the 15 `*_object_types.c` from `139d17d` (each row file only
reaches the classes of its own directory; kept in the graph, a row file would join its
directory's files into one cluster). `make_window.c`, `window_slots.c`, `forget_window.c`,
`teardown.c` and `object_type.c` stay in the set so that the rows compare with the earlier
tables; the last column keeps each of them in the graph with `modules_drop.py`.

| commit | change | knot files / lines | library files / lines | above the knot files / lines | clusters | registry files / lines | knot with the file kept in the graph |
|---|---|---|---|---|---|---|---|
| `615505c` | baseline | 260 / 92,782 | 469 / 82,054 | 309 / 105,732 | 118 | 23 / 8,715 | `window_slots.c`: 432 / 136,617; `forget_window.c`: 423 / 133,014; `make_window.c`: 449 / 144,380; `teardown.c`: 260 / 92,782; all four: 460 / 148,155; `object_type.c`: 594 / 205,686 |
| `b7f7bbb` | window table | 260 / 92,782 | 470 / 82,116 | 309 / 105,732 | 118 | 25 / 9,238 | `window_slots.c`: 260 / 92,782; `forget_window.c`: 423 / 132,959; `make_window.c`: 423 / 133,085; `teardown.c`: 260 / 92,782; all four: 425 / 133,302 |
| `fc6fa1a` | window-manager handlers | 260 / 92,782 | 470 / 82,116 | 310 / 105,743 | 119 | 25 / 9,244 | each of the four: 260 / 92,782; all four: 260 / 92,782 (all in the library layer) |
| `139d17d` | object-type rows | 260 / 92,782 | 470 / 82,116 | 310 / 105,743 | 119 | 41 / 9,433 | `object_type.c`: 260 / 92,782; each of the four window files: 260 / 92,782; all five: 260 / 92,782 (all in the library layer) |
| `d5d292c` | startup-order test | 260 / 92,782 | 470 / 82,116 | 310 / 105,743 | 119 | 41 / 9,433 | as `139d17d` |

The main columns do not move, because the files that changed are cut in them; the change
shows in the last column. Kept in the graph, `window_slots.c` tied 172 files into the knot
before the window table, because every window opener calls `get_window` or `post_quit1` and
`post_quit1` called into the item, storage, navigation, memorial-dungeon, mail and session
code; after it, `window_slots.c` reaches only the containers, `mem_alloc`, `ui_window_find`, the
snap manager, `game_current_base_mode` (`world/mode_context.c`) and the window table, and ties
nothing in. `make_window.c` and `forget_window.c` kept 163 files
each in the knot until the two handlers of `fc6fa1a`. `core/object_type.c` tied 334 files into
the knot (594 against 260): the widgets, the actors and the renderer that ask a type reached
every class through it; as the lookup over an installed table it reaches only `core/fatal.c`.
With all five kept, they lie in the library layer, below the knot. The library layer gains
`window_table.c` (62 lines) at `b7f7bbb`; above the knot gains `plate_hooks.c` (11 lines, its
own cluster) at `fc6fa1a`.

### Validation

Every commit was validated with `build/depmap/validate.ps1`: release build with the log
grepped for `: warning ` and `LNK`, the full suite `tests\build.bat`, the seven standalone
suites, every `digest`/`passed` line of the suite against the baseline run of `615505c`,
`git diff --check` against the parent commit, and the dependency map (`measure.ps1`). The
window-table commit was validated in the worktree, clean at that commit; the others each in
their own export (`git archive` into `build/snap/<label>`), so that work could go on in the
worktree meanwhile. Logs: `build/windows-<label>-build.log`, `-tests.log`,
`-<suite>.log`, `-digests.txt` and `-digest-diff.txt`; summary in
`build/windows-validation.txt`; baseline in `build/windows-baseline-*.log` and
`-digests.txt`.

- Every commit builds without compiler or linker warnings and passes the full suite and the
  seven standalone suites.
- All 260 digest and result lines of the baseline are unchanged in every commit, among them
  the window lists `39f36d8f0948c9bf` (34,733 lines), packet dispatch `d77aaadbb5fc63b7`,
  effect factories `d3673c01`, effect trace `cbdaaf30ef3127d6` and EZ2STR file choice
  `1b0232b840610959`. From `d5d292c` on there is one more line, the startup-order result.
  The two result lines of `test_object_type` start with "PASS:", so the comparison does not
  collect them; they are the same in every commit's log (472 classes, 139 retired, 2,717
  comparisons, 205 accepted casts, 326 names, 306 window names), and the test asserts those
  counts.
- `git diff --check` passed for every commit.
- The first export runs of `fc6fa1a`, `139d17d` and `d5d292c` ran the build and the full suite
  correctly but not the standalone suites and the map: a loop variable of the validation
  script had the name of the export path (PowerShell names ignore case). Their remaining steps
  were run again on the same exports with the fixed script (`validate.ps1 -Rest`); those are
  the results above.

No live client was run.

### Found, not fixed

- `set_hwnd_to_game_mon` (`ui/mail/rodex_box.c`) is the Rodex manager's destroy: it destroys
  `g_rodex` and clears it. The name is a leftover of the decompilation; nothing else calls it.
  Renamed `rodex_destroy_instance` afterwards (`9f11693`; `main` `5572da7`), with the same
  code.
- The characterization test does not tell `WM_GET_SLOT` from a `windows_by_id` lookup for most
  IDs (see the detection table); the one-off check covered it for this change. Closed
  afterwards (`0fc7891`): a lookup case records `get_window` and `is_untracked_id` for every
  ID in both environments, with every window field and every `windows_by_id` entry holding
  its own window (W0+n and W1+id), without and with a mini-party plate. The trace only gains
  lines, 2,314 after each environment's field cases (2 x 2 x 576 records, 4 notes, 4
  `ui_window_find(145)` calls and their 2 `-> D0`): 37,047 lines, `2232167ff470caa7`.
  Exactly the 25 `WM_GET_SLOT` rows answer from their slot. `WM_GET_SLOT` dropped from 0x58
  gives `417e398825759e7b` (before: unchanged `39f36d8f0948c9bf`), from 0x1c
  `5cad0c5769dff71b`, and `get_window` ignoring it everywhere `636123f4647ddbf1`.
- `make_guild_tab`, `make_store_page` and the store-page closes keep their own ID-to-slot
  mapping next to the rows' slot column; they agree today and could read the row.
- `test_window_id_list` still gives 0xaa an "already open" factory result, but 0xaa has no row,
  so `make_window` returns NULL without calling it; the test's outcome is the same as before.
  Fixed afterwards (`8f7f567`): the case uses 0xe5, the navigation help, a tracked row
  without a slot whose real factory refuses while the window is open, and counts the
  refusal; removing 0xe5's row now fails the test. The same commit lists the
  ignored-results scenario in `test_startup_order.c`'s header comment.
- On `purist-portable`, `make_window*.c`, `window_slots.c`, `app.c`, `startup.c` and
  `client_init.c` carry small portability edits (`INT_PTR` to `intptr_t`, `tree_less_uint` to
  `tree_less_ptr`) inside code this branch moved; merging needs them re-applied in the moved
  functions.

### Step 4: plan (not started)

Step 4 extracts the feature clusters into folders and static libraries. Proposed order:

1. **The layers below the knot first**, since they never reach it (470 files, 82,116 lines at
   `b7f7bbb`): `base` (`std`, `json`, the base files of `core`: memory, UTF-8, text, `fatal`,
   `msg_str`), `platform` (`platform`, `fileio`), `engine` (`render`, `resources`) and
   `widgets` (`ui/controls`, and the window manager core: `window_slots.c`, `forget_window.c`,
   `teardown.c`, `make_window.c`, `window_table.c`). Their links need no code change, so
   they prove the checks; the include check may still find headers from above, which the
   map does not measure.
2. **Then the feature clusters above the knot.** Every cluster above it (all 119 at
   `d5d292c`) is used from outside only by registry files, so each can become a library
   that only the composition points reference. Each `ui/<feature>` library takes its
   `*_object_types.c` and its share of the window rows: `make_window_<cluster>.c` would be
   split by feature directory as the object-type rows already are. The largest clusters at
   `615505c` are `ui/social` guild (10 files, 4,446 lines), `ui/items` (6, 2,754),
   `ui/quests` achievements (5, 2,614), `ui/character` skills (4, 2,526) and `ui/social`
   messenger (4, 1,851).
3. **The game packet handlers** (`game` packets: 149 files, 55,655 lines above the knot) after
   the packet rows move next to their handlers, which the feature-tables section left for
   this step.
4. **The knot** (260 files: actor, session, `effects/rag`, world, the game core) stays one
   library until more hooks cut it.

The executable keeps only the composition points: `winmain.c`, `client_init.c`, `startup.c`,
`app.c`, the packet dispatchers and the registries (`effect_registry.c`, `window_registry.c`,
`object_type_registry.c`). They alone may reference every library.

**Checks in `build.bat`.** A table `tools/libraries.txt` names each library, its path prefixes
and the libraries it may use. Two checks run in every build:

- *Includes*, before compiling: `tools/check_includes.py` reads every `#include "..."` under
  `src` and fails the build when a file includes a header of a library it may not use. `/I`
  cannot enforce this, because all includes are written from the one root `src`.
- *Links*, after compiling: each library is archived with `lib` from its objects, and the
  executable links the libraries plus the composition objects. The upward check reuses the
  dependency map's input: `dumpbin /symbols` of each library's objects (as `symbols.bat` does,
  only for changed objects), and a script that fails when an undefined symbol of a library is
  defined only in a library above it or in the executable. This is the map's edge test turned
  into a gate, and it catches data references too, except communal globals without an
  initializer (`int g_x;`), which have no defining object.

Folders: the library table can work with today's paths, so libraries and checks can come
before any file moves. Moving files into `src/<library>/` is then a separate, scripted step.

**Effect on cherry-picks from `main`.** Without moves, cherry-picks keep applying by path. The
code restructured in step 3 conflicts wherever `main` changes it: a fix to a factory case, a
`post_quit1` case or a row of `object_type.c` on `main` has to be re-applied by hand in the
moved function or row file (the bodies are textually the same, so it is a copy). With moves,
git's rename detection follows a file only when the move commit is a pure `git mv` (no edits
in the same commit); files that were split, like `make_window.c`, do not follow.

**Effect on `purist-portable`.** It is 139 commits ahead of its merge base with `purist`
(`d725b49`) and touches 979 files under `src`, among them `make_window*.c`, `window_slots.c`,
`app.c`, `startup.c` and `client_init.c` (small portability edits inside code this branch
moved). Before step 4 moves files, the two lines should meet: merge `purist-portable` into
`purist` (or rebase it) once, resolving the moved-code conflicts by re-applying its edits, and
then run the move script on the common base. If they stay apart, the same move script run on
both branches keeps their paths equal, so later cherry-picks between them still apply by path.

## Merge of purist-portable (2026-11-04)

Decision: Patrick decided to bring `purist-portable` and `purist` together before step 4 moves any
files (see *Effect on `purist-portable`* in [Step 4: plan](#step-4-plan-not-started)). The port is
now part of `purist`; [docs/portability.md](portability.md) describes it.

### What was merged

`purist-portable` at `d327397` into `purist` at `aed1b3a`, merge base `d725b49`, on the branch
`purist-merge`: merge commit `3660577`, and `db5afd9` for the COM lifecycle test (below).

- `purist-portable`: 139 commits, 1,294 files. A platform layer so that the client builds and runs
  on 64-bit Linux, macOS and Android with Windows x86 as the reference: CMake (`CMakeLists.txt`,
  `cmake/`), SDL3 window and application layer, one input module (`core/user_input.c`,
  `core/window_events.c`, `core/touch.c`), platform window and display types, one text interface
  with GDI and FreeType backends, an OpenGL backend, Miles on SDL3 audio, glTF Granny models,
  stb JPEG instead of IJL, files, threads, sockets, clock, settings, desktop and crash reports
  behind `platform/`, pointer-width fixes (`intptr_t` message arguments, `tree_less_ptr`, ...),
  standard C instead of MSVC string and format functions, `build.bat x64` and `build.bat freetype`.
- `purist`: 102 commits, 1,238 files: the removals, the langtype 20 lock, the UTF-8 switch, the
  restructuring steps 1-3, effects as data and the fixes recorded above.
- 597 files were changed on both sides. `git merge` stopped with 328 conflicted files: 239 with
  content conflicts (420 conflict regions; 471 hunks in diff3 form) and 89 modify/delete
  conflicts; 771 changed files merged cleanly and portable added 190. The merged tree differs from
  `purist` in 1,195 files (+61,475/-5,998 lines) and from `purist-portable` in 1,256 files
  (+41,543/-71,106).

### How the conflicts were resolved

- Everything purist removed stays removed. All 89 modify/delete conflicts were purist removals
  (OTP, billing, protection leftovers, third and expanded classes, the eight optional systems,
  unreachable windows, the embedded fonts, the candidate window); portable's edits in them went
  with the files, among them its rename `embedded_fonts.c` -> `embedded_fonts_win32.c`. Where a
  purist deletion inside a conflict was a move, portable's edit was applied at the new place (list
  below).
- Text stepping, searching, fitting and conversions take purist's UTF-8 code (`core/utf8.h`,
  `strstr`, `CP_UTF8`) over portable's code-page wrappers (`platform_code_page_next/prev/find/
  byte_type`, `CODE_PAGE_SYSTEM`); where client text still crosses into UTF-16 in shared code it goes
  through `platform_code_page_to_utf16/from_utf16(CODE_PAGE_UTF8, ...)` with `uint16_t` buffers.
- Portable's mechanical replacements are applied on top of purist's version of each line:
  `platform_ticks_ms`, `int_min`/`int_max`, `snprintf`, `cstr_*`, `intptr_t` message arguments,
  `PlatformWindow`, the filesystem, settings, thread, net, clock and desktop calls. Purist code that
  merged without a conflict but used what portable replaced was converted the same way (six
  `_snprintf_s(..., _TRUNCATE, ...)` in purist's name fixes, `ShellExecuteA` in `url_link.c`,
  `fileio/resource_path.c`, `__int64` in `game_actor.h`, `wchar_t` UTF-16 units in `core/utf8.c`
  and `navi/navi_text_lines.c`).
- Call sites use portable's platform API, and the platform's Windows side uses purist's UTF-8
  wrappers, so Windows keeps purist's behaviour: `platform_message_box` calls
  `win_utf8_message_box`, the settings store (`platform/settings_win32.c`) calls the
  `win_utf8_reg_*` functions, and `platform_clipboard_set/get` are purist's `CF_UNICODETEXT` code
  (`language_copy_text` calls `platform_clipboard_set`).

### Resolutions that needed a design

Each entry: the file(s), purist's intent, portable's intent, the result.

- **Text** (`render/textdc.c`, `ctextdc.c`, `textdc.h`, `text_backend.h`, `text_gdi.c`,
  `text_freetype.c`, `fonts.h`, `fonts.c`, `fonts_ft.c`, `text_unicode.c`).
  - Purist: text is UTF-8, split into runs by Unicode script (`render/text_runs.c`); a run whose
    script the selected face lacks is drawn in the script's first installed Windows font (Hangul:
    Gulim, then Malgun Gothic; Han, kana, Thai, symbols, emoji in their own lists), on the selected
    face's baseline; W font calls and UTF-16 face tables; an extent cache keyed by the font handle
    (`text_dc_measure_font`); italic widths with surrogate pairs measured whole; the langtype-20
    face choice.
  - Portable: one text interface with GDI and FreeType backends (`text_backend.h`); the shared
    `textdc.c` split runs by charset tag and decoded each with the charset's code page.
  - Result: the shared `textdc.c`/`ctextdc.c` are purist's algorithm written against the backend
    interface (`FontHandle`, `text_dc_use_font`, `text_dc_measure_run`, `text_dc_draw_run`). Two
    backend calls were added for the fallback, `text_dc_font_covers` and `text_dc_font_ascent`: on
    GDI they are purist's `font_cache_covers`/`font_cache_ascent` on the text DC's `HDC`, on FreeType
    `font_covers` (the script's sample character in the font's own face; emoji always fall back)
    and `font_ascent`. Both backends implement `font_cache_get_script_font`: GDI is purist's
    `fonts.c` unchanged apart from portable's `MulDiv(x, 72, 72)` -> `x`; FreeType asks Fontconfig
    (or the font-folder scan) for the same Windows families in the same order, followed by the free
    fonts that stand in for them (Hangul: Gulim, Malgun Gothic, NanumGothic, Noto Sans CJK KR, ...;
    emoji: Segoe UI Emoji, Segoe UI Symbol, Noto Color Emoji, ...), with a language per script and
    the em made equal to the cell for the CJK scripts, as portable already did for CJK face
    substitutes. Purist's surrogate-pair italic measurement moved into `text_gdi.c`'s
    `text_dc_measure_run`. The FreeType glyph cache is now keyed by code point (pages up to
    U+10FFFF, surrogate pairs joined), so non-BMP text such as emoji reaches its fallback font
    instead of drawing two boxes. `fonts_ft.c` reads purist's UTF-16 face table and converts the
    family name to UTF-8. Portable's `text_dc_draw_in_charset` (one run in a charset's code page) became
    `text_dc_draw_plain` (one UTF-8 run in the selected face), used by FreeType's
    `text_dc_text_out_plain`; GDI keeps `TextOutA` there, as purist does. `textdc.c` uses portable's
    `CHARSET_*` constants (with a new `CHARSET_DEFAULT`) and `LANGUAGE_ID_TURKISH` instead of the
    Win32 macros, so it builds everywhere.
  - Same UTF-8 input on both backends: `platform/code_page_posix.c` now converts `CODE_PAGE_UTF8`
    itself with `core/utf8.c` and writes U+FFFD for invalid bytes and unpaired surrogates, as
    Windows' `MultiByteToWideChar`/`WideCharToMultiByte(CP_UTF8, 0, ...)` do, instead of failing in
    iconv (portable's notes said such a run disappeared). `platform_code_page_is_valid(UTF-8)` is
    `utf8_valid`.
- **Input** (`platform/language.c`, `language.h`, new `language_win32.c`/`language_posix.c`
  contents, `core/wndproc.c`, `core/window_events.c`, `core/app_win32.c`, `core/app_sdl.c`).
  - Purist: a Unicode window (`RegisterClassW`, `CreateWindowExW`, `PeekMessageW`,
    `DispatchMessageW`, `DefWindowProcW`); `WM_CHAR` UTF-16 units paired with `utf16_feed`;
    `CF_UNICODETEXT` paste and copy (`language_copy_text`); IME composition and result read at full
    length in UTF-16, the cursor converted to a byte offset; the IME draws its own candidate list
    next to the composition (`language_place_candidates`, `WM_IME_SETCONTEXT` clears only
    `ISC_SHOWUICOMPOSITIONWINDOW`); the candidate window `0x33`, the key-stroke window `0x55`,
    Zhuyin and the per-object code page removed.
  - Portable: the shared `language.c` on `platform_*` calls and the IME side split into
    `language_win32.c`/`language_posix.c`; keys, characters and the pointer go through
    `core/user_input.c` and `core/window_events.c` from the window procedure and from SDL; window
    creation and the message pump in `core/app_win32.c`/`app_sdl.c`.
  - Result: `language.c` is purist's editor logic on portable's platform calls
    (`platform_key_state`, `platform_clipboard_*`, `platform_ime_cancel`, `platform_user_language`
    with the `LANGUAGE_ID_*` constants, now public in `language.h`). `language_win32.c` is purist's
    IME code (no candidate list). `language_posix.c` keeps portable's `language_set_composition` and
    `language_commit`, now taking SDL's UTF-8 as it is (`app_sdl.c` no longer converts it to a client
    code page), cutting a long composition on a character boundary and converting SDL's cursor
    (characters) to a byte offset; its `language_place_candidates` keeps the place, and on desktop
    SDL `app_sdl.c` hands it to `SDL_SetTextInputArea` so that the IME opens its list below the
    composition's line (Android keeps its edit-box area for the on-screen keyboard).
    `window_events.c` lost `window_events_language_changed` (the candidate and key-stroke windows)
    and the service-type-15 test of the key handler; `wndproc.c` keeps portable's routing with
    purist's `DefWindowProcW` and IME context; `app_win32.c` got purist's W window class and W
    message pump without the Zhuyin reading-key hook.
- **Message boxes, registry and clipboard** (`platform/window_win32.c`, `settings_win32.c`,
  `window_sdl.c`, `win_utf8.c`, `error_message.c` and about 60 callers).
  - Purist: `win_utf8_message_box` and `win_utf8_reg_*` (UTF-8 to the W calls, CP949 for text that
    is not UTF-8) at every call site.
  - Portable: `platform_message_box`, the settings store `platform_settings_*`
    (`client_settings.ini` off Windows) and `platform_clipboard_*` at every call site.
  - Result: call sites use portable's API; its Windows side calls purist's wrappers
    (`platform_message_box` -> `win_utf8_message_box`, the settings store -> `win_utf8_reg_*`), and
    `platform_clipboard_set/get` are purist's `CF_UNICODETEXT` code. The SDL side shows message box
    text that is not UTF-8 as CP949, as `win_utf8.c` does, and passes clipboard text as UTF-8 (U+FFFD
    for invalid bytes). `win_utf8.c` is wrapped in `#ifdef _WIN32`. Purist's moved `error_msg`
    (`platform/error_message.c`) calls `platform_message_box`.
- **Startup** (`core/winmain.c`, `core/app.c`, `core/startup.c`, `core/client_init.c`).
  - Purist: `client_install_hooks()` first in `client_init()`; the startup and shutdown order
    recorded by `test_startup_order`; `rodex_destroy_instance`.
  - Portable: `WinMain` became `client_run(instance)`, shared with the POSIX `main` (and Android's),
    which first enters the game folder; `platform_desktop_start` in place of COM; fonts start with
    `fonts_init`.
  - Result: both; `client_run` calls `client_init()`, so the hooks and tables are installed before
    anything else on every platform. Nothing in purist's hook or table installation is
    Windows-specific. `test_startup_order` (extracted from `startup.c` and `app.c`) passes
    unchanged.
- **Moved code** carrying portable edits: `make_window_social.c`, `make_window_trade.c` (the
  window-table factories: `(int)(INT_PTR)` -> `(intptr_t)`); `messages.c` (purist's
  handler-table `ui_window_mgr_send_msg_` with portable's `intptr_t` signature); `ui/chat/whisper.c`
  (purist's moves of `place_whisper`, `open_whisper` and `ui_window_mgr_add_whisper_chat_to_whisper_wnd`
  with portable's casts from `messages.c` and `whisper_map.c`); `ui/items/item_collection.c`
  (`item_collection_show_link`) and `ui/controls/item_tag_button.c` (casts);
  `render/c3ddevice.c` (purist moved `borderless_rect` there; portable replaced `monitor_rects` with
  `platform_monitor_rects` and `HMONITOR` with `PlatformDisplay`; `monitor_rects` is gone);
  `actor/game_actor.c`/`.h` (`game_actor_set_counter`, moved from `rolling_cutter.c`, with
  `int64_t`). Portable's other edits in `make_window.c`, `make_window_character.c`,
  `window_slots.c` and `client_init.c` merged cleanly into purist's moved code.
- **Removed code that portable had moved**: `platform/code_page.c` (portable's new home of
  `platform/charset.c`) is deleted, since purist removed `get_charset_from_lang`,
  `get_code_page_from_charset` and `code_page_from_lang` with the charset tags;
  `platform/embedded_fonts_posix.c` is deleted with the embedded fonts.
- **`fileio/resource_path.c`** (purist's CP949 lookup of UTF-8 file names): the strict
  `MultiByteToWideChar(MB_ERR_INVALID_CHARS)` and `WideCharToMultiByte(WC_NO_BEST_FIT_CHARS, lossy)`
  became `utf8_valid` and `platform_code_page_to_utf16/from_utf16` with a CP949 round trip (a
  character CP949 lacks does not come back as itself), so it builds and behaves the same off
  Windows; `test_file_io` passes unchanged.
- **UI and game files** (about 160 files): purist's UTF-8 stepping, fitting, ellipsis and wrapping
  over portable's code-page wrappers; the guild-member sort through
  `platform_code_page_to_utf16(CODE_PAGE_UTF8, ...)`; the login, server-select and character-select
  settings through the settings store with purist's `CLIENT_LANGTYPE` default; `url_link.c` (purist)
  calls `platform_open_with_desktop` with purist's http/https check; `diag/perf.c` keeps purist's
  `RAGEXE_OVERLAY` gate with portable's counters; `private_airplane.c` keeps purist's deliberate
  16-byte `strncpy`; `actor/types.h` portable's `struct GameObject *` mark fields without the marks
  purist removed; `contents/connection.c` portable's socket layer (the `ws2_32` hook block purist
  had only touched for its message boxes is gone). The x86 layout asserts that purist had re-valued
  (`gauge.h`, `gauges.c`, `skill_tree.h`, `status_wnd.c`, `balloon.c`) are dropped, as portable
  dropped such runtime asserts everywhere.
- **Build**: `build.bat` keeps purist's `bin2c` step and gives it portable's per-variant response
  file; one `build\gen` serves every variant. CMake embeds `data/effects/effects.json` the same way
  (`cmake/EffectData.cmake` builds and runs `tools/bin2c.c`; a cross build, Android, runs
  `cmake/bin2c.cmake`, which writes the same file, checked byte for byte), the platform library
  starts from `src/core/utf8.c` (and `win_utf8.c` on Windows), and the client's glob picks up every
  source purist added; `test_text_freetype` gets `text_runs.c`.
- **Tests**: message-box stand-ins intercept `platform_message_box` (portable's way) instead of
  `win_utf8_message_box`; registry stand-ins replace `win_utf8_reg_*` under `settings_win32.c`;
  `test_text_render` runs purist's cases on portable's backend split (it includes `text_gdi.c` and
  mocks `platform_code_page_to_utf16`); `test_language_input` includes `language_win32.c` and
  `window_win32.c`; `test_canvas_owner` stubs the two font-cache calls `text_gdi.c` now makes; the
  CTest `test_platform_window` and `test_text_freetype` were rewritten for UTF-8 (clipboard round
  trips with U+FFFD; Hangul in UTF-8; script runs and the Hangul fallback instead of the removed
  charset tags); `test_platform_code_page` lost its charset checks. Purist's tests that extract
  production code follow the platform calls in it: `test_startup_order`'s message-box stand-in is
  `platform_message_box`, `test_map_enter_packet`'s clock stand-in also replaces
  `platform_ticks_ms`, `test_perf_overlay` replaces `getenv`, `platform_perf_counter`,
  `platform_perf_frequency` and `platform_key_state`, `test_url_link` compiles
  `platform/desktop_win32.c` under its `ShellExecuteA` stand-in (so it still checks the exact shell
  call), `test_monster_skill_info` gets `core/cstr.h`, and `extract_status_wnd_info.py` accepts the
  `intptr_t` message function. Build lines gained `win_utf8.c`, `utf8.c`, `code_page_win32.c`,
  `clock_win32.c`, `cstr.c`, `memory.c`, `winmm.lib` or `ole32.lib` where the merged sources need
  them. No expectation of a `tests\build.bat` test changed; only the three CTest programs above
  check new expectations, the merged UTF-8 behaviour.

### Validation

Logs are in the worktree's `build/` (`merge-*`, `wsl-*`); the baselines were run on clean exports
(`git archive`) of `aed1b3a` (`build/base`, logs `base-*`) and `d327397` (`build/port`, logs
`port-*`, `wsl-port-*`).

- **Windows x86, the reference** (`build.bat`, MSVC 2022, on `db5afd9`): the release build has
  no compiler or linker warnings (`merge-final-build.log`, grepped for `: warning ` and `LNK`).
  The full `tests\build.bat` passes in 379 s (`merge-final-tests.log`), and so do the seven
  standalone suites, each with the result line of `aed1b3a` (`merge-final-<suite>.log`). On
  `3660577` alone `build_com_lifecycle.bat` fails as it does on `d327397`; `db5afd9` fixes the
  test (see below). `git diff --check` passes for every commit.
- **Windows x64** (`build.bat x64`): builds without warnings (`merge-build-x64.log`), as
  portable's own `d327397` does (`port-build-x64.log`); no pointer-truncation warning anywhere,
  purist's new code included.
- **Windows, FreeType text** (`build.bat freetype`): builds without warnings
  (`merge-build-ft.log`; `port-build-ft.log` for `d327397`).
- **CMake with MSVC** (`-A Win32` and `-A x64`, Debug, separate build directories): configure
  and build without warnings; CTest passes 20 of 20 (Win32) and 19 of 19 (x64; the crash-dump test
  is Win32 only), the same tests and results as on `d327397` (`merge-cmake-<arch>-*.log`,
  `port-cmake-<arch>-*.log`; the logs are UTF-16).
- **Linux** (WSL Ubuntu 24.04, GCC 13.3, Ninja, `-DRAGE_CLIENT=ON`): the whole client (`ragexe`)
  and the scene harness (`ragexe_scenes`) of `db5afd9` build, and CTest passes 24 of 24
  (`wsl-merge-*.log`); `d327397` gives the same 24 of 24 (`wsl-port-*.log`). Among them,
  `text_freetype` drew Arial through DejaVu Sans and its Hangul runs through the Hangul fallback
  (NanumGothic here), `platform_window` checked the UTF-8 clipboard, and `gfx_gl` and `miles_sdl`
  ran on SDL's offscreen and dummy drivers. This WSL has no X11, Wayland or ALSA development
  packages and no sudo, so SDL was configured with `SDL_UNIX_CONSOLE_BUILD=ON` for both trees; that
  checks the build and the headless tests, not a desktop window. GCC reports 1,685 warnings against
  portable's 1,753, none for pointer and integer casts; the ones that are new are purist code that
  MSVC does not report (below).
- **macOS and Android** cannot be built here. The Android build is the only cross build, so it is
  the only one that writes the effect data with `cmake/bin2c.cmake`.
- No live client was run, and the scene captures (`tests/scenes`, which need game data) were not
  run.

### Digest and behaviour changes

All result lines of `tests\build.bat` (349 on `aed1b3a`, 261 of them with a digest or "passed";
352 and 263 merged) were compared, in order, with a run of the same suite on a clean export of
`aed1b3a` (`base-tests.log`; the lines in `merge-final-results-base.txt` and `-merged.txt`). They
are identical, among them packet dispatch
`d77aaadbb5fc63b7` (1,179,800 cases), effect factories `d3673c01`, effect trace `cbdaaf30ef3127d6`,
EZ2STR file choice `1b0232b840610959`, window lists `2232167ff470caa7` (37,047 lines), the
object-type counts (472 classes, 139 retired, 2,717 comparisons, 205 accepted casts, 326 names, 306
window names), the startup order (23 startup steps, 19 shutdown steps) and every other suite
digest, except for these lines, all from portable's own, intended changes:

- `test_renderer_storage` prints 16 `render <scenario> <draws>` trace digests; the draw counts are
  the same, the digests differ. Portable sorts the render queues by a texture serial instead of the
  texture pointer, breaks ties by submission order, and compares like with like in the multitexture
  sort (`d0a4589`, *Scene captures* in the portability notes). The same test on `d327397` gives
  exactly the merged digests (`port-renderer_storage.log`).
- The IJL ownership test is replaced by portable's stb JPEG codec and bitmap tests (`JPEG:` and
  `JPEG bitmap:` lines); JPEG bytes come from stb now.
- `test_math` describes its wider coverage ("transforms, ..., picking and projection results"),
  `test_touch` is new, and `test_crash_dump` prints its context sizes. The only other differing
  line is the run time that `tests/test_map_name_migration.py` prints.

Behaviour on Windows x86 is purist's. Beyond portable's own documented changes (stb JPEG and
quality 90 screenshots, draw order independent of heap addresses, `rand()` kept to 15 bits where the
client divides by that range, the icon menu's bottom border, settings writes through temporary
files), the merge itself changes nothing there. Off Windows, the merge changes portable's behaviour
where purist's intents now apply: the client's text is UTF-8 (SDL text, message boxes, the
clipboard and the FreeType backend take it as it is), the FreeType backend draws scripts the
selected face lacks in purist's per-script fallback order, invalid UTF-8 draws as U+FFFD instead of
dropping the run, non-BMP characters reach a font, and the IME's candidate list is placed below the
composition.

### Found, not fixed

- `actor/actor_sendmsg.c`, case `0x20`: `ui_window_send_msg(w, NULL, 0x22, (int)par1, ...)` cuts a
  pointer-sized argument to `int` on 64-bit builds. Both sides have the line; portable's pointer
  sweep missed it. Fixed afterwards (`ca17c78`).
- `platform_open_with_desktop` on Windows (`platform/desktop_win32.c`) passes the URL to
  `ShellExecuteA`, so a URL with non-ASCII characters (the client's text is UTF-8) is misread;
  purist's own `ShellExecuteA` call had the same limit. Fixed afterwards (`3ca656a`).
- GCC reports purist code that MSVC at `/W1` does not: unused variables `job`
  (`actor/pc_layers.c`), `tint_blue`, `tint_green`, `tint_grey` (`effects/skills/skill.c`), `effect`
  (`game/skill_cast.c`, `game/skill_cast_look.c`) and `i` (`game/skill_fail.c`), leftovers of the
  removals, and possible truncation of names in `ui/chat/balloon.c` and `game/packets_social.c`.
  The variables are gone (`aed6618`); neither truncation can cut a name (below).
- `session_get_latest_achievements` (`session/session_achievements.c`) starts its search with
  `int next = (int)(INT_PTR)second`, the address of an output parameter cut to `int`; both sides,
  unchanged. Fixed afterwards (`b2681a9`).
- `tests\build_com_lifecycle.bat` failed on `purist-portable` itself (its extractor and its
  compile); fixed in `db5afd9`.
- Not verified here: the desktop SDL placement of the IME candidate list (no interactive IME), the
  FreeType Hangul fallback on Windows (`build.bat freetype` builds, the scene captures need game
  data), and the Android build's effect data path (`cmake/bin2c.cmake` was checked byte for byte on
  Windows, not in an NDK build).

### For the purist-portable session

- `purist-merge` carries the merge; once `purist` points at it, `purist-portable` can fast-forward
  with `git merge --ff-only purist` from its own worktree. The older feature branches `port/audio`
  (`422434e`) and `port/text2` (`487106c`) predate the merge and will conflict with it where they
  touch text or input.
- Client text is UTF-8 on every platform: do not convert client text with `CODE_PAGE_SYSTEM`,
  `CODE_PAGE_THREAD` or a language code page; `g_codePage` is `CODE_PAGE_UTF8`, and CP949 is only
  for resource names (`RESOURCE_NAME_CODE_PAGE`). `platform/code_page.c`, `get_charset_from_lang`,
  `get_code_page_from_charset` and `code_page_from_lang` are gone, and `struct Language` has no
  candidates, code page, keyboard language, input method or folder.
- Text API: `text_dc_measure_in_charset` is `text_dc_measure_font(self, out, text, len, font)`,
  `text_dc_draw_in_charset` is `text_dc_draw_plain(self, x, y, text, len)`, and a backend implements
  `text_dc_font_covers`, `text_dc_font_ascent` and `font_cache_get_script_font` as well.
  `window_events_language_changed` is gone (the candidate and key-stroke windows were removed).
- On Windows, `platform_message_box`, the settings store and the clipboard are purist's UTF-8 W
  calls (`platform/win_utf8.c`); tests that stub the registry stub `win_utf8_reg_*` under
  `settings_win32.c` and link `win_utf8.c`.
- CMake embeds `data/effects/effects.json` (`cmake/EffectData.cmake`); purist's new tests
  (`test_utf8`, `test_text_runs`, `test_effect_trace`, `test_window_lists`, ...) run only from
  `tests\build.bat`, not from CTest.

## Fixes after the merge of purist-portable (2026-11-04)

Decision: Patrick asked for the bugs the merge found to be fixed here, with a regression test
each, and for each to be checked on `main` and fixed there too where it is real. None is real on
`main` (below), so these are purist-only; the
[upstream notes](upstream/README.md#purist-only-fixes-after-the-merge-of-purist-portable-2026-11-04)
list them without patches.

### What each fix does

- Chat room title (`ca17c78`). Actor message `0x20` (`actor/actor_sendmsg.c`) carries the chat
  room an actor shows in `par1` (a `struct ChatRoomInfo *`, from `packets_chat.c`) and passes it as
  `val1` of window message `0x22`, which `chat_room.c` reads as that pointer. It was cut to `int`;
  it is `intptr_t` now. On a 64-bit build a room above 4 GB reached the window as another address.
- Latest achievements (`b2681a9`). `session_get_latest_achievements` started the second-latest
  search at `(int)(INT_PTR)second`. That start is read only while no latest achievement is known,
  which a completion time of -1 or less leaves so; a 32-bit stack address (positive) and -1 give
  the same answer there, so -1 keeps Win32's results for every input, while a 64-bit address cut
  to `int` could be negative and take a completion at -5 as the second latest.
- Desktop links (`3ca656a`). `platform_open_with_desktop` (`platform/desktop_win32.c`) calls the new
  `win_utf8_shell_execute` (`platform/win_utf8.c`), which converts its text like the other
  wrappers there (UTF-8, else CP949) and calls `ShellExecuteW`; text that cannot be converted for
  lack of memory is not opened. `url_link.c` keeps its http:// and https:// check.
- Unused locals (`aed6618`): `job` (`pc_render_robe`), `tint_blue`, `tint_green`, `tint_grey`
  (`skill_on_process`), `effect` (`skill_cast_target_look`, `skill_cast_play_cast_look`) and `i`
  (`game_mode_ack_to_use_skill`). GCC now reports 1,671 warnings, 14 fewer (each was counted for
  `ragexe` and `ragexe_scenes`).
- The two possible truncations GCC reports cannot cut a name, so they are left: the couple name
  (`game_mode_zc_couple_name`) is at most the 24-byte field in a 24-byte buffer, and a character
  name is at most 23 bytes with the server's NUL; the name balloon's lines are
  "[title] name (party)" and "guild [position]" in 259 characters, with name, party, guild and
  position from 24-byte packet fields and the title from the title table (longest 19 bytes): at
  most 71 characters.

### The pointer sweep

Every `(int)` cast of `par1`-`par4`, `val1`-`val4` and the mode-message arguments `a`-`d` (about
80) carries a number except case `0x20`. An x64 compile of every client source at `/W4`
(syntax only, `build/merge-fixes-sweep-x64-w4.log`) reports no pointer truncation or extension
(C4311, C4302, C4312); its 420 narrowings of `intptr_t` or `__int64` to `int`
(`build/merge-fixes-sweep-narrowings.txt`) are numbers handed to `int` parameters, and every
widening back to a pointer (`(T *)(intptr_t)x`, `MSG_PTR`) starts from a 64-bit value. Eight
casts through a pointer-sized type to `int` were found: two hashes, the achievements start fixed
above, and five in the three places listed below.

### Checked on `main`

- Case `0x20` and the achievements start: the same lines; on Win32 they change nothing (`int`
  message arguments; a 32-bit stack address compares like -1).
- `url_link.c` calls `ShellExecuteA` with the client's CP949 text, which is the Windows code page
  of a Korean system, as in the original client.
- The unused locals are used on `main` (the effects the removals took out are there).
- The couple name and the balloon lines are the same and cannot cut a name either; `main`'s
  `sprintf_s` cannot overflow at 71 characters.

### Found, not fixed

- Actor message `0x78` (`able_send_msg.c`) passes the address of its effect type, not the type,
  to `effect_mgr_is_registered` and `effect_mgr_has_effect`. The original does the same (`push edi`
  with the pointer at `0x751b2e`, then `mov dword ptr [edi], -1`). Its only sender
  (`body_layer.c`) passes no sender and ignores the answer, so the result is the same either way.
- `game_mode_msg_use_skill_to_ground` (`game/msg_skills.c`) passes the address of its
  `GroundSkillUse` as the skill ID of the homunculus and mercenary checks; the original does too
  (`0x790718`, then reads the structure through the same pointer at `0x790762`). On Win32 an
  address is never a homunculus or mercenary skill ID, so a mounted player cannot use any ground
  skill there; on 64-bit only the low 32 bits are compared. Fixing it (`use->skill_id`) would let a
  mounted player use those skills, a change from the original.
- `prim_standing_square_render` (`effects/rag/prim_kinds_129_134.c`) starts the blue channel at
  the low byte of the `view` pointer; a square without a shade is drawn with it. Not a width
  problem (the low byte is the same on 64-bit), but the colour depends on an address.
- `tests/test_com_lifecycle.c` still defines a `ShellExecuteA` macro that nothing uses now; the file
  belongs to the startup-table work and was left alone.

### Tests added

- `test_chat_room_title` extracts case `0x20` and `sm_window` and runs them against recording
  stubs: a room value with bits above bit 31 on 64-bit builds must reach the window whole.
- `test_latest_achievements` runs the extracted function on the real tree, with `second` at
  `0x12f0000000` on 64-bit builds (a negative low word): the two latest of four completions, and a
  lone completion at -5 that must leave both results alone.
- Both run in CTest, through the new `rage_extracted_test` helper that runs the extractor at
  build time, and in `tests\build.bat`. On 64-bit builds both fail on the previous code (MSVC x64
  and GCC on Linux); on Win32 both pass either way.
- `test_url_link` runs the real `win_utf8_shell_execute` with `ShellExecuteW` stubbed and
  `ShellExecuteA` trapped, and adds a Hangul URL and one with an accented letter and a character
  outside the BMP, whose UTF-16 must be exact: 38 cases. It fails on the previous
  `desktop_win32.c`. `test_tag_text_viewer` stubs `win_utf8_shell_execute`.

### Validation

- Windows x86: `build.bat` without warnings (`build/merge-fixes-build.log`); the full suite
  passed (`build/merge-fixes-full-tests.log`: packet dispatch `d77aaadbb5fc63b7`, effect trace
  `cbdaaf30ef3127d6`, window lists `2232167ff470caa7`), and the seven standalone suites
  (`build/merge-fixes-<suite>.log`). `git diff --check` passed for every commit.
- Windows x64: `build.bat x64` without warnings (`build/merge-fixes-build-x64.log`).
- CMake with MSVC x64 (Debug): configure and build without warnings; CTest passes 21 of 21, the 19
  of the merge and the two new ones (`build/merge-fixes-cmake-x64-*.log`).
- Linux (WSL Ubuntu 24.04, GCC 13.3, Ninja, `-DRAGE_CLIENT=ON`, SDL with
  `SDL_UNIX_CONSOLE_BUILD=ON` from the merge check's sources): `ragexe` and `ragexe_scenes` build
  with 1,671 warnings; CTest passes 26 of 26 with `SDL_VIDEO_DRIVER=offscreen` (`gfx_gl` skips
  without a video driver); `build/wsl-merge-fixes-*.log`. The two new tests, built with GCC against
  the merge's sources, fail (`build/wsl-merge-fixes-oldcheck.log`).

No live client was run.

## Restructuring: manager lifecycle table (2026-11-04)

The startup and shutdown table that [Startup and shutdown: characterization test and
design](#startup-and-shutdown-characterization-test-d5d292c-and-design) proposed, which Patrick
approved. Branch `purist-lifecycle`, from `purist` at `657555f`. No behavior change.

### The table (`346b667`)

- **Row type** (`core/manager_table.h`, new): `struct ManagerRow { name, create, destroy,
  destroy_rank, on_failure }` and the list `struct ManagerTable` (`MANAGER_TABLE(rows)`). `create`
  is `int (*)(void)`, nonzero on success, or NULL for a manager that is created elsewhere.
  `destroy` is `void (*)(void)`, or NULL for nothing to destroy. `destroy_rank` places the
  destroy at shutdown, lowest first; gaps are allowed. `on_failure` says what a create that
  returns 0 does: `MANAGER_STOP(text)` ends startup with `text`, `MANAGER_WARN(text)` shows
  `text` in a message box and goes on, `MANAGER_IGNORE` goes on. The header documents the
  columns, the order, the failure behaviour and a missing row.
- **Runner** (`core/manager_table.c`, new): `manager_table_create` runs the rows in list order
  and returns the `MANAGER_STOP` row whose create failed, or NULL; a warn row's failure calls
  `platform_message_box(NULL, text, "Error", 0)`, the call the two loads made.
  `manager_table_destroy` calls the destroys from the lowest rank to the highest. Both first
  check the table and stop the client with `fatal_error`, naming the rows, on two rows with a
  destroy and the same rank (one of them would not be destroyed) or a stop or warn row without
  its text. The runner reaches only `fatal_error`, `platform_message_box` and `snprintf`.
- **Composition point**: `g_clientManagers` in `core/client_managers.c` (new, declared in
  `client_managers.h`), the one list of the 19 rows, in create order. Five creates already have
  the row's form and are in the rows directly (`account_user_data_create`,
  `char_user_data_create`, `whisper_block_create`, `player_names_create`,
  `adventurer_agency_create`); 13 static adapters next to the list give the others that form:
  they pass `&g_effectTypes`, turn a `char` or `unsigned char` result into `int`, load the
  EZ2STR and map-sky instances, and do the three create-and-load pairs as before. Town info
  and champion monsters call the create, do not look at its result and return the load's, so
  the row warns when the file does not load. Renewed quests return 0 when the create fails,
  before the load, and 1 after the load, which has no result.
- **Entry points unchanged**: `app_init_managers` (`core/startup.c`) runs
  `manager_table_create(&g_clientManagers)` and builds the `InitResult` as before (`init_failed`
  with the stop row's text, or "good"); `app_destroy_singletons` (`core/app.c`) runs
  `manager_table_destroy(&g_clientManagers)`. `client_run` (`core/winmain.c`), which `WinMain`,
  the POSIX `main` and Android's `main` call, calls both at the same places as before, so every
  platform runs the same table; `winmain.c`, `app_win32.c` and `app_sdl.c` did not change, and
  the table has no platform-specific code. `startup.c` and `app.c` include `client_managers.h`
  instead of the 19 manager headers. The comments in `effects/effect_registry.h` and
  `effect_types.h` that said `app_init_managers` passes `g_effectTypes` now name the
  effect-manager row.

The rows, in create order:

| # | name | create | on failure | destroy | rank |
|---|---|---|---|---|---|
| 1 | skill-action manager | none: `client_init` (`skill_actions_register`) | | `skill_action_mgr_destroy_instance` | 1 |
| 2 | EZ2STR effect manager | `ez2_str_effect_mgr_load(ez2str_effect_mgr_instance())` | ignore | `ez2str_effect_mgr_destroy` | 2 |
| 3 | effect manager | `effect_mgr_create(&g_effectTypes)` | stop: `CEffectMgr::CreateInstance()` | `effect_mgr_destroy_instance` | 17 |
| 4 | particle pools | `effect_memory_create` | stop: `Particle pools` | `effect_memory_destroy` | 16 |
| 5 | time scheduler | `time_scheduler_create` | stop: `CTimeScheduler::CreateInstance()` | `time_scheduler_destroy` | 15 |
| 6 | town info | `town_info_create`, then `town_info_load` | warn: `TownInfo file Init` | `town_info_destroy` | 14 |
| 7 | icon sheet | `icon_sheet_create` | stop: `CCooltimerMgr::CreateInstance()` | `icon_sheet_destroy` | 13 |
| 8 | renewed quests | `renew_quest_create`, then `renew_quest_load` | stop: `CRenewQuestMgr::CreateInstance()` | `renew_quest_destroy` | 12 |
| 9 | champion monsters | `champion_monster_create`, then `champion_monster_load` | warn: `ChampionMonster file Init` | `champion_monster_destroy` | 11 |
| 10 | map properties | `map_property_mgr_create` | stop: `CMapPropertyMgr::CreateInstance()` | `map_property_mgr_release` | 10 |
| 11 | Rodex | `rodex_create_instance` | stop: `CRodexSystemMgr::CreateInstance()` | `rodex_destroy_instance` | 9 |
| 12 | account data | `account_user_data_create` | stop: `CAccountLinkedUserDataMgr::CreateInstance()` | `account_user_data_destroy` | 7 |
| 13 | character data | `char_user_data_create` | stop: `CCharacterLinkedUserDataMgr::CreateInstance()` | `char_user_data_destroy` | 8 |
| 14 | whisper block | `whisper_block_create` | stop: `CWhisperBlockMgr::CreateInstance()` | `whisper_block_destroy` | 6 |
| 15 | player names | `player_names_create` | stop: `CPlayerNameDictionary::CreateInstance()` | `player_names_destroy` | 5 |
| 16 | guild manager | `guild_mgr_create` | stop: `CGuildMgr::CreateInstance()` | `guild_mgr_destroy` | 4 |
| 17 | adventurer agency | `adventurer_agency_create` | stop: `CAdventurerAgencyMgr::CreateInstance()` | `adventurer_agency_destroy` | 18 |
| 18 | join-party manager | `ui_request_join_party_mgr_create` | stop: `UIRequestJoinPartyMgr::CreateInstance()` | `ui_request_join_party_mgr_delete` | 19 |
| 19 | map sky | `map_sky_mgr_load(map_sky_mgr_instance())` | ignore | `map_sky_mgr_destroy` | 3 |

One row per manager, not per call: each of the 19 shutdown steps is one row's destroy, and the
create column holds the calls that `app_init_managers` made for that manager. A row per call
(23 steps) would have given the three loads rows without a destroy. In the test's terms the
table is the same as the two functions: 23 startup steps, 14 checked creates (the stop rows), 4
ignored results (the EZ2STR and map-sky loads, and the town-info and champion-monster creates
inside their adapters), 2 warning loads, 19 shutdown steps.

- **Order and failure**: the list order is the create order, `destroy_rank` the destroy order,
  which is today's and not the reverse of the create order. When a stop row fails,
  `app_init_managers` returns its text and `client_run` shows it and returns 1 without calling
  `app_destroy_singletons`, so the managers created before it are not destroyed (the process
  ends anyway). That is unchanged and recorded in `manager_table.h`; the same holds when
  `app_init_3d`, `c3d_device_create_renderer` or `connection_startup` fail after the managers,
  where `client_run` also skips `sound_mgr_destroy`, `platform_timer_period_end` and
  `crash_report_uninstall`.
- **A missing row**: startup does not create the manager and shutdown does not destroy it. Code
  that uses it then finds no instance, or one that its `*_instance()` function makes on first
  use (the EZ2STR, map-sky and skill-action managers have such functions) and that nothing
  destroys before the process ends.

### Left outside the table

Three more sequences in the files that the plan named (`client_init.c`, `winmain.c`,
`session/session_lifetime.c`) stay as they are; the design already set the first two apart:

- **`client_init`** (`core/client_init.c`): 24 `atexit` registrations, most right after the
  construction they undo, interleaved with 11 initializations that register no release (the
  message strings, fonts, texture cache, the two skin names, five item and table loaders and
  `skill_actions_register`). The releases run in reverse at process exit, after `client_run`
  has returned. No test records that order, so it needs its own characterization first.
- **`client_run`'s own steps** (`core/winmain.c`): the crash report, the sound manager, the
  window, the 3D device, the renderer and the connection at startup, and the window manager,
  connection, renderer, sound, timer period and crash report around `app_destroy_singletons` at
  shutdown. Each failure has its own result and cleanup label (`com_done` up to the window,
  `done` from the managers on), which `test_com_lifecycle` records; they are not rows of one
  shape.
- **The session's managers** (`session/session_lifetime.c`): `session_construct` creates the
  user hotkeys, the barter markets, pet auto-feeding and achievements (each with a message box
  on failure) and the private airplane, attendance and map-info managers (results not looked
  at); `session_destruct` destroys them, except map info, which `client_run` destroys with
  `map_info_destroy`. They belong to the lifetime of `g_session1`, which `client_init`
  constructs and releases at exit, not to `app_init_managers`.

Making shutdown the exact reverse of startup is still a separate change: it needs each destroy
checked for a dependency on a manager that would then be gone already.

### Tests changed

- `tests/test_startup_order.c` did not change (byte for byte). Its stubs replace every create
  and destroy and the message box; the table and the runner are now compiled from the sources.
- `tests/extract_startup_order.py` extracts the same three functions (`init_failed` and
  `app_init_managers` from `startup.c`, `app_destroy_singletons` from `app.c`) and starts
  `startup_order.inc` with `#include "core/client_managers.h"`.
- `tests/build_startup_order.bat` compiles `src/core/client_managers.c` and
  `src/core/manager_table.c` with the test, at `/W3 /WX` as before.
- `test_com_lifecycle` did not change; it extracts `client_run`'s tail from `winmain.c`, which
  did not change.

The test's header comment still says it characterizes the order "before startup and shutdown
become one table"; it was left so that the file stays unchanged.

Detection proof (each change made in `client_managers.c` at `346b667`,
`tests\build_startup_order.bat` run, the file restored byte for byte;
`build/lifecycle-mutations.log`):

| Change | Result |
|---|---|
| none | passed: 23 startup steps, 19 shutdown steps |
| account-data and character-data rows swapped (create order) | `all succeed: step 14 is char_user_data_create, expected account_user_data_create` |
| their ranks 7 and 8 swapped (destroy order) | `destroy: step 6 is char_user_data_destroy, expected account_user_data_destroy` |
| Rodex row dropped | `all succeed: 22 steps, expected 23` |
| skill-action row dropped (no create) | `destroy: 18 steps, expected 19` |
| map properties `MANAGER_IGNORE` instead of stop | `map_property_mgr_create: 23 steps, expected 13` |
| town info `MANAGER_IGNORE` instead of warn | `town_info_load: 23 steps, expected 24` |
| map sky `MANAGER_STOP("x")` instead of ignore | assertion `init.ok == 1` |
| guild manager rank 5, which player names has | `fatal_error: manager table: player names and guild manager have destroy rank 5` |
| map properties `MANAGER_STOP(NULL)` | `fatal_error: manager table: map properties stops or warns without a text` |

### Validation

Logs are in the worktree's `build/` (`lifecycle-*`). The baseline ran on a clean export of
`657555f` (`build/base`, logs `lifecycle-base-*`), the final runs on a clean export of
`346b667` (`build/final`, logs `lifecycle-final-*`).

- **Windows x86, the reference** (`build.bat`, MSVC 2022): the release build has no compiler
  or linker warnings (`lifecycle-final-build.log`, grepped for `: warning ` and `LNK`), as on
  the base (`lifecycle-base-build.log`). The full `tests\build.bat` passes in 390 s
  (`lifecycle-final-tests.log`), and so do the seven standalone suites, each with the base's
  result line (`lifecycle-final-<suite>.log`, `lifecycle-base-<suite>.log`), among them
  `COM lifecycle: 28 cases, 0 failures`.
- **Digests**: the 314 result lines of the suite (every line with a digest, "passed",
  "PASS", cases, steps, failures or `render`) are identical to the base's, in order
  (`lifecycle-final-results-base.txt` and `-results-change.txt`); the whole log differs from
  the base's only in the run time that `tests/test_map_name_migration.py` prints
  (`lifecycle-final-results-diff.txt`). Among them: the startup order (23 startup steps, 14
  checked creates, 4 ignored results, 2 warning loads, 19 shutdown steps), packet dispatch
  `d77aaadbb5fc63b7` (1,179,800 cases), effect factories `d3673c01`, effect trace
  `cbdaaf30ef3127d6`, EZ2STR file choice `1b0232b840610959`, window lists `2232167ff470caa7`
  (37,047 lines), the object-type counts (472 classes, 139 retired, 2,717 comparisons, 205
  accepted casts, 326 names, 306 window names) and the 17 `render` lines of
  `test_renderer_storage`.
- **Windows x64 and FreeType text** (`build.bat x64`, `build.bat freetype`): both build without
  warnings (`lifecycle-final-build-x64.log`, `lifecycle-final-build-ft.log`).
- **CMake with MSVC** (`-A Win32` and `-A x64`, Debug, separate build directories): configure
  and build without warnings; CTest passes 20 of 20 (Win32) and 19 of 19 (x64), as recorded for
  the merge (`lifecycle-final-cmake-<arch>-*.log`; the CTest logs are UTF-16). On Windows CMake
  builds only the CTest programs, not the client.
- **Linux** (WSL Ubuntu 24.04, GCC 13.3, Ninja, `-DRAGE_CLIENT=ON`; SDL configured with
  `SDL_UNIX_CONSOLE_BUILD=ON` from the merge check's download, as for the merge): `ragexe` and
  `ragexe_scenes` of `346b667` build, `client_managers.c` and `manager_table.c` among them, and
  CTest passes 24 of 24 with `SDL_VIDEO_DRIVER=offscreen`; `657555f` gives the same 24 of 24.
  GCC reports 1,685 warnings for both trees, none in the new files. Logs:
  `lifecycle-wsl-<base|final>-{configure,build,ctest}.log` (copied from `~/lifecycle-check` in
  WSL).
- **`test_startup_order` with GCC** (a scratch build outside the suite: an empty `windows.h`,
  `__declspec` defined away, otherwise as `build_startup_order.bat` compiles it): the unchanged
  test passes on Linux against `657555f`'s two functions and against `346b667`'s table, with
  the same result line and no warnings at `-Wall` (`lifecycle-wsl-startup-order-gcc.log`).
- `git diff --check` passes for both commits.

Not run: the macOS and Android builds (not possible here), a live client, the scene captures
(they need game data) and the dependency map (its scripts are in another worktree's untracked
`build/depmap`).

### Found, not fixed

- `load_ez2str_effects` and `load_map_sky` (`core/client_managers.c`) pass whatever
  `ez2str_effect_mgr_instance()` and `map_sky_mgr_instance()` return to the loads. Both return
  NULL when `mem_try_alloc` fails; `map_sky_mgr_load` then reads `self->lua`, and the EZ2STR load
  uses its manager once the file has parsed. `app_init_managers` made the same calls. Fixed
  afterwards (`5b707c7`; `main` `91ba2d9`).
- `client_run` (`core/winmain.c`) jumps to `done` when `app_init_3d`,
  `c3d_device_create_renderer` or `connection_startup` fails after the managers were created,
  and so skips `app_destroy_singletons`, `sound_mgr_destroy`, `platform_timer_period_end` and
  `crash_report_uninstall`, as for a failed manager; the earlier note named only the manager
  case. Kept.
- On Linux, CTest's `gfx_gl` is skipped ("No available video device") with the console-only
  SDL build unless `SDL_VIDEO_DRIVER=offscreen` is set. The merge notes say it ran on SDL's
  offscreen driver without naming the variable, and `docs/portability.md` does not mention it.
  It does now (`b2f43c3`, *Build and test*).
- `tests/test_startup_order.c`'s header comment still describes the test as recorded "before
  startup and shutdown become one table" (left so that the file stays unchanged). It now says
  that the test checks the table (`b2f43c3`).

## Restructuring: library table and checks (2026-11-04)

The first part of step 4, as planned in [Step 4: plan](#step-4-plan-not-started): a table of
libraries with the files each owns and the libraries each may use, an include check and a link
check that enforce it, the code changes that were clear moves of a definition or a hook, and a
`build.bat libs` variant that links the client from one static library per table library. No
file moved. Branch `purist-libs`, on `purist` at `8fd86bb`: `83e6952`, `53b850b`, `c84012f`
(code), `2c79f7d` (table and checks), `49d2cf5` (static libraries). No behavior change.

The work started from `purist` before the manager allocation-failure fix and was rebased onto
it; the one conflict was the end of `tests\build.bat`, where both added a call (the fix's test
runs first, the library checks last). The fix changes `core/client_managers.c` (app),
`core/map_sky.c` and `effects/ez2str_effect_mgr.c`, which pass both checks without new
exceptions.

### Measurement

The dependency-map scripts that the earlier steps kept in an untracked `build/depmap` are now in
`tools/depmap`: `depmap.py` (the file graph), `modules.py` (knot, library layer, clusters; its
`REGISTRY` set is the 23 files of the first measurements, `make_window_login.c`,
`window_registry.c`, `object_type_registry.c`, the `*_object_types.c` row files and
`core/client_managers.c`), `symbols.bat` (one `dumpbin /symbols` listing for a whole object
folder, about 2 seconds for the 1,133 objects instead of one dumpbin run per object) and
`objsyms.py`, the symbol reader that the map and the link check share.

The map has one gap, recorded in step 1: a global without an initializer is a communal symbol in
MSVC C, which dumpbin lists as `UNDEF` with its size as the value, so the map never drew an edge
for it. `objsyms.py` counts such an object as defining the symbol, and `depmap.py --communal`
draws those edges. The link check always does. At `8fd86bb` there are 187 communal globals; the
largest are `g_session1` (414 using files), `g_windowMgr` (351) and `g_renderer` (149).

| tree | edges | knot files / lines | library layer files / lines | above the knot files / lines | clusters |
|---|---|---|---|---|---|
| `8fd86bb` (`purist`) | without communal globals | 260 / 92,799 | 514 / 99,400 | 313 / 106,152 | 119 |
| `8fd86bb` | with communal globals | 441 / 138,164 | 270 / 42,610 | 376 / 117,577 | 160 |
| `c84012f` (after the three code commits) | without | 260 / 92,797 | 519 / 99,442 | 313 / 106,136 | 119 |
| `c84012f` | with | 273 / 96,266 | 507 / 96,908 | 312 / 105,201 | 127 |

The 42 registry and startup files (9,373 lines at `8fd86bb`) are cut in every row. Both trees
were measured on the release objects of the validation exports (`build/map-*.json`,
`build/mod-*.json`). Without
communal globals the numbers match the last published ones (260 / 92,782 at `d5d292c`, plus the
lifecycle and merge commits since). With them, the knot was 441 files: the renderer, resource and
widget component (the 73 files below the knot) reached the knot through one global,
`g_weather`, which `resources/world_res.c` wrote (below), and `g_windowMgr` and `g_session1`
were defined in a feature file and a registry file. After the code commits the knot with
communal globals is the published knot plus 13 files that read `g_navi`, `g_guildInfo`,
`g_mapPropertyQuiet` or `g_silentWhisperAcks` (the navigation helpers, `game/chat.c`,
`game/map_property.c`, `game/mode_whisper.c`, `game/talk_commands.c`, `ui/chat/whisper.c`,
`ui/quests/private_airplane_wnd.c`). The table is derived from that map, since the link check
counts communal globals.

### Code changes

**Definitions next to their declarations (`83e6952`).** Moved unchanged, same initializers:

| Global | From | To | Declared in |
|---|---|---|---|
| `g_windowMgr` | `ui/wndmgr/wndmgr_lifetime.c` (builds the party areas, a feature) | `ui/window_manager_state.c` (new) | `ui/window_manager_state.h` |
| `g_session1` | `session/session_lifetime.c` (a composition point) | `session/state.c` (new) | `session/state.h` |
| `g_hMainWnd` | `core/startup.c` | `core/window_state.c` (new) | `core/window_state.h` (was `core/app.h`) |
| `g_fullscreenMode`, `g_savedWidth`, `g_savedHeight`, `g_screenWidth`, `g_screenHeight`, `g_quitRequested` | `core/app.c` | `core/window_state.c` | `core/window_state.h` (`g_fullscreenMode`, `g_quitRequested` were in `core/app.h`) |
| `g_windowActive`, `g_activatedNow` | `core/window_events.c` | `core/window_state.c` | `core/window_state.h` (were in `core/app.h`) |
| `g_textureMode`, `g_displayWidth`, `g_displayHeight`, `g_sprHalfWidth`, `g_sprHalfHeight`, `g_spriteMode`, `g_trilinearFilter` | `ui/render/graphic_setting.c` | `render/render_options.c`, beside `g_lightmap` | `render/render_options.h` (were in `ui/render/graphic_setting.h`, which includes it) |
| `g_rodex` | `game/packets_rodex.c` | `ui/mail/rodex_box.c`, beside `rodex_create_instance` | `game/rodex_mail.h` |

`core/app.h` includes `core/window_state.h` as before, so its includers see the same
declarations. Sixteen files below app that included `core/app.h` only for these globals include
`core/window_state.h` instead (`render/c3ddevice.c` also `platform/code_page.h`, for the charset
constants that came through `core/app.h`), and six below the graphics settings window that
included `ui/render/graphic_setting.h` include `render/render_options.h`; three included it for
nothing and no longer do. Test change: `tests/container_memory_unreached.h` defines `g_rodex`
for the mail probe, which includes `packets_rodex.c`, as it defines its other globals.

Not moved: `g_clientDir` (`core/app.c`). Its home is `core/client_path.c`, but
`test_emblem_cache`, `test_screenshot_save` and the CMake `test_text_freetype` compile
`client_path.c` and define an initialized `g_clientDir` of their own, which GCC rejects next to
a second definition; it is an exception (below).

**The world light (`53b850b`).** `c3d_world_res_load` copied the map's ambient and diffuse light
into the actor layer's `g_weather`. It now calls a hook:

| Hook | Declared / defined in | Installed with | NULL |
|---|---|---|---|
| `g_worldResLight` | `resources/world_res.h` / `world_res.c` | `weather_take_map_light` (`actor/weather_hooks.c`, new) | the light is not passed on |

The adapter makes the same three copies (tint from the ambient light, light and maximum light
from the diffuse light). `test_world_resource` includes the adapter and installs it as
`client_install_hooks()` does; its assertions did not change.

**The code page (`c84012f`).** `g_codePage`, the code page that conversions of client text name,
is declared in `platform/code_page.h` and read by `platform/code_page_posix.c`, but was defined
in `platform/clientinfo.c`, the `clientinfo.xml` reader. It is now defined in the new
`platform/code_page.c`, the shared file of the code-page module (the `<module>.c` of the
portability notes), with the same initializer, `CODE_PAGE_UTF8`. The CMake platform library
that the tests link does not compile `code_page.c`; the tests that use `g_codePage` define their
own, as before.

### The library table (`tools/libraries.txt`, `2c79f7d`)

Each library has a note, the libraries it may use and its paths, relative to `src`: `dir/` is
everything below a directory, a name with `*` or `?` a glob within one directory, anything else
one file. The most specific pattern names a file (a file before a glob, a glob before a
directory, a deeper directory before a shallower one); a file in no library, a file two equally
specific patterns name, a pattern that owns no file, a library cycle and an exception without
`why` or `remove` are errors. `uses` is transitive: a library may use what the libraries it uses
may use. `app` uses `*`, every library, and no library may use it. A header goes with the `.c`
of its name; a header without one is in its directory's library unless only lower libraries
include it and everything it includes is usable from there, in which case it is in the lowest
library that needs it (124 headers, mostly structure and constant headers of the knot's
directories that gamedata uses).

| Library | `.c` files | lines | other files | uses | holds |
|---|---|---|---|---|---|
| `base` | 33 | 6,029 | 44 | nothing | `std`, `json`, memory, UTF-8 and text formatting, XML, the type queries, `fatal`, `base.h`, the Windows type names |
| `platform` | 40 | 13,056 | 29 | `base` | `platform/` (files, threads, sockets, clock, settings, code pages, desktop, crash dumps, the text-input editor and IME), client paths, the main window's state |
| `fileio` | 14 | 1,749 | 16 | `platform` | `fileio/` (game files, GRF, resource paths and names), the table and message-string readers, the JPEG codec |
| `render` | 41 | 12,467 | 44 | `fileio` | `render/`, the display options, the frame-time overlay (`diag/perf.c`) |
| `resources` | 37 | 6,631 | 35 | `render` | the resource manager (`core/res_mgr.c`, `fileio/res_get.c`, `fileio/res_cache.c`) and `resources/`, the cooldown timers the status icons draw |
| `widgets` | 93 | 20,097 | 76 | `resources` | `ui/controls`, the UIWindow base, the window-manager core (`ui/wndmgr`: list, slots, window table, forget, teardown, messages, modal loop, keyboard, snapping), skins, the current mode and its dispatch |
| `gamedata` | 250 | 36,631 | 187 | `widgets` | the rest of the library layer: the game-side modules below the knot |
| `knot` | 275 | 96,676 | 152 | `gamedata` | the 273 knot files, `game/mode_run.c` and `game/lifecycle.c` (the game mode's run loop and operations) |
| `feat-packets` | 138 | 52,436 | 40 | `knot` | the game packet handlers and the windows they drive (one cluster) |
| `feat-game` | 64 | 14,686 | 12 | `knot` | the other game handler clusters |
| `feat-effects` | 40 | 11,669 | 4 | `knot` | the effect classes of `effects/skills` |
| `feat-social` | 17 | 7,586 | 4 | `knot` | `ui/social` windows |
| `feat-character` | 12 | 5,665 | 4 | `knot` | `ui/character` windows |
| `feat-items` | 10 | 4,176 | 1 | `knot` | `ui/items` windows |
| `feat-quests` | 9 | 4,333 | 3 | `knot` | `ui/quests` windows |
| `feat-shops` | 5 | 2,504 | 0 | `knot` | `ui/shops` windows |
| `feat-other` | 15 | 2,082 | 9 | `knot` | the other clusters (chat, map, world map, mail, crash report) and the hook adapters (`*_hooks.c`), which only app installs |
| `app` | 41 | 9,268 | 10 | every library | the composition points (below) |

1,134 `.c` files and 670 other files (headers and the five `.def` tables), 1,804 in all.

How the plan's groups came out:

- **Split where clean.** `fileio`, `render` and `resources` are three libraries because the
  resource types create textures through render while render loads files through the file layer:
  with the resource manager counted as resources, only 18 uses go upward (render to resources).
  One library "fileio and resources" below render would have 26 upward uses, above render 11
  plus the file reads.
- **Not split.** The widgets and the window-manager core are one library: the widgets call the
  manager's capture, focus and quit-list functions (48 uses from 17 files, plus `g_windowMgr`),
  and the manager calls the widgets' methods. Ordering gamedata below the widgets instead gives
  1,826 upward uses.
- **The rest of the library layer** (250 files: session storage and tables, item and skill
  information, contents managers, Lua bindings, chat and link text, world, actor and navigation
  helpers, login records) is one library, `gamedata`. It never uses the knot; the knot uses it.
- **Feature clusters.** The 127 clusters above the knot are grouped by the directory most of
  their files are in. No cluster uses another, so the nine libraries have no link edges between
  them; only headers cross (below).
- **App.** `core/winmain.c`, `client_init.c`, `startup.c`, `app.c`, `client_managers.c`,
  `manager_table.c`, `object_type_registry.c`, `wndproc.c`, `app_win32.c`, `app_sdl.c`,
  `effects/effect_registry.c`, the two packet dispatchers, `ui/wndmgr/window_registry.c` and the
  five `make_window_*.c` row files, the `*_object_types.c` row files, `game/mode_msg.c` (the game
  mode's message switch, which names a handler of every feature), the login mode
  (`login/mode.c`, `login_states.c`, `login_dispatch.c`, `loginmode_vtables.c`), the main loop
  (`world/mode_loop.c`) and the session's construction (`session/session_lifetime.c`). The
  former registry files that only read tables now are library code: `core/object_type.c` is in
  `base`, `make_window.c`, `window_slots.c`, `forget_window.c` and `teardown.c` in `widgets`.

### The checks

- **Include check** (`tools/depmap/check_includes.py`). Reads every `#include` line of every file
  under `src`, whatever `#if` it is in, and resolves it as the compiler does: next to the
  including file, then from `src`. An include that resolves to no file under `src` (system,
  vendor) is not checked. It fails when the included file's library is neither the including
  file's library nor one it may use, unless the table lists the pair. 11,072 includes are
  checked.
- **Link check** (`tools/depmap/check_links.py`). Reads the symbol listing of a build's objects,
  `dumpbin /symbols` (`symbols.bat`) or `nm -P -A` (`--nm OBJDIR` runs nm itself), and maps each
  object to its source (build.bat names objects after the unique basename; CMake keeps the path).
  For every external symbol an object uses, it looks up the objects that define it, with communal
  globals counted as defined where they are declared without an initializer; the use is allowed
  when one definer is in the object's library or one it may use, or when the table lists the
  (source, symbol) pair. Symbols that no client object defines (C runtime, Windows, the vendored
  libraries, `build\gen\effect_data_embedded.c`) are not checked. Names are matched as the
  compiler wrote them (`_f`, `@f@8`) and listed in the table without decoration. 29,764 uses of
  client symbols are checked in the x86 build (29,876 in x64, 29,786 with FreeType).
- Both run with `--strict`, which also fails on an exception that matches nothing, so removing a
  use requires removing its exception and the list stays the to-do list.
- **How to run.** `tools\check_libraries.bat` runs both; the link check reads `build\obj` and
  runs `build.bat` first (log in `build\check_libraries-build.log`) when an object is missing or
  older than a file under `src` (`tools/depmap/stale_objects.py`). `tests\build.bat` calls it
  after its last test, so the suite runs both. `build.bat check`, combinable with `x64`,
  `freetype` and `dbg`, runs it on the objects of the variant it just built. CTest has
  `library_includes` (every platform) and, for the Linux client (`RAGE_CLIENT`), `library_links`
  on `CMakeFiles/ragexe.dir` through nm. `python tools/depmap/libtable.py` prints the libraries
  with their sizes.

### Exceptions

398 entries in 15 groups: 269 includes and 129 symbol uses, from 296 files. Each group's `why`
and `remove` are in the table; in short:

| Group | includes | uses | why | what removes it |
|---|---|---|---|---|
| `knot-headers-below` | 108 | 0 | gamedata, widgets, render and resources files include knot headers for types and constants (`navi/navi_manager.h`, `session/session_state.h`, `world/world.h`, ...) | the types in headers of the lower library |
| `game-mode-messages` | 25 | 44 | knot, feature and gamedata code calls `game_mode_send_msg` (`game/mode_msg.c`, app) | the message switch as rows that app installs, the lookup in the knot |
| `link-text-in-widgets` | 23 | 25 | edit, text-viewer, list and fit widgets turn item and navigation links into text through `ui/chat`, `iteminfo` and `ui/draw_key.c` | link-text hooks in `g_uiHooks`, or the link parsing in the widgets |
| `session-below-the-game` | 23 | 11 | render and widget files read options from `g_session1` | the settings each library reads in a structure of its own, or hooks |
| `feature-headers` | 29 | 0 | headers of a feature library included from another feature, gamedata, the knot or the widgets | the declarations in a lower header, or no include |
| `app-header` | 24 | 0 | `core/app.h` and `session/session_imports.h` included below app, mostly for what they include | the headers that declare what is used |
| `widgets-reach-game-state` | 12 | 7 | Lua calls from buttons and the keyboard, the mouse, the drive state, the mail sort | hooks; narrower includes |
| `resource-manager-in-render` | 4 | 14 | the texture cache and the frame and HUD drawing load through the resource manager; the sprite drawing reads ACT motions | a texture-source hook, the motion passed in |
| `main-loop-state` | 0 | 14 | `mode_mgr_switch`, `check_system_message`, `g_loginStartMode`, `g_mapServerAidPending` are in app files | the switch in the mode manager, the pump a platform call, the login globals below the knot |
| `window-manager-below-widgets` | 5 | 4 | `fileio/resource_errors.c` and `platform/language.c` call the window manager (recorded in step 2) | a hook each |
| `render-reaches-game-state` | 5 | 3 | the drive state's frame note, `g_isVoodoo`, `g_registryKeyPath` | a frame hook, the values passed at device creation |
| `stray-includes` | 8 | 0 | single higher includes (`std/std.h` includes `render/math3d.h`, ...) | the narrower header or no include |
| `client-dir` | 0 | 4 | `g_clientDir` is in `core/app.c` | define it in `core/client_path.c`; three tests set it at run time |
| `packet-dispatch` | 3 | 1 | `game/net_poll.c` calls the dispatcher; two files include its header unused | a dispatch hook; drop the includes |
| `knot-calls-features` | 0 | 2 | the run loop sends the keep-alive and draws the windows (both in `feat-packets`) | handlers installed at startup |

The largest single targets: `game_mode_send_msg` (44 uses, 25 includes of `game/mode_msg.h`),
`core/app.h` (22 includes), `session/state.h` and `g_session1` (16 includes, 8 uses below
gamedata) and `navi/navi_manager.h` (8 includes). Converting the game mode's message switch to
rows, as step 3 did for the packets, is the single change that removes the most (69 entries).

### Mutation proof

Each change made in the worktree at `49d2cf5`, the check run, the files restored with
`git checkout` (`build/libs-mutations.log`; the same results before the rebase are in
`build/libs-mutations-prerebase.log`):

| Change | Result |
|---|---|
| `std/vec.c` (base) includes `ui/wndmgr/wndmgr.h` (widgets) | include check fails: `base may not use widgets: src/std/vec.c:2: includes ui/wndmgr/wndmgr.h` |
| `render/math3d.c` (render) includes `session/session_state.h` (knot) | include check fails: `render may not use knot` |
| `game/battlefield.c` drops its unused `game/packet_dispatch.h` include | include check fails: `1 include exceptions match no include: packet-dispatch: game/battlefield.c game/packet_dispatch.h` |
| `render/math3d.c` calls `weather_take_map_light` (`actor/weather_hooks.c`, feat-other); `std/vec.c` returns the address of `g_session1` (gamedata, a communal global), built with `build.bat` | link check fails: `base may not use gamedata: src/std/vec.c: uses g_session1` and `render may not use feat-other: src/render/math3d.c: uses weather_take_map_light`; rebuilt after the restore: passes |
| `std/vec.c` in the paths of both base and platform | `std/vec.c: equally specific paths of base, platform name it` |
| base uses widgets | `libraries use each other: base -> widgets -> resources -> render -> fileio -> platform -> base` |
| an exception without `remove` | `exception stray-includes needs a why and a remove` |

The `g_session1` case is caught only because communal globals count as definitions; the map
before this step had no edge for it.

### Static libraries (`49d2cf5`)

`build.bat libs` builds the client from the table's libraries. It compiles the same sources with
the same flags as the variant it combines with (`x64`, `freetype`, `dbg`, `check`), then:

- `tools/depmap/symbols.bat` lists the objects' symbols, and `tools/depmap/lib_objects.py`
  assigns each object to its library by the table and writes one response file per library,
  `app.rsp` and `libraries.txt` (the libraries from the highest to the lowest, the link order);
- `lib.exe` archives each library except app into `build\obj<variant>_libs\<library>.lib`;
- the linker gets app's objects and the 17 libraries instead of every object, and writes
  `build\main_libs_<variant>.exe` (`main_libs_rebuild.exe`, `main_libs_dbg.exe`, ...).

Objects that define no external symbol (the other platform's files, empty here) stay out of the
libraries, since `lib.exe` warns about them and nothing could pull them in. `build\gen`'s
`effect_data_embedded.obj` goes with `effects/effect_data.c`, its only user. The linker takes
from a library only the objects something references; since all registration is explicit
(steps 1 to 3), nothing the client needs is left behind. The default build is unchanged: same
sources, flags, objects and executable.

Checked on the exports of `8fd86bb` and of this branch: `build.bat libs`, `libs x64`,
`libs freetype`, `libs dbg` and `libs check` build without compiler, librarian or linker
warnings (1,107, 1,109 and 1,108 objects in the libraries for x86, x64 and FreeType). The
linker maps of the libraries executables list the objects of the default ones minus four, in
all three variants: `dead_word_table.obj`, `navi_strings.obj`, `res_maps.obj` and
`touch.obj`, which nothing references (below).

### Validation

Logs are in the worktree's `build/`. The baseline ran on a clean export of `purist` at
`8fd86bb` (`build/snap/pbase`, logs `libs-pbase-*`), the change on a clean export of this branch
(`build/snap/ptip`, logs `libs-ptip-*`).

- **Windows x86, the reference** (`build.bat`, MSVC 2022): the release build has no compiler
  or linker warnings, as on the base. The full `tests\build.bat` passes in 408 s, and so do the
  seven standalone suites, each with the base's result line.
- **Digests**: the 316 result lines of the base's suite (every line with a digest, "passed",
  "PASS", cases, steps, failures or `render`) are identical, in order; the change adds the two
  lines of the library checks at the end (`libs-pbase-results.txt`, `libs-ptip-results.txt`).
  Among them: the startup order, packet dispatch `d77aaadbb5fc63b7`, effect factories
  `d3673c01`, effect trace `cbdaaf30ef3127d6`, EZ2STR `1b0232b840610959`, window lists
  `2232167ff470caa7`, the object-type counts and the `render` lines.
- **Windows x64 and FreeType text** (`build.bat x64`, `build.bat freetype`): no warnings.
- **The checks on every variant** (`build.bat check`, `x64 check`, `freetype check`): both pass,
  with 29,764, 29,876 and 29,786 uses of client symbols checked (`libs-ptip-more.txt`).
- **The libraries link**: above.
- **CMake with MSVC** (`-A Win32` and `-A x64`, Debug): configure and build without warnings;
  CTest passes 23 of 23 (Win32) and 22 of 22 (x64), `library_includes` among them.
- **Linux** (WSL Ubuntu 24.04, GCC 13.3, Ninja, `-DRAGE_CLIENT=ON`, console SDL as for the
  merge): both trees build with the same 1,671 GCC warnings (one moves a line in
  `game/talk_type.c`, which lost an include), none in the new files; CTest passes 26 of 26 on
  the base and 28 of 28 on the change, the two new ones being `library_includes` and
  `library_links` (`libs-wsl-<pbase|ptip>-*.log`).
- **Mutations**: above, on the branch after the rebase.
- `git diff --check` passes for every commit.

Not run: the macOS and Android builds (not possible here), a live client and the scene
captures (they need game data).

### Found, not fixed

- Four objects of the default link are not referenced by anything, which the libraries link
  shows by leaving them out:
  - `contents/dead_word_table.c` defines `g_dead_word_table`, 5,636 bytes without an
    initializer, and nothing uses it.
  - `effects/navi_strings.c` and `fileio/res_maps.c` contain only includes.
  - `core/touch.c` (363 lines), the touch input, is called only from `core/app_sdl.c`, which
    is compiled out on Windows (`#ifndef _WIN32`), so the Windows clients link it for nothing.

  The first three are pruning candidates; `touch.c` is used off Windows.

### Step 4b: moving the files (plan)

What makes the moves cheap: every `#include` under `src` is written from `src` (the only one
without a directory is `base.h`, at the root), and `build.bat` already compiles whatever is
under `src`. A file moved from `src/<path>` to `src/<library>/<path>`, with every
`src/<library>` an include directory, is found by the same `#include` line, so the moved files
do not change.

1. **One commit per library, from the bottom** (`base`, `platform`, ... `feat-*`, `app` last).
   Each commit is `git mv` of the library's files to `src/<library>/<old path>` plus what a
   script derives from the old-to-new path list, in the same commit so that every commit builds:
   - the include directories (`build.bat`, `cmake/`, the test scripts), one per library,
     generated from `tools/libraries.txt`;
   - the 43 source paths in `CMakeLists.txt` and `cmake/*.cmake`;
   - the 992 source paths in 128 test scripts and the 275 `../src/...` includes in 134 test
     files;
   - the table: the library's paths become `<library>/`, and its exceptions' file names get the
     prefix.

   The moved files are unchanged, so git sees exact renames (100% similarity). Object names stay
   the unique basenames that `build.bat` needs.
2. **After the last move** the table's paths are one directory per library, and the libraries
   link (`build.bat libs`) can become the default link if Patrick wants it.

**Cherry-picks from `main`.** `git cherry-pick` follows exact renames, so a `main` fix to a
file that moved applies to its new path. It does not follow a file `main` adds: directory-rename
detection only works when a whole directory moved to one place, and `core/`, `game/`, `ui/`,
`session/` and others are split across libraries. Such a file needs a manual `git mv` after
the pick. The alternative is to run the same move script on `main`, so both branches keep the
same paths.

**`purist-portable`** is fully merged into `purist` (no commit of its own ahead), so the moves
need no second run there unless it gets new work.

**Patrick decides:**

- The folder scheme: `src/<library>/<old path>` as above (include lines unchanged, deeper
  paths such as `src/knot/game/mode_run.c`), or flatter folders with rewritten includes.
- Whether `main` gets the same moves, or `main` fixes that add files are moved by hand.
- Whether the libraries link becomes the default after the moves.

## Foundation fixes ported from main (2026-11-06)

Decision: Patrick asked for `main`'s fixes to be applied to `purist`, carefully, now that its
structure differs. `main` had 64 commits that `purist` did not have and had not recorded as
done, already fixed or not needed in the [upstream notes](upstream/README.md):

- 52 from 2026-11-02, which reached `main` with its merge of GitHub's `main` (`af2a04f7`): the
  file readers (SPR, ACT, GND, GAT, RSW, RSM2, IMF, PAL, BMP, PNG, GIF, WAV, GRF, loose files),
  STR playback, the renderer and graphic settings, text and link parsing, and a naming pass;
- 12 from 2026-11-05: ten from kbirk/fenrir-ragexe, the Star Gladiator name bound and the
  whip shimmer.

They were picked onto `purist-libs` (after the library table) in `main`'s order with
`git cherry-pick -x`, one at a time; Kevin Birk's keep his authorship. A pick that conflicted
or that does not build on `purist`'s other platforms was resolved by hand, and its message says
how under "On purist". The [upstream notes](upstream/README.md#foundation-fixes-ported-from-main-2026-11-06)
list every commit with its `purist` commit or why it is left out.

| | commits |
|---|---|
| picked as they are | 33 |
| adapted | 24 |
| left out | 7 |

### Adapted

- **Removed features.** Where `main`'s change also touched code `purist` removed, only the rest
  was taken: the job-level maximum without the third, fourth, expanded and Doram classes (its
  test checks the classes `purist` has), the live display apply without the launcher page, the
  skill use pose without the third-class skills `0x917`, `0x91a` and `0x920`, the item link
  card loop without item grades, the NPC dialog without the German prefix, the network loop
  without replay playback, the item restriction test without the language-folder lookup (and
  without its two command-line modes for the old strict reader), the talk-option test without
  the PC-bang flag.
- **Already done differently.** The cached screen-size values were behind once guards on
  `purist`; the guards go with them. The name copies of `ac4f50ec` were already text copies
  (left out, below).
- **The platform layer.** The game loop's sleep goes through a new `platform_thread_sleep`
  (`Sleep`, `nanosleep`); the draw batch's owner is `platform_thread_id()`; the empty-file
  check uses the platform file calls; the GIF decoder keeps `platform_ticks_ms`; the OpenGL
  backend answers `gfx_can_set_mode_live` (any adapter, when it has a context) and reports the
  renamed `trilinear` capability.
- **MSVC-only calls.** `_snprintf_s` (Star Gladiator name) became `utf8_copy_prefix`, which
  also cuts by whole characters; `sscanf_s` (broadcast colour) became `sscanf`;
  `__declspec(thread)` (sin/cos cache) became `__declspec(thread)` under MSVC and
  `_Thread_local` elsewhere, since MSVC knows `_Thread_local` only with `/std:c11`, which the
  test scripts do not pass.
- **UTF-8.** The broadcast annotation cut (`33df070c`) cuts the repeated text with
  `utf8_prefix`, so it never splits a character; its test checks such a cut and fails with a
  plain byte cut. The bracket-wrap test keeps its UTF-8 cases beside `main`'s new ASCII ones.
- **giflib.** With `main`'s own GIF decoder, giflib also leaves the CMake build (`rage_gif` and
  the client's link) and the two `purist`-only test scripts that compiled or linked it.
- **Names.** `996806c1` renames constants and functions whose names contradicted their values.
  On `purist` some had already been renamed, and several constants are defined per `.c` file
  with different values (`CHAT_COLOR_YELLOW` is `0x00ffff` in one file and `0xffff00`, really
  cyan, in another), so a name is changed only in the files where `main` changed it, and
  everywhere only when one header defines it. No value changes.
- **Tests.** `tests\build.bat` gets each new test before the library checks, which stay last.
  The draw-batch tests link the platform thread file. The name-text extractor reads
  `WND_ITEM_COLLECTION` from its header, as `main`'s does since the naming pass.

### Left out

| `main` | Why |
|---|---|
| `ac4f50ec` names as text | already on `purist` (`b2d7102e`) |
| `bcc333b2` RoDEX TaeKwon names | already on `purist` (empty pick) |
| `a38ec2e1`, `e20a417a` README | `purist`'s README lists its deviations, not `main`'s fixes |
| `e9d8d0ab` Swap button | the swap-equipment window is removed |
| `dd6cec07` embedded fonts | the embedded Korean fonts went with the langtype 20 lock |
| `91c5764c` macro detector | the macro detector is removed |

### What changes in the recorded results

Behaviour changes are the point of these fixes; these are the recorded results they move, each
traced to one commit by running the test before and after every commit that could touch it
(`build/trace-bisect.txt`, `build/render-bisect.txt`):

- **Effect trace** (`tests/test_effect_trace.c`): only `c828e76b` (STR layers finished by the
  original's rules) changes it, to 2,083,072 steps, 36,991 primitives and digest
  `5c269a851ffa6be1` (was 2,031,553, 36,986, `cbdaaf30ef3127d6`); the expected digest moves in
  that commit. The EZ2STR digest `1b0232b840610959` stays.
- **The 16 `render` lines of `test_renderer_storage`**: only `b715096f` (the texture arguments
  of blend modes 6, 7 and 12) changes them.
- Result lines that name new cases: item links (extra cards), ACT and SPR owners (more
  versions, prefixes and uploads), wave/palette, the bitmap fixtures (37 instead of 23, no
  giflib), GIF emblems, the attribute grid (an unsupported GAT version), STR playback (the
  completion rules), item tags, item restrictions (the tokenizer), file I/O (empty loose files),
  water numerics, the scene resource (versions 0.x, unknown objects). Every other result line of
  the suite is unchanged, among them packet dispatch `d77aaadbb5fc63b7`, effect factories
  `d3673c01`, EZ2STR `1b0232b840610959`, window lists `2232167ff470caa7`, the object-type counts
  and the startup order. New lines: the job-level maximum and the draw batch.

### Validation

Clean exports of `purist-libs` before the port (`build/snap/ptip`) and after it
(`build/snap/pfinal3`, `pfinal2` for the variants and CMake), logs `libs-<label>-*` in the
worktree's `build/`.

- **Windows x86, x64 and FreeType** (`build.bat`, `x64`, `freetype`; the same with `check` and
  `libs`): no compiler, librarian or linker warnings. The include and link checks pass with the
  same 269 and 129 exceptions; no ported commit needed a new one.
- **The full `tests\build.bat`** passes, and so do the seven standalone suites with their
  unchanged result lines. The result lines differ from the export before the port only as listed
  above.
- **CMake with MSVC** (Win32 and x64, Debug): builds without warnings; CTest passes 23 of 23 and
  22 of 22.
- **Linux** (WSL Ubuntu 24.04, GCC 13.3, `-DRAGE_CLIENT=ON`): the client and the tests build and
  CTest passes 28 of 28 (`library_includes` and `library_links` among them). GCC reports 1,665
  warnings instead of 1,671: giflib's are gone, and `ui/shops/merchant_shop.c` lost its
  `-Wformat-security` warning with the price-limit notice's restored argument (`7d405c00`). No
  new warning in a client file. This build found what MSVC could not: `_snprintf_s`,
  `sscanf_s` and `GetCurrentThreadId` in shared code and the GL backend's stale capability name
  (all adapted above).
- `git diff --check` passes for every commit.

Not run: macOS and Android, a live client, the scene captures (they need game data).

### Found, not fixed

- On `main`, `broadcast_text_strip_annotation` (`33df070c`) cuts the repeated text at a byte
  count, which can split a CP949 double-byte character at the end of a 1,024-byte broadcast.
  `purist` cuts on a character boundary (above).

## Ranking the knot's cuts (2026-11-06)

Decision: Patrick asked for a ranking of what would shrink the knot most, before any more of it is
cut (step 5 of the restructuring: what is still knotted). `tools/depmap/knot_cuts.py` computes it
from the same graph as the library table: link-time references between `.c` files with communal
globals counted, the 42 registry and startup files cut. At `dd3d5ba` (release objects of the
validation export `build/snap/pfinal3`) the knot is 273 files / 96,292 lines, as in the table.

A cut drops edges. `lower X`: X's calls into the rest of the knot go (they become hooks, or what X
needs moves below the knot). `upper X`: the rest of the knot no longer reaches X (a table or hook
that app installs, or the definition it uses moves out of X). `edge A -> B`: A no longer uses B.
X is a file, a directory or a stem group. Each step takes the cut with the most knot lines freed
per symbol to reroute, applies it and measures again; the symbols are the work. It is greedy, so
a later step can be cheap only because of the earlier ones, and the order is a suggestion.

### The result

26 cuts, 39 symbols used from 34 files, dissolve the knot: what is left is
`effects/rag/rag_effect.c` and `rag_str_anim.c` (373 lines), which call each other inside one
directory. The first 14 cuts, 19 symbols, take it to 110 files / 30,438 lines.

| | cut | symbols to reroute | knot after (files / lines) | kind |
|---|---|---|---|---|
| 1 | upper `effects/rag/rag_effect_process.c` | `rag_effect_on_process` | 243 / 77,700 | ops entry |
| 2 | `effects/rag/rag_effect.c` -> `effect_prim.c` | `effect_prim_send_msg`, `prim_triangle_construct` | 219 / 62,986 | move |
| 3 | upper `actor/actor_sendmsg.c` | `game_actor_send_msg` (2 ops entries, 8 calls in 5 files) | 205 / 57,804 | ops entry |
| 4 | upper `world/mode_mgr.c` | `mode_nothing` | 192 / 55,093 | move |
| 5 | lower `session/session_items.c` | `session_refresh_equip_wnd` | 181 / 52,540 | hook |
| 6 | upper `game/mode_whisper.c` | `g_silentWhisperAcks` | 177 / 50,744 | move |
| 7 | `ui/quests/navigation_cmd.c` -> `ui/chat/chat_window.c` | `ui_chat_window_edit`, `ui_new_chat_wnd_on_enter` | 168 / 46,426 | hook |
| 8 | upper `world/objects.c` | `world_remove_object` | 155 / 44,836 | hook |
| 9 | upper `ui/shops/storage*` | `ui_bitmap_edit_ctrl_ops` | 152 / 43,517 | move |
| 10 | upper `ui/character/basic_info_wnd.c` | `ui_basic_info_wnd_set_height` | 149 / 42,221 | hook |
| 11 | upper `game/world_objects.c` | `game_mode_launch_arrow`, `game_mode_launch_plant_attack`, `game_mode_make_corpse`, `game_mode_make_granny_corpse` | 133 / 37,062 | hook |
| 12 | upper `actor/render_misc.c` | `render_object_render` (4 ops entries) | 125 / 33,842 | ops entry |
| 13 | upper `actor/attr_state.c` | `g_playerBlind` | 117 / 32,130 | move |
| 14 | upper `actor/launch.c` | `game_actor_make_corpse` | 110 / 30,438 | ops entry |
| 15 | `navi/navi_object_queries.c` -> `navi/navi_manager.c` | `g_navi` | 103 / 29,174 | move |
| 16 | upper `actor/weather.c` | `g_weather` | 98 / 27,955 | move |
| 17 | upper `game/actor_names.c` | `game_mode_get_actor_name` (6 files) | 93 / 26,749 | hook |
| 18 | lower `contents/client_lua.c` | `client_lua_register_functions` | 88 / 25,646 | table |
| 19 | upper `game/input.c` | `g_shiftDown`, `game_mode_pick_ground_cell` | 80 / 23,519 | hook |
| 20 | `ui/social/mini_party_info.c` -> `game/target_actor.c` | `game_mode_click_actor` | 70 / 19,600 | hook |
| 21 | lower `navi/navi_manager.c` | `navi_manager_arrive`, `navi_manager_load_grid_images`, `navi_manager_reset`, `ui_navigation_invalidate` | 49 / 13,075 | hook |
| 22 | upper `session/session_job.c` | `session_get_drawn_job` | 36 / 10,487 | move |
| 23 | upper `session/session_party.c` | `session_get_party_member_by_name` | 24 / 6,997 | move |
| 24 | lower `contents/` | `game_mode_notify_listed`, `world_find_pc_by_char_id` | 16 / 5,084 | hook |
| 25 | lower `actor/game_actor.c` | `game_mode_remove_listed`, `render_object_load_act_res` | 9 / 2,617 | hook |
| 26 | upper `actor/` | `render_object_construct`, `render_object_dc_render_object`, `render_object_return_zero` | 2 / 373 | ops entry |

Where the files go: cuts 1 and 2 free 54 files / 33,306 lines, 52 of them in `effects/rag` (the
effect kinds and primitives), 3 to 14 most of `actor`, the effect classes in `effects/` and the
feature windows still tied in (chat, shops, social, character), 15 to 21 `navi` and the game
input, 22 to 26 the rest of `session`, `contents` and `actor`.

The kinds, read from the code at each symbol (the tool does not decide them):

- **move** (9 cuts): a definition in the wrong file, as in step 1 of the restructuring. Two
  three-line helpers of the primitives used by their master (2); the empty `mode_nothing` in the
  mode manager, whose other code saves user data on quit (4); globals defined by a file above
  their writers or readers: `g_silentWhisperAcks` (written by the session's party code),
  `g_playerBlind` and `g_weather` (read by ground and model drawing), `g_navi` (read by the
  navigation queries) (6, 13, 16, 15); an
  edit control's method table defined in the storage window and used by the navigation window
  (9); two session getters that only read session data, in files that also call upward (22, 23).
- **ops entry** (5 cuts): a method table in a base file that names a subclass's or a handler's
  function: the effect's `on_process` (1, the 239-kind switch), the actor's `send_msg` (3),
  `render` (12), the corpse launcher (14), the render object's defaults (26). The table moves to
  the file of the function, or the subclass passes its table to the base constructor.
- **hook** (11 cuts): an upward call, as in step 2: the session refreshing windows (5, 10), the
  actor and world asking the game mode to make corpses, arrows and names or to pick a cell
  (11, 17, 19, 20, 24, 25), the world destroying an object (8), the navigation command and
  manager calling the chat and navigation windows (7, 21).
- **table** (1 cut): `client_lua_register_functions` registers every Lua function, a central
  list as in step 3 (18).

### The second SCC

The next SCC is 78 files / 18,786 lines of the lower libraries (`render` 26, `widgets` 24,
`resources` 21, `gamedata` 4, `platform` 2, `fileio` 1). Seven cuts, 19 symbols, dissolve it, and
only the first crosses libraries: `ui_window_mgr_get_focus_edit`,
`ui_window_mgr_is_chat_edit_focused` and `ui_window_mgr_send_msg_` called from `platform`,
`fileio` and `gamedata` files, which the table already lists as exceptions. The other six are
cycles inside `render` and `resources`, which the table allows.

### What it does not show

- A cut that frees files from the knot does not by itself put them in the right library; the
  table is derived again after the cuts.
- One symbol can be many call sites (`game_actor_send_msg` 8 calls, `g_weather` 13 uses), and a
  hook is only right where the call really goes upward; a getter that only reads session data
  moves down instead.
- Each cut keeps behaviour only if it is checked like steps 1 to 3: the effect trace for 1 and 2,
  the packet dispatch and window-list checksums for the rest.

Recorded: `build/knot-cuts-tip.txt` and `.json` in the `purist-libs` worktree. Rerun after each
cut with `python tools/depmap/knot_cuts.py <checkout> <symbols> <out.json>` (`symbols.bat` writes
the listing).

## Knot cuts 1 and 2: the effect processes and primitives (2026-11-06)

Decision: Patrick chose the first two cuts of the [ranking](#ranking-the-knots-cuts-2026-11-06).
Three commits, behaviour unchanged:

- `71a0ad7` **cut 1**: `rag_effect.c`'s method table named `rag_effect_on_process`, the switch
  over every effect ID. Its `on_process` is now a small function that calls
  `g_ragEffectProcess` (`effects/rag/rag_effect.h`), which `client_install_hooks()` sets to
  `rag_effect_on_process`. NULL: an effect ends at its first frame, as when its process returns 0
  (`world_process_actors` then removes and destroys it). `test_effect_trace` installs the hook.
- `a010e31` **cut 2**: `prim_triangle_construct` and `effect_prim_send_msg`, which the effect base
  calls in its constructor and when it lets go of its primitives, move unchanged from
  `effect_prim.c` to the new `effects/rag/effect_prim_shared.c`, which uses nothing of the
  client.
- `3d98e62` **the table**: the 54 files that left the knot become the library `feat-rag`, which
  uses the knot: `effects/rag/` except the effect base (`rag_effect.*`, `rag_str_anim.*`,
  `rag_str_playback.c`, `effect_prim_shared.c`), plus `effects/effect_objects.c` and
  `session/session_efst_image.c`. `effect_prim.h`, `prim_records.h` and `rag_effect_types.h` stay
  with the knot, since `rag_effect.h` needs their types; the table's note says so. No new
  exception.

### Measured

| | knot files / lines | library layer | above the knot | clusters |
|---|---|---|---|---|
| `dd3d5ba` (before) | 273 / 96,292 | 507 / 97,137 | 312 / 105,217 | 127 |
| `a010e31` (after cut 2) | 219 / 62,995 | 508 / 97,161 | 366 / 138,506 | 128 |

As the ranking predicted (219 / 62,986, plus the nine lines of the hook). The freed primitives
call each other, so above the knot they are an SCC of their own (24 files), as the effect classes
of `effects/skills` are clusters. `knot_cuts.py` on the new objects gives the ranking's cuts 3 to
26 unchanged (36 symbols) and the same second SCC (`build/knot-cuts-12.txt`).

### Proof

- **Effect trace:** both digests unchanged after each commit (`5c269a851ffa6be1`,
  `1b0232b840610959`). Without the hook installed in the test it fails: 8,865 steps, no
  primitives, digest `d4f729a52d9dfac3`.
- **The table:** on the objects from before the two cuts (`build/snap/pfinal3`), the link check
  with the new table fails on exactly the three uses they removed: `rag_effect.c` uses
  `effect_prim_send_msg`, `prim_triangle_construct` and `rag_effect_on_process`. A change that
  ties the effect base to the processes or the primitives again fails it.

### Validation

Clean exports of each commit (`build/snap/k1`, `k2`, `k3`), logs `libs-k<n>-*` in the worktree's
`build/`.

- **Each commit:** `build.bat` without compiler, librarian or linker warnings; the full
  `tests\build.bat` and the seven standalone suites pass. Every result line is the one of the
  export before the cuts (`pfinal3`) except the two library-check lines: one more file, include
  and use (`effect_prim_shared.c` and the hook's include), and at `3d98e62` 19 libraries
  instead of 18.
- **At `3d98e62`:** `build.bat x64` and `freetype`, the same three with `check`, and `libs`
  (one static library per table library, `feat-rag` among them): no warnings; the include and
  link checks pass with the same 269 and 129 exceptions. CMake with MSVC, Win32 and x64 (Debug):
  no warnings, CTest 23 of 23 and 22 of 22. Linux (WSL, GCC 13.3): CTest 28 of 28, 1,665
  warnings as before.
- `git diff --check` passes for each commit.

Not run: macOS and Android, a live client.

## Knot cuts 3 to 25: the knot is gone (2026-11-06)

Decision: Patrick asked to continue after cuts 1 and 2, merging into `purist` later. The cuts
followed the [ranking](#ranking-the-knots-cuts-2026-11-06), measured again after each; one
commit per cut, behaviour unchanged. From cut 20 on, the ranking was rerun on the code of the
cut before (`topcuts`, the ranking's candidates on the SCC that holds `actor/game_object.c`),
since the cuts before had changed the graph; cut 21 there is a different cut from the
ranking's 21, which freed the same navigation files.

| cut | commit | change | kind | game knot after (files / lines) |
|---|---|---|---|---|
| | `2ba04c5` | after cuts 1 and 2 | | 219 / 62,995 |
| 3 | `70f8f5d` | a game actor's messages: `game_actor_handle_msg` through `g_gameActorHandleMsg` | hook | 205 / 57,823 |
| 4 | `18c5241` | the empty `mode_nothing` to `world/mode_context.c` | move | 192 / 55,112 |
| 5 | `b45a2e1` | the three session redraws that reach above the window manager to `session/session_redraws_game.c` | split | 181 / 52,492 |
| 6 | `d9f1a0c` | `g_silentWhisperAcks` and `g_mapPropertyQuiet` to `game/client_state.c`, with their header | move | 176 / 50,510 |
| 7 | `f99852b` | the navigation command reaches the chat window through `g_windowMgrHandlers` (new `chat_enter`) | hook | 167 / 46,202 |
| 8 | `bd256fc` | `world_remove_object` to `world/objects_remove.c` | move | 154 / 44,612 |
| 9 | `8d3ffc5` | `ui_bitmap_edit_ctrl_ops` to `ui/controls/edit_variants.c`, with the control | move | 151 / 43,293 |
| 10 | `701c8f8` | the basic info window's layout from the session through `g_sessionBasicInfoHeight` | hook | 148 / 41,999 |
| 11 | `e1e5236` | arrows, plant attacks and corpses through `g_actorSpawns` (`actor/actor_spawns.h`) | table | 132 / 36,842 |
| 12 | `3833d7a` | render objects drawn by `render_object_draw` through `g_renderObjectDraw` | hook | 124 / 33,630 |
| 13 | `66bea37` | `g_playerBlind` to `actor/player_blind.c` | move | 117 / 32,104 |
| 14 | `5a37d3d` | the plain actor corpse to `actor/actor_corpse.c` | move | 111 / 30,440 |
| 15 | `227d1b5` | `g_navi` to `navi/navi_instance.c` | move | 103 / 28,951 |
| 16 | `ad0a14e` | `g_weather` to `actor/weather_instance.c` | move | 98 / 27,732 |
| 17 | `6c8c5c0` | `game_mode_get_actor_name` to `game/actor_name_lookup.c` | move | 93 / 26,526 |
| 18 | `919cf1c` | the client's Lua functions registered through `g_clientLuaFunctions` | hook | 88 / 25,423 |
| 19 | `02e0df1` | `g_shiftDown` and the ground cell pick to `game/input_queries.c` | move | 80 / 23,296 |
| 20 | `3166d95` | a party plate's click through `g_uiHooks.click_actor` | hook | 70 / 19,374 |
| 21 | `96d72fd` | the navigation tables loaded from the session through `g_sessionNaviTables` | hook | 50 / 13,067 |
| 22 | `a38015e` | the session's job queries to `session/session_job_queries.c` | move | 37 / 10,547 |
| 23 | `2e4b969` | the party member lookups to `session/session_party_lookups.c` | move | 25 / 7,036 |
| 24 | `8ad6b84` | the listed-window functions to `game/mode_listed.c`, the character-ID lookup to `world/actor_lookup.c` | move | 17 / 5,088 |
| 25 | `397ec03` | the render object's resource accessors to `actor/render_object_res.c` | move | 9 / 2,586 |

15 of the 23 are moves of a definition to a file of its own or to its header's library, as in
step 1 of the restructuring; 8 go through a hook or a table that `client_install_hooks()` fills
in, as in step 2. Each hook says where it is declared who installs it and what happens while it
is NULL.

### What is left

No SCC holds the game any more. From cut 20 on the largest is the lower libraries' 78 files
(the [second SCC](#the-second-scc)); the rest are components of their own:

| files / lines | what |
|---|---|
| 78 / 18,786 | render, resources and widgets (the table's exceptions and the cycles inside `render` and `resources`) |
| 24 / 14,697 | the effect primitives (`feat-rag`) |
| 12 / 4,953 | the navigation: manager, route, guide and its windows |
| 9 / 2,586 | the actor and effect object model: the method tables in `actor/game_object.c`, the actor message, process and state code, the effect base |
| 9 / 4,318 | the chat line and the item tags it shows |
| 8 / 2,782 | the session's job, name and sprite lookups with the world view and resource paths |
| 8 / 1,938 | the party plates |
| 8 / 3,585 | the guild windows |
| 7 / 989 | the map entry and its login steps |

The library table still has one `knot` library for the files that were in the knot; the checks
pass with it, but it no longer names one SCC. Splitting it into libraries along these components
is the next step and needs a decision on how fine the libraries should be.

### Validation

Clean exports of each commit, logs `libs-v01` to `v03`, `w04` to `w13` and `x14` to `x23` in the
worktree's `build/`.

- **Each commit:** `build.bat` without compiler, librarian or linker warnings; the include and
  link checks pass with the same 269 and 129 exceptions; the full `tests\build.bat` and the seven
  standalone suites pass. Every result line is the one of the export before the cuts
  (`pfinal3`), the effect trace, packet dispatch, effect factory, window list and startup order
  digests among them, except the two library-check lines (more files, includes and uses, and 19
  libraries since `feat-rag`).
- **The first full runs failed** on three tests that compile or include a file whose definition
  moved or whose call became a hook: `test_incoming_whisper` (includes `game/mode_whisper.c`),
  `test_lua_boundaries` (includes `contents/client_lua.c` and counts its registration stub's
  calls) and `test_weather_state` (compiles `actor/weather.c`). Each now compiles or includes the
  new file or installs the hook, in the commit that needs it, and passes with its result line
  unchanged. Every test that compiles or includes one of the changed files was checked.
- **At `397ec03`:** `build.bat x64` and `freetype`, the same three with `check`, and `libs`: no
  warnings. CMake with MSVC, Win32 and x64 (Debug): no warnings, CTest 23 of 23 and 22 of 22.
  Linux (WSL, GCC 13.3): CTest 28 of 28, 1,665 warnings as before.
- `git diff --check` passes for each commit.

Not run: macOS and Android, a live client. No test drives the hooks' installation in
`client_install_hooks()` (`core/client_init.c` is in no test), as for the hooks before.

## Splitting the knot library (2026-11-06)

Decision: after the knot cuts Patrick asked for a proposal to split the library table's `knot`
library, and chose the four layers below over a finer split of `game-parts`.

**By directory does not work.** With the 235 `.c` files grouped by directory (actor with the
effect base, session, game, world, navi, contents, the effect classes of `effects/skills`, ui),
every group uses every other one through some file. The best of all orders of the eight groups,
with each file in the lowest group at or above its own that is at or above everything it uses and
a local search toward the files' own groups, still puts 124 files (33,221 lines) in another
directory's library: most directories hold low helpers, part of the actor model and high game
mode code.

**Around the actor core it layers.** Taken against the SCC of `actor/game_object.c` (the 9 files
left after cut 25), the files fall into what the core uses, the core, what uses the core and what
does neither, and between these four there is no use against the order:

| library | uses | `.c` files | lines | what |
|---|---|---|---|---|
| `game-state` | `gamedata` | 51 | 9,996 | what the core uses: the session's data and lookups, the world and its lookups, game, actor and effect helpers, the mercenary AI commands |
| `actors` | `game-state` | 9 | 2,586 | the core: the method tables of `actor/game_object.c`, the actor message, process and state code, the effect base, `game/mode_util.c` |
| `game-parts` | `game-state` | 72 | 15,684 | neither: guild, party, agency and account data and windows, effect classes that do not touch actors, navigation data, smaller pieces, which do not use each other |
| `game-world` | `actors`, `game-parts` | 103 | 35,216 | what uses the core: the actor classes, the game mode's actions and the windows tied to the actors, the navigation route and guide, the run loop (`game/mode_run.c`) and `game/lifecycle.c`; one connected piece |

The feature libraries use `game-world` where they used `knot`.

Two commits:

- `3209b12` moves `g_isSkipFrame`, `g_followLastMoveTick` and `g_followLostSinceTick` from the run
  loop to the new `game/mode_callbacks.c`, with their header in `gamedata`: the core and the
  effects read them, and the layering would not allow the run loop below them. This and
  `game/lifecycle.c` (the game mode's method table, which names the run loop's functions) were
  the only link-check failures of the trial table.
- `54ce159` writes the table. A header goes with the `.c` of its name unless a lower library
  includes it for its types or globals, as `effects/rag/effect_prim.h` did; the 21 such headers
  are named in the notes of `game-state` (17) and `game-parts` (4). Patterns of the old library
  that no longer name a file are gone. No new exception: still 269 include and 129 link
  exceptions.

**Proof.** A call from `session/session_lookups.c` (`game-state`) to `game_object_construct`
(`actors`) builds, and the link check then fails on exactly that use (`game-state may not use
actors`). `build.bat libs` links the client from 21 static libraries, the four new ones among
them.

### Validation

Clean exports of both commits (`build/snap/s1`, `s2`; the table commit was validated as
`44e1e41`, the same tree as `54ce159` before its message was corrected), logs `libs-s1-*` and
`libs-s2-*` in the worktree's `build/`.

- **Both commits:** `build.bat` without warnings; the full `tests\build.bat` and the seven
  standalone suites pass, every result line as before the cuts (`pfinal3`) except the two
  library-check lines (22 libraries at `54ce159`).
- **At `54ce159`:** `build.bat x64` and `freetype`, the same three with `check`, and `libs`: no
  warnings; CMake with MSVC, Win32 and x64 (Debug): CTest 23 of 23 and 22 of 22; Linux (WSL,
  GCC 13.3): CTest 28 of 28, 1,665 warnings as before.

Not run: macOS and Android, a live client.

## Consolidation and rename to custom (2026-11-08)

Patrick asked for the branches to be cleaned up around three lines: `main`, `plv` and this one,
renamed from `purist` to `custom` because it is his client with opinionated changes rather than
only a reduced base.

- `purist-portable` and the `port/*` branches were already merged; they and their worktrees are
  deleted. `purist-libs` (the library layering and the knot split) and the effect gallery
  (`claude/happy-pike-eb58aa`) are merged with `--no-ff`; the gallery's index row for the
  no-canvas fix was taken into the newer index by hand.
- The fixes `main` had that `custom` did not are picked, recorded in
  [the upstream index](upstream/README.md#foundation-fixes-ported-from-main-2026-11-08). Of
  `main`'s reverts of its own fixes, Patrick asked for each to be checked: two rested on a
  misreading (the whip shimmer, the NPC music status) and are followed, with the quest journal's
  extra tab; the others undo deliberate improvements and are not.
- The oracle tests passed nowhere on `custom` as picked; Patrick had them adapted in a follow-up
  the same day (below). The parity harness, the no-comment pass and `main`'s README and
  `docs/changes.md` rewrites are left out.
- The executables are `ragexe*.exe`, as on `main`; `AGENTS.md` says how to work on this branch.

### Validation

- `build.bat check` and `build.bat x64`: no compiler, librarian or linker warnings; the include
  and link checks pass (1,822 files in 22 libraries, 1,151 objects).
- The full `tests\build.bat` passes after the merges and again at the end, 268 result lines (one
  more than after the merges, for the vending retry test).

Not run: `freetype` and `libs` variants, CMake, Linux, macOS and Android, a live client.

### The oracle tests on custom

Each mismatch of the picked tests was read and traced to a deliberate change: the removed jobs and
skills (left out through `tests/oracle/custom_removed.h`, with the ranges of the 4th-class and
third-class removals above), the predicates and the pet act text that went with them, the
skill-action manager's removed skill and object halves (the original's calls are no longer
recorded), and the langtype 20 lock (the original runs with `g_serviceType` 20; under 0 it
launches Korea's Slow Poison effect at `0x806d11`). No reconstruction bug was found. All six tests
pass: 629,006 calls, no mismatch, no fault (`tests/oracle/README.md` has each count).
`build.bat check` and the full `tests\build.bat`, which now runs them before the library checks,
pass with 268 result lines.

## Camera unlock (2026-11-08)

Patrick asked for the zoom and the tilt to be unlocked, with no zoom-out limit, a closer zoom-in
and no snap-back of the zoom.

- **Zoom.** `VIEW_DISTANCE_LIMIT` (10,000, in `game/camera.h`) is the one far limit for the free
  and the fixed (indoor) camera: the setup in `camera.c`, the Ctrl-drag and wheel zooms in
  `mouse_camera.c` and the saved `Outdoor_ViewDistance`/`Indoor_ViewDistance` clamps. It replaces
  `g_maxViewDistance` (400, or 480 with the wide zoom option) and the indoor 300.
  `g_viewDistanceMin` is 100 instead of 230. `g_fixedViewDistanceMax` stays at 300: the sprite
  depth steps scale with it, and Ctrl and a right double-click still reset the zoom to it. The
  Star Gladiator's triple zoom stays, and `pc_cap_zoom`, which pulled a standing Star Gladiator
  back to the far limit, goes with `game_camera_limit_distance`, its only use.
- **Depth range.** The original draws camera-space depth 10..1500 (`frustum.h` and the
  `1.0067114` factor in `rp_project.c`); beyond 1,500 nothing would be drawn at all.
  `RENDER_NEAR_Z`/`RENDER_FAR_Z` in `render/render_api.h` (10 and 40,000, so the triple zoom
  stays in view) feed both. The depth is hyperbolic, so its precision at the usual distances is
  set by the near plane and barely changes. The fog keeps the original's 1,490-unit scale, so far
  zoom ends in fog as the maps define it.
- **Tilt.** `VIEW_PITCH_STEEPEST`/`VIEW_PITCH_FLATTEST` (-85 and -5) bound the tilt on every
  map, with or without Ctrl, in the setup, the Shift-drag and Shift-wheel tilts and the saved
  latitudes. The view divides by the sine and cosine of the tilt, so it stays clear of -90 and 0.
- **The eye outside the map.** `clamp_eye_to_ground` keeps the eye above the ground's height at
  its position. Zoomed far out at a low tilt the eye leaves the map, where `c3d_attr_get_height`
  answers 0; below a map whose ground lies higher, the view looked at the map from underneath.
  The eye now keeps above `c3d_attr_get_height_nearest`, the height at the nearest point of the
  map (half a cell inside its edge); inside the map nothing changes.
- **Left as they were.** Maps with a camera preset keep the preset's bounds. The indoor heading
  still turns only 20 degrees either way. The wide zoom option's checkbox and command still save
  and print their setting, but no longer change the distance.

Tests: `test_frustum` expects the far plane at `RENDER_FAR_Z` and tests boxes around it;
`projection_reference.c` maps depth with the same constants; `test_game_context` loses the
checks of the removed `game_camera_limit_distance`. `test_effect_trace`, which links
`rp_project.c`, has the digest `2842b5a04b52ece4` for its 2,083,072 steps; with `RENDER_FAR_Z` set
back to 1,500 the same build gives the old `5c269a851ffa6be1`, so the depth range is the only
change in it.

## Sprite billboards without warp (2026-11-08)

With the tilt unlocked, Patrick saw characters deform at steep angles. A sprite quad is drawn in
screen space with a depth per row (`render_object_clip_to_quad` and the character previews:
`1 / (view z + rows * g_spriteRowDepthStep)`), so it sorts against the ground like an upright
figure. The same values went into each corner's `rhw`, and the GPU interpolates the texture by
`rhw`, so the image was stretched toward its top or bottom. `g_spriteRowDepthStep` grows with
tan(tilt): at the original's -45 the difference across a sprite is small, at -85 about 11 times
larger.

`render_object_submit_spr_quad`, which also draws the effect sprites, and the character-preview
emitter in `character_render.c` now give all four corners the top row's `rhw` and keep each
row's `z`. The depth test is unchanged, and so is the sorting: the draw queues key a sprite by its
first corner's `rhw`, which was the top row's in the original too. (A first version took the
foot's `rhw`; that moved each shadow behind the figure standing on it in the sorted list, so the
figure's nearly transparent edge pixels wrote depth first and left an outline of bare ground in
the shadow when zoomed in.) The Direct3D 9 backend takes quads with one
`rhw` through its flat path, and the GL backend builds `w` from `rhw` and depth from `z`.

That left the tops of the sprites cut off along a line at steep tilts. The row depth models an
upright figure that the tilt foreshortens, but a sprite keeps its full height on screen, so near
top-down the step (`g_spriteRowDepthStep`, tan of the tilt) grew about elevenfold and carried the
upper rows through the near plane. `view_process_camera` now computes it with the tilt held to the
original's range, -65..-25: inside it nothing changes, outside it the step keeps its value at the
edge. A first version held the flat sprites' step (`g_depthPerPixelCot`, cot) the same way and cut
the shadows into the ground: that step is the ground's own slope, so it follows the real tilt.

Then the head was cut by the body and the shadow by the ground. A row's rhw was
`1 / (depth + rows * step)`, a curve, while the GPU interpolates z linearly between a quad's top
and bottom, so a tall quad (the body) and a short one (the head) disagreed at the same row by more
than the layer offsets (`pc_layer_depth_bias*`, `g_spriteLayerDepthStep`) once the camera came
closer or tilted further than the original allowed. `sprite_row_rhw` (`render/rp_project.c`)
replaces the curve by its tangent at the anchor, `(depth - shift) / depth^2`, kept at or above
`1 / RENDER_FAR_Z`. Every layer of a figure then lies on one line and only the layer offsets order
them. It serves the upright row depths: `render_object_clip_to_quad`, the character previews in
`character_render.c`, and the effect sprites in `prim_sprite_particles.c`, `prim_kinds_148_155.c`
and `prim_kinds_166_173.c`. At the original's zoom (230 and more) and tilt it differs from the
curve by about one percent at most.

The shadows were still cut when zoomed in. A sprite lying flat on the ground (the slanted path of
`render_object_clip_to_quad`, `render_object_render_shadow` in `pc_layers.c`,
`character_render_shadow_quad` and the flat quads of `prim_kinds_166_173.c`) took a fixed 0.16 /
`scaleY` camera-space units per screen row times cot of the tilt (`g_depthPerPixelCot`). Near the
centre of the screen the ground plane's own 1/z changes by cot(tilt) / (`proj[1]` * depth) per row,
and the two agree only at a depth of about 292 (`proj[1]` is 3.8 times the screen height), near the
default distance of 300; zoomed in to 100 the fixed step was about three times too steep and sank
the shadow's far half into the ground. A first fix used that centre-of-screen value, which still
cut the shadow at a nearly level tilt: there tan(tilt) is small, so a character standing some rows
off the centre, or a tilt read from the integer-degree sine table, or the eye held above the ground
by `clamp_eye_to_ground`, changes the slope by a fifth or more.

`sprite_flat_row_rhw` now takes the slope from the view matrix itself. With `up` the world's up
axis in view space (entries 3..5 of the matrix) and `pos` the anchor in view space, a ground point
on the screen row `t = vy / vz` satisfies `up . (vz * (u, t, 1)) = up . pos`, so its 1/z is linear
in the row with the slope `up_y / (proj[1] * up . pos)`. That holds for any zoom, tilt and screen
position and whatever moved the eye. The world view stores its up axis in `g_viewGroundUp` when it
builds the matrix (`view_build_view_matrix`); the shadow, the character preview and the effect quads
pass their own matrix. `g_depthPerPixelCot` has no use left and is gone; `sprite_ground_lift` takes
the same slope.

The feet then sank into the ground at some angles. An upright sprite's rows below its anchor (the
feet reach a few sprite pixels below it) get farther away row by row, while the ground below the
anchor comes nearer, so those rows are always a little behind the ground. The original covers that
with its scale bias (`self->scale * g_spriteScaleDepthBias`), which fits its own zoom and tilt; at
the default distance and -45 the gap over eight sprite pixels is about half that bias, but zoomed
in with a steep tilt, or tilted towards level (where the ground's slope, cot, grows), the gap is
larger. `render_object_clip_to_quad` now raises the bias of an upright sprite to
`sprite_ground_lift`, the gap over `SPRITE_FOOT_ROWS` (eight) sprite pixels, whenever that is the
larger. The bias is per object, the same for every layer, so the layer order is unchanged.

Near a level view the shadow then sank again and, at the lowest tilts, vanished. A shadow is a
screen-space image of fixed height, while the ground it lies on shrinks to a thin band as the view
grows level (down to nothing where `clamp_eye_to_ground` holds the eye at the ground's height). Its
upper rows then stand for ground far away or for no ground at all, and the plane slope behind them
grows without bound. `render_object_render_shadow` now foreshortens the shadow's height with the
ground: `sprite_ground_squash` is the sine of the angle between the view ray and the ground over
sin 45, about 1 at the original's default view (1.28 at its steepest, -65), up to 1.5 looking
straight down and towards 0 near level. That also keeps the shadow's depth finite when the eye is
almost in the ground's plane. Flat sprites are moved towards the camera by a thousandth of their
depth (`FLAT_DEPTH_SLACK`, on top of the original's 1e-5) to stay above the ground's own triangles,
and `sprite_ground_lift` lifts an upright sprite's feet by twice that, so the feet stay above the
shadow. The character previews and the flat effect quads keep their heights.

## Anisotropic texture filtering (2026-11-08)

With the tilt unlocked the camera can look along the ground at grazing angles. Each screen pixel
there covers a long, thin footprint of the texture, which a linear minification filter blurs (it
takes its mip level from the long side) or, without mipmaps, aliases. (The flat grey ground seen
at the same time had another cause; see the next section.) Both backends
now minify with anisotropic filtering where the device has it, up to 16 samples: Direct3D 9 sets
`D3DTEXF_ANISOTROPIC` and `D3DSAMP_MAXANISOTROPY` wherever the client asks for linear
minification (`max_anisotropy` reads `D3DPTFILTERCAPS_MINFANISOTROPIC` and `MaxAnisotropy` from
the adapter's caps), and OpenGL sets `GL_TEXTURE_MAX_ANISOTROPY` on mipmapped linear samplers when
`GL_EXT_texture_filter_anisotropic` or `GL_ARB_texture_filter_anisotropic` is there. Screen-aligned
quads (the UI, sprites) have no anisotropy, so they look as before. The GL backend is not built on
Windows; it was not compiled for this change.

## Ground cells at the camera's plane (2026-11-08)

Near the flattest tilt the ground of Prontera's main street turned into one flat colour (white,
grey, khaki or olive, changing from frame to frame), sometimes with black blocks, while the
light map's shadows stayed. It is not from this day's depth changes: it shows with `RENDER_FAR_Z`
back at 1500 too. The original's camera never meets it.

`tests/scenes/scripts/camera_level_view.lua` reproduces it: the scene drives the camera to the
closest zoom and the flattest tilt, which no saved camera can then change. Ruled out one by one
on that scene: the far plane, mipmapping (the ground textures have one level with Trilinear off),
the texture binding cache (`gfx_set_texture` binding every time), the light map (stage 1 off),
water, fog, the opaque and mesh lists, the texture coordinate indices, point sampling, the sorted
two-texture list (every entry matched its primitive), and the texture itself (read back from the
device at the moment it was drawn: the full cobblestones). Painted white, the flat area was the
ground pass's own.

The cause: at the closest zoom and a tilt of -5 the eye is about nine units above the ground,
and the ground cells under and behind it reach the camera's plane. The ground and its water are
drawn as pre-transformed vertices (`renderer_project_vertex`), so a corner at or behind the eye
gets a negative `rhw` and lands hundreds of thousands of pixels off screen on the other side. The
quad becomes a triangle across the view, in front of the street, with its texture sampled at
about one point. `c3d_ground15_draw_surface` and `water_quad` now leave out a quad with such a
corner (`behind_eye`). Those cells lie below the bottom of the view; on the scene the street is
textured to the bottom edge.

Models have the same failure. Their triangles are projected on the CPU as well
(`c3d_node_v1_render`, `c3d_node_v2_render`), and with the eye inside a building its walls and
roof around the eye reached the camera's plane and smeared their textures in stretched bands
across the view. Both loops now leave out a triangle with a corner at or behind the eye
(`behind_eye` in `model_node_render.c`); the eye inside a building then sees out through it.
Clipping such triangles at the near plane instead would keep their visible parts; it is not done.

### Commits of the camera work (2026-11-08)

`7f94f40c` unlocks the zoom and the tilt, `7540b60b` keeps the sprite billboards and shadows whole,
`9e10dc18` filters textures anisotropically, `b2ae8e24` keeps the eye above the map outside it and
`bc8f1ab7` leaves out the ground, water and model triangles behind the camera. Validation at
`bc8f1ab7`: `build.bat check` and `build.bat x64` without warnings, the include and link checks
pass (1,822 files in 22 libraries, 1,151 objects), and the full `tests\build.bat` passes with 268
result lines, the six oracle tests among them. The scene `camera_level_view` shows the street
textured at the closest zoom and the flattest tilt; Patrick checked the zoom, the tilt, the sprites,
the shadows and the camera inside a building in the client. Not run: `freetype` and `libs`, CMake,
Linux, macOS and Android.

## Ground drawn from device-projected meshes (2026-11-08)

Patrick asked how hard dynamic shadows would be on `custom`, for buildings rather than sprites,
and then for a first step on a branch from `custom`. Shadow mapping needs the device to know where
each triangle lies in the world, and every vertex of the client reached it projected by the CPU
(`renderer_project_vertex`, `XYZRHW`). The houses' shadows on the ground are already in the map's
lightmaps; what is missing is shadow on the models themselves. This step moves the ground, the
largest and simplest static geometry, to meshes the device projects; models and the shadow pass
are not done.

- **The device API.** `render/gfx.h` gains `gfx_mesh_create`/`gfx_mesh_destroy` (world-space
  vertices with colour and two texture coordinates, 16-bit triangle-list indices),
  `gfx_set_world_view` and `gfx_draw_mesh`. `struct GfxWorldView` carries the CPU's own projection:
  the 4x3 view matrix, `proj[0..1]`, the screen centre, `RENDER_NEAR_Z` and
  `RENDER_DEPTH_SCALE`, so a point lands on the pixel and depth `project_vertex` gives it, and the
  fog start and end that reproduce `rp_vertex_fog_alpha` (`renderer_world_view` in
  `rp_project.c`, which also covers its case of fog near equal to far). Direct3D 9 draws a mesh
  through the fixed-function pipeline: the view matrix as `D3DTS_VIEW`, a projection built from
  the current viewport, linear vertex fog on the view depth (range fog off) and lighting off, set
  around the draw and put back after it. The vertex and index buffers are `D3DPOOL_MANAGED`; the
  mesh keeps its arrays, and the three places that release the device drop its buffers so the
  next draw makes them on the new device. The GL vertex shader has a mesh branch: one matrix to
  the pre-transformed vertex's pixel space times the view depth, then the same pixel-centre
  mapping as before, and the fog factor from the depth, rounded down to a step as the CPU's is.
  The buffers are made on the first draw, on the main thread, as textures are.
- **The original's vertex-buffer path goes.** `gfx_draw_vbuffer`, `gfx_vbuffer`, `struct RPMesh`,
  `struct VertexBuffer`, the `RENDER_OPAQUE_MESH`/`RENDER_ALPHA_MESH`/`RENDER_OVERLAY_MESH` queues,
  their flushes and the `l_opaque_mesh`/`l_alpha_mesh` timing phases had no producer; the one
  mesh queue now is `RENDER_GROUND_MESH`.
- **The ground.** `world/ground_draw.c` builds a mesh per block of 32 by 32 cells on the ground's
  first draw (`GND_GPU_BLOCK`, at most 12,288 vertices): four vertices and two triangles per
  surface in the corner order of `c3d_ground15_draw_surface`'s strip, grouped into runs of one
  texture and one lightmap page. `c3d_ground15_render` queues each block that shares a cell with
  the scene-graph leaf once a frame, with `renderer_add_mesh_draw`, and `flush_ground_mesh`
  draws them right after the ground's two-texture quads, with their states. The device clips what
  the CPU path left out with `off_screen` and `behind_eye`. Water keeps its per-cell loop.
- **When the CPU path stays.** The meshes hold each surface's own colour, which the CPU path uses
  only while lightmaps are on, the two-texture path is supported and the weather colour is
  exactly white (`g_weather.color` is 1, so the scaling changes nothing); otherwise the whole
  ground takes the old path that frame. Surfaces with `halfAlpha` take colour alpha 0x80 there
  unless the player is blind, so they stay on the CPU path then and join the meshes while the
  player is blind.
- **Depth ties.** The device rounds the depth differently from `project_vertex`, by a few steps
  of the depth buffer, and the ground is drawn after the models with `LESSEQUAL`. Where a model
  lies on the ground (Prontera's flower beds, which z-fight with it in the original too) a row of
  pixels changes sides. A bias of 2^-20 to push the ground back was tried: it kept the flower
  beds' edges but flipped many more of the z-fighting pixels (954 instead of 82 in
  `prontera_crowd`), so the meshes use none.

Validation, in `C:\Users\Patrick Kuhlmann\GIT\client-files` with `RAGEXE_MSAA=1`: all twelve scene
captures from a scene build of the unchanged objects repeat exactly, and the meshes' captures
differ from them by at most one colour step (the fog factor, which the fixed-function pipeline
does not round down) except in the flower beds' z-fighting: 82 pixels in `prontera_crowd` and 182
or 183 in each of the six captures of `granny_dead`, `granny_models`, `guild_flags` and
`hunter_traps` (all in Prontera), none in `camera_level_view`, `iz_int03_enter`, `keyboard` and
`windows_open`. `build.bat check` passes (1,822 files in 22 libraries, 1,151 objects), and so does
the full `tests\build.bat`. `test_projection` now checks that `renderer_world_view` puts points on
the pixels and depths of `renderer_project_vertex`, with the fog of `rp_vertex_fog_alpha` to one
step, for three fog ranges, near equal to far among them; `test_renderer_storage` feeds the
ground-mesh queue and counts the queues with `RENDER_PASS_COUNT`, its render digests as before, and
`test_effect_trace` keeps the digest `2842b5a04b52ece4`. `gfx_gl.c` compiles without warnings
against SDL3's headers (x64 `cl /W3`); it was not run, and `test_gfx_gl` draws no mesh. Not run
either: `x64`, `freetype` and `libs` builds, CMake, Linux, macOS and Android. Next: the models
(`model_node_render.c`, whose colours follow blindness, highlight, the actor's alpha and the
ground lighting per frame), then a shadow-map pass from the map's light direction.

## Model faces drawn from device-projected meshes, and the weather colour (2026-11-08)

Patrick asked for the models next. A model node's faces are lit once when the model is placed
(`c3d_node_light`: `FaceInfo` colours and the ground-lit `lightInts`), so like the ground they
can live in a static mesh; what changes per frame is the node's matrix and a few states.

- **What goes into a node's mesh.** `world/model_node_render.c` builds it on the node's first
  draw: three vertices per face with the position, the `FaceInfo` texture coordinates and the
  colour `face_colours` gives (the `lightInts` while `g_weather.ground_lighting` is on, the
  `FaceInfo` colour otherwise, alpha 0xff), grouped into runs of one texture, one-sided and
  twoSided apart. Faces of an alpha-keyed texture (format 1) stay out, as do version 2 faces whose
  texture has a texture-coordinate animation (`uv_xform_find`): those go into the sorted alpha
  lists or need new coordinates every frame. A node with more than 21,845 faces or a corner
  outside its vertices keeps the CPU path. The mesh is rebuilt when `ground_lighting` changes.
- **When it is drawn.** While the actor is opaque (alpha 0xff), the player is not blind (that
  path uses the weather tint) and the node is not highlighted; otherwise the node's faces take the
  old path that frame. The mesh's draws go into `RENDER_MODEL_MESH`, flushed right after
  `RENDER_OPAQUE_CLAMP` with its states, where those faces' quads went. The faces the mesh holds are
  skipped in the CPU loops, and a node whose faces are all in it skips the CPU projection.
- **Projection and culling.** A child node is drawn with its parent's matrix to view space as its
  "view", so each node's own matrix to view space (`m`, the one `project()` uses) is the mesh's
  view matrix; no separate model matrix is needed. The CPU loops cull by the screen winding, and the
  two versions test it opposite ways: version 1 keeps the faces that turn counter-clockwise on
  screen (y down), version 2 the clockwise ones, swapped for a mirrored actor (`mirrored`, from the
  signs of its scale), and twoSided faces are kept either way. The draws carry the matching cull
  mode (`GFX_CULL_CW`, the new `GFX_CULL_CCW`, or none), which `flush_mesh_queue` sets and puts
  back.
- **The weather colour on the device.** The ground and the models drew from meshes only while
  `g_weather.color` was white, but that colour is `1 - (1 - ambient) * (1 - diffuse)` of the map's
  own light, white only where one of them is, and it changes while weather darkens the light; the
  indoor map `iz_int03` never used the meshes. `struct GfxWorldView` gains `colourScale`, which
  `renderer_add_mesh_draw` takes with each draw. GL multiplies and rounds down in the vertex shader
  as `face_colours` and `c3d_ground15_draw_surface` do. Direct3D 9 turns on fixed-function
  lighting for a draw whose scale is not white, with the global ambient light set to the scale, the
  material's ambient colour and alpha from the vertex, no emissive or specular and light 0 off, so
  the colour is the vertex colour times the scale to within a step; the states, the material and
  light 0 are put back after the draw. A white scale draws without lighting, as before.
- **Destroying a mesh** no longer flushes the vertex batch; meshes are made, drawn and destroyed on
  the drawing thread, as `gfx.h` now says.

Validation, in `client-files` with `RAGEXE_MSAA=1`, against the same captures of the unchanged
objects (pixels differing by more than one step, then by more than 16):

| capture | > 1 | > 16 |
|---|---|---|
| `prontera_crowd` | 1,176 | 114 |
| `granny_dead` (2), `granny_models`, `guild_flags` (2), `hunter_traps` | 242 to 272 | 209 to 231 |
| `camera_level_view`, moving and still | 17 and 244 | 12 and 204 |
| `iz_int03_enter`, `keyboard`, `windows_open` | 12, 3, 10 | 6, 2, 4 |

In `prontera_crowd`, 1,095 of them are new against the ground-only build, nearly all off by two to
seven steps in the water over the fountain's basin (the basin's texture coordinates interpolated a
little differently under the blend). The others are the flower beds' row, rows of single pixels
along thin model edges, and places where a ground cell's wall and a model's face lie in one plane;
no face is missing or added. `iz_int03` is now drawn from the meshes, and about 40,000 of its
pixels differ by one step (the scale's rounding), the rest on the edges of a hanging chain.
`build.bat check` and the full `tests\build.bat` pass; `test_renderer_storage` queues a model
draw with a cull mode as well, and `test_projection` checks the colour scale `renderer_world_view`
passes on. Not run: GL, `x64`, `freetype`, `libs`, CMake and the other platforms. No frame-time
measurement was made.

## Shadows on the models (2026-11-08)

Patrick asked for the shadow pass after the models moved to device-projected meshes. The ground's
lightmaps already hold the shadows the map's editor baked for the buildings, so only the models
receive the new shadows; on the ground they would darken what is already dark. Only the models'
opaque meshes cast them.

- **The colour in shadow.** A model face's lit colour is the map's ambient plus its diffuse light
  times the face's N.L, baked when the model is placed (`c3d_node_light`). It now also stores,
  per face corner, the colour without the diffuse part: `C3dNode.shadowColours` beside the
  `FaceInfo` colours (the ambient times the same 128-or-255 factor; unlit models, shade type 0,
  keep their own colour) and `shadowGround` beside `lightInts` (the ground's cell colour added as
  for the lit one). The mesh vertex carries it as `GfxMeshVertex.shadow`, which Direct3D 9 reads
  as the specular colour. A face turned away from the light has the same two colours, so it does
  not darken twice.
- **The shadow map.** `renderer_shadow_frame` (`rp_project.c`), called from `view_on_render`,
  builds an orthographic view along the map's light over a square of 1.5 times the camera's
  distance (200 to 2,500 units) around the point it looks at, 3,000 units deep either way, as a
  matrix from view space (the space every model draw already reaches with its own matrix) to the
  light's. The renderer keeps the light from `renderer_set_light`. `flush_opaque_lists` first draws
  this frame's `RENDER_MODEL_MESH` draws into the shadow map (`gfx_shadow_begin`/`gfx_shadow_end`),
  then draws them with `gfx_shadow_receive` on.
- **Which way the light points.** The map's light vector is the direction the light travels:
  Prontera's is (0.5, 0.707, 0.5), and +y is down in the world. Taken as the direction towards the
  light, the shadow map looked up from under the map and darkened every face that looks up (the
  fountain's basin floor came out black in a view of the shadow test alone).
- **Direct3D 9.** The fixed-function pipeline cannot compare depths, so the shadow pass and the
  receiving draws use the backend's first shaders, HLSL compiled at the first shadow frame by
  `D3DCompile` from `d3dcompiler_47.dll` (Windows 8.1 and later carry it; loaded with
  `LoadLibrary`, so the build needs nothing new) for `vs_2_0`/`ps_2_0`. The shadow pass writes the
  light depth into a 2048 by 2048 `R32F` render target with its own D24X8 depth surface; the
  receiving pixel shader does what the stages do for an opaque mesh (texture times colour, the
  colour scaled by the weather colour and rounded down as before) and blends between the shadow and
  the lit colour by four shadow-map taps interpolated by the sample's place between texels. The
  alpha test stays fixed-function, and so does the fog, from the vertex shader's fog output. Both
  targets are `D3DPOOL_DEFAULT`, dropped before a reset and wherever the device is released, and
  made again on the next shadow frame; the shaders go with the device. Without the DLL, without an
  `R32F` render target, or with `RAGEXE_SHADOWS=0`, `gfx_shadow_begin` answers 0 and the models are
  drawn as before. The GL backend answers 0 for now.
- **Bias and filtering.** A pixel counts as lit while it lies no more than 4 world units behind
  the depth in the map. With 2 units, faces at a slant to the light shadowed themselves in places;
  8 looked the same as 4 on the lit faces. Four taps at half a texel (point samples averaged) left
  the statue's shadow in the fountain in steps of a texel (about 0.8 units at the default zoom);
  weighting the four nearest texels by the sample's position smooths it.

Validation, in `client-files` with `RAGEXE_MSAA=1`: the house's roof overhang now shades the
wall under it, the statue throws a soft shadow into the basin, and the towers' sides away from the
light keep their colour. Against the unchanged objects, 47,846 pixels of `prontera_crowd` differ by
more than a step, 43,939 to 45,631 in the other Prontera scenes, 8,170 and 11,550 in
`camera_level_view`, and 2,962, 2,972 and 312 on `iz_int03` (`iz_int03_enter`, `keyboard`,
`windows_open`; a second run moved the first two by about ten pixels). A view of the shadow test alone (lit
white, shadowed black) was checked on `prontera_crowd` and `camera_level_view`. `build.bat check`
and the full `tests\build.bat` pass; `test_renderer_storage` stubs the shadow calls. Not done:
shadows from models outside the view (only the frame's model draws cast, so a shadow can end at
the screen's edge), shadows on the ground, sprites or effects, the GL backend, a frame-time
measurement, and a setting in the options window.

## Sharper shadows: cascades, steady texels, offset and soft edges (2026-11-09)

Patrick found the first shadows blocky and asked for shadows that look exact on Direct3D 9: a
map fitted to the view, more texels, cascades, texels that stay put, and a bias that holds on
slanted surfaces. Seeing them sharp, he asked for the soft border the first, coarse map had.

- **Why they were blocky.** One 2048 map covered a square of 1.5 times the camera's distance in
  radius, about 1,700 units across in `prontera_crowd`: 0.8 units a texel, while one world unit is
  about six screen pixels at the default zoom (RO's view is 15 degrees high), so a texel covered
  about five pixels. Most of the square lay off screen.
- **Fitted cascades.** `renderer_shadow_frame` now cuts the view into four slices of view depth
  (`GFX_SHADOW_CASCADES`), between where the view's four corner rays enter and leave a band from
  300 units above to 60 below the camera's focus (+y down), where the receiving models stand, no
  further than four times the camera's distance (at most 8,000). The slices grow with the depth
  (0.7 of the way from even to geometric steps). Each cascade's map covers its slice's part of the
  band as the light sees it, plus a margin, in its own 2048 by 2048 `R32F` target; the four share
  one depth surface (80 MB with it, where one 4096 map with its depth surface takes 128 MB and
  spends the same texels evenly). The light's map is orthographic, so a caster off screen that
  shades something in view lands on the same place of the map and is kept; only its depth range
  (3,000 units either way) bounds it. By the view's geometry (not measured), a texel at the default
  view is now about 0.15 units, under a screen pixel.
- **Choosing a cascade per pixel** needs dynamic branching, so the receiving shaders move to
  `vs_3_0`/`ps_3_0`; the backend asks for shader model 3 (in hardware vertex processing) and
  `GFX_SHADOW_SIZE` textures and draws no shadows without them. Shader model 3 leaves the fog to
  the pixel shader, which blends towards `D3DRS_FOGCOLOR` while `D3DRS_FOGENABLE` is on, by the
  same linear factor. The shadow pass still uses `vs_2_0`/`ps_2_0`, once per cascade
  (`gfx_shadow_cascade`).
- **Steady texels.** A cascade's half size is rounded up to a multiple of 8 units and its centre
  kept to whole texels, so its texels stay put while the camera moves and edges do not crawl.
- **Offset and bias.** Each model vertex now carries its face's normal
  (`GfxMeshVertex.nx`/`ny`/`nz`, `D3DFVF_NORMAL`; zero for the ground, which does not receive). The
  receiving vertex shader moves the lookup off the surface towards the light by 1.5 texels times
  the sine of the angle between the surface and the light, and the constant bias stays 4 units: at
  1 unit, Prontera's flower beds, which lay their flowers a unit or two over the grass, shaded the
  grass under them.
- **Soft edges.** The lookup is the share of 16 points on a disc of 0.5 units radius that are lit
  (point samples of the map), so edges fade over about a unit, as the first coarse map blurred
  them. Over that disc a slanted surface moves along the light, which shaded a band of a beam that
  lies at a steep angle to it, so the bias grows by the disc's radius times the tangent of the
  slant (up to 8 times). With a radius of 1.5 the edges spread over three units and the shadows of
  thin casters, such as a lamp post's across the fountain, faded away.

Validation, in `client-files` with `RAGEXE_MSAA=1`, captures compared with the first shadow
build and with each step: the statue's and the lamp post's shadows in the fountain keep their
shapes with smooth edges instead of steps of a texel; the house's beam keeps its shadow (its
brightness across the edge measured the same in all three builds); the flower bed shows no shadow
of its own flowers. Against the unchanged objects, 39,575 pixels of `prontera_crowd` differ by
more than a step, 30,237 to 32,027 in the other Prontera scenes, 46,272 and 85,752 in
`camera_level_view`, and 2,585, 2,559 and 306 on `iz_int03`. A view of the shadow test alone (lit
white, shadowed black) found the band the soft edges shaded on the beam and showed it gone with the
slope bias. `build.bat check` and the full
`tests\build.bat` pass. Not done: shadows from models outside the frame's draws, the GL backend,
a frame-time measurement, and a setting for the softness.

## The shadow maps replace the baked shadows (2026-11-09)

Patrick asked for the moving windmill sails at yuno_fild01's south gate to shade the ground, and
then said what the shadow maps are for: to replace the shadows the map's editor baked into the
lightmaps, matching them.

- **One map for models and ground.** The ground's meshes now cast into the shadow maps as the
  models' do (each ground surface gets its face's normal, so the offset and the slope bias work on
  it), and both receive them. A first version kept a second set of maps, cast only by animated
  model nodes, for the ground alone, so that it would not darken the baked shadows twice; with
  the baked shadows replaced there is nothing to darken twice, and it went.
- **The ground's receiving shader** (`ground_hlsl`, `GFX_SHADOW_GROUND`) does what its two stages
  do, lightmap colour plus intensity times texture times colour, but takes the intensity as lit
  down to the darkness of a shadow and lets the shadow maps darken it instead; texels darker than
  a shadow keep their own darkness (`saturate(intensity / darkness)`). Where the shadow maps are
  not drawn (no shader model 3, `RAGEXE_SHADOWS=0`, the GL backend, a surface on the CPU path),
  the baked shadows stay as before.
- **How dark.** The baked intensities are two levels: 128 in shadow, 255 in the sun, some 45
  percent of the texels each in the GNDs of yuno_fild01, prontera and iz_int03, the rest on the
  edges between. A shadow is therefore half the lit colour (`SHADOW_DARKNESS`, 128/255), for the
  ground and the models alike. That replaces the colour without the diffuse light that each model
  corner (`shadowColours`, `shadowGround`) and ground corner carried before, which was darker than
  the baked shadows (the map's ambient alone, 0.3 at yuno_fild01); the vertex loses that colour.
- **Which way.** The light vector is the direction the light travels. Mirroring x was tried after
  one windmill's shadow seemed to fall the wrong way, but it had been compared with another
  windmill's baked shadow: a test build drew, on every ground pixel, the baked intensity and the
  shadow test side by side, and the share of pixels shaded in both over those shaded in either
  came to 0.49, 0.72 and 0.37 on `yuno_gate`, `prontera_crowd` and `camera_level_view` as it is,
  against 0.29, 0.09 and 0.25 with x mirrored, and at most 0.35 with z or both.

Validation, in `client-files` with `RAGEXE_MSAA=1`: side by side with the unchanged objects, the
shadows on the ground of the three maps fall where the baked ones did, about as dark; the
windmill's sails shade the grass as they turn. `build.bat check` and the full `tests\build.bat`
pass. Not done: alpha-keyed faces (tree leaves) and texture-animated faces do not cast, models
outside the frame's draws and models fading, highlighted or seen blind do not cast, the ground's
half-alpha surfaces keep the baked shadows and its water takes none, and the models' ground-lit colours
(`lightInts`, from the lightmap under them) still carry the baked shadows.

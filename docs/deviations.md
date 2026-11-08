# Where this tree is not the original

An inventory, made 2026-11-08, of every place the lifted client is known to behave differently
from the 2022-04-06 executable on purpose. It is the audit the review of upstream's fixes asked
for: the reconstruction bugs we find are accidents, but the lift also carries deliberate changes
that arrived with it, and until now nothing in this repository listed them.

## Where they come from

This tree is the plain-C rendering of herc-client-integration's reference tree, which was lifted
from herc-client's FASM reassembly. Both ancestors mark their changes with `HERC PATCH (name)`:
herc-client in `text/*.inc` and `src/*.c`, the reference tree in its `src/`. The comments were
stripped when this tree was generated, so the markers are not here, but the changes are. The
list below was built from the markers of both ancestors (294 names in the reference tree, 67 in
herc-client, with the lift's own mechanics removed), from herc-client's `faithdiff_expected.tsv`
(the byte-level residue it explains), and from reading each site in this tree's `src/`. Three
of herc-client's edits carry no marker at all and were found through the oracle tests; there may
be more, and only a run of herc-client's `tools/faithdiff.py` against a fresh build, with every
expected-residue rule re-checked, would say.

"Original" below is what the executable does; "main" is this tree today.

## Start-up plumbing: what it takes to run outside Gravity's service

| name | original | main |
| --- | --- | --- |
| `force_1sak1` | goes on only with `1rag1`, `1sak1`, `1gld1`, `2rag2`, `1eve1` or `1pk1` on the command line, else shows "run the client from the launcher" and exits | the search result is dropped (`winmain.c`); any command line runs |
| `skip_iprocess_check` | for service korea, runs the IProcess module scan and refuses to start when it fails | does not call IProcess at all |
| `disable_gameguard` | runs the nProtect GameGuard blocks of the game and login modes | `gameguard_*` are empty (`security/gameguard_mgr.c`) |
| `disable_findhack` | the two anti-cheat scans run inside `CNProtectGameGuardMgr::CreateInstance` and refuse the start on a hit | the scans are gone with the manager |
| *(untagged)* RagHash.dat | loads the signed file-integrity list `RagHash.dat` at start-up and refuses to start when it is missing, has fewer than two entries or fails its checksum | `gg_hash_manager_startup` builds an empty list and never reads the file; herc-client NOPed the load and forced the success flag, both without a marker |
| `disable_gdi_tamper` | a schedule compares the `TextOutW` import with a fresh `GetProcAddress`, looks for a debugger, and on a mismatch disconnects at random | the schedule does not exist |
| `remove_hardcoded_addr` | chooses a `*.ragnarok.co.kr` account server by `g_serverType` | builds `<address>:<port>` from `clientinfo.xml` |
| `hkcu_registry` | the settings live under `HKEY_LOCAL_MACHINE` | under `HKEY_CURRENT_USER` (30 sites) |
| `retire_inactive_patcher` | the start-up's patch applier opens `data.grf ` (with the trailing space) and returns before writing | the open is kept for its side effect, the applier is gone |
| `use_revel_xml`, `load_revel_grf` | reads `clientinfo.xml` and `data.grf` | **restored**: herc-client's `revel.xml` and `revel.grf` are not here |
| `prefer_lua`, `skip_luafiles514_prefix` | loads `LuaFiles514\<name>.lub`, then `.lua` | **restored** (`contents/client_lua.c`) |

These are the same set `tests/parity/original_patches.txt` patches into the original so it can
run in the parity harness, which is the check that the list is complete for start-up: an
original that stops where the lift does not is a deviation this table lacks.

## Protocol: what the server sees

| name | original | main |
| --- | --- | --- |
| `service_type_branch` (login) | sends the exe-digest packet `0x0204` only for service 6, or for korea with `g_serverType == 1`, and replaces the digest with a fixed key (`82 d1 2c 91 ..`, `65 e3 64 31 ..`, zeros, or `c7 0a 94 c2 ..` by service and server type) | always sends `0x0204` with the exe's real MD5 (`login/account.c`) |
| `service_type_branch` (messages) | takes a separate path for service korea when loading the message strings (the `g_serviceType == 0` branch herc-client jumps over at `core_0000.inc:1197`; what it loads there has not been read) | reads `msgStringTable.txt` for every service (`core/msg_str.c`) |
| `crash_on_unknown_packet` | an unknown packet id flushes the connection's queues | the same; the reference tree's optional fault is not built here |

The digest packet matters to anyone recording the original against a server: for a korean
clientinfo the original never sends it, and the lift always does.

## Behaviour a player can see

| name | original | main |
| --- | --- | --- |
| `yield_not_sleep` | `Sleep(1)` after every frame | `Sleep(g_windowActive == 0 && fullscreen ? 1 : 0)` since 988d58e1 (2026-11-02): neither the original's millisecond nor herc-client's yield |
| `mode_list_cap` | the graphics settings list modes up to 2048 x 1440 | up to 8192 x 4096 |
| `getjob_one_mask` | `CGameActor::GetJob` asks for the effect-state mask through a virtual call before each of its forty rules | asks once and tests the bits inline (`actor/getjob.c`); meant to be the same answer, not yet proven by the oracle |
| `index_actors_by_aid` | finds an actor by account id by walking the world's list | keeps an index beside the list (`actor/actorindex.c`); meant to be the same answer |
| `borderless_toggle`, `live_resize` | fullscreen is an exclusive mode switch; a size change recreates the device | a borderless window is offered (`ISBORDERLESS`), and a windowed size change is applied live |
| `altgr_text` | Ctrl+Alt is Ctrl+Alt | AltGr types the layout's third-level character (`ui/wndmgr/key_down.c`) |

herc-client's own UI work (`ui_scale`, `ui_hires`, `custom_sprites`, `chat_*`, `channel_types`,
`keep_chat_focus`, `word_keys`, `pick_box_exact`, `zero_offscreen_quad`, `imf_bounds`,
`skin_*`, `chat_history_size`, `hitbox_overlay`) is **not in this tree**: the reference tree was
lifted before or without it, and this tree widens pick boxes, keeps 240 history lines and draws
sprites as the original does.

## Rendering and performance

The renderer is the largest deviation of all: the original draws through DirectDraw 7 and
Direct3D 7, this tree through Direct3D 9 (`render/gfx_d3d9.c`), which is why the parity harness
cannot compare the two pixel for pixel. On top of that backend the reference tree brought:
`msaa` (a multisampled scene target, 4 samples), `batch_draws` (consecutive quads become one
indexed list), `skip_repeated_mode`, `skip_redundant_state`, `skip_unchanged_builds`,
`skip_flip_lock`, `bind_by_backend`, `cache_res_keys`, `cache_sincos`,
`cache_billboard_rotation`, `cache_text_extents`, `device_hung` (a fresh device for a hung one),
`stale_driver_guid` (a registry GUID for a device that is gone), `vsync_present_interval`,
`ui_soft_invalidate` and `ui_ondraw_hook` (windows that redraw only when something changed),
`perf_overlay`, and `save_surface_rt`, which is a repair: the original's fade-out saved its frame
into a surface `StretchRect` refuses, so every restore showed black.

All of these are meant to draw the same picture faster. None has been checked against the
original frame for frame, and the parity harness, once it hooks DirectDraw, would be the way.

## Diagnostics the original does not have

`support_log`, `packet_log` and `packet_trace`, `crash_report` (a minidump beside the exe),
`drive_state` (a state file rewritten four times a second when asked), `filemgr_lock` and
`filemgr_lookup_lock` (counters around the resource caches), `ui_invalidate_census`. They add
behaviour but change none.

## Corrections of the original's own bugs

The reference tree fixed things it found while lifting, each marked. They are deviations in the
strict sense and this list keeps them visible: `attack_record_init` (an uninitialised byte that
coloured hit numbers blue at random), `ncash_button_overrun` (a loop one past its tables),
`save_surface_rt` (above), `player_name_time_wrap` (the 32-bit tick rollover),
`degree_normalization` (-90 is 270, not a read before a table), `hex_unsigned_input`,
`font_lookup_resources`, `line_layout_progress` (a character wider than the row no longer loops
forever), `line_layout_font_bounds`, `literal_pet_prefix`, `radio_style_replace`,
`radio_value_initialized`, `invalid_button_state`, `invalid_timer_kind`, `invalid_volcano_kind`,
`item_link_button_failure`, `chat_navi_allocation_failure`, `guild_page_request_flags`,
`forget_destroyed_windows`, `hide_ui_effect_pointer`, `discarded_login_string`,
`retired_dangling_item_name`, `retire_unused_random`, `retire_unused_vertex_buffers`,
`attendance_reward_format`, `achievement_default_fields`, `defined_skin_colors`,
`defined_effect_coordinates`, `defined_number_conversion`, `ime_cursor_result`,
`balloon_text_lines`, `book_text_ownership`, `bound_npc_dialog_text`, `button_label_alias`,
`button_skin_default`, `effect_duration_table`, `lua_compat`, `json_no_throw`. `fix_proceed_case0`
and `fix_objclick_case0` are not in this class: they repair labels the disassembler misplaced,
and restore the original's behaviour.

## The lift's own mechanics

The remaining 200-odd names (`native_*`, `typed_*`, `bounded_*`, `shared_*`, `c_*`, `*_owner`,
`*_interfaces`, `*_lifetimes` and their kin) describe how C++ objects, ownership and containers
became C. They are the shape of the translation, not changes of behaviour, and the `bounded_*`
family adds length checks on wire data that the original did not have.

## What is left to decide

Each row of the first four tables is a decision for the tree's owner: keep it as a documented
deviation, or restore the original. The ones with consequences outside the client are the exe
digest packet and the message-table source; the one that is a policy choice is the frame sleep;
`getjob_one_mask` and `index_actors_by_aid` should be proven equal by the oracle rather than
decided. Everything in the start-up table must stay, or the client cannot start outside Gravity's
service, but it should stay knowingly.

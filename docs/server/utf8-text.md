# UTF-8 text

`hercules-utf8-text.patch` is a separate server change for the purist client,
which sends and stores names, chat, titles and other text as UTF-8. It has not
been applied to either existing server checkout. The client's own data files are
converted separately with [`tools/utf8-data`](../../tools/utf8-data/README.md).

Unlike the emblem and agency patches, it targets the HampterDev/Hercules fork: commit
`73334c49924b869d87d5f93c510e96e1a6924951` (`origin/main`, "Merge pull request #58
from revelro/feat/remove-char-server", 2025-12-09). It also applies cleanly to
`feat/tournament` at `36d527d80`. The upstream base of the other patches
(`84da0df`, Release v2024.09) is not in the fork's history, and the code changed
here (map-side character selection, `sql_async`) exists only in the fork. The fork
already builds with `PACKETVER=20220406`.

No packet layout changes. Character, party and guild names stay within
`NAME_LENGTH` (23 bytes plus NUL), so a name holds 7 Hangul syllables or
11 Cyrillic letters.

## Server changes

- **Name trimming.** `TRIM_CHARS` in `src/map/charselect.c` no longer contains
  `\255` and `\xA0`. Those Latin-1 bytes are continuation bytes in UTF-8. Every
  Cyrillic Р/Э and many Hangul syllables were turned into spaces on rename.
  Trimming now covers ASCII whitespace and SUB only.
- **Name validation.** New `strlib->utf8_is_valid_name()` accepts only well-formed
  UTF-8: shortest form, no surrogates, at most U+10FFFF. It also rejects these
  characters:
  - controls: U+0000–U+001F, U+007F–U+009F
  - U+00A0 and U+00AD, the targets of the old Latin-1 trim bytes
  - bidirectional controls: U+061C, U+200E, U+200F, U+202A–U+202E, U+2066–U+2069

  It is used by:
  - character creation and rename
  - party creation
  - guild creation
  - pet and homunculus rename

  Client replies are unchanged: char 0xFF "denied", party 1, guild 2.
- **Character creation** now trims and validates like rename. Before this, the fork
  checked only for duplicates, so control characters were accepted, and so was the
  reserved whisper name `Server`.
- **Rename duplicate check** uses `=` instead of `LIKE`, so `_` and `%` in a name
  are no longer wildcards.
- **Boundary-safe truncation.** New `safestrncpy_utf8()` and `strlib->utf8_boundary()`
  drop a character that a length limit would cut short. They act only when the
  text fills the buffer, and they read no more bytes than `safestrncpy` did. They
  are used for:
  - chat room titles: create, change and the shared `chat_createchat`
  - vending titles: stored copy and store board
  - buying store title
  - mail title and body
  - RODEx title and body
  - NPC input string
  - Talkie Box and Graffiti text
  - guild notice
  - guild leave and expulsion reason broadcast
  - `/b` and `/lb`
- **Mail body bound.** A 200-byte mail body was copied with `n = 201` into a
  200-byte array: one byte past the end. The copy is now bounded by the array, so
  the limit is 199 bytes.
- **Connection charset.** `conf/global/sql_connection.conf` sets `default_codepage`
  (map, api and log connections) and `codepage` (login account, ipban and loginlog)
  to `latin1`. The async SQL workers never set a charset. They now apply the same
  one through the new `sql_async->set_encoding()`, which map and api call before
  `sql_async->init()`. A worker that cannot set it does not use the connection.
- **Online name lookup.** In exact mode, `map->nick2sd` prefers a byte-exact match.
  It accepts an ASCII case-insensitive match only when that match is unique. Two
  online characters `Foo` and `foo` can no longer receive each other's whispers.
  Partial-name mode already worked this way.
- **Interfaces.** New members:
  - `strlib`: `utf8_is_valid_name`, `utf8_boundary`, `safestrncpy_utf8_`
  - `sql_async`: `set_encoding`

  The HPMHooking files were not regenerated, so plugins cannot hook these members
  until `tools/HPMHookGen` is run.

## Database

**Decision: keep the latin1 tables and use a latin1 connection.** A latin1
connection over latin1 columns converts nothing, so the client's UTF-8 bytes are
stored and returned unchanged:
- Existing rows stay as they are.
- Column sizes remain byte counts, matching `NAME_LENGTH`.
- It matches what the Windows build already does: the bundled MariaDB Connector/C
  3.1.5 defaults to latin1.

A utf8mb4 connection over the same columns is worse. MariaDB in strict mode
rejects Cyrillic or Hangul with error 1366; servers in non-strict mode store `?`.
Reading raw bytes through utf8mb4 also inflates them: an 8-byte name comes back
as 17 bytes.

A utf8mb4 migration is not included, and I don't recommend one now. It would mean
converting every text column through a binary step. It also requires every stored
byte sequence to already be valid UTF-8. Names saved by older CP949 or Latin-1
clients would fail or be garbled.

**Upgrade:** `sql-files/2026-11-03--12-00.sql`. It is in `sql-files/` itself
because the fork keeps its upgrade files there instead of upstream's
`sql-files/upgrades/`. The fork's update checker is disabled. The upgrade sets
seven columns to `latin1_bin`:
- `char.name`
- `party.name`
- `guild.name` and `guild.master`
- `guild_member.name`
- `guild_alliance.name`
- `guild_expulsion.name`

`latin1_swedish_ci` is not just case-insensitive here: it treats many unrelated
UTF-8 byte sequences as equal. On MariaDB 10.5, 54,592 of the 63,360 BMP
characters have the same sort key as some other character. That includes all
11,172 Hangul syllables; for example, `가` (EA B0 80) equals `밀` (EB B0 80).
With a `가나` already stored:
- creating `밀나` failed as a duplicate;
- a lookup for `밀나` returned `가나`'s character;
- a party name check for `밀나` found `가나`'s party.

Going to a binary collation cannot create duplicates, so `name_key` and the
`guild_expulsion` primary key stay valid. Each column is changed only while it is
a latin1 column of the type the fork ships. Otherwise its statement fails with
`Unknown column 'refused: ...'` and the column is left alone, because a utf8mb4
column must not be converted to latin1. The `sql_updates` row is written only
when all seven columns are `latin1_bin`. `1main.sql` carries the same column
definitions and the `sql_updates` row for fresh installs.

Steps:

```text
mysqldump ragnarok char party guild guild_member guild_alliance guild_expulsion > before-utf8-text.sql
mysql ragnarok -e "SELECT TABLE_NAME, COLUMN_NAME, COLLATION_NAME FROM information_schema.COLUMNS WHERE TABLE_SCHEMA = DATABASE() AND COLUMN_NAME IN ('name','master') AND TABLE_NAME IN ('char','party','guild','guild_member','guild_alliance','guild_expulsion')"
mysql --default-character-set=latin1 ragnarok < sql-files/2026-11-03--12-00.sql
```

The first query should list `latin1_swedish_ci` (or another latin1 collation)
before the upgrade, and `latin1_bin` after it. Run the file without `--force`: the
mysql client then stops at the first refusal. If any column is utf8mb4, don't use
this patch's latin1 setting. Such a database needs `utf8mb4` connections and
`utf8mb4_bin` instead. Running the upgrade again fails on the duplicate
`sql_updates` row, as other Hercules upgrades do.

## Configuration

Both keys in `conf/global/sql_connection.conf` must say `latin1`. Don't leave them
empty: the client library then picks its own default, and Linux distribution
libraries often default to utf8mb4. If your copy of this file has local changes,
set the two keys by hand:

```text
	default_codepage: "latin1"
	codepage: "latin1"
```

Any other tool that writes these tables, such as a control panel or phpMyAdmin,
must also connect with latin1 and treat the bytes as UTF-8. Otherwise it writes
converted text or shows mojibake.

## Behaviour changes

- **Character and party names are case-sensitive.** `Foo` and `foo` can both exist.
  Guild names were never unique in the fork: there is no check and no key. That is
  unchanged.
- **Exact case is now required for SQL name lookups:**
  - creation and rename duplicate checks
  - the party name check
  - mail and RODEx recipients
  - `@block`, `@ban`, `@charban`, `@unblock`, `@unban` and `@charunban` by name
    (`loginif.c`)
  - the `@accinfo` name search, whose `LIKE` becomes case-sensitive
- **Online lookups** keep working in any ASCII case while the name is unambiguous.
  This covers whispers, party and guild invites, friend requests and `@commands`
  on online players. When two online names differ only in case, a non-exact query
  finds nobody. `party->searchname` and `guild->searchname`, used by GM tools and
  scripts, still return the first ASCII case-insensitive match.
- **Rejected names.** Names in legacy encodings (CP949, Latin-1), names with
  controls or bidirectional controls, and new characters named `Server` are
  rejected.
- **Login is unchanged.** The `login` table is not touched. `case_sensitive` in
  `sql_connection.conf` still controls `userid` matching. Its unique key still uses
  `latin1_swedish_ci`, so different UTF-8 user IDs can collide. Enable
  `case_sensitive` before allowing non-ASCII user IDs, and consider `latin1_bin`
  for `login.userid` later.
- **Mail body limit** is 199 bytes instead of 200.

## Apply later

From the intended server checkout, inspect and check the patch first:

```text
git apply --stat <client>/docs/server/hercules-utf8-text.patch
git apply --check <client>/docs/server/hercules-utf8-text.patch
git apply <client>/docs/server/hercules-utf8-text.patch
```

Inside a git checkout of the fork, plain `git apply` handles CRLF working files.
For a loose CRLF source export, add `--ignore-space-change`. The patch uses LF.
Against the current uncommitted `feat/tournament` working tree, two places need
a manual merge:
- `conf/global/sql_connection.conf`, because of local credential and comment edits
- the `TRIM_CHARS` hunk in `src/map/charselect.c`, next to the new
  `CHARSELECT_RECONNECT_AUTH_TIMEOUT_MS` define

The other 15 files apply.

Then rebuild the map, login and api servers. Run the database upgrade and set
the configuration before the first start with the purist client.

## Validation performed

- `git apply --cached --check` succeeds against the exact blobs of `73334c4` and
  `36d527d80`. `git diff --check` is clean. The patch also applies to a fresh
  CRLF export with `--ignore-space-change`. Inside the CRLF worktree, the reverse
  check `git apply -R --check` passes without options.
- MSVC v143 Win32 Release builds of map-server, login-server and api-server
  succeed. The isolated source export is built before and after the patch, and
  the warnings are identical by file and code.
- GCC 13.3 (WSL Ubuntu) compiles all 12 changed translation units with
  `-Wall -Wextra`, again with warnings identical to the base. A full Linux build
  was not possible: WSL has no MySQL or PCRE development packages.
- `utf8-text-validation/test_utf8_text.c` compiles the patched `strlib.c` itself.
  It stubs only the allocator, log and nullpo interfaces, and runs 162 checks with
  AddressSanitizer under MSVC and with ASan and UBSan under GCC; all pass. Source
  buffers have their exact size, so any read beyond the copy limit would abort the
  run. The checks cover:
  - valid, ill-formed, control and bidi names
  - the old and new `TRIM_CHARS`
  - every cut position of mixed 1–4-byte text
  - the chat, store, mail, guild notice and `/b` call-site expressions
  - the mail body bound
- A throwaway MariaDB 10.5.4 instance, with its own data directory on port 33999,
  confirmed:
  - the `가나`/`밀나` and `Alice`/`alice` false duplicates and wrong matches
    before the upgrade, and correct behaviour after it;
  - unchanged keys and byte-exact round trips;
  - the refusal path on a utf8mb4 database (nothing changed, no `sql_updates` row);
  - the patched `1main.sql` on latin1 and utf8mb4 default databases;
  - the connection-charset results above.
- Against that instance, the base map-server opened 4 connections and only 2 ran
  `SET NAMES latin1`: the main and log handles. The two async workers stayed on
  the library default. The patched map-server sets latin1 on all 4, and its
  workers then run the startup fame and clan queries. Config edits for this run
  stayed in the scratch export; the login port pointed at an unused port.

The test, build scripts, SQL checks and MariaDB connection logs are in
[`utf8-text-validation/`](utf8-text-validation/); the MSVC build logs were not
kept. `test_msvc.bat <patched tree>` reruns the helper checks. No live game
server, client, account or player database was used. Before deployment, verify
with the purist client:
- creating, renaming and logging in Cyrillic and Hangul characters, including
  names containing Р, Э, 갠 and 갭
- creating parties and guilds
- whispers to names that differ only in case
- long Hangul chat, shop, mail and RODEx titles and bodies
- a GM `@ban` by name
- persistence across a restart

## Risks and not covered

- **utf8mb4 tables.** The fork's `emotes`, `hotkeys` and `adventurer_agency` tables
  are utf8mb4, and the api server uses them. Through latin1 connections they store
  UTF-8 as double-encoded text. That round-trips through the server, but external
  tools show mojibake, and `utf8mb4_general_ci` compares the double-encoded form.
  The latin1 Windows default already did this.
- **Legacy text.** The truncation helper does not understand legacy encodings. It
  can drop the last byte of CP949 text that exactly fills a buffer. Invisible
  characters such as U+200B and homoglyphs are not blocked.
- **Pre-existing, unchanged:**
  - `guild->validate_name_local` is unused, and guild names are not unique.
  - `char_name_option` is hard-coded to 0 with no config key, and its checks are
    byte-wise; enabling it later needs code-point handling.
  - `clif_parse_GuildChangeNotice` frees an offset pointer when a notice starts
    with a `|xx|` language marker. Fix that separately.
  - The 2012-only `PARTY_RECRUIT` notice is not compiled for this client and is
    unchanged.
- **Report only:**
  - `conf/emotes.conf:37-43` holds UTF-8 Korean default emote commands (`/기쁨` …).
    That suits a UTF-8 client, and no C code in this fork reads the file.
  - `npc/jobs/2-1/priest.txt` lines 1336, 1346 and 1355, in the `npc` submodule
    (revelro/npc-scripts), use CP949 `cutin` image names. They name client data
    files and must stay CP949. Other legacy-encoded NPC dialogue will display
    incorrectly on a UTF-8 client until the scripts are converted.

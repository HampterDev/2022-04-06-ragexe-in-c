# Adventurer's Agency over the game connection

The custom client keeps the Adventurer's Agency windows and replaces their HTTP
advertisement service with binary packets on the existing map-server connection.
Listing, filtering, viewing your advertisement, registering/editing it, deleting
it and checking an advertisement before applying no longer use HTTP or JSON.
The existing application/invitation/acceptance packets remain unchanged.

`hercules-agency-packets.patch` targets Hercules commit
`84da0df5ea655c8339430d2065dbe3b0b1a40577` (Release v2024.09), built with
`PACKETVER=20220406`. This is a **custom protocol extension**, requiring the patch
or equivalent server implementation. An unpatched server does not implement the
new packets. Neither existing server checkout was modified.

## Server behavior and scope

- Advertisements belong to the authenticated party leader. Account ID, character
  ID and displayed name come from the server session. A request cannot set them
  or delete another leader's advertisement.
- Advertisements live in the map-server's party records. Logout, transfer to
  another map-server and a leadership change remove the old advertisement;
  disbanding frees it with the party. Missing/offline leaders and full parties
  are removed from the directory when examined. A full party cannot register.
- The directory covers **one map-server process**, including all maps hosted by
  it. It is not a shared directory across multiple map-server processes. Listings
  are not persisted across server restarts or logout. A multi-process deployment
  would need a char-server/shared directory implementation.
- Listing order is ascending party ID, ten results per page. An out-of-range
  page is clamped to the last page; an empty directory reports page 1 of 1.
- Filters combine overlapping level ranges, any selected role, exact purpose
  (zero means any), and a case-sensitive byte substring in leader name or memo.
  A zero role mask means any role. Text uses the same legacy encoding as game
  names and chat, without an HTTP UTF-8 conversion.
- Each connection is limited to one request per operation every 250 ms. Different
  operations, such as listing plus reading your own advertisement, can run together.
- The extension does not change the older party-booking board or its packets.
  Existing join/acceptance processing remains the server's usual party flow.

## Wire format, version 1

All integers are unsigned, little endian, with no structure padding. Packet
length includes the four-byte ID/length prefix. Strings are counted byte strings
without terminators. Requests may not contain NUL, control bytes below 32 or DEL
in text. The client rejects malformed lengths, invalid entries and stale replies.

Client request: `0x0bf0`, 24–174 bytes.

| Offset | Bytes | Field |
|---|---:|---|
| 0 | 2 | Packet ID `0x0bf0` |
| 2 | 2 | Total packet length |
| 4 | 1 | Version, `1` |
| 5 | 1 | Operation, below |
| 6 | 4 | Nonzero request serial, echoed in reply |
| 10 | 2 | Page, 1–65535; use 1 except for list |
| 12 | 4 | Leader account ID for detail; zero otherwise |
| 16 | 2 | Minimum level, 0–999 |
| 18 | 2 | Maximum level, minimum through 999 |
| 20 | 1 | Role mask: bit 0 tank, 1 healer, 2 damage, 3 support |
| 21 | 1 | Purpose, UI catalog index 0–6 |
| 22 | 2 | Text byte count, 0–150 |
| 24 | variable | Search text or advertisement memo |

Operations: **1** own advertisement; **2** list/search; **3** advertisement detail;
**4** register or replace own advertisement; **5** delete own advertisement.
Only list and register use filter/text fields. Other operations send minimum 0,
maximum 999, zero roles/purpose and empty text. Registration text is capped by
the existing UI's memo limit as well as the protocol limit.

Server reply: `0x0bf1`, 16–1906 bytes.

| Offset | Bytes | Field |
|---|---:|---|
| 0 | 2 | Packet ID `0x0bf1` |
| 2 | 2 | Total packet length |
| 4 | 1 | Version, `1` |
| 5 | 1 | Request operation |
| 6 | 4 | Request serial |
| 10 | 1 | Status: 0 success, 1 invalid request, 2 forbidden, 3 missing, 4 busy |
| 11 | 1 | Entry count, 0–10 |
| 12 | 2 | Actual page, at least 1 |
| 14 | 2 | Total pages, at least actual page |
| 16 | variable | Entries concatenated without padding |

Own/detail replies contain at most one entry. Register/delete replies and error
replies contain none. A successful empty own reply means no advertisement.
The detail operation returns status 3 for a vanished advertisement. Invalid
protocol versions/operations, zero serials and requests shorter than the header
are ignored. The client releases unanswered requests after ten seconds.

Each entry is: account ID `u32`, character ID `u32`, minimum level `u16`, maximum
level `u16`, role mask `u8`, purpose `u8`, name byte count `u8`, memo byte count
`u8`, then name bytes and memo bytes. IDs must be positive signed-32-bit values;
names contain 1–23 bytes and memos 0–150 bytes. Neither contains embedded NUL.

Client requests are correlated separately by operation. New queries replace old
ones; mutations are serialized, with deletion allowed after a pending registration
on the ordered TCP stream. Closing the application window cancels its pending
detail lookup. Replies cannot reopen closed windows or apply to another visible
advertisement. Resetting the game/login state discards pending work and cached
listings while preserving the request sequence.

## Apply later

From the intended server checkout:

```text
git apply --stat <client>/docs/server/hercules-agency-packets.patch
git apply --check <client>/docs/server/hercules-agency-packets.patch
git apply <client>/docs/server/hercules-agency-packets.patch
```

Then rebuild the map server. No database migration or char-server change is
required for this single-process directory. Adapt and review against the intended
fork before deployment. If the checkout uses CRLF source files, add
`--ignore-space-change` to the check/apply commands; the patch uses the baseline
Git blobs' LF line endings. The Agency and emblem patches apply independently and
in either order.

## Validation

- Client release build passed. Packet tests cover encoding, ten-entry pages,
  empty results, all truncated reply prefixes, allocation failures, malformed
  records, stale/duplicate replies, failed sends, cancellation, clock wraparound,
  timeout and disconnect/reset. No HTTP code is linked into these tests.
- UI tests exercise closed windows, target matching, error recovery, empty own
  advertisements, level/already-in-party checks and the existing join message.
- The actual patched Hercules handler is extracted and compiled in a map fixture.
  Tests cover ownership, registration/deletion, filters, pagination, malformed
  requests, throttling and stale/full listings. Its emitted reply is consumed by
  the real client decoder.
- Isolated MSVC Win32 Release map-server builds passed both for the Agency patch
  alone and together with the emblem patch. Existing server warnings remain.
- Both patch orders passed `git apply --check` against the baseline Git source
  and produced byte-identical combined files.
- All 1,179,800 dispatcher traces were compared with the previous client: only
  the new Agency response changed. The remaining HTTP ownership/submission,
  async queue, JSON and game-mode layout tests passed.

Reproduce the focused tests from the client checkout:

```text
tests\build_agency_packets.bat
tests\build_agency_server.bat <patched-Hercules-directory>
tests\build_agency_packets.bat "<absolute-client-path>\build\tests\agency_server\server-list.bin"
```

Logs are under `build/purist-agency-*.log`. No live account, server process or
database was used. Live validation remains: two clients registering/searching,
applying and accepting; cancellation; leadership changes; logout/reconnect; empty
and multi-page results; and non-ASCII names/memos with the server's encoding.

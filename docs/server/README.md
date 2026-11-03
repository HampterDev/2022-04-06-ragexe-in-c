# Guild emblems over the game connection

`hercules-emblem-packets.patch` is a separate server change for the purist client.
It has not been applied to either existing server checkout. It targets Hercules
commit `84da0df5ea655c8339430d2065dbe3b0b1a40577` (Release v2024.09); the eventual
server fork may need adaptation. Build with `PACKETVER=20220406` for this client.

The separate [Adventurer's Agency patch](agency-packets.md) adds recruitment
listings over game packets. It can be applied before or after the emblem patch.
The [UTF-8 text patch](utf8-text.md) targets the HampterDev/Hercules fork instead;
it changes name validation, text truncation and the database connection charset.
For CRLF source checkouts, use `git apply --ignore-space-change` for checking and
applying either patch; their baseline Git source uses LF.

## Protocol

All fields use the existing little-endian packet encoding.

| Direction | Packet | Layout and representation |
|---|---|---|
| Client to map server | `0x0b46` | 10 bytes: ID (2), guild ID (4), emblem version (4). Existing Hercules request handler. |
| Client to map server | `0x0153` | ID (2), unsigned total length (2), raw BMP or GIF bytes. Image limit 51,200 bytes; total limit 51,204. The authenticated session identifies the uploading guild. |
| Map server to client | `0x0b36` | ID (2), length (2), part (2), guild ID (4), version (4), raw image bytes. Existing clear (0), data (1), complete (2) sequence. |

Uploads use the existing packet ID with a different image representation for
modern clients. Stock Hercules expects compressed BMP data on `0x0153` and rejects
these raw uploads. Deploy the patch with the purist client before testing uploads.
The modern branch uses the same client-generation guards as Hercules's chunked
emblem response. Older compiled client generations retain compressed BMP uploads.
A legacy client cannot display a GIF stored by a modern client; this patch is for
a server using the purist 2022 client, not mixed-generation image compatibility.

## Server changes

- Validate raw 24x24 BMP/GIF uploads. Honor configured BMP/GIF size limits and GIF
  frame-count limits, with a hard 51,200-byte GIF ceiling and 24x24 canvas.
- Use the existing compressed-BMP validator for BMP policy checks, including the
  transparent-pixel limit. Read 24-bit pixels as three bytes; the previous bitfield
  structure occupied four bytes under MSVC and could read beyond the pixel array.
- Decode GIF records incrementally with the bundled giflib, checking dimensions,
  palettes, frame data and termination without accumulating all decoded frames.
- Preserve guild-master, dead/trading/vending checks and the existing guild-change
  path, including skill restrictions, version updates and persistence dispatch.
- Send existing database emblems stored as compressed BMPs as raw BMPs to the
  modern receiver. Raw BMP/GIF entries already match the modern receiver.
- Pass the complete assembled image buffer to the char-server guild update after
  interserver chunk assembly, rather than the final packet's payload pointer.
- Wire giflib decoding into the map-server Visual Studio and Makefile builds.

The patch does not change the client's local `.ebm` cache encoding. The independent
client foundation commit `7fe9620` fixes the legacy response writer's cache name.

## Apply later

From the intended server checkout, inspect and check the patch first:

```text
git apply --stat <path-to-client>/docs/server/hercules-emblem-packets.patch
git apply --check <path-to-client>/docs/server/hercules-emblem-packets.patch
git apply <path-to-client>/docs/server/hercules-emblem-packets.patch
```

Then rebuild the map and char servers with the matching packet version. Existing
HTTP services are unaffected; other client HTTP features remain pending review.

## Validation performed

- `git apply --check` succeeds against the stated Hercules source revision.
- MSVC Win32 Release map-server and char-server builds succeed in an isolated
  source archive with `PACKETVER=20220406`. Warnings remain in pre-existing server
  code; Linux build changes have been inspected but not built here.
- Isolated tests compile the actual modified upload handler and BMP validator,
  plus the real GIF decoder. Modern raw BMP/GIF uploads preserve payload bytes;
  every truncated GIF prefix, trailing garbage, invalid dimensions/offsets,
  size/frame limits, short packets and forbidden session states are rejected.
- The BMP transparency regression fails before the pixel-stride fix and passes
  afterward. A separately compiled legacy handler still accepts compressed BMPs
  and rejects raw modern files.
- Client tests separately verify packet bytes, queue retry, bounded chunk
  assembly, cache naming and BMP/GIF rendering without HTTP.

Local validation logs are under the client `build/` directory:
`purist-emblem-server-tests.log`, `purist-emblem-server-final-build.log`,
`purist-emblem-char-server-build.log`, and `purist-emblem-*-tests.log`.
The isolated server source and test harness are also retained under `build/`.

No live server, database or account was used. Before deployment, verify guild-master
upload of BMP and animated GIF, download with an empty client cache, version changes,
visibility to a second client, and persistence across a server restart. Also verify
an existing compressed BMP database entry. The changed interserver assembly handoff
was compiled but has not been exercised against a live char server.

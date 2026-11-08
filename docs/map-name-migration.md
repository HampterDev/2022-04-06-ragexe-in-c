# Map names in the custom client

The custom client loads readable map names from `System/mapInfo_true.lub` only.
`mapNameTable.txt` and its language-folder variants are no longer read. Supply a
populated Lua file with the client; the old empty `mapTbl` supplies no names.
Unknown maps retain the existing default-name behavior.

## Convert existing data

Extract the `data/mapNameTable.txt` that your client actually uses, accounting
for its archive priority and any localized or loose replacement. Do not merge
whole catalogs from different GRFs: previously, the file loader selected one
file rather than merging their contents.

Run this from the repository root, writing to a new file:

```powershell
python tools/migrate_map_names.py path/to/mapNameTable.txt --base path/to/original/mapInfo_true.lub --output path/to/new/mapInfo_true.lub
```

Create the output directory first. The tool refuses to overwrite files. `--base`
accepts Lua source, including source named `.lub`; it rejects compiled bytecode.
Omit `--base` if you have no existing Lua settings to preserve.

The output is one self-contained Lua file. It registers the converted names,
then runs the original Lua `main`, preserving existing name overrides. Map-entry
banners have been removed: the old third argument to `AddMapDisplayName` is
ignored, and `AddMapSignName` / `AddMapBackgroundBmp` are accepted as no-ops so
legacy catalogs continue loading their later names. New entries need only
`AddMapDisplayName(map, name)`. No banner data or rendering is retained. Duplicate text entries retain their last
value. Non-ASCII bytes are escaped without transcoding, so Korean or other
legacy client encodings survive conversion. Malformed records stop conversion
instead of silently dropping names. The text file is not needed at runtime.

Install the reviewed output as the client's loose `System/mapInfo_true.lub`.
The current map-information loader uses ordinary Lua file loading. Keep your
original file when reviewing the migration; do not feed generated output back
into `--base`.

The single Lua catalog now owns the names across connection changes. Switching
the old UAE/English text folders no longer replaces map names. Multiple-language
catalog selection would require a separate design; it is not added by this change.

## Prepared local conversion

`build/map-name-migration/System/mapInfo_true.lub` is the prepared catalog from
the local `client-files/data.grf`, preserving all 943 names and the original
loose Lua file. The source client opens `data.grf` at startup and defaults to
archive-first reads; it does not open `revel.grf`. The optional `event.grf` path
has no archive present in the inspected asset folder.

A separate conversion of `revel.grf` is under
`build/map-name-migration/revel/System/mapInfo_true.lub` for reference. It is not
merged into the default conversion. Its 1,176 unique names also match the loose
`data/mapnametable.txt` byte-for-byte; use this variant when that loose table is
your intended source (for example, with `<readfolder>` enabled). No live client
was used to verify runtime selection. Source-entry hashes and conversion counts
are recorded in `build/map-name-migration/sources.json`. These generated assets
stay under the ignored build directory, outside the source distribution.

No installed client files or GRFs were modified. Deploy the prepared catalog
alongside the custom executable when testing this change.

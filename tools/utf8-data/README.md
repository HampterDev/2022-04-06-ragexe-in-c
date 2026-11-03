# utf8-data

This tool converts the display text in the data files the purist client loads from
CP949 to UTF-8, for the client build that runs locked to langtype 20 with code page
65001. Names that become file paths (sprites, textures, sounds, illustrations) keep
their CP949 bytes, because GRF entry names and the client's path literals are CP949.
It also repairs three loose files that an editor damaged by saving CP949 as UTF-8.

It reads `client-files` and never writes to it. Converted and repaired files go to a
separate folder, and reports go to `build\utf8-data\`.

## Usage

From the repository root:

```
cmd /c "set NoDefaultCurrentDirectoryInExePath=&& tools\utf8-data\build_luadump.bat"
python tools\utf8-data\convert.py      [--client-files DIR] [--out DIR] [--report DIR]
python tools\utf8-data\validate.py     [--client-files DIR] [--out DIR] [--report DIR]
python -m unittest discover -s tools\utf8-data\tests
```

- The defaults are `..\client-files` as input, `..\client-files-utf8` as output, and
  `build\utf8-data\` for `convert-report.txt/.json` and `validate-report.txt`.
- Start from an empty output folder. A rerun overwrites files but does not delete
  old ones.
- `build_luadump.bat` builds `build\utf8-data\luadump.exe` from `luadump.c` and
  `vendor\lua-5.1.4`, as a 32-bit build like the client. Without the 32-bit build it
  could not load the 32-bit Lua 5.1 bytecode in the `.lub` files. The conversion, the
  validation and most of the tests need it.

## Which copy of a file is converted

`utf8data/sources.py` finds each file the same way the client does:

- Names opened through the file manager get a `data\` prefix. `data.grf` is read
  before the loose file unless `clientinfo.xml` has `<readfolder/>`. Revel RO's
  `clientinfo.xml` does not have it.
- Lua scripts try `<name>.lub` before `<name>.lua`.
- `System\...` files are read from disk only: `lua_data_run`, `item_info_mgr` and
  `OngoingQuestInfoList`.

The tool converts the copy the client loads. The report lists every loose copy that
loses to that copy as "shadowed (not loaded)". Most of the loose `.lua` files under
`data\luafiles514` are shadowed by `.lub` files in `data.grf`.

## Rules

The rules live in `utf8data/rules.py`, with one entry per file the purist client
loads (checked against `src/` at 46ecf0b).

- **How fields are classified.** Each Lua file is run in `luadump` with stand-ins for
  the globals and client functions it uses. Every string it produces is classified
  by its field path (`tbl / # / identifiedDisplayName`), or by the argument it is
  passed as (`AddItem(arg 3)`). The classes are `display`, `resource` and `keep`.
  Anything no rule matches is `keep`.
- **When a value is converted.** A string value is converted only if every place it
  appears is `display`. A value that is also a resource name or a table key stays
  CP949 and is reported as a conflict. Non-ASCII table keys are never changed.
- **Unreachable strings.** Strings that no loaded table reaches stay CP949 and are
  listed. These are strings in function bodies and entries overwritten by a later
  duplicate key.
- **Encoding is decided per file.** Short CP949 words are often valid UTF-8 by
  accident: 킴 is the bytes C5 B4, which is also U+0174. So a file is treated as
  already UTF-8 only if all of its non-ASCII strings are, and such files pass
  through unchanged.
- **Bytecode files.** Only string constants are rewritten, including their length
  prefixes. The header, instructions, numbers, debug info and every other byte stay
  identical. `luabc.structure_without_strings` checks this.
- **Lua source files.** Only string literal tokens are rewritten. Escapes and layout
  are kept. Comments are converted only when the whole file ends up as UTF-8.
- **Text tables** (`msgStringTable.txt` and similar). Each `#` field is converted on
  its own. A field that is not valid CP949 is kept and reported.
- **No BOM.** Output never starts with a UTF-8 BOM (`pipeline.write` refuses one),
  because Lua 5.1 does not skip it.

## Repairs

An editor that opens CP949 as UTF-8 and saves turns every byte outside a valid UTF-8
sequence into U+FFFD. A candidate original is accepted only if repeating that damage
on it reproduces the damaged string exactly, and only if it is the one candidate that
fits under the same table key or on the same line. Candidates come from `data.grf`,
which includes entries a later duplicate key overwrites, recovered from the bytecode,
and from client-files commit 5095725f, which predates the damage in 73cf5a87. When
two originals damage the same way (마법 and 물리), the table key decides, and the
report lists those strings.

| File | How it is repaired |
|---|---|
| `data\msgstringtable.txt` | Lines restored from `data.grf`, then written as UTF-8 |
| `data\luafiles514\lua files\Datainfo\jobName.lua` | Sprite names restored as CP949 |
| `data\luafiles514\lua files\StateIcon\stateiconinfo.lua` | Descriptions restored and written as UTF-8 |

The report lists every string that could not be recovered.

## Output and deployment

- `<out>\System\...`: copy these over the client's `System\` folder.
- `<out>\data.grf\data\...`: the client reads these from `data.grf`. They take effect
  only once they are packed into `data.grf` at the same paths, with any GRF tool;
  this tool has no GRF writer. You can use them as loose files with `<readfolder/>`
  instead, but then every stale loose file wins as well.
- `<out>\data\...`: the repaired loose copies. The client only reads them with
  `<readfolder/>`, or if they are packed into `data.grf`.

Deploy the output only together with the UTF-8 client. The current client decodes
text as CP1252 and would show the converted text as mojibake.

## Limits

- The tool converts encodings; it translates nothing. Korean stays Korean.
- Data is evaluated with stand-ins for other files' globals. Data whose contents
  depend on those values would need a rule that loads the real files.
- A new data file, or a new field holding display text, needs a rule. Until it has
  one, its strings are kept and reported.

# Effect data

`effects.json` holds the visual effects that are data rather than C code: the STR file of
226 RagEffect IDs, the frames of the 195 RagEffect IDs that only play their STR, and the
STR file of 510 EZ2STR effect types. It is the source of truth for them.

`build.bat` compiles `tools/bin2c.c` and turns the file into the byte array `g_effectData`,
which is linked into the executable, so players never need the file. The client reads it
once, on the first effect lookup (`src/effects/effect_data.c`).

## Trying a change without rebuilding

Set `RAGEXE_EFFECT_DATA` to the full path of a file in this format before starting the
client. Unset or empty, the variable does nothing. When the file cannot be read or breaks
one of the rules below, the client shows the reason and the file name in a message box and
uses its built-in data.

## Format

```json
{
  "Format": "ragexe effect data 1",
  "RagEffect": { "<id>": { "Str": {...}, "Play": {...} }, ... },
  "Ez2StrFile": { "<type>": {...}, ... }
}
```

The three top-level keys are required and no others are allowed. Rows are keyed by a
decimal ID from 0 to 65535 without sign or leading zero, each ID once. Unknown keys, a key
given twice, a wrong type or a value out of range reject the whole file.

### RagEffect

Effects that `game_actor_launch_effect` creates (`src/effects/rag`). A row has `Str`, `Play`
or both.

`Str` is the STR file `rag_effect_init` loads:

| Key | Value |
|---|---|
| `File` | Required. At most 63 bytes in CP949. With `Variants` it holds one `%d`. |
| `MinFile` | Used instead of `File` when the min-effects flag `map_rule_flags[0]` is not 0. |
| `Variants` | 2 to 99. `%d` becomes `1 + rand() % Variants`; this draws one `rand()` at launch. Not with `MinFile`. |
| `Loops` | `str_max_loops`: how many more times the STR plays after its first run. |

`Play` means the effect's frames are this STR's play, run by `rag_str_play`
(`src/effects/rag/rag_str_bindings.c`) instead of a case in `rag_effect_process*.c`. On
every frame it runs, in order, the cues without `AfterStr`, the STR step
(`rag_str_step`) at `Offset`, and the cues with `AfterStr`.

| Key | Value |
|---|---|
| `Timeline` | A list of cues: `{"At": frame, "AfterStr": true, "Do": [operations]}`. Without `At` a cue runs on every frame. `At` is compared with the effect's frame counter, which a rewind of the STR sets back to 0. |
| `Offset` | `[x, y, z]`, the screen offset of the STR step. Default `[0, -80, 0]`. |
| `OnStrEnd` | What happens when the step reports the end of the STR: `"Stop"` (default) ends the effect, `"Continue"` lets it run to its duration, `"Rewind"` starts the STR again. |
| `StopFrame` | With `"Stop"`: the frame counter is set to this value as the effect ends. |

An operation has exactly one action:

| Operation | Effect |
|---|---|
| `{"Follow": true}` | The effect moves to its master's position. |
| `{"Wave": "file.wav", "Height": y}` | Plays the wave at the effect's position, `y` above it (`rag_play_wave`; default height 0). |
| `{"Wave": "file.wav", "AtPlayer": true}` | Plays the wave at the player (`sound_play_wave` at 0, 0, 0). |
| `{"Quake": n}` | Sends game message `0x7a` (quake) with parameter `n`. |
| `{"Duration": n}` | Sets the effect's duration to `n` frames. |

Numbers given as floats (`Offset`, `Height`) must be exactly representable as a 32-bit
float, as the C constants they replace were.

### Ez2StrFile

Types of the EZ2STR effect class (`src/effects/skills/ez2str_effect.c`) and the STR file
`ez2_str_effect_open_for_type` (`ez2str_effect_files.c`) loads for them. Type 1412, which
plays the name it was given, and the headgear types from 2384 up, named by Lua, stay in C.

| Key | Value |
|---|---|
| `File` | Required. At most 255 bytes in CP949. With `Variants` it holds one `%d`. |
| `MinFile` | Used instead of `File` when `map_rule_flags[0]` is 1. (For RagEffect it is "not 0".) |
| `Open` | `true`: loaded through `ez2_str_particle_open`, which looks for a `min_` file itself when the flag is set. |
| `FullOnly` | `true`: loaded only when `map_rule_flags[0]` is 0. |
| `Variants` | As for RagEffect. |

`MinFile`, `Open`, `FullOnly` and `Variants` exclude each other.

### Names

Strings are UTF-8 in the file. The loader converts them to CP949, the code page of the
client's resource names and string literals, and rejects characters CP949 lacks. A `%` is
allowed only as the `%d` of `Variants`.

## Checking a change

`tests\build_effect_trace.bat` runs every effect ID through the client code with several
inputs and compares one digest; a change to this file changes the digest of the IDs it
touches, so update `EXPECTED_DIGEST` in `tests/test_effect_trace.c` once the change is
intended (`--digests FILE` lists the digest of every ID, `--trace FILE --ids FIRST LAST`
writes what happened frame by frame). `tests\build_effect_data.bat` checks the loader's
rules and the fallback.

## Building elsewhere

`tools/bin2c.c` is plain C99. A build without `build.bat`, such as the Linux and macOS port,
compiles it with the host compiler, runs
`bin2c OUT.c g_effectData data/effects/effects.json` and compiles `OUT.c` with the client.

## Origin

The file was generated once by `tools/convert_str_bindings.py` from the C code of `purist`
at `2239d84` (the `rag_effect_init` STR switch, the STR-only dispatch cases and the EZ2STR
table), and the trace digests were the same before and after that code was deleted. Edit the
JSON directly from now on; the converter has nothing left to read.

# granny_export

The client draws the emperium, the WoE guardians, the guild flag and the treasure box as
Granny 2.1 models (`data\model\3dmob\*.gr2`) with animations from `data\model\3dmob_bone\*.gr2`.
The original decodes them with the game's `granny2.dll`, which exists only for 32-bit Windows. The
client is 64-bit on Windows and also runs on Linux, macOS and Android, so it reads the same models
from glTF 2.0 files that this tool writes once per game installation, using `granny2.dll` to do the
decoding. The tool is therefore a 32-bit program, like the oracle tests (and the `lua_chunk` that
writes the Lua test fixture); it compiles zlib in from `vendor\zlib-1.2.11` and needs nothing else
built first.

```bat
tools\granny_export\build.bat
build\granny_export.exe <game directory>
```

On Linux or macOS, run the Windows build of the tool under Wine; its output is the same bytes:

```sh
cd <game directory>
wine /path/to/granny_export.exe .
```

The tool reads every `.gr2` from the game's `data` folder and from the GRF archives in
`DATA.INI` (in that order, first match wins) and writes `data\model\gltf\<crc32>.glb`, named by
the CRC-32 of the `.gr2` file it came from. The client computes the same CRC from the bytes it
loads, so the converted files follow the data they were made from and can also be packed into a
GRF. For the kRO data that is 21 files, 4.5 MB.

What a file holds:

- models: the skeleton as nodes with the rest pose, the inverse bind matrices as the skin,
  every mesh with positions, normals, texture coordinates and up to four bone weights, one
  primitive per triangle group, the materials and their textures as PNG;
- animations: every bone's translation, rotation and scale, sampled by Granny 60 times a
  second over the animation and interpolated linearly. Animation files in `3dmob_bone` are
  sampled on the model they animate (`<id>_<part>.gr2` on the model whose name ends in `_<id>`),
  as the client plays them.

`extras` carry what the client needs beyond plain glTF: the source file, a mesh's Granny material
bindings and whether it is rigid, a texture's alpha, the animation's duration. The files open in
Blender and other glTF tools, in Granny's own axes and units.

`src/resources/granny_gltf.c` implements the part of the Granny API the client calls on these files.
The scenes `granny_models`, `granny_dead` and `guild_flags` (`tests/scenes`) compared it with
`granny2.dll` while there was a 32-bit client. In the last comparison, on 2026-11-11, the 64-bit
scene harness on the same clock captured `granny_dead` and `guild_flags` within 1 per channel,
`guild_flags_later` with 2 pixels of the flag's tassel apart, and `granny_dying` and `granny_models`
with a few steps of shading over the models and more along their edges: 292 and 7,058 pixels more
than 1 apart, 16 and 365 more than 16 ([portability notes](../../docs/portability.md#linux-client)).

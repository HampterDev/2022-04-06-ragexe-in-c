# granny_export

The client draws the emperium, the WoE guardians, the guild flag and the treasure box as
Granny 2.1 models (`data\model\3dmob\*.gr2`) with animations from `data\model\3dmob_bone\*.gr2`.
Windows decodes them with the game's `granny2.dll`. There is no Granny for 64-bit Windows, Linux, macOS
or Android, so those builds read the same models from glTF 2.0 files that this tool writes once
per game installation, using `granny2.dll` to do the decoding.

```bat
build.bat
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

`src/resources/granny_gltf.c` implements the part of the Granny API the client calls on these
files. The scenes `granny_models` and `granny_dead` (`tests/scenes`) compare it with
`granny2.dll`: with the Windows scene harness on the same clock the two match except for single
pixels along edges.

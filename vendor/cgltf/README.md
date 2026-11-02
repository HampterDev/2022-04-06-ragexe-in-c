# cgltf

`cgltf.h` is [cgltf v1.14](https://github.com/jkuhlmann/cgltf/tree/v1.14) by Johannes Kuhlmann,
unchanged. The MIT licence is at the end of the header.

Upstream SHA-256: `19db3bdf85f6aa991a3b9d056641bcb39a2ef11cd5211e7a9f6b6c307d9ebc64`

The portable client reads the glTF files `tools/granny_export` makes from the game's Granny
models with it (`src/resources/granny_gltf.c`, which holds the implementation). Header only:
the `rage_cgltf` CMake target only adds the include directory.

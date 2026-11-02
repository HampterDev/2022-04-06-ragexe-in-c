# dr_mp3 MP3 decoder

`dr_mp3.h` (v0.7.3) comes from
[mackron/dr_libs commit 5690d467](https://github.com/mackron/dr_libs/tree/5690d4671d7ad07ae6021756d7222eb159745f06).
The public-domain/MIT-0 licence is included at the end of the header.

Original upstream SHA-256:

- `dr_mp3.h`: `d80e6800ddde29d50988ce894affe601858802d1bb8f047f98b84b33ae66c11a`

`dr_mp3_impl.c` compiles the decoder once, without its stdio file helpers
(`DR_MP3_NO_STDIO`, set for every user of the library). It is built only off
Windows, where it decodes the MP3 background music and MP3 sound effects that
Miles Sound System decodes on Windows. The caller opens files through the
platform filesystem layer and hands the decoder memory.

No local patches.

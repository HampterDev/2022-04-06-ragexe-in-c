# stb JPEG implementation

`stb_image.h` and `stb_image_write.h` come from
[nothings/stb commit d14d46f6](https://github.com/nothings/stb/tree/d14d46f6b44456eb88beac58cba833a4135d4c89).
The MIT/public-domain licence is included at the end of each header.

Original upstream SHA-256:

- `stb_image.h`: `cbd5f0ad7a9cf4468affb36354a1d2338034f2c12473cf1a8e32053cb6914a05`
- `stb_image_write.h`: `594c2fe35d49488b4382dbfaec8f98366defca819d916ac95becf3e75f4200b3`

`stb_impl.c` enables only JPEG decoding and callback-based writing. Filename
encoding and file ownership belong to callers. The client's padded BGR image
representation is converted at the codec boundary; stb uses packed RGB.

Local patch: the JPEG writer's bit buffer and its left-shift operands use unsigned
integers. The upstream signed shifts trigger UBSan even for the owned quadrant
fixture. The patch preserves the intended modulo-32-bit shift behavior.

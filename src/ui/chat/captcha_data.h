#ifndef UI_CHAT_CAPTCHA_DATA_H
#define UI_CHAT_CAPTCHA_DATA_H
#include <stddef.h>

enum {
    CAPTCHA_WIDTH = 220,
    CAPTCHA_HEIGHT = 90,
    CAPTCHA_CHUNK_LIMIT = 1024,
    CAPTCHA_PACKED_LIMIT = 65535,
    CAPTCHA_BITMAP_LIMIT = 65536
};

struct CaptchaData {
    unsigned char *bytes;
    size_t size;
};
struct CaptchaStream {
    struct CaptchaData packed;
    size_t received;
};
enum CaptchaAppend { CAPTCHA_REJECTED = -1, CAPTCHA_PENDING, CAPTCHA_COMPLETE };

void captcha_data_clear(struct CaptchaData *data);
void captcha_stream_clear(struct CaptchaStream *stream);
int captcha_stream_begin(struct CaptchaStream *stream, size_t size);
enum CaptchaAppend captcha_stream_append(struct CaptchaStream *stream, const void *chunk, size_t size);
int captcha_bitmap_valid(const void *bitmap, size_t size);
int captcha_data_pack(struct CaptchaData *out, const void *bitmap, size_t size);
int captcha_data_unpack(struct CaptchaData *out, const struct CaptchaData *packed);
#endif

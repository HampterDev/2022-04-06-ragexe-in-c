#include "platform/code_page.h"
#include "render/fonts.h"
#include "render/render_api.h"
#include "diag/perf.h"
#include "render/text_unicode.h"
#include <string.h>

#define EXTENT_SLOTS  1024
#define EXTENT_MAXLEN 255

struct extent_entry {
    DWORD hash;
    HFONT font;
    int face, size, charset, bold, italic, len;
    SIZE extent;
    DWORD tick;
    char str[EXTENT_MAXLEN + 1];
};

static struct extent_entry extent_cache[EXTENT_SLOTS];
static DWORD extent_tick;

static DWORD fnv1a(DWORD h, const void *p, size_t n)
{
    const BYTE *b = (const BYTE *)p;
    while (n--)
        h = (h ^ *b++) * 16777619u;
    return h;
}

static int measure(struct TextDC *self, SIZE *out, const char *str, int len, int charset)
{
    struct TextUnicode text;
    SIZE result = {0};
    int i, sum = 0, ok;
    if (!text_unicode_init(&text, str, len, CP_UTF8))
        return 0;
    ok = GetTextExtentPoint32W(self->hdc, text.data, text.length, &result);
    if (ok && self->italic == 1) {
        for (i = 0; i < text.length; i++) {
            ABC abc;
            if (!GetCharABCWidthsW(self->hdc, text.data[i], text.data[i], &abc)) {
                ok = 0;
                break;
            }
            sum += abc.abcA + abc.abcB;
        }
        result.cx = sum;
    }
    text_unicode_release(&text);
    if (ok) *out = result;
    return ok;
}

static int entry_matches(const struct extent_entry *e, DWORD hash, HFONT font, const struct TextDC *t,
                         const char *str, int len, int charset)
{
    return e->hash == hash && e->font == font && e->len == len && e->face == t->face && e->size == t->size
        && e->charset == charset && e->bold == t->bold && e->italic == t->italic
        && memcmp(e->str, str, (size_t)len) == 0;
}

SIZE *text_dc_measure_in_charset(struct TextDC *self, SIZE *out, const char *str, int len, int charset)
{
    HFONT font;
    HGDIOBJ old;
    int measured;
    DWORD hash;
    struct extent_entry *a, *b, *e;

    out->cx = 0;
    out->cy = 0;
    if (!str || len <= 0)
        return out;
    font = font_cache_get_font(&g_fontCache, self->face, self->size, charset, self->bold, self->italic);
    old = font ? SelectObject(self->hdc, font) : NULL;
    if (!old || old == HGDI_ERROR)
        return out;
    if (len > EXTENT_MAXLEN) {
        g_perf.textMeasured++;
        perf_begin(PERF_TEXT);
        measure(self, out, str, len, charset);
        perf_end(PERF_TEXT);
        return out;
    }
    hash = fnv1a(2166136261u, &font, sizeof(font));
    hash = fnv1a(hash, &self->face, sizeof(self->face));
    hash = fnv1a(hash, &self->size, sizeof(self->size));
    hash = fnv1a(hash, &charset, sizeof(charset));
    hash = fnv1a(hash, &self->bold, 1);
    hash = fnv1a(hash, &self->italic, 1);
    hash = fnv1a(hash, str, (size_t)len);
    a = &extent_cache[hash & (EXTENT_SLOTS - 1)];
    b = &extent_cache[(hash & (EXTENT_SLOTS - 1)) ^ 1];
    e = entry_matches(a, hash, font, self, str, len, charset) ? a
      : entry_matches(b, hash, font, self, str, len, charset) ? b : NULL;
    if (e) {
        g_perf.textHits++;
        *out = e->extent;
        e->tick = ++extent_tick;
        return out;
    }
    g_perf.textMeasured++;
    perf_begin(PERF_TEXT);
    measured = measure(self, out, str, len, charset);
    perf_end(PERF_TEXT);
    if (!measured) return out;
    e = a->tick <= b->tick ? a : b;
    e->hash = hash;
    e->font = font;
    e->face = self->face;
    e->size = self->size;
    e->charset = charset;
    e->bold = self->bold;
    e->italic = self->italic;
    e->len = len;
    e->extent = *out;
    e->tick = ++extent_tick;
    memcpy(e->str, str, (size_t)len);
    return out;
}

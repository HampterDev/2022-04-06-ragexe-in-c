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
    int face, size, bold, italic, len;
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

/* Italic text is measured without the overhang of each character's right side. A
   surrogate pair is one character, measured as a whole. */
static int italic_width(HDC dc, const WCHAR *text, int length, LONG *width)
{
    int i, sum = 0;
    for (i = 0; i < length; i++) {
        if (text[i] >= 0xd800 && text[i] <= 0xdbff && i + 1 < length
            && text[i + 1] >= 0xdc00 && text[i + 1] <= 0xdfff) {
            SIZE pair;
            if (!GetTextExtentPoint32W(dc, text + i, 2, &pair))
                return 0;
            sum += pair.cx;
            i++;
        } else {
            ABC abc;
            if (!GetCharABCWidthsW(dc, text[i], text[i], &abc))
                return 0;
            sum += abc.abcA + abc.abcB;
        }
    }
    *width = sum;
    return 1;
}

static int measure(struct TextDC *self, SIZE *out, const char *str, int len)
{
    struct TextUnicode text;
    SIZE result = {0};
    int ok;
    if (!text_unicode_init(&text, str, len, CP_UTF8))
        return 0;
    ok = GetTextExtentPoint32W(self->hdc, text.data, text.length, &result);
    if (ok && self->italic == 1)
        ok = italic_width(self->hdc, text.data, text.length, &result.cx);
    text_unicode_release(&text);
    if (ok) *out = result;
    return ok;
}

static int entry_matches(const struct extent_entry *e, DWORD hash, HFONT font, const struct TextDC *t,
                         const char *str, int len)
{
    return e->hash == hash && e->font == font && e->len == len && e->face == t->face && e->size == t->size
        && e->bold == t->bold && e->italic == t->italic && memcmp(e->str, str, (size_t)len) == 0;
}

SIZE *text_dc_measure_font(struct TextDC *self, SIZE *out, const char *str, int len, HFONT font)
{
    HGDIOBJ old;
    int measured;
    DWORD hash;
    struct extent_entry *a, *b, *e;

    out->cx = 0;
    out->cy = 0;
    if (!str || len <= 0 || !font)
        return out;
    old = SelectObject(self->hdc, font);
    if (!old || old == HGDI_ERROR)
        return out;
    if (len > EXTENT_MAXLEN) {
        g_perf.textMeasured++;
        perf_begin(PERF_TEXT);
        measure(self, out, str, len);
        perf_end(PERF_TEXT);
        return out;
    }
    hash = fnv1a(2166136261u, &font, sizeof(font));
    hash = fnv1a(hash, &self->face, sizeof(self->face));
    hash = fnv1a(hash, &self->size, sizeof(self->size));
    hash = fnv1a(hash, &self->bold, 1);
    hash = fnv1a(hash, &self->italic, 1);
    hash = fnv1a(hash, str, (size_t)len);
    a = &extent_cache[hash & (EXTENT_SLOTS - 1)];
    b = &extent_cache[(hash & (EXTENT_SLOTS - 1)) ^ 1];
    e = entry_matches(a, hash, font, self, str, len) ? a
      : entry_matches(b, hash, font, self, str, len) ? b : NULL;
    if (e) {
        g_perf.textHits++;
        *out = e->extent;
        e->tick = ++extent_tick;
        return out;
    }
    g_perf.textMeasured++;
    perf_begin(PERF_TEXT);
    measured = measure(self, out, str, len);
    perf_end(PERF_TEXT);
    if (!measured) return out;
    e = a->tick <= b->tick ? a : b;
    e->hash = hash;
    e->font = font;
    e->face = self->face;
    e->size = self->size;
    e->bold = self->bold;
    e->italic = self->italic;
    e->len = len;
    e->extent = *out;
    e->tick = ++extent_tick;
    memcpy(e->str, str, (size_t)len);
    return out;
}

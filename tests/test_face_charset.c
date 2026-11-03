#include <windows.h>
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "platform/clientinfo.h"
#include "platform/code_page.h"
#include "platform/language.h"
#include "platform/language_api.h"
#include "render/device.h"
#include "render/fonts.h"
#include "render/textdc.h"

/* Run the production text_dc_select_font with the production g_3dDevice tables
   for every face, language index, service type and folder language. Faces past
   the 12-entry faceCharsets table must not read past it, and must get the
   charset that the bytes after the table gave them where it mattered:
   - faces 12-0x13 read 0, ANSI_CHARSET;
   - SCDream (0x1d) read 0x10101, which fonts truncate to DEFAULT_CHARSET;
   - the EOT faces 0x14-0x1c, whose LOGFONT has a fixed charset, get
     ANSI_CHARSET, which decodes text with the same code page as what they read.
   With language index 0, faces 0x14 and up keep the first entry. */

int g_languageIndex;
int g_serviceType;
static struct Language language;
struct Language *g_language = &language;

#include "face_charset.inc"

#define TABLE_SIZE (sizeof g_3dDevice.faceCharsets / sizeof g_3dDevice.faceCharsets[0])

/* What the old lookup g_3dDevice.faceCharsets[face] read: the four bytes at that
   position of g_3dDevice, past the table for faces 12 and up. */
static int read_at(int face)
{
    int value;

    assert(face >= 0 && offsetof(struct C3dDevice, faceCharsets) + (face + 1) * sizeof value
           <= sizeof g_3dDevice);
    memcpy(&value, (const BYTE *)&g_3dDevice + offsetof(struct C3dDevice, faceCharsets) + face * sizeof value,
           sizeof value);
    return value;
}

static int cases;

static void check_face(int lang, int a)
{
    struct TextDC dc;
    int face, before;

    memset(&dc, 0, sizeof dc);
    g_languageIndex = lang;
    text_dc_select_font(&dc, a, 12, 0, 0);
    face = dc.face;
    assert(face >= 0 && face <= FONT_FACE_SCDREAM);
    before = lang == 0 && face >= FONT_FACE_EOT_FIRST ? g_3dDevice.faceCharsets[0] : read_at(face);
    if ((size_t)face < TABLE_SIZE || (lang == 0 && face >= FONT_FACE_EOT_FIRST)) {
        assert(dc.charset == before);
    } else if (face < FONT_FACE_EOT_FIRST) {
        assert(before == ANSI_CHARSET && dc.charset == ANSI_CHARSET);
    } else if (face < FONT_FACE_SCDREAM) {
        assert(before == 0 || before == 0x101 || before == 0x1010100 || before == 0x1010101);
        assert(dc.charset == ANSI_CHARSET);
        assert(get_code_page_from_charset(before) == get_code_page_from_charset(dc.charset));
    } else {
        assert(before == 0x10101 && (BYTE)before == DEFAULT_CHARSET && dc.charset == DEFAULT_CHARSET);
        assert(get_code_page_from_charset(before) == get_code_page_from_charset(dc.charset));
    }
    ++cases;
}

int main(void)
{
    static const int services[] = { 0, 6, 0xc, 0xe, 0x12 };
    static const WORD folders[] = { 0, 0x1f };
    size_t s, f;
    int lang, a;

    assert(TABLE_SIZE == 12);
    for (s = 0; s < sizeof services / sizeof *services; ++s)
        for (f = 0; f < sizeof folders / sizeof *folders; ++f) {
            g_serviceType = services[s];
            language.folder_lang = folders[f];
            for (lang = 0; lang <= 12; ++lang)
                for (a = 0; a <= FONT_FACE_SCDREAM; ++a)
                    check_face(lang, a);
        }
    printf("Face charsets: %d selections; faces past the table keep their charsets without reading past it.\n",
           cases);
    return 0;
}

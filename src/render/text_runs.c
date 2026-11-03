#include "render/text_runs.h"
#include "core/utf8.h"

struct ScriptRange {
    unsigned long first, last;
    int script;
};

/* Sorted by first code point. Symbols are the letterlike, number, arrow, math, technical,
   enclosed, box, shape and dingbat blocks, plus the two Korean punctuation marks: CP949
   has most of them as two-byte characters, which the Korean client drew in Gulim. The
   rest of General Punctuation stays in the selected face, which has the common marks. */
static const struct ScriptRange ranges[] = {
    {0x0300, 0x036f, TEXT_SCRIPT_COMMON},
    {0x0e00, 0x0e7f, TEXT_SCRIPT_THAI},
    {0x1100, 0x11ff, TEXT_SCRIPT_HANGUL},
    {0x200b, 0x200d, TEXT_SCRIPT_COMMON},
    {0x2025, 0x2025, TEXT_SCRIPT_SYMBOL},  /* two-dot leader */
    {0x203b, 0x203b, TEXT_SCRIPT_SYMBOL},  /* reference mark */
    {0x2100, 0x2bff, TEXT_SCRIPT_SYMBOL},
    {0x2e80, 0x2fdf, TEXT_SCRIPT_HAN},
    {0x3000, 0x303f, TEXT_SCRIPT_HAN},
    {0x3040, 0x30ff, TEXT_SCRIPT_KANA},
    {0x3130, 0x318f, TEXT_SCRIPT_HANGUL},
    {0x3190, 0x31bf, TEXT_SCRIPT_HAN},
    {0x31c0, 0x31ef, TEXT_SCRIPT_HAN},
    {0x31f0, 0x31ff, TEXT_SCRIPT_KANA},
    {0x3200, 0x4dbf, TEXT_SCRIPT_HAN},
    {0x4e00, 0x9fff, TEXT_SCRIPT_HAN},
    {0xa960, 0xa97f, TEXT_SCRIPT_HANGUL},
    {0xac00, 0xd7ff, TEXT_SCRIPT_HANGUL},
    {0xf900, 0xfaff, TEXT_SCRIPT_HAN},
    {0xfe00, 0xfe0f, TEXT_SCRIPT_COMMON},
    {0xfe30, 0xfe4f, TEXT_SCRIPT_HAN},
    {0xff00, 0xff64, TEXT_SCRIPT_HAN},
    {0xff65, 0xff9f, TEXT_SCRIPT_KANA},
    {0xffa0, 0xffdc, TEXT_SCRIPT_HANGUL},
    {0xffe0, 0xffe6, TEXT_SCRIPT_HAN},
    {0x1f000, 0x1faff, TEXT_SCRIPT_EMOJI},
    {0x20000, 0x3ffff, TEXT_SCRIPT_HAN},
    {0xe0100, 0xe01ef, TEXT_SCRIPT_COMMON},
};

int text_script_of(unsigned long code_point)
{
    size_t low = 0, high = sizeof ranges / sizeof ranges[0];

    if (code_point == ' ')
        return TEXT_SCRIPT_COMMON;
    while (low < high) {
        size_t mid = low + (high - low) / 2;

        if (code_point < ranges[mid].first)
            high = mid;
        else if (code_point > ranges[mid].last)
            low = mid + 1;
        else
            return ranges[mid].script;
    }
    return TEXT_SCRIPT_DEFAULT;
}

size_t text_script_run(const char *text, size_t length, enum TextScript *script)
{
    size_t at = 0, size;
    int current = TEXT_SCRIPT_COMMON, next;
    long code_point;

    while (at < length) {
        unsigned char byte = (unsigned char)text[at];

        if (byte < 0x80) {
            next = byte == ' ' ? TEXT_SCRIPT_COMMON : TEXT_SCRIPT_DEFAULT;
            size = 1;
        } else {
            code_point = utf8_decode(text + at, length - at, &size);
            next = code_point == UTF8_INVALID ? TEXT_SCRIPT_DEFAULT : text_script_of((unsigned long)code_point);
        }
        if (next != TEXT_SCRIPT_COMMON) {
            if (current == TEXT_SCRIPT_COMMON)
                current = next;
            else if (next != current)
                break;
        }
        at += size;
    }
    *script = current == TEXT_SCRIPT_COMMON ? TEXT_SCRIPT_DEFAULT : (enum TextScript)current;
    return at;
}

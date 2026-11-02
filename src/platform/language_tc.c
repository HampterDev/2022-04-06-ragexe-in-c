#include <string.h>
#include "platform/language.h"
#include "core/fatal.h"

enum ZhuyinPlace {
    ZHUYIN_INITIAL, ZHUYIN_MEDIAL, ZHUYIN_FINAL, ZHUYIN_TONE, ZHUYIN_UNKNOWN
};

static const struct {
    BYTE key;
    BYTE place;
    WCHAR symbol;
} zhuyin_keys[] = {
    { '1',            ZHUYIN_INITIAL,  L'ㄅ' },
    { '2',            ZHUYIN_INITIAL,  L'ㄉ' },
    { '3',            ZHUYIN_TONE,     L'ˇ' },
    { '4',            ZHUYIN_TONE,     L'ˋ' },
    { '5',            ZHUYIN_INITIAL,  L'ㄓ' },
    { '6',            ZHUYIN_TONE,     L'ˊ' },
    { '7',            ZHUYIN_TONE,     L'˙' },
    { '8',            ZHUYIN_FINAL,    L'ㄚ' },
    { '9',            ZHUYIN_FINAL,    L'ㄞ' },
    { '0',            ZHUYIN_FINAL,    L'ㄢ' },
    { 'A',            ZHUYIN_INITIAL,  L'ㄇ' },
    { 'B',            ZHUYIN_INITIAL,  L'ㄖ' },
    { 'C',            ZHUYIN_INITIAL,  L'ㄏ' },
    { 'D',            ZHUYIN_INITIAL,  L'ㄎ' },
    { 'E',            ZHUYIN_INITIAL,  L'ㄍ' },
    { 'F',            ZHUYIN_INITIAL,  L'ㄑ' },
    { 'G',            ZHUYIN_INITIAL,  L'ㄕ' },
    { 'H',            ZHUYIN_INITIAL,  L'ㄘ' },
    { 'I',            ZHUYIN_FINAL,    L'ㄛ' },
    { 'J',            ZHUYIN_MEDIAL,   L'ㄨ' },
    { 'K',            ZHUYIN_FINAL,    L'ㄜ' },
    { 'L',            ZHUYIN_FINAL,    L'ㄠ' },
    { 'N',            ZHUYIN_INITIAL,  L'ㄙ' },
    { 'M',            ZHUYIN_MEDIAL,   L'ㄩ' },
    { 'O',            ZHUYIN_FINAL,    L'ㄟ' },
    { 'P',            ZHUYIN_FINAL,    L'ㄣ' },
    { 'Q',            ZHUYIN_INITIAL,  L'ㄆ' },
    { 'R',            ZHUYIN_INITIAL,  L'ㄐ' },
    { 'S',            ZHUYIN_INITIAL,  L'ㄋ' },
    { 'T',            ZHUYIN_INITIAL,  L'ㄔ' },
    { 'U',            ZHUYIN_MEDIAL,   L'ㄧ' },
    { 'V',            ZHUYIN_INITIAL,  L'ㄒ' },
    { 'W',            ZHUYIN_INITIAL,  L'ㄊ' },
    { 'X',            ZHUYIN_INITIAL,  L'ㄌ' },
    { 'Y',            ZHUYIN_INITIAL,  L'ㄗ' },
    { 'Z',            ZHUYIN_INITIAL,  L'ㄈ' },
    { VK_OEM_MINUS,   ZHUYIN_FINAL,    L'ㄦ' },
    { VK_OEM_1,       ZHUYIN_FINAL,    L'ㄤ' },
    { VK_OEM_COMMA,   ZHUYIN_FINAL,    L'ㄝ' },
    { VK_OEM_PERIOD,  ZHUYIN_FINAL,    L'ㄡ' },
    { VK_OEM_2,       ZHUYIN_FINAL,    L'ㄥ' },
};

struct Language *language_tc_construct(struct LanguageTC *self)
{
    size_t i;

    language_construct(&self->base);
    self->base.input_method = LANGUAGE_INPUT_ZHUYIN;
    self->key_count = 0;
    for (i = 0; i < 0x100; ++i)
        self->table[i].kind = ZHUYIN_UNKNOWN;
    for (i = 0; i < sizeof zhuyin_keys / sizeof zhuyin_keys[0]; ++i) {
        struct ZhuyinKey *key = &self->table[zhuyin_keys[i].key];
#ifdef _WIN32
        BOOL used_default = FALSE;
        int bytes = WideCharToMultiByte(950, WC_NO_BEST_FIT_CHARS,
            &zhuyin_keys[i].symbol, 1, (char *)key->symbol, sizeof key->symbol,
            NULL, &used_default);
#else
        int used_default = 0;
        uint16_t symbol = zhuyin_keys[i].symbol;
        int bytes = platform_code_page_from_utf16(950, &symbol, 1, (char *)key->symbol, sizeof key->symbol);
#endif

        if (bytes != sizeof key->symbol || used_default)
            fatal_error("Cannot encode Zhuyin symbols as CP950");
        key->zero = 0;
        key->kind = zhuyin_keys[i].place;
    }
    return &self->base;
}

const char *language_tc_reading(struct LanguageTC *self)
{
    char places[9] = {0};
    unsigned i;
    BYTE kind;

    for (i = 0; i < self->key_count; ++i) {
        kind = self->table[self->keys[i]].kind;
        if (kind < ZHUYIN_UNKNOWN)
            memcpy(places + kind * 2, self->table[self->keys[i]].symbol, 2);
    }
    memcpy(self->base.reading, places, sizeof places);
    self->base.reading_length = 8;
    return self->base.reading;
}

int language_tc_reading_length(struct LanguageTC *self)
{
    if (self->key_count == 1 && self->table[self->keys[0]].kind == ZHUYIN_UNKNOWN)
        return 0;
    return self->key_count;
}

void language_tc_on_key(struct LanguageTC *self, BYTE key)
{
    BYTE kind;
    unsigned i;

    if (key == VK_BACK) {
        if (self->key_count)
            --self->key_count;
        return;
    }
    kind = self->table[key].kind;
    for (i = 0; i < self->key_count; ++i) {
        if (self->table[self->keys[i]].kind == kind) {
            memmove(self->keys + i, self->keys + i + 1, self->key_count - i - 1);
            --self->key_count;
            break;
        }
    }
    self->keys[self->key_count++] = key;
}

void language_tc_read_composition(struct LanguageTC *self)
{
    size_t size;
    char *text = self->base.composition;

    language_read_composition(&self->base);
    size = self->base.composition_length;
    if (size > 2 && strcmp(text + size - 2, "?") == 0) {
        self->base.composition_length = size - 2;
        text[size - 2] = 0;
    }
}

void language_tc_clear_keys(struct LanguageTC *self)
{
    self->key_count = 0;
}

void language_tc_on_composition_changed(struct LanguageTC *self)
{
    BYTE last;

    if (self->key_count == 0)
        return;
    last = self->keys[self->key_count - 1];
    self->key_count = 0;
    if (self->table[(BYTE)last].kind < 4)
        self->keys[self->key_count++] = last;
}

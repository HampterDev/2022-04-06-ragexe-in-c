#include "platform/code_page.h"

enum {
    LANGUAGE_ARABIC = 0x01, LANGUAGE_BULGARIAN = 0x02, LANGUAGE_CHINESE = 0x04, LANGUAGE_CZECH = 0x05,
    LANGUAGE_GREEK = 0x08, LANGUAGE_HEBREW = 0x0d, LANGUAGE_HUNGARIAN = 0x0e, LANGUAGE_JAPANESE = 0x11,
    LANGUAGE_KOREAN = 0x12, LANGUAGE_POLISH = 0x15, LANGUAGE_ROMANIAN = 0x18, LANGUAGE_RUSSIAN = 0x19,
    LANGUAGE_CROATIAN = 0x1a, LANGUAGE_SLOVAK = 0x1b, LANGUAGE_THAI = 0x1e, LANGUAGE_TURKISH = 0x1f,
    LANGUAGE_UKRAINIAN = 0x22, LANGUAGE_BELARUSIAN = 0x23, LANGUAGE_SLOVENIAN = 0x24, LANGUAGE_ESTONIAN = 0x25,
    LANGUAGE_LATVIAN = 0x26, LANGUAGE_LITHUANIAN = 0x27, LANGUAGE_VIETNAMESE = 0x2a, LANGUAGE_MACEDONIAN = 0x2f
};

enum { SUBLANGUAGE_CHINESE_TRADITIONAL = 0x01, SUBLANGUAGE_CHINESE_SIMPLIFIED = 0x02 };

int get_charset_from_lang(unsigned short lang)
{
    switch (lang & 0x3ff) {
    case LANGUAGE_ARABIC:
        return CHARSET_ARABIC;
    case LANGUAGE_BULGARIAN: case LANGUAGE_RUSSIAN: case LANGUAGE_UKRAINIAN: case LANGUAGE_BELARUSIAN:
        return CHARSET_RUSSIAN;
    case LANGUAGE_CHINESE:
        switch (lang >> 10) {
        case SUBLANGUAGE_CHINESE_TRADITIONAL:
            return CHARSET_CHINESEBIG5;
        case SUBLANGUAGE_CHINESE_SIMPLIFIED:
            return CHARSET_GB2312;
        default:
            return CHARSET_GREEK;
        }
    case LANGUAGE_CZECH: case LANGUAGE_HUNGARIAN: case LANGUAGE_POLISH: case LANGUAGE_ROMANIAN: case LANGUAGE_CROATIAN:
    case LANGUAGE_SLOVAK: case LANGUAGE_SLOVENIAN: case LANGUAGE_MACEDONIAN:
        return CHARSET_EASTEUROPE;
    case LANGUAGE_GREEK:
        return CHARSET_GREEK;
    case LANGUAGE_HEBREW:
        return CHARSET_HEBREW;
    case LANGUAGE_JAPANESE:
        return CHARSET_SHIFTJIS;
    case LANGUAGE_KOREAN:
        return CHARSET_HANGUL;
    case LANGUAGE_THAI:
        return CHARSET_THAI;
    case LANGUAGE_TURKISH:
        return CHARSET_TURKISH;
    case LANGUAGE_ESTONIAN: case LANGUAGE_LATVIAN: case LANGUAGE_LITHUANIAN:
        return CHARSET_BALTIC;
    case LANGUAGE_VIETNAMESE:
        return CHARSET_VIETNAMESE;
    default:
        return CHARSET_ANSI;
    }
}

unsigned int get_code_page_from_charset(int charset)
{
    switch (charset) {
    case CHARSET_SHIFTJIS:    return 932;
    case CHARSET_HANGUL:      return 949;
    case CHARSET_GB2312:      return 936;
    case CHARSET_CHINESEBIG5: return 950;
    case CHARSET_GREEK:       return 1253;
    case CHARSET_TURKISH:     return 1254;
    case CHARSET_VIETNAMESE:  return 1258;
    case CHARSET_HEBREW:      return 1255;
    case CHARSET_ARABIC:      return 1256;
    case CHARSET_BALTIC:      return 1257;
    case CHARSET_RUSSIAN:     return 1251;
    case CHARSET_THAI:        return 874;
    case CHARSET_EASTEUROPE:  return 1250;
    default:                  return 1252;
    }
}

unsigned int code_page_from_lang(unsigned short lang)
{
    return get_code_page_from_charset(get_charset_from_lang(lang));
}

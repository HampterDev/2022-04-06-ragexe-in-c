#include "platform/language.h"

int get_charset_from_lang(WORD lang)
{
    switch (lang & 0x3ff) {
    case LANG_ARABIC:
        return ARABIC_CHARSET;
    case LANG_BULGARIAN: case LANG_RUSSIAN: case LANG_UKRAINIAN: case LANG_BELARUSIAN:
        return RUSSIAN_CHARSET;
    case LANG_CHINESE:
        switch (lang >> 10) {
        case SUBLANG_CHINESE_TRADITIONAL:
            return CHINESEBIG5_CHARSET;
        case SUBLANG_CHINESE_SIMPLIFIED:
            return GB2312_CHARSET;
        default:
            return GREEK_CHARSET;
        }
    case LANG_CZECH: case LANG_HUNGARIAN: case LANG_POLISH: case LANG_ROMANIAN: case LANG_CROATIAN:
    case LANG_SLOVAK: case LANG_SLOVENIAN: case LANG_MACEDONIAN:
        return EASTEUROPE_CHARSET;
    case LANG_GREEK:
        return GREEK_CHARSET;
    case LANG_HEBREW:
        return HEBREW_CHARSET;
    case LANG_JAPANESE:
        return SHIFTJIS_CHARSET;
    case LANG_KOREAN:
        return HANGUL_CHARSET;
    case LANG_THAI:
        return THAI_CHARSET;
    case LANG_TURKISH:
        return TURKISH_CHARSET;
    case LANG_ESTONIAN: case LANG_LATVIAN: case LANG_LITHUANIAN:
        return BALTIC_CHARSET;
    case LANG_VIETNAMESE:
        return VIETNAMESE_CHARSET;
    default:
        return ANSI_CHARSET;
    }
}

UINT get_code_page_from_charset(int charset)
{
    switch (charset) {
    case SHIFTJIS_CHARSET:    return 932;
    case HANGUL_CHARSET:      return 949;
    case GB2312_CHARSET:      return 936;
    case CHINESEBIG5_CHARSET: return 950;
    case GREEK_CHARSET:       return 1253;
    case TURKISH_CHARSET:     return 1254;
    case VIETNAMESE_CHARSET:  return 1258;
    case HEBREW_CHARSET:      return 1255;
    case ARABIC_CHARSET:      return 1256;
    case BALTIC_CHARSET:      return 1257;
    case RUSSIAN_CHARSET:     return 1251;
    case THAI_CHARSET:        return 874;
    case EASTEUROPE_CHARSET:  return 1250;
    default:                  return 1252;
    }
}

UINT code_page_from_lang(WORD lang)
{
    return get_code_page_from_charset(get_charset_from_lang(lang));
}

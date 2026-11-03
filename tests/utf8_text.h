#ifndef TESTS_UTF8_TEXT_H
#define TESTS_UTF8_TEXT_H

/* UTF-8 sample characters. The tests are compiled with /execution-charset:.949, so
   Korean literals would become CP949 bytes; these escapes stay UTF-8. Adjacent string
   literals keep each escape from running into the next character. */
#define U8_E_ACUTE "\xc3\xa9"          /* é, 2 bytes */
#define U8_ZHE "\xd0\x96"              /* Ж, 2 bytes */
#define U8_EURO "\xe2\x82\xac"         /* €, 3 bytes */
#define U8_GA "\xea\xb0\x80"           /* 가 */
#define U8_NA "\xeb\x82\x98"           /* 나 */
#define U8_DA "\xeb\x8b\xa4"           /* 다 */
#define U8_HAN "\xed\x95\x9c"          /* 한 */
#define U8_GUK "\xea\xb5\xad"          /* 국 */
#define U8_EO "\xec\x96\xb4"           /* 어 */
#define U8_AP "\xec\x95\x9e"           /* 앞 */
#define U8_I "\xec\x9d\xb4"            /* 이 */
#define U8_REUM "\xeb\xa6\x84"         /* 름 */
#define U8_DWI "\xeb\x92\xa4"          /* 뒤 */
#define U8_IP "\xec\x9e\x85"           /* 입 */
#define U8_RYEOK "\xeb\xa0\xa5"        /* 력 */
#define U8_GIN "\xea\xb8\xb4"          /* 긴 */
#define U8_A "\xec\x95\x84"            /* 아 */
#define U8_TEM "\xed\x85\x9c"          /* 템 */
#define U8_GRIN "\xf0\x9f\x98\x80"     /* 😀, 4 bytes */

#endif

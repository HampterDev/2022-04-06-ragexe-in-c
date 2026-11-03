/* Standalone checks of the UTF-8 helpers added by hercules-utf8-text.patch.
 * Compiles the patched src/common/strlib.c itself; only Hercules' allocator,
 * log and nullpo interfaces are stubbed. Build with AddressSanitizer: source
 * buffers are allocated with their exact size, so reading past the copy limit
 * aborts the run. */
#include "common/strlib.c"
#include "common/mmo.h"      /* NAME_LENGTH, MAIL_BODY_LENGTH, MAX_GUILDMES1, struct mail_message */
#include "map/mapdefines.h"  /* CHATROOM_TITLE_SIZE, MESSAGE_SIZE, CHAT_SIZE_MAX */

#include <stdarg.h>

/* ---- stubs for the interfaces strlib.c uses ---- */
void *malloc_proxy(size_t size, const char *file, int line, const char *func) { (void)file; (void)line; (void)func; return malloc(size); }
void *calloc_proxy(size_t num, size_t size, const char *file, int line, const char *func) { (void)file; (void)line; (void)func; return calloc(num, size); }
void free_proxy(void *p, const char *file, int line, const char *func) { (void)file; (void)line; (void)func; free(p); }
char *strdup_proxy(const char *p, const char *file, int line, const char *func) { size_t n = strlen(p) + 1; char *d = malloc(n); (void)file; (void)line; (void)func; memcpy(d, p, n); return d; }
char *strndup_proxy(const char *p, size_t size, const char *file, int line, const char *func)
{
	size_t n = strnlen(p, size); char *d = malloc(n + 1); (void)file; (void)line; (void)func;
	memcpy(d, p, n); d[n] = '\0'; return d;
}
static void *stub_realloc(void *p, size_t size, const char *file, int line, const char *func) { (void)file; (void)line; (void)func; return realloc(p, size); }
static void stub_show(const char *fmt, ...) { va_list ap; va_start(ap, fmt); vfprintf(stderr, fmt, ap); va_end(ap); }
static void stub_assert(const char *file, int line, const char *func, const char *target, const char *title)
{ fprintf(stderr, "%s:%d %s: %s %s\n", file, line, func, target, title); }

static struct malloc_interface malloc_stub;
static struct showmsg_interface showmsg_stub;
static struct nullpo_interface nullpo_stub;
struct malloc_interface *iMalloc = &malloc_stub;
struct showmsg_interface *showmsg = &showmsg_stub;
struct nullpo_interface *nullpo = &nullpo_stub;

/* ---- tiny harness ---- */
static int failures = 0, checks = 0;
#define CHECK(cond) do { ++checks; if (!(cond)) { ++failures; fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } } while (0)

/* exact-size copy of len bytes, without a terminator unless the bytes contain one */
static char *exact(const char *bytes, size_t len) { char *p = malloc(len ? len : 1); memcpy(p, bytes, len); return p; }

static bool zero_from(const char *buf, size_t from, size_t size)
{
	size_t i;
	for (i = from; i < size; i++)
		if (buf[i] != '\0')
			return false;
	return true;
}

/* well-formedness reference: a name check without the character exclusions */
static bool well_formed(const char *s)
{
	size_t len = strlen(s), i = 0;
	while (i < len) {
		uint32 cp;
		int n = strlib_utf8_decode((const unsigned char *)s + i, len - i, &cp);
		if (n == 0)
			return false;
		i += n;
	}
	return true;
}

#define HANGUL_GA "\xEA\xB0\x80"        /* U+AC00 */
#define HANGUL_GAEN "\xEA\xB0\xA0"      /* U+AC20, contains byte 0xA0 */
#define HANGUL_GAEP "\xEA\xB0\xAD"      /* U+AC2D, contains byte 0xAD */
#define CYRILLIC_ER "\xD0\xA0"          /* U+0420 Р, contains byte 0xA0 */
#define CYRILLIC_E "\xD0\xAD"           /* U+042D Э, contains byte 0xAD */
#define EMOJI "\xF0\x9F\x98\x80"        /* U+1F600 */

static void test_names(void)
{
	/* accepted */
	CHECK(strlib->utf8_is_valid_name("Alice"));
	CHECK(strlib->utf8_is_valid_name("A B"));
	CHECK(strlib->utf8_is_valid_name(CYRILLIC_ER CYRILLIC_E "\xD0\xB9"));
	CHECK(strlib->utf8_is_valid_name(HANGUL_GA HANGUL_GAEN HANGUL_GAEP));
	CHECK(strlib->utf8_is_valid_name("\xC3\xA9t\xC3\xA9"));               /* été */
	CHECK(strlib->utf8_is_valid_name(EMOJI "x"));
	CHECK(strlib->utf8_is_valid_name("\xE2\x80\x8B"));                     /* U+200B is not a bidi control: allowed */
	CHECK(strlib->utf8_is_valid_name("\xEF\xBF\xBD"));                     /* U+FFFD */
	CHECK(strlib->utf8_is_valid_name("\xF4\x8F\xBF\xBF"));                 /* U+10FFFF */

	/* not well-formed */
	CHECK(!strlib->utf8_is_valid_name(""));
	CHECK(!strlib->utf8_is_valid_name(NULL));
	CHECK(!strlib->utf8_is_valid_name("\xC0\x80"));                        /* overlong NUL */
	CHECK(!strlib->utf8_is_valid_name("\xC1\xBF"));                        /* overlong */
	CHECK(!strlib->utf8_is_valid_name("\xE0\x80\x80"));                    /* overlong */
	CHECK(!strlib->utf8_is_valid_name("\xE0\x9F\xBF"));                    /* overlong U+07FF */
	CHECK(!strlib->utf8_is_valid_name("\xF0\x80\x80\x80"));                /* overlong */
	CHECK(!strlib->utf8_is_valid_name("\xF0\x8F\xBF\xBF"));                /* overlong U+FFFF */
	CHECK(!strlib->utf8_is_valid_name("\xED\xA0\x80"));                    /* surrogate U+D800 */
	CHECK(!strlib->utf8_is_valid_name("\xED\xBF\xBF"));                    /* surrogate U+DFFF */
	CHECK(!strlib->utf8_is_valid_name("\xF4\x90\x80\x80"));                /* above U+10FFFF */
	CHECK(!strlib->utf8_is_valid_name("\xF5\x80\x80\x80"));
	CHECK(!strlib->utf8_is_valid_name("\xFF"));
	CHECK(!strlib->utf8_is_valid_name("a\x80"));                           /* stray continuation */
	CHECK(!strlib->utf8_is_valid_name("ab\xE3\x81"));                      /* truncated */
	CHECK(!strlib->utf8_is_valid_name("\xE3\x41\x82"));                    /* bad continuation */
	CHECK(!strlib->utf8_is_valid_name("\xB0\xA1"));                        /* CP949 가 */
	CHECK(!strlib->utf8_is_valid_name("Caf\xE9"));                         /* Latin-1 é */

	/* control and invisible-direction characters */
	CHECK(!strlib->utf8_is_valid_name("a\x01" "b"));
	CHECK(!strlib->utf8_is_valid_name("a\tb"));
	CHECK(!strlib->utf8_is_valid_name("a\x7F"));
	CHECK(!strlib->utf8_is_valid_name("a\xC2\x85"));                       /* U+0085 NEL (C1) */
	CHECK(!strlib->utf8_is_valid_name("a\xC2\x9F"));                       /* U+009F (C1) */
	CHECK(!strlib->utf8_is_valid_name("a\xC2\xA0" "b"));                   /* U+00A0 */
	CHECK(!strlib->utf8_is_valid_name("a\xC2\xAD" "b"));                   /* U+00AD */
	CHECK(!strlib->utf8_is_valid_name("\xD8\x9C" "a"));                    /* U+061C */
	CHECK(!strlib->utf8_is_valid_name("a\xE2\x80\x8E"));                   /* U+200E */
	CHECK(!strlib->utf8_is_valid_name("a\xE2\x80\x8F"));                   /* U+200F */
	CHECK(!strlib->utf8_is_valid_name("\xE2\x80\xAA" "a"));                /* U+202A */
	CHECK(!strlib->utf8_is_valid_name("\xE2\x80\xAE" "evil"));             /* U+202E */
	CHECK(!strlib->utf8_is_valid_name("\xE2\x81\xA6" "a"));                /* U+2066 */
	CHECK(!strlib->utf8_is_valid_name("\xE2\x81\xA9" "a"));                /* U+2069 */
	CHECK(strlib->utf8_is_valid_name("\xE2\x80\xAF"));                     /* U+202F is not excluded */
}

static void test_trim_chars(void)
{
	/* the TRIM_CHARS values before and after the patch (src/map/charselect.c) */
	const char *const old_trim = "\255\xA0\032\t\x0A\x0D ";
	const char *const new_trim = "\032\t\x0A\x0D ";
	char name[NAME_LENGTH];

	safestrncpy(name, "  " CYRILLIC_ER CYRILLIC_E " " HANGUL_GAEN HANGUL_GAEP "\t", sizeof name);
	normalize_name(name, new_trim);
	CHECK(strcmp(name, CYRILLIC_ER CYRILLIC_E " " HANGUL_GAEN HANGUL_GAEP) == 0);
	CHECK(strlib->utf8_is_valid_name(name));

	safestrncpy(name, CYRILLIC_ER CYRILLIC_E HANGUL_GAEN HANGUL_GAEP, sizeof name);
	normalize_name(name, old_trim);                    /* the old behaviour corrupts every such character */
	CHECK(strcmp(name, "\xD0 \xD0 \xEA\xB0 \xEA\xB0") == 0);
	CHECK(!strlib->utf8_is_valid_name(name));
}

static void test_boundary(void)
{
	CHECK(strlib->utf8_boundary("", 0) == 0);
	CHECK(strlib->utf8_boundary(NULL, 5) == 0);
	CHECK(strlib->utf8_boundary("abc", 3) == 3);
	CHECK(strlib->utf8_boundary("ab\xE3\x81", 4) == 2);
	CHECK(strlib->utf8_boundary("ab\xE3", 3) == 2);
	CHECK(strlib->utf8_boundary("ab\xE3\x81\x82", 5) == 5);
	CHECK(strlib->utf8_boundary("\xC3", 1) == 0);
	CHECK(strlib->utf8_boundary("x\xC3\xA9", 3) == 3);
	CHECK(strlib->utf8_boundary("\xF0\x9F\x98", 3) == 0);
	CHECK(strlib->utf8_boundary("\xF0\x9F", 2) == 0);
	CHECK(strlib->utf8_boundary(EMOJI, 4) == 4);
	CHECK(strlib->utf8_boundary("a" EMOJI, 4) == 1);
	/* not UTF-8: left alone rather than guessed at */
	CHECK(strlib->utf8_boundary("a\x80", 2) == 2);
	CHECK(strlib->utf8_boundary("\x80\x80\x80\x80", 4) == 4);
	CHECK(strlib->utf8_boundary("a\xFF", 2) == 2);
	CHECK(strlib->utf8_boundary("a\xC0", 2) == 2);
	/* only the first len bytes are examined */
	CHECK(strlib->utf8_boundary("ab\xE3\x81\x82", 4) == 2);
	{
		char *p = exact("xy\xE3\x81", 4);
		CHECK(strlib->utf8_boundary(p, 4) == 2);
		free(p);
	}
	/* every cut of a valid string yields a valid prefix */
	{
		const char *s = "A" HANGUL_GA "b" EMOJI CYRILLIC_E "z";
		size_t len = strlen(s), cut;
		for (cut = 0; cut <= len; cut++) {
			char tmp[64];
			size_t keep = strlib->utf8_boundary(s, cut);
			CHECK(keep <= cut && cut - keep <= 3);
			memcpy(tmp, s, keep);
			tmp[keep] = '\0';
			CHECK(keep == 0 || well_formed(tmp));
		}
	}
}

static void test_safestrncpy_utf8(void)
{
	char buf[64];

	/* fits: unchanged and NUL-padded like safestrncpy */
	memset(buf, 'X', sizeof buf);
	safestrncpy_utf8(buf, "abc", 10);
	CHECK(strcmp(buf, "abc") == 0 && zero_from(buf, 3, 10) && buf[10] == 'X');

	/* exactly full on a character boundary: 12 Hangul = 36 bytes into a chat title (37) */
	{
		char title[CHATROOM_TITLE_SIZE];
		char src[64] = "";
		int i;
		for (i = 0; i < 12; i++) strcat(src, HANGUL_GA);
		safestrncpy_utf8(title, src, sizeof title);
		CHECK(strlen(title) == 36 && strcmp(title, src) == 0);
	}

	/* full and cut inside a character: "a" + 12 Hangul = 37 bytes, buffer keeps 36 */
	{
		char title[CHATROOM_TITLE_SIZE];
		char legacy[CHATROOM_TITLE_SIZE];
		char src[64] = "a";
		int i;
		for (i = 0; i < 12; i++) strcat(src, HANGUL_GA);
		safestrncpy_utf8(title, src, sizeof title);
		CHECK(strlen(title) == 34 && strncmp(title, src, 34) == 0 && well_formed(title));
		CHECK(zero_from(title, 34, sizeof title));
		safestrncpy(legacy, src, sizeof legacy);           /* what the server stored before */
		CHECK(strlen(legacy) == 36 && !well_formed(legacy));
	}

	/* 4-byte character cut after two bytes */
	safestrncpy_utf8(buf, "abc" EMOJI, 6);
	CHECK(strcmp(buf, "abc") == 0 && zero_from(buf, 3, 6));

	/* chat title path: title is not NUL-terminated, n = min(len+1, CHATROOM_TITLE_SIZE);
	 * the exact-size source makes ASan catch any read of byte len */
	{
		char title[CHATROOM_TITLE_SIZE];
		const char *bytes = "Hi " HANGUL_GA HANGUL_GAEP;       /* 9 bytes */
		int len = 9;
		char *src = exact(bytes, (size_t)len);
		safestrncpy_utf8(title, src, min(len + 1, CHATROOM_TITLE_SIZE));
		CHECK(strcmp(title, bytes) == 0);
		free(src);

		/* a client that claims more bytes than fit: 40 bytes, cut at 36 */
		{
			char big[40];
			memcpy(big, "abcd", 4);
			for (len = 4; len + 3 <= 40; len += 3) memcpy(big + len, HANGUL_GAEN, 3);
			src = exact(big, (size_t)len);                  /* len == 40 */
			safestrncpy_utf8(title, src, min(len + 1, CHATROOM_TITLE_SIZE));
			CHECK(strlen(title) == 34 && well_formed(title));
			free(src);
		}
	}

	/* vending/buying store title: 80-byte field without terminator, 79 kept */
	{
		char message[MESSAGE_SIZE];
		char field[80];
		char *src;
		int i;
		field[0] = 'S';
		for (i = 1; i + 3 <= 80; i += 3) memcpy(field + i, HANGUL_GAEP, 3);   /* 1 + 26*3 = 79 bytes */
		field[79] = (char)0xEA;                                               /* lead byte in the last slot */
		src = exact(field, sizeof field);
		safestrncpy_utf8(message, src, MESSAGE_SIZE);
		CHECK(strlen(message) == 79 && well_formed(message));
		field[0] = 'S'; field[1] = 'S';
		for (i = 2; i + 3 <= 80; i += 3) memcpy(field + i, HANGUL_GAEP, 3);   /* 2 + 26*3 = 80 bytes */
		memcpy(src, field, sizeof field);                                     /* the 79-byte limit cuts the last one */
		safestrncpy_utf8(message, src, MESSAGE_SIZE);
		CHECK(strlen(message) == 77 && well_formed(message));
		free(src);
	}

	/* mail body: body_len 200 now copies into the 200-byte array with n = 200 (was 201) */
	{
		struct mail_message *msg = malloc(sizeof *msg);
		char body_field[MAIL_BODY_LENGTH];
		char *src;
		int body_len = MAIL_BODY_LENGTH, i;
		for (i = 0; i + 3 <= MAIL_BODY_LENGTH; i += 3) memcpy(body_field + i, HANGUL_GA, 3);  /* 198 bytes */
		body_field[198] = 'x'; body_field[199] = 'y';
		src = exact(body_field, sizeof body_field);
		memset(msg, 0x55, sizeof *msg);
		safestrncpy_utf8(msg->body, src, min(body_len + 1, (int)sizeof(msg->body)));
		CHECK(strlen(msg->body) == 199 && msg->body[198] == 'x' && well_formed(msg->body));
		CHECK(*(unsigned char *)&msg->status == 0x55);         /* the field after body is untouched */
		free(src);
		free(msg);
	}

	/* n == 1 and n == 0 */
	memset(buf, 'X', sizeof buf);
	safestrncpy_utf8(buf, "abc", 1);
	CHECK(buf[0] == '\0' && buf[1] == 'X');
	safestrncpy_utf8(buf, "abc", 0);
	CHECK(buf[0] == '\0' && buf[1] == 'X');

	/* a NUL inside the limit: no cut even if bytes after it are partial */
	{
		char *src = exact("ab\0\xE3\x81", 5);
		safestrncpy_utf8(buf, src, 6);
		CHECK(strcmp(buf, "ab") == 0);
		free(src);
	}

	/* exhaustive: every prefix length of a mixed string, every buffer size */
	{
		const char *s = "R" CYRILLIC_ER "x" HANGUL_GAEN EMOJI "\xC3\xA9" "end";
		size_t len = strlen(s), n;
		for (n = 1; n <= len + 2; n++) {
			char *src = exact(s, len + 1);
			char out[64];
			memset(out, 'X', sizeof out);
			safestrncpy_utf8(out, src, n);
			CHECK(strlen(out) < n && strncmp(out, s, strlen(out)) == 0 && well_formed(out));
			CHECK(n - 1 - strlen(out) <= 3 || strlen(s) < n - 1);
			CHECK(zero_from(out, strlen(out), n) && out[n] == 'X');
			free(src);
		}
	}
}

/* guild notice and /b broadcast call sites */
static void test_call_sites(void)
{
	/* clif_parse_GuildChangeNotice: aStrndup to 59 bytes, then drop a cut-short character */
	{
		char field[MAX_GUILDMES1];
		char *msg1, *src;
		int i;
		field[0] = '!';
		for (i = 1; i + 3 <= MAX_GUILDMES1; i += 3) memcpy(field + i, HANGUL_GA, 3);   /* 1 + 19*3 = 58 */
		memcpy(field + 58, "\xEA\xB0", 2);                                              /* 60 bytes, last char cut */
		src = exact(field, sizeof field);
		msg1 = aStrndup(src, MAX_GUILDMES1 - 1);
		if (strlen(msg1) == MAX_GUILDMES1 - 1)
			msg1[strlib->utf8_boundary(msg1, MAX_GUILDMES1 - 1)] = '\0';
		CHECK(strlen(msg1) == 58 && well_formed(msg1));
		aFree(msg1);
		free(src);
	}

	/* clif_parse_Broadcast: strncat with len capped at a UTF-8 boundary */
	{
		char command[sizeof "kami" + 2 + CHAT_SIZE_MAX] = "@kami ";
		char text[400];
		char *src;
		int len = (int)sizeof text, i;
		for (i = 0; i + 3 <= len; i += 3) memcpy(text + i, HANGUL_GAEP, 3);
		text[399] = 'z';
		src = exact(text, sizeof text);
		if (len >= (int)(sizeof command - strlen(command)))
			len = (int)strlib->utf8_boundary(src, sizeof command - strlen(command) - 1);
		strncat(command, src, len);
		CHECK(strlen(command) < sizeof command && well_formed(command));
		CHECK(strlen(command) >= sizeof command - 1 - 2);
		free(src);
	}
}

int main(void)
{
	malloc_stub.realloc = stub_realloc;
	showmsg_stub.showError = stub_show;
	showmsg_stub.showWarning = stub_show;
	showmsg_stub.showDebug = stub_show;
	showmsg_stub.showInfo = stub_show;
	showmsg_stub.showStatus = stub_show;
	showmsg_stub.showNotice = stub_show;
	showmsg_stub.showFatalError = stub_show;
	nullpo_stub.assert_report = stub_assert;
	strlib_defaults();

	test_names();
	test_trim_chars();
	test_boundary();
	test_safestrncpy_utf8();
	test_call_sites();

	printf("UTF-8 helper checks: %d run, %d failed\n", checks, failures);
	return failures != 0;
}

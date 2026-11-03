#ifndef PLATFORM_LANGUAGE_API_H
#define PLATFORM_LANGUAGE_API_H

struct Language;
extern struct Language *g_language;

const char *language_charset_tag(struct Language *self, unsigned char folder_lang);

#endif

#ifndef PLATFORM_CODE_PAGE_H
#define PLATFORM_CODE_PAGE_H

extern unsigned int g_codePage;

/* Resource and file names in the GRF archives and data folder are CP949. */
#define RESOURCE_NAME_CODE_PAGE 949
extern unsigned char g_useCharsetTag;

int get_charset_from_lang(unsigned short lang);
unsigned int get_code_page_from_charset(int charset);

#endif

#ifndef PLATFORM_CLIENTINFO_H
#define PLATFORM_CLIENTINFO_H

#include "platform/error_message.h"
#include "platform/language_api.h"
#include "fileio/file_manager.h"
#include "std/string.h"
#include "platform/service.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "platform/client_config.h"

#include "platform/win_types.h"
#include "base.h"
#include "std/std.h"
#include "core/xml.h"
#include "platform/language.h"

extern int g_clientVersion;
extern BYTE g_hideAccountList;
extern BYTE g_passwordEncrypt;
extern BYTE g_passwordEncrypt2;
extern BYTE g_clientInfoRead;
extern char g_accountServerAddr[0x81];
extern struct Vec g_adminAids;
extern struct Vec g_yellowAids;
extern struct Vec g_loadingScreens;

/* Receives each "weather" line of etcinfo.txt: weather effect `effect` for map `map`,
   both lowercased; the receiver owns and frees both strings. client_install_hooks()
   (core/client_init.c) installs weather_add_etc_script (actor/weather.c). NULL: the line
   is ignored and both strings are freed. Defined in platform/clientinfo.c. */
extern void (*g_clientInfoWeatherScript)(struct String map, struct String effect);

void uint_sort(unsigned *first, unsigned *last);
struct XmlElement *get_client_info(void);
void clientinfo_load(const char *xml_name);
BYTE clientinfo_is_admin_aid(int aid);
BYTE clientinfo_is_yellow_aid(int aid);
void select_client_info2(int connection);
void set_option(struct XmlElement *node);

#endif

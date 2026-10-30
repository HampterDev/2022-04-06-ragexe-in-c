#ifndef PLATFORM_CLIENTINFO_H
#define PLATFORM_CLIENTINFO_H

#include "platform/error_message.h"
#include "platform/language_api.h"
#include "fileio/file_manager.h"
#include "std/string.h"
#include "platform/service.h"
#include "actor/weather.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "platform/client_config.h"

#include <windows.h>
#include "base.h"
#include "std/std.h"
#include "core/xml.h"
#include "platform/language.h"

extern int g_clientVersion;
extern int g_languageIndex;
extern BYTE g_hideAccountList;
extern BYTE g_passwordEncrypt;
extern BYTE g_passwordEncrypt2;
extern BYTE g_clientInfoRead;
extern BYTE g_showNameA;
extern BYTE g_showNameB;
extern int g_nameFontHeight;
extern BYTE g_accountOnCmdLine;
extern char g_accountServerAddr[0x81];
extern struct Vec g_adminAids;
extern struct Vec g_yellowAids;
extern struct Vec g_loadingScreens;

extern struct String g_licenceText;
extern INIT_ONCE g_licenceTextGuard;

void uint_sort(unsigned *first, unsigned *last);
struct XmlElement *get_client_info(void);
const char *get_licence_text_(void);
void clientinfo_load(const char *xml_name);
void init_client_info_(void);
void load_loading_screens(int count);
void init_default_service_type_(void);
void init_language_(void);
BYTE clientinfo_is_admin_aid(int aid);
BYTE clientinfo_is_yellow_aid(int aid);
void select_client_info_(int connection, int subconnection);
void select_client_info2(int connection);
void set_option(struct XmlElement *node);
void session_set_text_type(int a, int b);

#endif

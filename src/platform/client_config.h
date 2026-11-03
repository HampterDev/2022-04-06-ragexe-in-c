#ifndef PLATFORM_CLIENT_CONFIG_H
#define PLATFORM_CLIENT_CONFIG_H

/* The client always runs as langtype 20 (service "eu"); clientinfo.xml's <langtype> and
   <servicetype> are ignored. The number survives in the shortcut registry value names and
   as the server-select window's default before its saved choice is read. */
#define CLIENT_LANGTYPE 20

struct XmlDocument;
extern struct XmlDocument g_clientInfoDocument;

extern const char *g_serverAddress;
extern const char *g_serverPort;
extern const char *g_serverDomain;
extern const char *g_windowClassName;
extern const char *g_registryKeyPath;

extern unsigned char g_startupSecondMod4;

#endif

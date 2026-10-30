#ifndef PLATFORM_CLIENT_CONFIG_H
#define PLATFORM_CLIENT_CONFIG_H

struct XmlDocument;
extern struct XmlDocument g_clientInfoDocument;

extern const char *g_serverAddress;
extern const char *g_serverPort;
extern const char *g_serverDomain;
extern const char *g_registrationWeb;
extern const char *g_licenceFile;
extern const char *g_clientExeName;
extern const char *g_patcherExeName;
extern const char *g_runClientMessage;
extern const char *g_windowClassName;
extern const char *g_registryKeyPath;

extern unsigned char g_startupSecondMod4;

#endif

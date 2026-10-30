#ifndef SECURITY_GAMEGUARD_H
#define SECURITY_GAMEGUARD_H

#include <windows.h>

struct GameGuard;
extern struct GameGuard *g_gameGuard;
int gameguard_startup(void);
void gameguard_shutdown(void);
void gameguard_set_error(struct GameGuard *guard, const char *text);
int gameguard_has_error(struct GameGuard *guard);
void gameguard_check(struct GameGuard *guard);
void gameguard_on_connect(struct GameGuard *guard);
void gameguard_authenticate(struct GameGuard *guard);
void gameguard_send_auth_answers(struct GameGuard *guard);
void gameguard_set_window(struct GameGuard *guard);
void gameguard_set_account(struct GameGuard *guard);

#endif

#ifndef UI_MEDIA_CONNECT_SNS_H
#define UI_MEDIA_CONNECT_SNS_H

#include <windows.h>

BYTE connect_sns_mgr_create_instance(void);
void connect_sns_mgr_destroy_instance(void);
int connect_sns_mgr_is_window_closed(void);

struct ConnectSnsMgr;
extern struct ConnectSnsMgr *g_connectSnsMgr;

#endif

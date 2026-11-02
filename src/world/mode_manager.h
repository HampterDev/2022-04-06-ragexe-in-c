#ifndef WORLD_MODE_MANAGER_H
#define WORLD_MODE_MANAGER_H

#include "base.h"

struct Mode;

enum ModeType {
    MODE_TYPE_LOGIN = 0,
    MODE_TYPE_GAME = 1
};

struct ModeMgr {
    int loopCond;
    struct Mode *curMode;
    char curModeName[40];
    char nextModeName[40];
    int curModeType;
    int nextModeType;
};

extern struct ModeMgr g_modeMgr;

struct Mode *client_current_mode(void);

void mode_mgr_run(struct ModeMgr *manager, int type, const char *name);
void mode_mgr_switch(struct ModeMgr *manager, int type, const char *name);
void mode_mgr_quit(struct ModeMgr *manager);
struct Mode *mode_mgr_get_login_mode(struct ModeMgr *manager);

#endif

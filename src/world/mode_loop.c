#include "world/mode_manager.h"
#include "world/mode_types.h"
#include "login/lifecycle.h"
#include "game/lifecycle.h"
#include <string.h>
#include "core/app.h"

void mode_mgr_run(struct ModeMgr *self, int type, const char *name)
{
    self->curModeType = type;
    self->nextModeType = type;
    strcpy(self->curModeName, name);
    strcpy(self->nextModeName, name);
    while (self->loopCond && !g_quitRequested) {
        struct Mode *mode;

        self->curModeType = self->nextModeType;
        strcpy(self->curModeName, self->nextModeName);
        switch (self->curModeType) {
        case MODE_TYPE_LOGIN:
            mode = self->curMode = login_mode_create();
            if (!mode)
                break;
            login_mode_on_init(mode, self->curModeName);
            login_mode_on_run(mode);
            login_mode_on_exit(mode);
            login_mode_destroy(mode);
            break;
        case MODE_TYPE_GAME:
            mode = self->curMode = (struct Mode *)game_mode_create();
            if (!mode)
                break;
            game_mode_on_init(mode, self->curModeName);
            game_mode_on_run(mode);
            game_mode_on_exit(mode);
            game_mode_destroy(mode);
            break;
        default:
            mode = self->curMode = NULL;
            break;
        }
        if (!mode) {
            self->loopCond = 0;
            break;
        }
        self->curMode = NULL;
    }
}

void mode_mgr_switch(struct ModeMgr *self, int type, const char *name)
{
    if (self->curMode)
        self->curMode->loop_cond = 0;
    strcpy(self->nextModeName, name);
    self->nextModeType = type;
}

#include "world/mode_manager.h"
#include "world/mode_types.h"
#include "login/lifecycle.h"
#include "game/lifecycle.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int g_quitRequested;
static struct ModeMgr *manager;
static int creations, live, fail_on, quit_during_init, switch_after_login;
static char events[64];
static size_t event_count;

struct TestMode {
    struct Mode base;
    int type, phase;
};

static void event(char code)
{
    assert(event_count + 1 < sizeof events);
    events[event_count++] = code;
    events[event_count] = '\0';
}

static void initialize(struct Mode *mode, const char *name, int type)
{
    struct TestMode *test = (struct TestMode *)mode;
    assert(manager->curMode == mode && test->phase == 0 && test->type == type);
    assert(manager->curModeType == test->type);
    assert(strcmp(name, test->type == MODE_TYPE_LOGIN ? "login.rsw" : "prontera.rsw") == 0);
    test->phase = 1;
    if (quit_during_init)
        g_quitRequested = 1;
    event('I');
}

static int run(struct Mode *mode, int type)
{
    struct TestMode *test = (struct TestMode *)mode;
    assert(manager->curMode == mode && test->phase == 1 && test->type == type);
    test->phase = 2;
    event('R');
    if (test->type == MODE_TYPE_LOGIN && switch_after_login && !g_quitRequested) {
        mode_mgr_switch(manager, MODE_TYPE_GAME, "prontera.rsw");
        assert(mode->loop_cond == 0);
    } else {
        manager->loopCond = 0;
    }
    return 17;
}

static void leave(struct Mode *mode, int type)
{
    struct TestMode *test = (struct TestMode *)mode;
    assert(manager->curMode == mode && test->phase == 2 && test->type == type);
    test->phase = 3;
    event('X');
}

static void destroy(struct Mode *mode, int type)
{
    struct TestMode *test = (struct TestMode *)mode;
    assert(manager->curMode == mode && test->phase == 3 && test->type == type);
    assert(live == 1);
    --live;
    event('D');
    free(test);
}

void login_mode_on_init(struct Mode *mode, const char *name) { initialize(mode, name, MODE_TYPE_LOGIN); }
int login_mode_on_run(struct Mode *mode) { return run(mode, MODE_TYPE_LOGIN); }
void login_mode_on_exit(struct Mode *mode) { leave(mode, MODE_TYPE_LOGIN); }
void login_mode_destroy(struct Mode *mode) { destroy(mode, MODE_TYPE_LOGIN); }
void game_mode_on_init(struct Mode *mode, const char *name) { initialize(mode, name, MODE_TYPE_GAME); }
int game_mode_on_run(struct Mode *mode) { return run(mode, MODE_TYPE_GAME); }
void game_mode_on_exit(struct Mode *mode) { leave(mode, MODE_TYPE_GAME); }
void game_mode_destroy(struct Mode *mode) { destroy(mode, MODE_TYPE_GAME); }

static struct Mode *create(int type)
{
    struct TestMode *mode;
    assert(live == 0);
    ++creations;
    event(type == MODE_TYPE_LOGIN ? 'L' : 'G');
    if (creations == fail_on)
        return NULL;
    mode = calloc(1, sizeof *mode);
    assert(mode);
    mode->base.loop_cond = 1;
    mode->type = type;
    ++live;
    return &mode->base;
}

struct Mode *login_mode_create(void) { return create(MODE_TYPE_LOGIN); }
struct GameMode *game_mode_create(void) { return (struct GameMode *)create(MODE_TYPE_GAME); }

static void reset(struct ModeMgr *owner)
{
    assert(live == 0);
    memset(owner, 0, sizeof *owner);
    owner->loopCond = 1;
    manager = owner;
    creations = fail_on = quit_during_init = g_quitRequested = 0;
    switch_after_login = 1;
    event_count = 0;
    events[0] = '\0';
}

int main(void)
{
    struct ModeMgr owner;
    reset(&owner);
    mode_mgr_run(&owner, MODE_TYPE_LOGIN, "login.rsw");
    assert(strcmp(events, "LIRXDGIRXD") == 0);
    assert(creations == 2 && live == 0 && !owner.curMode && !owner.loopCond);

    reset(&owner);
    fail_on = 1;
    mode_mgr_run(&owner, MODE_TYPE_LOGIN, "login.rsw");
    assert(strcmp(events, "L") == 0 && !owner.curMode && !owner.loopCond);

    reset(&owner);
    fail_on = 2;
    mode_mgr_run(&owner, MODE_TYPE_LOGIN, "login.rsw");
    assert(strcmp(events, "LIRXDG") == 0 && !owner.curMode && !owner.loopCond);

    reset(&owner);
    quit_during_init = 1;
    mode_mgr_run(&owner, MODE_TYPE_LOGIN, "login.rsw");
    assert(strcmp(events, "LIRXD") == 0 && !owner.curMode && live == 0);

    reset(&owner);
    g_quitRequested = 1;
    mode_mgr_run(&owner, MODE_TYPE_LOGIN, "login.rsw");
    assert(!creations && !owner.curMode);

    reset(&owner);
    mode_mgr_run(&owner, 99, "unknown");
    assert(!creations && !owner.curMode && !owner.loopCond);
    mode_mgr_switch(&owner, MODE_TYPE_GAME, "prontera.rsw");
    assert(owner.nextModeType == MODE_TYPE_GAME && !owner.curMode);
    assert(strcmp(owner.nextModeName, "prontera.rsw") == 0);
    assert(live == 0);
    puts("mode lifecycle: transition order, exactly-once destruction, allocation failures and quit paths passed");
    return 0;
}

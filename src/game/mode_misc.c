#include "game/cells.h"
#include "game/gamemode.h"
#include "game/mode_misc.h"
#include "effects/skills/dummy_effect_launcher.h"
#include "actor/render_ops.h"
#include "game/world_objects.h"
#include "resources/sprite.h"
#include "resources/animation.h"
#include "iteminfo/item_api.h"
#include <windows.h>
#include "actor/render_object.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "game/mode_callbacks.h"
#include "ui/window_access.h"
#include "contents/connection_api.h"
#include <stdlib.h>
#include <string.h>
#include "game/objects.h"
#include "game/item_info.h"
#include "game/camera.h"
#include "actor/game_actor.h"
#include "session/session_items.h"
#include "ui/wndmgr/wndmgr.h"
#include "world/world.h"

void game_mode_refresh_own_hp(struct GameMode *self)
{
    if (self->status_bars != 0)
        ui_window_refresh(self->status_bars);
    if (g_windowMgr.messenger_group_wnd != 0)
        ui_window_send(g_windowMgr.messenger_group_wnd, NULL, 6, 0x22, ((unsigned)g_session1.ownAid), 0, 0);
    ui_window_mgr_command_plate_of(&g_windowMgr, ((unsigned)g_session1.ownAid), g_session1.hp, g_session1.maxHp);
}

static int has_item(const struct ItemInfo *item)
{
    return item->index != 0 && item->count > 0;
}

char game_mode_has_resurrection_token(struct GameMode *self)
{
    struct ItemInfo item, tmp;
    char found;

    session_get_inventory_item_by_id(&g_session1, &item, 0x1dc5, 0);
    if (self->world->is_pvp != 0 || self->world->is_siege != 0 || self->world->is_event_pvp != 0) {
        found = 0;
    } else if (has_item(&item)) {
        found = 1;
    } else {
        item_info_assign(&item, session_get_inventory_item_by_id(&g_session1, &tmp, 0x1ab1, 0));
        item_info_destruct(&tmp);
        if (has_item(&item)) {
            found = 1;
        } else {
            item_info_assign(&item, session_get_inventory_item_by_id(&g_session1, &tmp, 0x18ac, 0));
            item_info_destruct(&tmp);
            if (has_item(&item)) {
                found = 1;
            } else {
                item_info_assign(&item, session_get_inventory_item_by_id(&g_session1, &tmp, 0x1895, 0));
                item_info_destruct(&tmp);
                if (has_item(&item)) {
                    found = 1;
                } else {
                    item_info_assign(&item, session_get_inventory_item_by_id(&g_session1, &tmp, 0x62de, 0));
                    item_info_destruct(&tmp);
                    found = has_item(&item) ? 1 : 0;
                }
            }
        }
    }
    item_info_destruct(&item);
    return found;
}

void game_mode_mouse_pointer_msg2(struct GameMode *self, int value)
{
    if (self->mouse_pointer != 0)
        render_object_send(self->mouse_pointer, NULL, 2, (long long)value, 0, 0, 0);
}

struct ShortNameBuffer {
    int thread;
    char text[16];
};

struct Tree g_shortNameBuffers;
INIT_ONCE g_shortNameBuffersGuard;
void name_buffers_atexit(void);

char *game_mode_short_name(struct String name)
{
    struct ShortNameBuffer *buffer;
    const char *text;
    char *buf;
    int thread;
    size_t at;

    if (once_begin(&g_shortNameBuffersGuard)) {
        tree_init(&g_shortNameBuffers);
        atexit(name_buffers_atexit);
        once_complete(&g_shortNameBuffersGuard);
    }
    thread = (int)GetCurrentThread();
    buffer = tree_get(&g_shortNameBuffers, &thread, sizeof thread, sizeof *buffer, tree_less_int);
    buf = buffer->text;
    text = string_cstr(&name);
    if (name.size != 0) {
        for (at = name.size - 1; text[at] != '@'; at--)
            if (at == 0)
                goto copy;
        if (at >= 4)
            string_erase(&name, at - 4, 3);
    }
copy:
    _snprintf_s(buf, sizeof buffer->text, (size_t)-1, "%s", string_cstr(&name));
    string_free(&name);
    return buf;
}

#define DUMMY_LAUNCHER_MSG_START 0xe

const struct RenderObjectOps dummy_effect_launcher_ops = {
    {
        dummy_effect_launcher_deleting_destruct,
        dummy_effect_launcher_on_process,
        dummy_effect_launcher_send_msg,
        render_object_render,
        game_object_dir_of,
    },
    game_object_set_render_info,
    render_object_set_action_base,
    render_object_update_motion_base,
    render_object_return_zero,
};

unsigned long long game_mode_launch_dummy_effect(struct GameMode *self, const Vec3 *at, int effect_id,
                                               int frames, int value)
{
    struct AbleToMakeEffect *launcher;

    if (frames <= 0)
        return 0;
    launcher = mem_alloc(sizeof(struct DummyEffectLauncher));
    if (launcher == 0)
        return 0;
    able_to_make_effect_construct(launcher);
    launcher->ops = &dummy_effect_launcher_ops.base;
    render_object_send(launcher, NULL, DUMMY_LAUNCHER_MSG_START, MSG_ARG_PTR(at), effect_id, frames, value);
    list_push_back(&self->world->gameObjects, &launcher, sizeof launcher);
    return launcher->serial;
}

void game_mode_on_change_state(struct Mode *mode, int state)
{
    struct GameMode *self = (struct GameMode *)mode;
    (void)self;
    (void)state;
}

void game_mode_add_world_object(struct GameMode *self, void *object)
{
    list_push_back(&self->world->gameObjects, &object, sizeof object);
}

void name_buffers_atexit(void)
{
    tree_free(&g_shortNameBuffers, NULL);
}

struct String *game_mode_get_map_name(struct GameMode *self, struct String *out)
{
    const char *name = self->map_name;

    string_init(out);
    string_assign_cstr(out, name);
    return out;
}

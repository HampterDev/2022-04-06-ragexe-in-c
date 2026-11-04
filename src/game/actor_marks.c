#include "game/actor_marks.h"
#include "actor/actor_ops.h"
#include "ui/window_access.h"
#include "contents/client_lua_calls.h"
#include "session/state.h"
#include <string.h>
#include "game/objects.h"
#include "effects/skills/msg_effect.h"
#include "platform/clientinfo.h"
#include "session/session_skills.h"
#include "ui/character/character.h"
#include "ui/social/social.h"
#include "ui/ui_std.h"
#include "world/world.h"
#include "actor/actor_common.h"

static int actor_hidden(struct GameActor *actor)
{
    if (clientinfo_is_admin_aid(((unsigned)g_session1.ownAid)))
        return 0;
    return is_effect_state_star2(actor_get_effect_state(actor)) || is_effect_state_burrow(actor_get_effect_state(actor))
           || effect_state_bit6(actor_get_effect_state(actor));
}

void game_mode_make_emblem_wnd(struct GameMode *self, struct GameActor *actor, int guild_id)
{
    struct UIWindow *wnd;

    if (actor == 0 || guild_id == 0 || actor->siege_wnd != 0 || clientinfo_is_admin_aid(actor->aid) || actor_hidden(actor))
        return;
    wnd = (struct UIWindow *)ui_emblem_wnd_new();
    ui_window_send(wnd, NULL, 0x22, actor->aid, guild_id, 0, 0);
    ui_window_mgr_add_to_window_list(&g_windowMgr, wnd);
    actor->siege_wnd = wnd;
    list_push_back(&self->listed, &wnd, sizeof wnd);
}

void game_mode_make_word_display_wnd(struct GameMode *self, struct GameActor *actor)
{
    struct UIWindow *wnd;

    (void)self;
    if (actor->word_display_wnd != 0 || clientinfo_is_admin_aid(actor->aid) || actor_hidden(actor))
        return;
    wnd = (struct UIWindow *)ui_word_display_wnd_new();
    ui_window_send(wnd, NULL, 0x22, actor->aid, 0, 0, 0);
    ui_window_mgr_add_window(&g_windowMgr, wnd);
    actor->word_display_wnd = wnd;
}

void *game_mode_make_emotion_effect(struct GameMode *self, const float *pos, int emotion, unsigned aid)
{
    struct String names[2];
    int action = 0;
    void *mem, *effect;

    if (client_lua_call(g_session1.luaState, "GetEmotionAction", "d>d", emotion, &action) && action == -1)
        action = 0;
    string_init(&names[0]);
    string_init(&names[1]);
    string_assign(&names[0], "이팩트\\emotion.spr", 0x12);
    string_assign(&names[1], "이팩트\\emotion.act", 0x12);
    mem = mem_alloc(sizeof(struct MsgEffect));
    effect = mem != 0 ? msg_effect_construct(mem) : 0;
    list_push_back(&self->world->gameObjects, &effect, sizeof effect);
    render_object_send(effect, NULL, 0x16, (long long)aid, 1, 0, 0);
    render_object_send(effect, NULL, 0xe, (intptr_t)pos, (intptr_t)names, (long long)action, 0);
    string_free(&names[1]);
    string_free(&names[0]);
    return effect;
}

enum {
    MSG_EFFECT_START = 0xe,
    MSG_EFFECT_SET_STYLE = 0x16,
};
#define MSG_EFFECT_STYLE_MISS 0xd

static struct MsgEffect *msg_effect_new(struct GameMode *mode)
{
    void *mem = mem_alloc(sizeof(struct MsgEffect));
    struct MsgEffect *effect = mem != 0 ? msg_effect_construct(mem) : 0;

    if (effect != 0)
        list_push_back(&mode->world->gameObjects, &effect, sizeof effect);
    return effect;
}

void *game_mode_make_miss_effect(struct GameMode *self, const float *pos, unsigned argb, unsigned aid)
{
    struct String names[2];
    struct MsgEffect *effect;

    string_init(&names[0]);
    string_init(&names[1]);
    string_assign(&names[0], "이팩트\\msg.spr", 0xe);
    string_assign(&names[1], "이팩트\\msg.act", 0xe);
    effect = msg_effect_new(self);
    if (effect != 0) {
        msg_effect_send_msg((struct GameObject *)effect, NULL, MSG_EFFECT_SET_STYLE, (long long)aid,
            MSG_EFFECT_STYLE_MISS, 0, 0);
        msg_effect_send_msg((struct GameObject *)effect, NULL, MSG_EFFECT_START, (intptr_t)pos,
            (intptr_t)names, 0, 0);
        effect->base.argb = argb;
        effect->base.cell_lit = 0;
    }
    string_free(&names[1]);
    string_free(&names[0]);
    return effect;
}

#define FX_DIR      "이팩트\\"
#define MONSTER_DIR "몬스터\\"
#define CART        "손수레"
#define SUNO_CART   "슈노" CART
#define BABY        "베이비"
#define WARG        "워그"
#define FALCON      "매"
#define ICE         "얼음땡"
#define GIFT_CART   "선물상자카트"
#define PORING_LOADED_CART "포링싣은카트"
#define PORING_CART "포링카트"
#define MADO_CART   "마도카트"
#define MINI        "_미니"
#define EMOTION     FX_DIR "emotion"
#define MSG         FX_DIR "msg"

#define SPR(path) path ".spr", sizeof(path ".spr") - 1
#define ACT(path) path ".act", sizeof(path ".act") - 1

struct MsgEffectKind {
    const char *spr;
    unsigned spr_len;
    const char *act;
    unsigned act_len;
    int action;
    int style;
};

static const struct MsgEffectKind g_msgEffectKinds[0x47] = {
      { SPR(FX_DIR "mvp"), ACT(FX_DIR "mvp"), 0, 2 },
      { SPR(FX_DIR CART), ACT(FX_DIR CART), 0, 3 },
      { SPR(EMOTION), ACT(EMOTION), 8, 4 },
      { SPR(FX_DIR ICE), ACT(FX_DIR ICE), 0, 5 },
      { SPR(FX_DIR ICE), ACT(FX_DIR ICE), 1, 6 },
      { SPR(FX_DIR "smoke"), ACT(FX_DIR "smoke"), 0, 6 },
      { SPR(FX_DIR "Status-curse"), ACT(FX_DIR "Status-curse"), 0, 7 },
      { SPR(FX_DIR "Status-sleep"), ACT(FX_DIR "Status-sleep"), 0, 8 },
      { SPR(FX_DIR "Status-stun"), ACT(FX_DIR "Status-stun"), 0, 9 },
      { SPR(EMOTION), ACT(EMOTION), 0xa, 0xa },
      { SPR(MSG), ACT(MSG), 1, 0xb },
      { SPR(MSG), ACT(MSG), 2, 0xc },
      { SPR(MSG), ACT(MSG), 2, 0xf },
      { SPR(MSG), ACT(MSG), 3, 0x11 },
      { SPR(FX_DIR FALCON), ACT(FX_DIR FALCON), 0, 0x12 },
      { SPR(MSG), ACT(MSG), 0, 0 },
      { SPR(FX_DIR CART "1"), ACT(FX_DIR CART "1"), 0, 3 },
      { SPR(FX_DIR CART "2"), ACT(FX_DIR CART "2"), 0, 3 },
      { SPR(FX_DIR CART "3"), ACT(FX_DIR CART "3"), 0, 3 },
      { SPR(FX_DIR CART "4"), ACT(FX_DIR CART "4"), 0, 3 },
      { SPR(MSG), ACT(MSG), 1, 0x18 },
      { SPR(FX_DIR SUNO_CART), ACT(FX_DIR SUNO_CART), 0, 3 },
      { SPR(FX_DIR "bluemsg"), ACT(MSG), 2, 0x19 },
      { SPR(FX_DIR FALCON "2"), ACT(FX_DIR FALCON "2"), 0, 0x12 },
      { SPR(FX_DIR FALCON), ACT(FX_DIR BABY FALCON), 0, 0x12 },
      { SPR(FX_DIR CART), ACT(FX_DIR BABY CART), 0, 3 },
      { SPR(FX_DIR CART "1"), ACT(FX_DIR BABY CART "1"), 0, 3 },
      { SPR(FX_DIR CART "2"), ACT(FX_DIR BABY CART "2"), 0, 3 },
      { SPR(FX_DIR CART "3"), ACT(FX_DIR BABY CART "3"), 0, 3 },
      { SPR(FX_DIR CART "4"), ACT(FX_DIR BABY CART "4"), 0, 3 },
      { SPR(FX_DIR SUNO_CART), ACT(FX_DIR BABY SUNO_CART), 0, 3 },
      { SPR("cursors"), ACT("cursors"), 0xa, 0x1a },
      { SPR(MONSTER_DIR "demon_pungus"), ACT(MONSTER_DIR "demon_pungus"), 0, 0x12 },
      { SPR(MONSTER_DIR "explosion"), ACT(MONSTER_DIR "explosion"), 0, 0x12 },
      { SPR(MONSTER_DIR "lude"), ACT(MONSTER_DIR "lude"), 0, 0x12 },
      { SPR(MONSTER_DIR WARG), ACT(MONSTER_DIR WARG), 0, 0x1b },
      { SPR(EMOTION), ACT(EMOTION), 0x50, 0x1c },
      { SPR(EMOTION), ACT(EMOTION), 0x51, 0x1c },
      { SPR(EMOTION), ACT(EMOTION), 0x52, 0x1d },
      { SPR(EMOTION), ACT(EMOTION), 0x53, 0x1d },
      { SPR(EMOTION), ACT(EMOTION), 0x54, 0x1e },
      { SPR(EMOTION), ACT(EMOTION), 0x55, 0x1e },
      { SPR(MONSTER_DIR WARG), ACT(MONSTER_DIR BABY WARG), 0, 0x1f },
      { SPR(FX_DIR "shield"), ACT(FX_DIR "shield"), 0, 0x20 },
      { SPR(FX_DIR "costumebody"), ACT(FX_DIR "costumebody"), 0, 0x21 },
      { SPR(FX_DIR CART "5"), ACT(FX_DIR CART "5"), 0, 3 },
      { SPR(FX_DIR CART "5" BABY), ACT(FX_DIR CART "5" BABY), 0, 3 },
      { SPR(FX_DIR CART "6"), ACT(FX_DIR CART "6"), 0, 3 },
      { SPR(FX_DIR CART "6" BABY), ACT(FX_DIR CART "6" BABY), 0, 3 },
      { SPR(FX_DIR CART "7"), ACT(FX_DIR CART "7"), 0, 3 },
      { SPR(FX_DIR CART "7" BABY), ACT(FX_DIR CART "7" BABY), 0, 3 },
      { SPR(FX_DIR CART "8"), ACT(FX_DIR CART "8"), 0, 3 },
      { SPR(FX_DIR CART "8" BABY), ACT(FX_DIR CART "8" BABY), 0, 3 },
      { SPR(FX_DIR "OWL"), ACT(FX_DIR "OWL"), 0, 0x12 },
      { SPR(FX_DIR GIFT_CART), ACT(FX_DIR GIFT_CART), 0, 3 },
      { SPR(FX_DIR GIFT_CART MINI), ACT(FX_DIR GIFT_CART MINI), 0, 3 },
      { SPR(FX_DIR PORING_LOADED_CART), ACT(FX_DIR PORING_LOADED_CART), 0, 3 },
      { SPR(FX_DIR PORING_LOADED_CART MINI), ACT(FX_DIR PORING_LOADED_CART MINI), 0, 3 },
      { SPR(FX_DIR PORING_CART), ACT(FX_DIR PORING_CART), 0, 3 },
      { SPR(FX_DIR PORING_CART MINI), ACT(FX_DIR PORING_CART MINI), 0, 3 },
      { SPR(EMOTION), ACT(EMOTION), 0x5d, 0x22 },
      { SPR(EMOTION), ACT(EMOTION), 0x5e, 0x23 },
      { SPR(EMOTION), ACT(EMOTION), 0x5f, 0x1e },
      { SPR(EMOTION), ACT(EMOTION), 0x60, 0x1d },
      { SPR(EMOTION), ACT(EMOTION), 0x61, 0x24 },
      { SPR(FX_DIR MADO_CART), ACT(FX_DIR MADO_CART), 0, 3 },
      { SPR(FX_DIR MADO_CART MINI), ACT(FX_DIR MADO_CART MINI), 0, 3 },
      { SPR(EMOTION), ACT(EMOTION), 5, 0x25 },
      { SPR(EMOTION), ACT(EMOTION), 0x1c, 0x26 },
      { SPR(FX_DIR "WINDHAWK_HAWK"), ACT(FX_DIR "WINDHAWK_HAWK"), 0, 0x12 },
      { SPR(FX_DIR "WINDHAWK_WOLF"), ACT(FX_DIR "WINDHAWK_WOLF"), 0, 0x1b },
};
#define MSG_EFFECT_KIND_DEFAULT 0xf

void *game_mode_make_msg_effect(struct GameMode *self, unsigned kind, const struct Vec3 *pos, unsigned aid,
                                        unsigned char drawn)
{
    const struct MsgEffectKind *k = &g_msgEffectKinds[kind <= 0x46 ? kind : MSG_EFFECT_KIND_DEFAULT];
    struct String names[2];
    struct MsgEffect *effect;

    string_init(&names[0]);
    string_init(&names[1]);
    string_assign(&names[0], k->spr, k->spr_len);
    string_assign(&names[1], k->act, k->act_len);
    effect = msg_effect_new(self);
    if (effect != 0) {
        msg_effect_send_msg((struct GameObject *)effect, NULL, MSG_EFFECT_SET_STYLE, (long long)aid, k->style, 0, 0);
        effect->drawn = drawn;
        msg_effect_send_msg((struct GameObject *)effect, NULL, MSG_EFFECT_START, (intptr_t)pos,
            (intptr_t)names, k->action, 0);
    }
    string_free(&names[1]);
    string_free(&names[0]);
    return effect;
}

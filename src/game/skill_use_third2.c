#include "session/state.h"
#include "game/skill_use.h"

enum {
    WAVE_FIRST = 1,
    EFFECT_FIRST = 0,
};

#define W(name) "effect\\" name ".wav"

static const struct SimpleLook third2_looks[] = {
    { 0x8db, 0x0b7, ON_CASTER }, { 0x8dc, 0x30b, ON_CASTER }, { 0x8e3, 0x311, ON_TARGET },
    { 0x915, 0x38e, ON_CASTER }, { 0x97d, 0x34a, ON_CASTER }, { 0x99d, 0x3a6, ON_CASTER },
    { 0x9be, 0x38f, ON_CASTER }, { 0x9bf, 0x38f, ON_CASTER }, { 0x9c0, 0x38f, ON_CASTER },
    { 0x9c1, 0x38f, ON_CASTER },
};

static const struct WaveLook third2_wave_looks[] = {
    { 0x907, 0x376, ON_CASTER, EFFECT_FIRST, W("LG_REFLECTDAMAGE") },
    { 0x909, 0x376, ON_CASTER, EFFECT_FIRST, W("LG_REFLECTDAMAGE") },
    { 0x90b, 0x376, ON_CASTER, EFFECT_FIRST, W("LG_SHIELDSPELL") },
    { 0x90e, 0x38c, ON_CASTER, EFFECT_FIRST, W("LG_PRESTIGE") },
    { 0x91d, 0x376, ON_CASTER, EFFECT_FIRST, W("SR_CRESCENTELBOW") },
    { 0x91f, 0x376, ON_CASTER, EFFECT_FIRST, W("SR_LIGHTNINGWALK") },
    { 0x922, 0x376, ON_CASTER, EFFECT_FIRST, W("SR_RAISINGDRAGON") },
    { 0x92b, 0x376, ON_CASTER, EFFECT_FIRST, W("샤프슈팅") },
    { 0x92c, 0x376, ON_CASTER, EFFECT_FIRST, W("mon_탄지신통") },
    { 0x8d8, 0x30a, ON_CASTER, WAVE_FIRST, W("F_SIDESLIDE") },
    { 0x8d9, 0x30a, ON_CASTER, WAVE_FIRST, W("B_SIDESLIDE") },
    { 0x8dd, 0x21f, ON_CASTER, WAVE_FIRST, W("ef_decagility") },
    { 0x8e1, 0x30e, ON_CASTER, WAVE_FIRST, W("mon_금강불괴") },
    { 0x8e2, 0x1f4, ON_CASTER, WAVE_FIRST, W("t_등록") },
    { 0x8f1, 0x32b, ON_CASTER, WAVE_FIRST, W("bodypaint") },
    { 0x8f3, 0x32c, ON_CASTER, WAVE_FIRST, W("deadlyinfect") },
    { 0x8f4, 0x32d, ON_TARGET, WAVE_FIRST, W("enervation") },
    { 0x8f5, 0x32e, ON_TARGET, WAVE_FIRST, W("groomy") },
    { 0x8f6, 0x32f, ON_TARGET, WAVE_FIRST, W("ignorance") },
    { 0x8f7, 0x330, ON_TARGET, WAVE_FIRST, W("laziness") },
    { 0x8f8, 0x331, ON_TARGET, WAVE_FIRST, W("unlucky") },
    { 0x8f9, 0x332, ON_TARGET, WAVE_FIRST, W("weakness") },
    { 0x92e, 0x34f, ON_TARGET, EFFECT_FIRST, W("스윙댄스") },
    { 0x92f, 0x34f, ON_TARGET, EFFECT_FIRST,
      W("연인들을위한심포니") },
    { 0x930, 0x34f, ON_TARGET, EFFECT_FIRST, W("AB_judex") },
    { 0x94d, 0x350, ON_TARGET, EFFECT_FIRST,
      W("풍차를향해돌격") },
    { 0x94e, 0x350, ON_TARGET, EFFECT_FIRST, W("메아리의노래") },
    { 0x977, 0x35d, ON_TARGET, EFFECT_FIRST,
      W("순환하는자연의소리") },
    { 0x978, 0x35e, ON_TARGET, EFFECT_FIRST, W("AB_ancilla") },
    { 0x979, 0x360, ON_TARGET, EFFECT_FIRST, W("AB_renovatio") },
    { 0x97b, 0x364, ON_TARGET, EFFECT_FIRST, W("마나의노래") },
    { 0x97c, 0x362, ON_TARGET, EFFECT_FIRST,
      W("워그와함께춤을") },
    { 0x97e, 0x366, ON_TARGET, EFFECT_FIRST,
      W("새터데이나이트피버") },
    { 0x97f, 0x368, ON_TARGET, EFFECT_FIRST, W("SR_LIGHTNINGWALK") },
    { 0x980, 0x36a, ON_TARGET, EFFECT_FIRST,
      W("멜로디오브싱크") },
    { 0x981, 0x36c, ON_TARGET, EFFECT_FIRST,
      W("비욘드오브워크라이") },
    { 0x982, 0x36e, ON_TARGET, EFFECT_FIRST,
      W("언리미티드허밍보이스") },
};

static const char WAVE_HOVERING[] = "effect\\HOVERING.wav";
static const char WAVE_REPRODUCE[] = "effect\\reproduce.wav";
static const char WAVE_AUTO_SHADOW[] = "effect\\autoshadow.wav";
static const char WAVE_SHADOW_FORM[] = "effect\\shadowform.wav";
static const char WAVE_INVISIBILITY[] = "effect\\invisibility.wav";
static const char WAVE_CURSED_CIRCLE[] = "effect\\SR_CURSEDCIRCLE.wav";
static const char WAVE_WINDMILL[] = "effect\\SR_WINDMILL.wav";
static const char WAVE_POWER_VELOCITY[] = "effect\\SR_POWERVELOCITY.wav";
static const char WAVE_CRESCENT_ELBOW[] = "effect\\SR_CRESCENTELBOW.wav";
static const char WAVE_HEAL_EFFECT[] = "_heal_effect.wav";
static const char WAVE_SUFFRAGIUM[] = "effect\\priest_suffragium.wav";
static const char WAVE_HOWLING_OF_LION[] = "effect\\SR_HOWLINGOFLION.wav";

enum {
    SKILL_HOVERING = 0x8d7,
    SKILL_REPRODUCE = 0x8ed,
    SKILL_AUTO_SHADOW = 0x8ee,
    SKILL_SHADOW_FORM = 0x8ef,
    SKILL_INVISIBILITY = 0x8f2,
    SKILL_SC_STRIPACCESSARY = 0x8fa,
    SKILL_LG_EARTHDRIVE = 0x913,
    SKILL_CURSED_CIRCLE = 0x91e,
    SKILL_WINDMILL = 0x921,
    SKILL_POWER_VELOCITY = 0x925,
    SKILL_SR_CRESCENTELBOW_AUTOSPELL = 0x926,
    SKILL_SR_GENTLETOUCH_CURE = 0x929,
    SKILL_SR_GENTLETOUCH_ENERGYGAIN = 0x92a,
    SKILL_HARMONIZE = 0x94f,
    SKILL_SIREN = 0x974,
    SKILL_DEATH_VALLEY = 0x975,
    SKILL_LULLABY = 0x976,
    SKILL_SPELL_FIST = 0x98d,
    SKILL_STRIKING = 0x993,
    SKILL_SO_ARRULLO = 0x997,
    SKILL_GN_CART_TORNADO = 0x9ac,
    SKILL_GN_CARTCANNON = 0x9ad,
    SKILL_CART_BOOST = 0x9ae,
    SKILL_GN_BLOOD_SUCKER = 0x9b0,
    SKILL_GN_SPORE_EXPLOSION = 0x9b1,
    SKILL_GN_WALLOFTHORN = 0x9b2,
    SKILL_HOWLING_MANDRAGORA = 0x9bc,
    SKILL_GN_SLINGITEM = 0x9bd,
    SKILL_AB_SECRAMENT = 0x9d3,
    SKILL_HOWLING_OF_LION = 0x9d5,
};

static void spawn_pair_on(struct GameActor *actor, int effect_1, int effect_2)
{
    Vec3 origin = { 0.0f, 0.0f, 0.0f };

    spawn_effect(actor->aid, effect_1, origin);
    spawn_effect(actor->aid, effect_2, origin);
}

static int effect_from_placed_at(struct GameActor *from, int effect_id, struct GameActor *to)
{
    struct RagEffect *effect = actor_launch_effect(from, effect_id);
    Vec3 at;

    if (!effect)
        return 0;
    at = actor_pos(to);
    effect_set_pos(effect, &at);
    return 1;
}

static void shadow_form_look(const struct SkillCast *cast, struct GameActor *caster)
{
    Vec3 ids;
    struct RagEffect *effect;

    if (caster) {
        play_skill_wave(WAVE_SHADOW_FORM);
        actor_launch_effect(caster, 0x33a);
    }
    ids.z = 0.0f;
    ids.x = (float)(unsigned)cast->src_aid;
    ids.y = (float)(unsigned)cast->target_aid;
    if (ids.y == 0.0f || ids.x == 0.0f || !caster)
        return;
    effect = actor_launch_effect(caster, 0x333);
    if (effect)
        effect_set_pos(effect, &ids);
}

static void scaled_pair_look(const struct SkillCast *cast, struct GameActor *caster)
{
    Vec3 at = actor_pos(caster);
    struct Effect *effect;
    float scale = 1.0f;

    switch (cast->effect_level) {
    case 1: case 2:
        scale = 0.5f;
        break;
    case 3: case 4:
        scale = 0.7f;
        break;
    case 5:
        scale = 0.9f;
        break;
    }
    effect = spawn_effect(0, 0x6aa, actor_pos(caster));
    if (effect)
        spawned_effect_zoom(effect, scale);
    effect = spawn_effect(0, 0x6ab, at);
    if (effect)
        spawned_effect_zoom(effect, scale);
}

static void harmonize_look(struct GameActor *caster, struct GameActor *target)
{
    Vec3 ids;
    struct RagEffect *effect;

    ids.x = 0.0f;
    ids.z = 0.0f;
    ids.y = (float)(unsigned)target->aid;
    effect = actor_launch_effect(target, 0x351);
    if (effect)
        effect_set_pos(effect, &ids);
    ids.y = (float)(unsigned)caster->aid;
    effect = actor_launch_effect(caster, 0x351);
    if (effect)
        effect_set_pos(effect, &ids);
    play_skill_wave("effect\\하모나이즈.wav");
}

static void poison_buster_look(struct GameActor *target)
{
    Vec3 ids;

    actor_launch_effect(target, 0x376);
    actor_launch_effect(target, 0x1b9);
    play_skill_wave("effect\\S포이즌버스터.wav");
    ids = aid_as_pos(target);
    effect_place(actor_launch_effect(target, 0x3a3), ids);
}

static void suffragium_look(const struct SkillCast *cast, struct GameActor *caster, struct GameActor *target)
{
    if (caster)
        play_skill_wave_at(WAVE_SUFFRAGIUM, caster->pos.x - target->pos.x, 0.0f, caster->pos.z - target->pos.z);
    effect_mgr_spawn(g_effectMgr, target->aid, g_session1.map_rule_flags[0] == 1 ? 0x5ad : 0x507, actor_pos(target), target->aid,
                    cast->effect_level, 0, 0);
}

int skill_use_third2(const struct SkillCast *cast)
{
    struct GameActor *caster = cast->caster, *target = cast->target;

    if (play_simple_look(cast, third2_looks, SIMPLE_LOOK_COUNT(third2_looks)))
        return 1;
    if (play_wave_look(cast, third2_wave_looks, (int)(sizeof(third2_wave_looks) / sizeof(third2_wave_looks[0]))))
        return 1;

    switch (cast->skill_id) {
    case 0x900:
        return 1;

    case SKILL_HOVERING:
        if (!caster)
            return 1;
        play_skill_wave(WAVE_HOVERING);
        actor_launch_effect(caster, 0x21f);
        actor_launch_effect(caster, 0x30a);
        return 1;

    case SKILL_REPRODUCE:
        if (caster)
            spawn_pair_on(caster, 0x631, 0x632);
        play_skill_wave(WAVE_REPRODUCE);
        return 1;

    case SKILL_AUTO_SHADOW:
        if (caster)
            spawn_pair_on(caster, 0x6c2, 0x6c3);
        play_skill_wave(WAVE_AUTO_SHADOW);
        return 1;

    case SKILL_SHADOW_FORM:
        shadow_form_look(cast, caster);
        return 1;

    case SKILL_INVISIBILITY:
        play_skill_wave(WAVE_INVISIBILITY);
        return 1;

    case SKILL_SC_STRIPACCESSARY:
        if (target)
            effect_from_placed_at(target, 0x334, target);
        return 1;

    case SKILL_LG_EARTHDRIVE:
        if (caster)
            scaled_pair_look(cast, caster);
        return 1;

    case SKILL_CURSED_CIRCLE:
        if (!caster)
            return 1;
        actor_launch_effect(caster, 0x376);
        render_object_play_motion_range((struct RenderObject *)caster, 0x60, 3, 3, 0x60, 3, 1, 3000, 0);
        play_skill_wave(WAVE_CURSED_CIRCLE);
        return 1;

    case SKILL_WINDMILL:
        if (!caster)
            return 1;
        actor_launch_effect(caster, 0x1b3);
        actor_launch_effect(caster, 0x376);
        play_skill_wave(WAVE_WINDMILL);
        return 1;

    case SKILL_POWER_VELOCITY:
    case SKILL_SR_GENTLETOUCH_CURE:
    case SKILL_SR_GENTLETOUCH_ENERGYGAIN:
        if (!caster || !target)
            return 1;
        actor_launch_effect(caster, 0x376);
        actor_launch_effect(target, 0x376);
        play_skill_wave(cast->skill_id == SKILL_POWER_VELOCITY ? WAVE_POWER_VELOCITY
                        : cast->skill_id == SKILL_SR_GENTLETOUCH_CURE          ? WAVE_HEAL_EFFECT
                                                               : "effect\\세크리파이스.wav");
        return 1;

    case SKILL_SR_CRESCENTELBOW_AUTOSPELL:
        if (!caster || !target)
            return 1;
        actor_launch_effect(caster, 0x376);
        if (effect_from_placed_at(caster, 0x37c, target))
            play_skill_wave(WAVE_CRESCENT_ELBOW);
        return 1;

    case SKILL_HARMONIZE:
        if (target && caster)
            harmonize_look(caster, target);
        return 1;

    case SKILL_SIREN:
        if (target)
            play_skill_wave("effect\\세이렌의목소리.wav");
        return 1;

    case SKILL_DEATH_VALLEY:
        if (!target)
            return 1;
        actor_launch_effect(target, 0x35d);
        actor_launch_effect(target, 0x355);
        play_skill_wave("effect\\사망의골짜기에서.wav");
        return 1;

    case SKILL_LULLABY:
        actor_launch_effect(target, 0x3c9);
        play_skill_wave("effect\\안식의자장가.wav");
        return 1;

    case SKILL_SPELL_FIST:
        if (!caster)
            return 1;
        play_skill_wave("effect\\S스펠피스트.wav");
        play_skill_wave("effect\\S파이어워크.wav");
        play_skill_wave("effect\\S일렉트릭워크.wav");
        return 1;

    case SKILL_STRIKING:
        if (!caster)
            return 1;
        play_skill_wave("effect\\S스트라이킹.wav");
        actor_launch_effect(caster, 0x376);
        actor_launch_effect(caster, 0x1b9);
        return 1;

    case SKILL_SO_ARRULLO:
        if (target)
            poison_buster_look(target);
        return 1;

    case SKILL_GN_CART_TORNADO:
        if (!caster)
            return 1;
        actor_launch_effect(caster, 0x555);
        if (g_session1.map_rule_flags[0] == 1)
            spawn_pair_on(caster, 0x5dd, 0x5de);
        else
            spawn_pair_on(caster, 0x555, 0x556);
        return 1;

    case SKILL_GN_CARTCANNON: {
        struct Effect *effect;

        if (!caster || !target)
            return 1;
        effect = effect_mgr_spawn(g_effectMgr, target->aid, 0x54d, actor_pos(caster), target->aid, -1, 0, 0);
        if (effect)
            spawned_effect_delay(effect, 200);
        play_skill_wave("effect\\mon_침투경.wav");
        return 1;
    }

    case SKILL_CART_BOOST:
        if (!caster)
            return 1;
        play_skill_wave("effect\\G카트부스트.wav");
        spawn_pair_on(caster, 0x68b, 0x68c);
        return 1;

    case SKILL_GN_BLOOD_SUCKER:
        if (!caster || !target)
            return 1;
        play_skill_wave("effect\\S바큠익스트림.wav");
        effect_from_placed_at(target, 0xd8, caster);
        return 1;

    case SKILL_GN_SPORE_EXPLOSION:
        game_mode_launch_effect_at(cast->mode, 0x392, caster, target, 4);
        return 1;

    case SKILL_GN_WALLOFTHORN:
        play_skill_wave("effect\\G가시나무덫.wav");
        return 1;

    case SKILL_HOWLING_MANDRAGORA:
        if (!caster)
            return 1;
        spawn_pair_on(caster, 0x667, 0x668);
        send_quake();
        play_skill_wave("effect\\G하울링오브만드라고라.wav");
        return 1;

    case SKILL_GN_SLINGITEM:
        if (!caster || !target)
            return 1;
        actor_launch_effect(caster, 0x376);
        if (effect_from_placed_at(caster, 0x1b7, target))
            play_skill_wave("effect\\G스포어익스플로젼.wav");
        return 1;

    case SKILL_AB_SECRAMENT:
        if (target)
            suffragium_look(cast, caster, target);
        return 1;

    case SKILL_HOWLING_OF_LION:
        if (!caster)
            return 1;
        spawn_pair_on(caster, 0x628, 0x629);
        play_skill_wave(WAVE_HOWLING_OF_LION);
        return 1;
    }
    return 0;
}

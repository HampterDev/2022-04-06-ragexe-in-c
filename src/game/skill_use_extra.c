#include "game/skill_use.h"

static const char WAVE_2008CAT[] = "effect\\2008cat.wav";
static const char WAVE_DREAM_DEVICE[] = "dream_device_edq.wav";
static const char WAVE_WE_WISH[] = "effect\\wewish.wav";
static const char WAVE_CURSED_CIRCLE[] = "effect\\SR_CURSEDCIRCLE.wav";
static const char WAVE_WL_JACK_FROST[] = "effect\\wl_jackfrost.wav";
static const char WAVE_AMON_RA[] = "amon_ra_die01.wav";
static const char WAVE_VALENTINE[] = "effect\\vallentine.wav";
static const char WAVE_ENTRY[] = "effect\\ef_entry.wav";

static const struct SimpleLook extra_looks[] = {
    { 0x242, 0x0dc, ON_CASTER }, { 0x2b1, 0x02a, ON_TARGET }, { 0x2b2, 0x025, ON_TARGET },
    { 0x2b5, 0x2bb, ON_TARGET }, { 0x2b6, 0x05b, ON_TARGET }, { 0x2cc, 0x408, ON_CASTER },
    { 0x2cd, 0x408, ON_CASTER }, { 0x2ce, 0x409, ON_CASTER }, { 0x2cf, 0x019, ON_CASTER },
    { 0x2d1, 0x41d, ON_CASTER }, { 0x2d4, 0x0b7, ON_CASTER }, { 0x2d7, 0x055, ON_TARGET },
    { 0x2ea, 0x0eb, ON_TARGET }, { 0x303, 0x640, ON_TARGET }, { 0x3ea, 0x257, ON_CASTER },
    { 0x3ed, 0x25a, ON_TARGET }, { 0x3f0, 0x100, ON_TARGET }, { 0x3f9, 0x102, ON_TARGET },
    { 0x3fa, 0x0ff, ON_TARGET }, { 0x3fb, 0x101, ON_TARGET },
};

enum {
    SKILL_CASH_ASSUMPTIO = 0x2b3,
    SKILL_ALL_CATCRY = 0x2b4,
    SKILL_ALL_DREAM_SUMMERNIGHT = 0x2b7,
    SKILL_WE_WISH = 0x2ba,
    SKILL_NPC_MILLENNIUMSHIELD = 0x2c3,
    SKILL_NPC_JACKFROST = 0x2d0,
    SKILL_NPC_WIDESUCK = 0x2d2,
    SKILL_NPC_CHEAL = 0x2d9,
    SKILL_NPC_SR_CURSEDCIRCLE = 0x2da,
    SKILL_NPC_MANDRAGORA = 0x2df,
    SKILL_NPC_VENOMIMPRESS = 0x2e2,
    SKILL_NPC_IGNITIONBREAK = 0x2e4,
    SKILL_NPC_PHANTOMTHRUST = 0x2e5,
    SKILL_NPC_HALLUCINATIONWALK = 0x2e7,
    SKILL_NPC_LEASH = 0x2eb,
    SKILL_NPC_WIDELEASH = 0x2ec,
    SKILL_BA_PANGVOICE = 0x3f2,
    SKILL_DC_WINKCHARM = 0x3f3,
    SKILL_BS_GREED = 0x3f5,
    SKILL_RK_ENCHANTBLADE = 0x7d1,
    ACTOR_MSG_EMOTION = 0x15,
};

static void caster_effect_at_target(struct GameActor *caster, struct GameActor *target, int effect_id)
{
    struct RagEffect *effect = actor_launch_effect(caster, effect_id);
    Vec3 at;

    if (!effect)
        return;
    at = actor_pos(target);
    effect_set_pos(effect, &at);
}

int skill_use_extra(const struct SkillCast *cast)
{
    struct GameActor *caster = cast->caster, *target = cast->target;

    if (play_simple_look(cast, extra_looks, SIMPLE_LOOK_COUNT(extra_looks)))
        return 1;

    switch (cast->skill_id) {
    case 0x259:
        return 1;

    case SKILL_CASH_ASSUMPTIO:
        if (!target)
            return 1;
        actor_launch_effect(target, 0x177);
        actor_launch_effect(target, 0x1b8);
        return 1;

    case SKILL_ALL_CATCRY:
        if (caster)
            play_skill_wave(WAVE_2008CAT);
        return 1;

    case SKILL_ALL_DREAM_SUMMERNIGHT:
        if (caster)
            play_skill_wave(WAVE_DREAM_DEVICE);
        return 1;

    case SKILL_WE_WISH:
        if (!caster)
            return 1;
        play_skill_wave(WAVE_WE_WISH);
        actor_launch_effect(caster, 0x2cd);
        return 1;

    case SKILL_NPC_MILLENNIUMSHIELD:
        skill_use_soul_strike_look(cast);
        return 1;

    case SKILL_NPC_JACKFROST:
        skill_use_jack_frost_ring_look(cast);
        return 1;

    case SKILL_NPC_WIDESUCK: {
        struct RagEffect *effect;
        Vec3 at;

        if (!caster || !target)
            return 1;
        effect = actor_launch_effect(target, 0x41c);
        if (!effect)
            return 1;
        at = actor_pos(caster);
        effect_set_pos(effect, &at);
        return 1;
    }

    case SKILL_NPC_CHEAL:
        skill_use_heal(cast);
        return 1;

    case SKILL_NPC_SR_CURSEDCIRCLE:
        if (!caster)
            return 1;
        actor_launch_effect(caster, 0x376);
        play_skill_wave(WAVE_CURSED_CIRCLE);
        return 1;

    case SKILL_NPC_MANDRAGORA:
        if (!caster)
            return 1;
        actor_launch_effect(caster, 0x376);
        send_quake();
        play_skill_wave("effect\\G하울링오브만드라고라.wav");
        return 1;

    case SKILL_NPC_VENOMIMPRESS:
        skill_use_water_look(cast);
        return 1;

    case SKILL_NPC_IGNITIONBREAK:
        play_skill_wave(WAVE_WL_JACK_FROST);
        actor_launch_effect(caster, 0x2d2);
        return 1;

    case SKILL_NPC_PHANTOMTHRUST:
        skill_use_glow_both_look(cast);
        return 1;

    case SKILL_NPC_HALLUCINATIONWALK:
        skill_use_hallucination_walk_look(cast);
        return 1;

    case SKILL_NPC_LEASH:
    case SKILL_NPC_WIDELEASH:
        if (target)
            spawn_effect(target->aid, 0x459, actor_pos(target));
        if (caster)
            spawn_effect(caster->aid, 0x459, actor_pos(caster));
        return 1;

    case SKILL_BA_PANGVOICE:
        if (!caster || !target)
            return 1;
        render_object_play_motion_range((struct RenderObject *)caster, 0x60, 4, 5, 0x60, 4, 1, 1800, 1);
        play_skill_wave(WAVE_AMON_RA);
        caster_effect_at_target(caster, target, 0x25e);
        return 1;

    case SKILL_DC_WINKCHARM:
        if (!caster || !target)
            return 1;
        render_object_send(caster, NULL, ACTOR_MSG_EMOTION, 0x1e, 0, 0, 0);
        play_skill_wave(WAVE_VALENTINE);
        caster_effect_at_target(caster, target, 0x25f);
        return 1;

    case SKILL_BS_GREED:
        if (caster)
            play_skill_wave(WAVE_ENTRY);
        return 1;

    case SKILL_RK_ENCHANTBLADE:
        skill_use_ninja_throw_look(cast);
        return 1;
    }
    return 0;
}

#include "session/job_sounds.h"
#include <stddef.h>
#include <stdlib.h>
#include "session/state.h"
#include "session/session_jobs.h"
#include "session/session_lua.h"

void job_hit_sounds_init(struct JobHitSounds *sounds)
{
    sounds->clothes = "player_clothes.wav";
    sounds->wooden = "player_wooden_male.wav";
    sounds->metal = "player_metal.wav";
}

const char *job_hit_sound(const struct JobHitSounds *sounds, unsigned index)
{
    if (index >= JOB_HIT_SOUND_COUNT)
        return NULL;
    switch (index) {
    case 3: case 6: case 11: case 12: case 17: case 19: case 20:
    case 24: case 25: case 54: case 57: case 62: case 63: case 68:
    case 70: case 71: case 76: case 79: case 84: case 85: case 90:
    case 92: case 93: case 96: case 100: case 106: case 109: case 112:
    case 115: case 118: case 119: case 122: case 125: case 126: case 129:
    case 261: case 262: case 272: case 273: case 274: case 275: case 278:
    case 279: case 304: case 307: case 310: case 313: case 314: case 354:
    case 355: case 356:
        return sounds->wooden;
    case 1: case 7: case 14: case 15: case 52: case 58: case 65:
    case 66: case 74: case 80: case 87: case 88: case 97: case 98:
    case 101: case 104: case 110: case 116: case 120: case 123: case 127:
    case 276: case 288: case 289: case 291: case 302: case 305: case 308:
    case 312: case 330: case 331: case 352: case 366:
        return sounds->metal;
    default:
        return sounds->clothes;
    }
}

void session_init_job_hit_wave_name(struct Session *self)
{
    job_hit_sounds_init(&self->jobHitSounds);
}

static const char *weapon_hit_wave(struct Session *self, int weapon)
{
    weapon = session_get_real_weapon_id(weapon);
    if (weapon == -1) {
        switch (rand() % 4) {
        case 0: return "_enemy_hit_normal1.wav";
        case 1: return "_enemy_hit_normal2.wav";
        case 2: return "_enemy_hit_normal3.wav";
        case 3: return "_enemy_hit_normal4.wav";
        }
    }
    if (self->weaponHitWaves.first == self->weaponHitWaves.last
        || VEC_COUNT(&self->weaponHitWaves, struct String) <= (unsigned)weapon)
        return NULL;
    if (session_get_drawn_job(self) == 0xfce || session_get_drawn_job(self) == 0x1081)
        return string_cstr(&VEC_AT(&self->weaponHitWaves, struct String, (size_t)weapon));
    if (weapon == 0) {
        switch (rand() % 4) {
        case 0: return "_hit_fist1.wav";
        case 1: return "_hit_fist2.wav";
        case 2: return "_hit_fist3.wav";
        case 3: return "_hit_fist4.wav";
        }
    }
    return string_cstr(&VEC_AT(&self->weaponHitWaves, struct String, (size_t)weapon));
}

const char *session_get_hit_wave_name(struct Session *self, int job)
{
    if (!(job >= 0x1f && job < 0x3e8) && job >= 0 && !(job > 0x3e8 && job < 0xf9f)
        && !((unsigned)(job - 0x4e35) <= 0x2eca))
        return job_hit_sound(&self->jobHitSounds, job > 0xf6e ? (unsigned)(job - 0xf6e) : (unsigned)job);
    return weapon_hit_wave(self, -1);
}

const char *session_get_weapon_hit_wave(struct Session *self, int weapon)
{
    return weapon_hit_wave(self, weapon);
}

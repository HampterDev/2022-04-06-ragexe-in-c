#ifndef SESSION_JOB_SOUNDS_H
#define SESSION_JOB_SOUNDS_H

enum { JOB_HIT_SOUND_COUNT = 28051 };

struct JobHitSounds {
    const char *clothes;
    const char *wooden;
    const char *metal;
};

void job_hit_sounds_init(struct JobHitSounds *sounds);
const char *job_hit_sound(const struct JobHitSounds *sounds, unsigned index);

struct Session;
void session_init_job_hit_wave_name(struct Session *session);
const char *session_get_hit_wave_name(struct Session *session, int job);
const char *session_get_weapon_hit_wave(struct Session *session, int weapon);

#endif

#ifndef CONTENTS_SOUND_H
#define CONTENTS_SOUND_H

#define SOUND_VOLUME_MAX 127

struct SoundSettings {
    int effects_on;
    int bgm_on;
    int bgm_overridden;
};
extern struct SoundSettings g_sound_settings;

int sound_mgr_create(void);
void sound_mgr_destroy(void);
void sound_play_stream(const char *name, int loop_count);
/* NULL or an empty path stops playback and clears the remembered map track. */
void sound_play_bgm(const char *name);
void sound_stop_stream(void);
int sound_stream_playing(void);
void sound_bgm_fade_in(void);
void sound_bgm_fade_out(void);

void sound_play_wave(const char *name, float x, float y, float z,
                     int max_distance, int full_distance, float volume);
void sound_end_samples(void);

int sound_bgm_volume(void);
int sound_bgm_fade_target(void);
int sound_effect_volume(void);
void sound_set_bgm_volume(int volume);
void sound_set_bgm_fade_target(int volume);
void sound_set_effect_volume(int volume);
void sound_set_effect_volume_3d(int volume);
void sound_set_listener_angle(float angle);
void sound_set_room_type(int room_type);

extern int g_sound_speaker_type;

#endif

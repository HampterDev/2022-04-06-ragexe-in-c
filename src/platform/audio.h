#ifndef PLATFORM_AUDIO_H
#define PLATFORM_AUDIO_H

#include <stddef.h>

/* Sound on the default playback device (platform/audio_sdl.c). The device mixes every voice and
   the music itself; nothing has a position. A gain is linear: 0 is silent, 1 the sound as it is. */

struct AudioSound;
struct AudioVoice;
struct AudioMusic;

/* opens the default playback device; with mono the first two channels carry one mono mix */
int audio_open(int mono);
/* closes the device opened by audio_open; stop every voice and the music first */
void audio_close(void);

/* decodes a RIFF WAVE (PCM or ADPCM) or MP3 file held in memory; NULL when it does not decode */
struct AudioSound *audio_sound_decode(const void *data, size_t size);
void audio_sound_free(struct AudioSound *sound);

/* plays a sound once from a copy of its samples */
struct AudioVoice *audio_voice_play(const struct AudioSound *sound, float gain);
int audio_voice_playing(struct AudioVoice *voice);
/* stops the voice and frees it */
void audio_voice_stop(struct AudioVoice *voice);

/* streams an MP3 file loop_count times, 0 for ever */
struct AudioMusic *audio_music_play(const char *path, int loop_count, float gain);
void audio_music_set_gain(struct AudioMusic *music, float gain);
/* whether the music has played its last loop to the end */
int audio_music_done(struct AudioMusic *music);
/* stops the music and frees it */
void audio_music_stop(struct AudioMusic *music);

#endif

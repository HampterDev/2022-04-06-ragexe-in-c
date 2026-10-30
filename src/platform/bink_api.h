#ifndef PLATFORM_BINK_API_H
#define PLATFORM_BINK_API_H

#include <stddef.h>
#include <stdint.h>

struct BinkHandle {
    int32_t width, height;
    uint32_t frames, frame_num;
    uint32_t last_frame_num, frame_rate, frame_rate_div;
    uint32_t read_error, open_flags, bink_type;
    uint32_t size, frame_size, snd_size;
    uint8_t rects_planes_and_rates[0xec - 0x34];
    int32_t paused;
};

struct BinkSound;
typedef int32_t (__stdcall *BinkSoundOpen)(struct BinkSound *sound, uint32_t frequency,
                                         int32_t bits, int32_t channels,
                                         uint32_t flags, struct BinkHandle *movie);
typedef BinkSoundOpen (__stdcall *BinkSoundSystemOpen)(uint32_t parameter);

__declspec(dllimport) struct BinkHandle *__stdcall _BinkOpen(const char *name, uint32_t flags);
__declspec(dllimport) void __stdcall _BinkClose(struct BinkHandle *movie);
__declspec(dllimport) const char *__stdcall _BinkGetError(void);
__declspec(dllimport) int32_t __stdcall _BinkWait(struct BinkHandle *movie);
__declspec(dllimport) int32_t __stdcall _BinkDoFrame(struct BinkHandle *movie);
__declspec(dllimport) int32_t __stdcall _BinkShouldSkip(struct BinkHandle *movie);
__declspec(dllimport) void __stdcall _BinkNextFrame(struct BinkHandle *movie);
__declspec(dllimport) void __stdcall _BinkGoto(struct BinkHandle *movie, uint32_t frame, int32_t flags);
__declspec(dllimport) int32_t __stdcall _BinkPause(struct BinkHandle *movie, int32_t pause);
__declspec(dllimport) void __stdcall _BinkSetVolume(struct BinkHandle *movie, uint32_t track, int32_t volume);
__declspec(dllimport) int32_t __stdcall _BinkCopyToBuffer(struct BinkHandle *movie, void *destination,
                                                      int32_t pitch, uint32_t height,
                                                      uint32_t x, uint32_t y, uint32_t flags);
__declspec(dllimport) int32_t __stdcall _BinkSetSoundSystem(BinkSoundSystemOpen open, uint32_t parameter);
__declspec(dllimport) BinkSoundOpen __stdcall _BinkOpenDirectSound(uint32_t parameter);

#endif

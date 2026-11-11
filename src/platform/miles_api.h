#ifndef PLATFORM_MILES_API_H
#define PLATFORM_MILES_API_H
#include "platform/win_types.h"
#define MSS_SAMPLE_DONE 2
#define MSS_STREAM_DONE 2
#define MSS_STREAM_PLAYING 4

void _AIL_set_redist_directory(const char *dir);
int _AIL_startup(void);
void _AIL_shutdown(void);
int _AIL_set_preference(unsigned number, int value);
void *_AIL_open_digital_driver(unsigned rate, int bits, int channels, unsigned flags);
void _AIL_close_digital_driver(void *driver);
void *_AIL_allocate_sample_handle(void *driver);
void _AIL_release_sample_handle(void *sample);
void _AIL_init_sample(void *sample);
int _AIL_set_sample_file(void *sample, const void *file, int block);
void _AIL_set_sample_volume(void *sample, int volume);
void _AIL_start_sample(void *sample);
void _AIL_end_sample(void *sample);
unsigned _AIL_sample_status(void *sample);
void *_AIL_open_stream(void *driver, const char *name, int stream_memory);
void _AIL_close_stream(void *stream);
void _AIL_start_stream(void *stream);
void _AIL_set_stream_volume(void *stream, int volume);
void _AIL_set_stream_loop_count(void *stream, int count);
int _AIL_stream_status(void *stream);
int _AIL_enumerate_3D_providers(void **next, void **dest, char **name);
int _AIL_open_3D_provider(void *provider);
void _AIL_close_3D_provider(void *provider);
int _AIL_3D_speaker_type(void *provider);
void _AIL_set_3D_speaker_type(void *provider, int type);
int _AIL_3D_room_type(void *provider);
void _AIL_set_3D_room_type(void *provider, int type);
void *_AIL_open_3D_listener(void *provider);
void _AIL_close_3D_listener(void *listener);
void *_AIL_allocate_3D_sample_handle(void *provider);
void _AIL_release_3D_sample_handle(void *sample);
int _AIL_set_3D_sample_file(void *sample, const void *file);
void _AIL_set_3D_sample_effects_level(void *sample, float level);
void _AIL_set_3D_position(void *object, float x, float y, float z);
void _AIL_set_3D_orientation(void *object, float face_x, float face_y, float face_z,
                             float up_x, float up_y, float up_z);
void _AIL_set_3D_sample_volume(void *sample, int volume);
void _AIL_set_3D_sample_distances(void *sample, float max_distance, float min_distance);
void _AIL_start_3D_sample(void *sample);
void _AIL_end_3D_sample(void *sample);
unsigned _AIL_3D_sample_status(void *sample);

#define AIL_FILE_ADPCM 2
#define AIL_FILE_ASI 13

struct AilSoundInfo {
    int format;
    const void *data;
    unsigned dataLen;
    unsigned rate;
    int bits;
    int channels;
    unsigned samples;
    unsigned blockSize;
    const void *initial;
};

int _AIL_file_type(const void *data, unsigned size);
int _AIL_WAV_info(const void *data, struct AilSoundInfo *info);
int _AIL_decompress_ADPCM(const struct AilSoundInfo *info, void **out,
                          unsigned *outSize);
int _AIL_decompress_ASI(const void *data, unsigned size, const char *name,
                        void **out, unsigned *outSize, void *callback);
void _AIL_mem_free_lock(void *p);

#endif

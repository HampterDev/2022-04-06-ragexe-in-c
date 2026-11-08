#ifndef PLATFORM_MILES_API_H
#define PLATFORM_MILES_API_H
#ifndef MSS_IMPORT
#define MSS_IMPORT __declspec(dllimport)
#endif
#define MSS_SAMPLE_DONE 2
#define MSS_STREAM_PLAYING 2

MSS_IMPORT void __stdcall _AIL_set_redist_directory(const char *dir);
MSS_IMPORT int __stdcall _AIL_startup(void);
MSS_IMPORT void __stdcall _AIL_shutdown(void);
MSS_IMPORT int __stdcall _AIL_set_preference(unsigned number, int value);
MSS_IMPORT void *__stdcall _AIL_open_digital_driver(unsigned rate, int bits, int channels, unsigned flags);
MSS_IMPORT void __stdcall _AIL_close_digital_driver(void *driver);
MSS_IMPORT void *__stdcall _AIL_allocate_sample_handle(void *driver);
MSS_IMPORT void __stdcall _AIL_release_sample_handle(void *sample);
MSS_IMPORT void __stdcall _AIL_init_sample(void *sample);
MSS_IMPORT int __stdcall _AIL_set_sample_file(void *sample, const void *file, int block);
MSS_IMPORT void __stdcall _AIL_set_sample_volume(void *sample, int volume);
MSS_IMPORT void __stdcall _AIL_start_sample(void *sample);
MSS_IMPORT void __stdcall _AIL_end_sample(void *sample);
MSS_IMPORT unsigned __stdcall _AIL_sample_status(void *sample);
MSS_IMPORT void *__stdcall _AIL_open_stream(void *driver, const char *name, int stream_memory);
MSS_IMPORT void __stdcall _AIL_close_stream(void *stream);
MSS_IMPORT void __stdcall _AIL_start_stream(void *stream);
MSS_IMPORT void __stdcall _AIL_set_stream_volume(void *stream, int volume);
MSS_IMPORT void __stdcall _AIL_set_stream_loop_count(void *stream, int count);
MSS_IMPORT int __stdcall _AIL_stream_status(void *stream);
MSS_IMPORT int __stdcall _AIL_enumerate_3D_providers(void **next, void **dest, char **name);
MSS_IMPORT int __stdcall _AIL_open_3D_provider(void *provider);
MSS_IMPORT void __stdcall _AIL_close_3D_provider(void *provider);
MSS_IMPORT int __stdcall _AIL_3D_speaker_type(void *provider);
MSS_IMPORT void __stdcall _AIL_set_3D_speaker_type(void *provider, int type);
MSS_IMPORT int __stdcall _AIL_3D_room_type(void *provider);
MSS_IMPORT void __stdcall _AIL_set_3D_room_type(void *provider, int type);
MSS_IMPORT void *__stdcall _AIL_open_3D_listener(void *provider);
MSS_IMPORT void __stdcall _AIL_close_3D_listener(void *listener);
MSS_IMPORT void *__stdcall _AIL_allocate_3D_sample_handle(void *provider);
MSS_IMPORT void __stdcall _AIL_release_3D_sample_handle(void *sample);
MSS_IMPORT int __stdcall _AIL_set_3D_sample_file(void *sample, const void *file);
MSS_IMPORT void __stdcall _AIL_set_3D_sample_effects_level(void *sample, float level);
MSS_IMPORT void __stdcall _AIL_set_3D_position(void *object, float x, float y, float z);
MSS_IMPORT void __stdcall _AIL_set_3D_orientation(void *object, float face_x, float face_y, float face_z,
                                    float up_x, float up_y, float up_z);
MSS_IMPORT void __stdcall _AIL_set_3D_sample_volume(void *sample, int volume);
MSS_IMPORT void __stdcall _AIL_set_3D_sample_distances(void *sample, float max_distance, float min_distance);
MSS_IMPORT void __stdcall _AIL_start_3D_sample(void *sample);
MSS_IMPORT void __stdcall _AIL_end_3D_sample(void *sample);
MSS_IMPORT unsigned __stdcall _AIL_3D_sample_status(void *sample);

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

MSS_IMPORT int __stdcall _AIL_file_type(const void *data, unsigned size);
MSS_IMPORT int __stdcall _AIL_WAV_info(const void *data, struct AilSoundInfo *info);
MSS_IMPORT int __stdcall _AIL_decompress_ADPCM(const struct AilSoundInfo *info, void **out,
                                                         unsigned *outSize);
MSS_IMPORT int __stdcall _AIL_decompress_ASI(const void *data, unsigned size, const char *name,
                                                       void **out, unsigned *outSize, void *callback);
MSS_IMPORT void __stdcall _AIL_mem_free_lock(void *p);

#endif

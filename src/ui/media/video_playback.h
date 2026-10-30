#ifndef UI_MEDIA_VIDEO_PLAYBACK_H
#define UI_MEDIA_VIDEO_PLAYBACK_H

struct VideoPlayback;
struct Texture;

enum VideoUpdate { VIDEO_UNCHANGED, VIDEO_REDRAW, VIDEO_FINISHED };
struct VideoPlayback *video_playback_create(void);
void video_playback_destroy(struct VideoPlayback *video);
int video_playback_open(struct VideoPlayback *video, const char *filename);
enum VideoUpdate video_playback_update(struct VideoPlayback *video);
struct Texture *video_playback_texture(const struct VideoPlayback *video);
void video_playback_pause(struct VideoPlayback *video);
void video_playback_volume(struct VideoPlayback *video, int volume);

#endif

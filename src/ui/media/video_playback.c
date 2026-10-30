#include "ui/media/video_playback.h"
#include "skills/bink_player.h"
#include "platform/bink_api.h"
#include "render/texture.h"
#include <stdlib.h>

struct VideoPlayback {
    struct BinkPlayer player;
    struct Texture *texture;
    int frame_pending;
};

struct VideoPlayback *video_playback_create(void)
{
    struct VideoPlayback *video = calloc(1, sizeof *video);
    if (video) bink_player_construct(&video->player);
    return video;
}

void video_playback_destroy(struct VideoPlayback *video)
{
    if (!video) return;
    texture_destroy(video->texture);
    bink_player_destruct(&video->player);
    free(video);
}

int video_playback_open(struct VideoPlayback *video, const char *filename)
{
    struct BinkPlayer replacement = {0};
    if (!video || !filename) return 0;
    bink_player_construct(&replacement);
    if (!bink_player_open(&replacement, filename, 0, 0)
            || !replacement.bink || replacement.bink->width <= 0 || replacement.bink->height <= 0) {
        bink_player_destruct(&replacement);
        return 0;
    }
    texture_destroy(video->texture);
    video->texture = NULL;
    video->frame_pending = 0;
    bink_player_destruct(&video->player);
    video->player = replacement;
    return 1;
}

enum VideoUpdate video_playback_update(struct VideoPlayback *video)
{
    struct Texture *texture;
    int advanced;
    if (!video) return VIDEO_FINISHED;
    advanced = bink_player_advance(&video->player);
    if (video->player.state == 2) return VIDEO_FINISHED;
    if (video->player.state != 1 || !video->player.bink) return VIDEO_UNCHANGED;
    if (advanced) video->frame_pending = 1;
    if (!video->frame_pending) return VIDEO_UNCHANGED;
    texture = video->texture;
    if (!texture) {
        texture = texture_create(video->player.bink->width, video->player.bink->height, 4);
        if (!texture) return VIDEO_UNCHANGED;
    }
    if (!bink_frame_to_texture(&video->player, texture)) {
        if (!video->texture) texture_destroy(texture);
        return VIDEO_UNCHANGED;
    }
    video->texture = texture;
    video->frame_pending = 0;
    return VIDEO_REDRAW;
}

struct Texture *video_playback_texture(const struct VideoPlayback *video)
{
    return video && video->player.state == 1 ? video->texture : NULL;
}

void video_playback_pause(struct VideoPlayback *video)
{
    if (video && video->player.bink)
        _BinkPause(video->player.bink, !video->player.bink->paused);
}

void video_playback_volume(struct VideoPlayback *video, int volume)
{
    if (!video || !video->player.bink) return;
    if (volume < 0) volume = 0;
    if (volume > 128) volume = 128;
    _BinkSetVolume(video->player.bink, 0, volume * 256);
}

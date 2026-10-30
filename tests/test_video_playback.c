#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "owned_storage_test_allocator.h"
#include "platform/bink_api.h"
#include "skills/bink_player.h"
#include "fileio/file.h"
#include "render/texture.h"

static int fail_open, fail_file, fail_texture, fail_copy, waiting, skips, errors;
static unsigned movies, textures, copies, next_frames, decoded, last_volume, pauses, file_closes;
static unsigned char encoded[] = "movie";
static void *checked_calloc(size_t n,size_t size)
{ void *p; assert(!size || n<=SIZE_MAX/size); p=checked_malloc(n*size); if(p)memset(p,0,n*size); return p; }
static struct BinkHandle *fake_open(const char *name,uint32_t flags)
{
    struct BinkHandle *b;
    assert(name && (flags==0x104000 || flags==0x4104000));
    if (flags==0x4104000) assert(name==(const char*)encoded);
    if(fail_open)return NULL;
    b=checked_calloc(1,sizeof *b); if(!b)return NULL;
    b->width=32;b->height=16;b->frames=4;b->frame_num=1;++movies;return b;
}
static void fake_close(struct BinkHandle *b) { assert(b&&movies);--movies;checked_free(b); }
static const char *fake_error(void) { return "fixture decoder error"; }
static int32_t fake_wait(struct BinkHandle *b) { assert(b);return waiting; }
static int32_t fake_frame(struct BinkHandle *b) { assert(b);++decoded;return 0; }
static int32_t fake_skip(struct BinkHandle *b) { assert(b);if(skips){--skips;return 1;}return 0; }
static void fake_next(struct BinkHandle *b) { assert(b);++next_frames;++b->frame_num; }
static int32_t fake_pause(struct BinkHandle *b,int32_t pause) { assert(b);++pauses;b->paused=pause;return pause; }
static void fake_volume(struct BinkHandle *b,uint32_t track,int32_t volume)
{ assert(b&&!track&&volume>=0&&volume<=32768);last_volume=(unsigned)volume; }
#define _BinkOpen fake_open
#define _BinkClose fake_close
#define _BinkGetError fake_error
#define _BinkWait fake_wait
#define _BinkDoFrame fake_frame
#define _BinkShouldSkip fake_skip
#define _BinkNextFrame fake_next
#define _BinkPause fake_pause
#define _BinkSetVolume fake_volume
#define malloc checked_malloc
#define calloc checked_calloc
#define free checked_free
#include "../src/skills/bink_decoder.c"
#include "../src/ui/media/video_playback.c"
#undef free
#undef calloc
#undef malloc

void error_msg(const char *message,int flags) { assert(message&&!flags);++errors; }
struct File *file_construct(struct File *file) { memset(file,0,sizeof *file);return file; }
unsigned char file_open(struct File *file,const char *name,int mode,unsigned char archive,unsigned char buffered)
{ assert(file&&name&&!mode&&archive&&!buffered);if(fail_file)return 0;file->buffer=encoded;file->size=sizeof encoded;return 1; }
void file_destruct(struct File *file) { assert(file);++file_closes;memset(file,0,sizeof *file); }
void surface_destroy(struct Surface *surface)
{ assert(surface&&textures);--textures;checked_free(surface); }
struct Texture *texture_create(int w,int h,int format)
{
    struct Texture *t;assert(w==32&&h==16&&format==4);
    if(fail_texture)return NULL;
    t=checked_calloc(1,sizeof *t);if(!t)return NULL;
    t->reqWidth=w;t->reqHeight=h;++textures;return t;
}
unsigned char bink_frame_to_texture(struct BinkPlayer *player,void *texture)
{ assert(player&&player->bink&&texture);if(fail_copy)return 0;++copies;return 1; }
static void decoder_failures(void)
{
    struct BinkPlayer player;
    bink_player_construct(&player);
    assert(bink_player_open(&player,"first",0,0));assert(player.state==1&&movies==1);
    fail_open=1;assert(!bink_player_open(&player,"missing",0,0));
    assert(!player.bink&&player.state==0&&!movies&&!bink_player_advance(&player));fail_open=0;
    fail_at=allocation_calls+1;assert(!bink_player_open(&player,"alloc",0,1));fail_at=0;
    assert(!player.file&&!outstanding);
    fail_file=1;assert(!bink_player_open(&player,"file",0,1));fail_file=0;
    assert(!player.file&&!outstanding);
    fail_open=1;assert(!bink_player_open(&player,"decoder",0,1));fail_open=0;
    assert(!player.file&&!outstanding);
    assert(bink_player_open(&player,"archive",1,1));assert(player.file&&movies==1);
    player.bink->frame_num=4;assert(bink_player_advance(&player));assert(player.bink->frame_num==1);
    bink_player_close(&player);bink_player_destruct(&player);assert(!movies&&!outstanding&&player.state==0);
}
int main(void)
{
    struct VideoPlayback *video;
    struct Texture *previous;
    struct BinkHandle *movie;
    unsigned old_copies;
    decoder_failures();
    fail_at=allocation_calls+1;assert(!video_playback_create());fail_at=0;
    video=video_playback_create();assert(video&&!video_playback_texture(video));
    assert(video_playback_update(video)==VIDEO_UNCHANGED);
    assert(video_playback_open(video,"first"));movie=video->player.bink;
    waiting=1;assert(video_playback_update(video)==VIDEO_UNCHANGED && !textures && !copies);
    waiting=0;fail_texture=1;assert(video_playback_update(video)==VIDEO_UNCHANGED);
    waiting=1;
    assert(!video_playback_texture(video)&&!textures);
    fail_texture=0;fail_copy=1;assert(video_playback_update(video)==VIDEO_UNCHANGED);
    assert(!video_playback_texture(video)&&!textures);
    fail_copy=0;assert(video_playback_update(video)==VIDEO_REDRAW);
    previous=video_playback_texture(video);assert(previous&&textures==1);
    old_copies=copies;assert(video_playback_update(video)==VIDEO_UNCHANGED&&copies==old_copies);
    fail_open=1;assert(!video_playback_open(video,"failed"));fail_open=0;
    assert(video->player.bink==movie&&video_playback_texture(video)==previous&&movies==1&&textures==1);
    fail_at=allocation_calls+1;assert(!video_playback_open(video,"alloc"));fail_at=0;
    assert(video->player.bink==movie&&video_playback_texture(video)==previous);
    video_playback_volume(video,128);assert(last_volume==32768);
    video_playback_volume(video,-1);assert(last_volume==0);
    video_playback_volume(video,INT32_MAX);assert(last_volume==32768);
    video_playback_pause(video);assert(movie->paused==1);video_playback_pause(video);assert(!movie->paused&&pauses==2);
    assert(video_playback_open(video,"replacement"));assert(movies==1&&!textures&&!video_playback_texture(video));
    waiting=0;
    assert(video_playback_update(video)==VIDEO_REDRAW);
    previous=video_playback_texture(video);waiting=0;fail_copy=1;
    assert(video_playback_update(video)==VIDEO_UNCHANGED&&video_playback_texture(video)==previous&&textures==1);
    fail_copy=0;waiting=1;assert(video_playback_update(video)==VIDEO_REDRAW);
    waiting=0;video->player.bink->frame_num=4;assert(video_playback_update(video)==VIDEO_FINISHED);
    assert(!movies&&!video_playback_texture(video));
    video_playback_destroy(video);video_playback_destroy(NULL);
    video_playback_pause(NULL);video_playback_volume(NULL,3);assert(video_playback_update(NULL)==VIDEO_FINISHED);
    assert(!video_playback_open(NULL,"none")&&!textures&&!outstanding&&errors>=3&&file_closes>=3);
    puts("Video playback: real C decoder lifetime, transactional movie replacement, frame allocation/upload retry, pause/volume, completion and cleanup passed.");
    return 0;
}

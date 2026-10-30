#ifndef SKILLS_BINK_PLAYER_H
#define SKILLS_BINK_PLAYER_H

#include "base.h"
#include "render/vec3.h"

struct BinkHandle;
struct File;

struct BinkPlayer {
    struct BinkHandle *bink;
    int state;
    int loop;
    struct File *file;
};

struct BinkPlayer *bink_player_construct(struct BinkPlayer *self);
void bink_player_destruct(struct BinkPlayer *self);
unsigned char bink_player_open(struct BinkPlayer *self, const char *name, int loop,
                                         unsigned char from_grf);
void bink_player_close(struct BinkPlayer *self);
unsigned char bink_player_advance(struct BinkPlayer *self);
unsigned char bink_frame_to_texture(struct BinkPlayer *self, void *texture);

#endif

#include "skills/bink_player.h"
#include "platform/bink_api.h"
#include "fileio/file.h"
#include "fileio/resource_api.h"
#include <stdlib.h>

#define BINK_OPEN_FILE 0x104000
#define BINK_OPEN_MEMORY 0x4104000

struct BinkPlayer *bink_player_construct(struct BinkPlayer *self)
{
    self->bink = NULL;
    self->state = 0;
    self->loop = 0;
    self->file = NULL;
    return self;
}

static void bink_player_release(struct BinkPlayer *self)
{
    self->state = 0;
    if (self->bink) {
        _BinkClose(self->bink);
        self->bink = NULL;
    }
    if (self->file) {
        file_destruct(self->file);
        free(self->file);
        self->file = NULL;
    }
}

void bink_player_destruct(struct BinkPlayer *self)
{
    bink_player_release(self);
}

void bink_player_close(struct BinkPlayer *self)
{
    bink_player_release(self);
}

unsigned char bink_player_open(struct BinkPlayer *self, const char *name, int loop,
                                         unsigned char from_grf)
{
    struct File *file;

    bink_player_release(self);
    if (from_grf == 1) {
        file = malloc(sizeof *file);
        file = file ? file_construct(file) : NULL;
        self->file = file;
        if (!file)
            return 0;
        if (!file_open(file, name, 0, 1, 0)) {
            bink_player_release(self);
            return 0;
        }
        self->bink = _BinkOpen((const char *)self->file->buffer, BINK_OPEN_MEMORY);
    } else {
        self->bink = _BinkOpen(name, BINK_OPEN_FILE);
    }
    if (!self->bink) {
        error_msg(_BinkGetError(), 0);
        bink_player_release(self);
        return 0;
    }
    self->loop = loop;
    self->state = 1;
    return 1;
}

unsigned char bink_player_advance(struct BinkPlayer *self)
{
    unsigned char decoded = 0;
    struct BinkHandle *bink;

    if (self->state != 1)
        return 0;
    if (!_BinkWait(self->bink)) {
        _BinkDoFrame(self->bink);
        if (_BinkShouldSkip(self->bink)) {
            do {
                _BinkNextFrame(self->bink);
                _BinkDoFrame(self->bink);
            } while (_BinkShouldSkip(self->bink));
        }
        bink = self->bink;
        if (bink->frame_num == bink->frames) {
            if (self->loop == 0) {
                bink_player_close(self);
                self->state = 2;
                return 0;
            }
            if (self->loop == 1) {
                bink->frame_num = 0;
                bink = self->bink;
            }
        }
        _BinkNextFrame(bink);
        decoded = 1;
    }
    return decoded;
}

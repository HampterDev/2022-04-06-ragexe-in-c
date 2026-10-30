#ifndef RESOURCES_ANIMATION_H
#define RESOURCES_ANIMATION_H
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <stddef.h>

struct SprClip {
    int x, y;
    int sprIndex;
    unsigned flags;
    unsigned char color[4];
    float zoomX, zoomY;
    int angle, clipType;
};
struct AttachPointInfo {
    int x, y, attr;
};
struct Motion {
    RECT range1, range2;
    struct SprClip *clips;
    size_t clip_count;
    int eventId;
    struct AttachPointInfo *attachments;
    size_t attachment_count;
};
struct SpriteAction {
    struct Motion *motions;
    size_t motion_count;
    float delay;
};
struct ActRes;

size_t animation_action_count(const struct ActRes *resource);
struct SpriteAction *act_res_get_action(const struct ActRes *self, unsigned action);
struct Motion *act_res_get_motion(const struct ActRes *self, unsigned action, unsigned motion);
struct SprClip *motion_get_clip(const struct Motion *self, unsigned index);
float act_res_get_delay(const struct ActRes *self, unsigned action);
const char *act_res_get_event_name(const struct ActRes *self, unsigned index);
unsigned actres_frame_count(const struct ActRes *self, unsigned action);
static inline size_t animation_attachment_count(const struct Motion *motion)
{
    return motion ? motion->attachment_count : 0;
}
static inline const struct AttachPointInfo *animation_attachment(const struct Motion *motion, size_t index)
{
    return motion && index < motion->attachment_count ? &motion->attachments[index] : NULL;
}
#endif

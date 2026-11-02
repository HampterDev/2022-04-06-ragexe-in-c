#ifndef RESOURCES_GRANNY_PLAYBACK_H
#define RESOURCES_GRANNY_PLAYBACK_H

struct C3dGrannyModelRes;
struct GrannyPlayback;
struct granny_world_pose;

struct GrannyPlayback *granny_playback_create(struct C3dGrannyModelRes *resource);
void granny_playback_destroy(struct GrannyPlayback *playback);
void granny_playback_stop(struct GrannyPlayback *playback);
int granny_playback_play(struct GrannyPlayback *playback, int animation, int loops);
int granny_playback_finished(const struct GrannyPlayback *playback, int animation);
struct granny_world_pose *granny_playback_world_pose(const struct GrannyPlayback *playback);
struct granny_world_pose *granny_playback_pose(struct GrannyPlayback *playback, float time);

#endif

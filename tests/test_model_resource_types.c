#include "resources/model_res.h"
#include "skills/world_video.h"
#include "game/context.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct WorldVideoList { int unused; };
struct WorldVideo { int index; };
static struct WorldVideoList videos;
static struct WorldVideo movie[3] = {{0}, {1}, {2}};
static unsigned opens, positions, errors;
static int absent, fail_open, fail_position;
static const char *const names[] = {"영화.bik", "second.bik", "last.bik"};
static Vec3 expected;

struct WorldVideoList *game_current_video_list(void)
{ return absent ? NULL : &videos; }
struct WorldVideo *world_video_list_open(struct WorldVideoList *list, const char *name, int loop)
{
    unsigned i = opens++;
    assert(list == &videos && loop && i < 3 && !strcmp(name, names[i]));
    return fail_open && i == 1 ? NULL : &movie[i];
}
int world_video_add_position(struct WorldVideo *video, Vec3 position)
{
    assert(video >= movie && video < movie + 3);
    assert(!memcmp(&position, &expected, sizeof position));
    ++positions;
    return !fail_position || video != &movie[1];
}
void error_msg(const char *message, int flags)
{ assert(message && !flags); ++errors; }

static void set_name(struct OwnedText *out, const char *name)
{
    out->data = (char *)name; out->size = strlen(name);
}

int main(void)
{
    struct C3dModelRes res = {0};
    struct ModelResNodeV1 a = {0};
    struct ModelResNodeV2 b = {0};
    struct ModelResNode *nodes[] = {&a.base, &b.base};
    struct OwnedText files[3] = {0};
    unsigned i;
    assert(!model_res_find_node(&res, "missing"));
    res.nodes = nodes; res.nodeCount = 2;
    set_name(&a.base.name, "기둥"); set_name(&b.base.name, "UpperNode");
    assert(model_res_find_node(&res, "기둥") == &a.base);
    assert(model_res_find_node(&res, "UpperNode") == &b.base);
    assert(!model_res_find_node(&res, "uppernode"));
    assert(!model_res_find_node(&res, "Upper"));

    res.videoNames.items = files; res.videoNames.size = 3;
    for (i = 0; i < 3; ++i) set_name(&files[i], names[i]);
    expected.x = -12.5f; expected.y = 0.125f; expected.z = 1234.75f;
    absent = 1; model_res_place_videos(&res, expected); assert(!opens && !positions);
    absent = 0; model_res_place_videos(&res, expected); assert(opens == 3 && positions == 3 && !errors);
    opens = positions = 0; fail_open = 1;
    model_res_place_videos(&res, expected); assert(opens == 3 && positions == 2 && !errors);
    opens = positions = 0; fail_open = 0; fail_position = 1;
    model_res_place_videos(&res, expected); assert(opens == 2 && positions == 2 && errors == 1);
    puts("model resource types: real node lookup and ordered video placement, Korean names, absent movies and failed positions passed");
    return 0;
}

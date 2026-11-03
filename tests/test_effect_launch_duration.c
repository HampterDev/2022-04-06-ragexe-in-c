#include "effect_launch_duration.inc"

#include <assert.h>
#include <stdio.h>

/* Run the production rag_effect_init against the real duration table. Server
   effect IDs reach it up to 60000; the table has EFFECT_DURATION_COUNT rows. */
struct Session g_session1;
static struct RagEffect effect;
static int set_calls, last_duration, cases;
static char loaded[64];

void rag_effect_set_duration(struct RagEffect *self, int frame)
{
    assert(self == &effect);
    last_duration = frame;
    ++set_calls;
}

void rag_str_load(struct RagEffect *self, const char *name)
{
    assert(self == &effect && strlen(name) < sizeof loaded);
    strcpy(loaded, name);
}

/* The embedded effect data (data/effects/effects.json) always loads. */
void error_msg(const char *text, int flags)
{
    fprintf(stderr, "%s\n", text);
    assert(!"effect data error");
}

static void check(int id, int duration, const char *file)
{
    struct RenderObject master;
    Vec3 offset = { 1.0f, 2.0f, 3.0f };
    int previous = set_calls;

    memset(&effect, 0, sizeof effect);
    memset(&master, 0, sizeof master);
    loaded[0] = 0;
    rag_effect_init(&effect, &master, id, offset);
    assert(set_calls == previous + 1 && effect.effect_id == id && effect.master == &master);
    assert(last_duration == duration);
    assert(strcmp(loaded, file) == 0);
    ++cases;
}

int main(void)
{
    _putenv("RAGEXE_EFFECT_DATA=");
    assert(EFFECT_DURATION_COUNT == 2385);
    check(0, 50, "");
    check(13, 9999, "SafetyWall.str");
    check(2340, 2340, "");
    check(2383, 9999, "");
    check(2384, 0, "");
    check(2385, 2385, "");
    check(2386, 2386, "");
    check(30000, 30000, "");
    check(60000, 60000, "");
    printf("Effect launch duration: %d cases passed (table rows in range; IDs past it last their ID).\n", cases);
    return 0;
}

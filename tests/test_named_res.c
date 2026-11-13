#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "fileio/resource_api.h"
#include "resources/named_res.h"

/* The resource manager as named_res sees it: a few names with a resource each, the lookups made
   and every resource's lock count. */
struct Res {
    const char *name;
    int locks;
};
static struct Res resources[] = {{"body.pal", 0}, {"head.pal", 0}, {"job.imf", 0}};
enum { RESOURCE_COUNT = sizeof resources / sizeof *resources };
static int lookups, lock_calls, unlock_calls;

struct ResMgr *g_resMgr(void)
{
    return (struct ResMgr *)resources;
}

void *res_mgr_get(struct ResMgr *self, const char *name)
{
    int i;
    assert(self == g_resMgr() && name && *name);
    ++lookups;
    for (i = 0; i < RESOURCE_COUNT; ++i)
        if (!strcmp(resources[i].name, name)) return &resources[i];
    return NULL;
}

void res_lock(struct Res *self)
{
    assert(self >= resources && self < resources + RESOURCE_COUNT);
    ++self->locks;
    ++lock_calls;
}

void res_unlock(struct Res *self)
{
    assert(self >= resources && self < resources + RESOURCE_COUNT && self->locks > 0);
    --self->locks;
    ++unlock_calls;
}

static int locks_held(void)
{
    int i, held = 0;
    for (i = 0; i < RESOURCE_COUNT; ++i) held += resources[i].locks;
    return held;
}

static void assign(struct NamedRes *named, const char *text)
{
    named_res_assign(named, text, strlen(text));
}

static void rules(void)
{
    struct NamedRes named, other;
    struct String name;

    named_res_init(&named);
    assert(!named_res_size(&named) && !strcmp(named_res_text(&named), ""));
    assert(!named_res_get(&named) && !named_res_get(&named) && !lookups);
    assign(&named, "body.pal");
    assert(!lookups && !locks_held() && !strcmp(named_res_text(&named), "body.pal") && named_res_size(&named) == 8);
    assert(named_res_get(&named) == &resources[0] && lookups == 1 && resources[0].locks == 1);
    assert(named_res_get(&named) == &resources[0] && lookups == 1 && resources[0].locks == 1);
    /* The same name again: let go, then looked up and locked once more. */
    assign(&named, "body.pal");
    assert(!resources[0].locks && lookups == 1);
    assert(named_res_get(&named) == &resources[0] && lookups == 2 && resources[0].locks == 1);
    /* A name without a resource: NULL, remembered, until the name is set again. */
    assign(&named, "absent.pal");
    assert(!resources[0].locks && !named_res_get(&named) && !named_res_get(&named) && lookups == 3);
    assign(&named, "");
    assert(!named_res_get(&named) && lookups == 3);
    assign(&named, "head.pal");
    assert(named_res_get(&named) == &resources[1] && resources[1].locks == 1 && lookups == 4);
    /* Take: the text moves in, the source is left empty, the old resource is let go. */
    string_init(&name);
    string_assign_cstr(&name, "job.imf");
    named_res_take(&named, &name);
    assert(!name.size && !strcmp(named_res_text(&named), "job.imf") && !resources[1].locks);
    assert(named_res_get(&named) == &resources[2] && resources[2].locks == 1 && lookups == 5);
    string_free(&name);
    /* A copy by name looks up on its own and holds its own lock. */
    named_res_init(&other);
    named_res_assign(&other, named_res_text(&named), named_res_size(&named));
    assert(named_res_get(&other) == &resources[2] && resources[2].locks == 2 && lookups == 6);
    named_res_free(&other);
    assert(resources[2].locks == 1);
    named_res_free(&named);
    assert(!locks_held() && lock_calls == unlock_calls);
    /* Freed before any lookup, or with a remembered NULL: nothing to let go. */
    named_res_init(&named);
    assign(&named, "absent.pal");
    named_res_free(&named);
    named_res_init(&named);
    assign(&named, "absent.pal");
    assert(!named_res_get(&named));
    named_res_free(&named);
    assert(!locks_held() && lock_calls == unlock_calls);
}

static unsigned random_state = 0x6b43a9b5u;
static unsigned next_random(void)
{
    random_state ^= random_state << 13;
    random_state ^= random_state >> 17;
    random_state ^= random_state << 5;
    return random_state;
}

/* Assign, take, get and free in a random order on a few holders, against a model of each holder's
   name and whether it has looked up since the name was set. */
static void fuzz(void)
{
    static const char *const names[] = {"", "body.pal", "head.pal", "job.imf", "absent.pal"};
    enum { HOLDERS = 6, NAMES = sizeof names / sizeof *names };
    struct NamedRes holders[HOLDERS];
    struct Model { int name, looked_up, live; } model[HOLDERS];
    int step, i;

    memset(model, 0, sizeof model);
    for (i = 0; i < HOLDERS; ++i) {
        named_res_init(&holders[i]);
        model[i].live = 1;
    }
    for (step = 0; step < 200000; ++step) {
        int h = (int)(next_random() % HOLDERS), op = (int)(next_random() % 8);
        int n = (int)(next_random() % NAMES), expected_lookups = lookups, held = 0;
        struct Res *expected = NULL;

        if (!model[h].live) {
            named_res_init(&holders[h]);
            model[h].name = 0;
            model[h].looked_up = 0;
            model[h].live = 1;
        } else if (op < 2) {
            assign(&holders[h], names[n]);
            model[h].name = n;
            model[h].looked_up = 0;
        } else if (op < 3) {
            struct String text;
            string_init(&text);
            string_assign_cstr(&text, names[n]);
            named_res_take(&holders[h], &text);
            assert(!text.size);
            string_free(&text);
            model[h].name = n;
            model[h].looked_up = 0;
        } else if (op < 7) {
            if (model[h].name >= 1 && model[h].name <= RESOURCE_COUNT) expected = &resources[model[h].name - 1];
            if (!model[h].looked_up && model[h].name) ++expected_lookups;
            assert(named_res_get(&holders[h]) == expected && lookups == expected_lookups);
            model[h].looked_up = 1;
        } else {
            named_res_free(&holders[h]);
            model[h].live = 0;
        }
        assert(!strcmp(model[h].live ? names[model[h].name] : "", model[h].live ? named_res_text(&holders[h]) : ""));
        for (i = 0; i < HOLDERS; ++i)
            if (model[i].live && model[i].looked_up && model[i].name >= 1 && model[i].name <= RESOURCE_COUNT) ++held;
        assert(locks_held() == held);
    }
    for (i = 0; i < HOLDERS; ++i)
        if (model[i].live) named_res_free(&holders[i]);
    assert(!locks_held() && lock_calls == unlock_calls);
}

int main(void)
{
    rules();
    fuzz();
    printf("Named resources: one lookup per name set, NULL remembered, an empty name never looked up, "
           "a lock let go on every new name, take and free, and 200000 fuzzed steps against a model "
           "(%d lookups, %d locks) passed.\n", lookups, lock_calls);
    return 0;
}

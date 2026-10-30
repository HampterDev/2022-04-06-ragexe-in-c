#include "std/list.h"
#include "skills/skill_info.h"
#include "std/tree.h"
#include "std/integer_map.h"
#include <string.h>
#include "ui/hotkeys/hotkey_bars.h"

int session_get_instant_var(struct Session *self, int key)
{
    struct IntInt64Pair *var = tree_find(&self->instantVars, &key, tree_less_int);

    if (!var)
        return 0;
    return (int)var->value;
}

void session_set_pair(struct Session *self, int key, long long value)
{
    struct IntInt64Pair *var = tree_get(&self->instantVars, &key, sizeof key, sizeof *var, tree_less_int);

    var->value = value;
}

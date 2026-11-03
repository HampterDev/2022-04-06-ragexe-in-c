#include "skills/skill_info.h"
#include "core/utf8.h"
#include <errno.h>
#include "core/fatal.h"
#include <stdlib.h>
#include <string.h>
#include "ui/ui_std.h"
#include "game/rodex_mail.h"
#include "ui/character/skill_tree.h"
#include "ui/items/styling_shop_std.h"
#include "ui/character/character_std.h"

#undef ELEM

BYTE text_within_columns(void *self, struct String text, int limit)
{
    BYTE narrower;

    (void)self;
    narrower = (int)utf8_columns(string_cstr(&text), text.size) < limit;
    string_free(&text);
    return narrower;
}

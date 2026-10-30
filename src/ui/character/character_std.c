#include "skills/skill_info.h"
#include <errno.h>
#include "core/fatal.h"
#include <mbctype.h>
#include <mbstring.h>
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
    int columns = 0;
    unsigned i;
    BYTE narrower;

    (void)self;

    _setmbcp(949);
    for (i = 0; i < text.size; i++) {
        switch (_mbsbtype((const unsigned char *)string_cstr(&text), i)) {
        case _MBC_SINGLE: columns++; break;
        case _MBC_TRAIL: columns += 2; break;
        default: break;
        }
    }
    narrower = columns < limit;
    string_free(&text);
    return narrower;
}

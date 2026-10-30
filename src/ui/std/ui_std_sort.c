#include <string.h>
#include "ui/ui_std.h"
#include "game/rodex_mail.h"
#include "ui/social/guild_manage.h"

static const struct Mail *ref_mail(const struct MailRef *ref)
{
    const struct MailPair *pair;

    if (ref->type < 0 || ref->type > 2)
        return NULL;
    pair = tree_find(&g_rodex->tabs[ref->type], &ref->id, mail_id_greater);
    return pair ? &pair->mail : NULL;
}

BYTE rodex_inbox_mail_before(const struct MailRef *a, const struct MailRef *b)
{
    const struct Mail *ma = ref_mail(a), *mb;

    if (!ma || !(mb = ref_mail(b)))
        return 0;
    if (ma->is_read == 0) {
        if (mb->is_read == 1)
            return 1;
    } else if (mb->is_read == 0) {
        return 0;
    }
    return ma->sent_time > mb->sent_time;
}

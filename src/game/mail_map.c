#include <string.h>
#include "game/packets_rodex.h"

int mail_id_greater(const void *a, const void *b)
{
    return *(const long long *)a > *(const long long *)b;
}

void mail_pair_destroy(void *value)
{
    struct MailPair *pair = value;

    mail_destroy(&pair->mail);
}

struct MailPair *mail_pair_construct(struct MailPair *out, const long long *id, const struct Mail *mail)
{
    out->id = *id;
    out->mail.is_read = mail->is_read;
    out->mail.type = mail->type;
    string_copy(&out->mail.sender, &mail->sender);
    out->mail.sent_time = mail->sent_time;
    out->mail.expire_time = mail->expire_time;
    string_copy(&out->mail.title, &mail->title);
    string_copy(&out->mail.text, &mail->text);
    out->mail.item_count = mail->item_count;
    memcpy(out->mail.items, mail->items, sizeof(out->mail.items));
    out->mail.zeny = mail->zeny;
    out->mail.deleted = mail->deleted;
    out->mail.sender_trusted = mail->sender_trusted;
    return out;
}

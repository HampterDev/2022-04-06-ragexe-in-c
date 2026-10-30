#include "session/state.h"
#ifndef GAMEMODE_RODEX_MAIL_H
#define GAMEMODE_RODEX_MAIL_H

#include "base.h"
#include "std/tree.h"

#define MAIL_ITEM_SLOTS 5
#define MAIL_ITEM_SIZE 0x3c

enum {
    MAIL_HAS_ZENY = 0x02,
    MAIL_HAS_ITEMS = 0x04,
    MAIL_FROM_SERVER = 0x08,
};

struct Mail {
    unsigned char is_read;
    unsigned char type;
    struct String sender;
    int sent_time;
    int expire_time;
    struct String title;
    struct String text;
    unsigned char item_count;
    unsigned char items[MAIL_ITEM_SLOTS][MAIL_ITEM_SIZE];
    long long zeny;
    unsigned char deleted;
    int sender_trusted;
};

struct MailPair {
    long long id;
    struct Mail mail;
};

struct Rodex {
    unsigned char constructed;
    long long open_mail_id;
    unsigned char open_type;
    unsigned char open_type_align[3];
    int mail_count;
    int unread;
    unsigned char list_complete;
    struct Tree tabs[3];
};
OFFSET_ASSERT(Rodex, constructed, 0x00);
OFFSET_ASSERT(Rodex, open_mail_id, 0x08);
OFFSET_ASSERT(Rodex, mail_count, 0x14);
OFFSET_ASSERT(Rodex, unread, 0x18);
OFFSET_ASSERT(Rodex, tabs, 0x20);
extern struct Rodex *g_rodex;

static __inline void mail_destroy(struct Mail *mail)
{
    string_free(&mail->text);
    string_free(&mail->title);
    string_free(&mail->sender);
}

int mail_id_greater(const void *a, const void *b);
void mail_pair_destroy(void *value);
struct MailPair *mail_pair_construct(struct MailPair *out, const long long *id, const struct Mail *mail);

#endif

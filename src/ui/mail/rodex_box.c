#include "core/memory.h"
#include "ui/window.h"
#include "game/rodex_mail.h"
#include "ui/mail/rodex_box.h"
#include "ui/ui_std.h"

static struct Rodex *rodex_construct(struct Rodex *self)
{
    int i;

    for (i = 0; i < 3; i++)
        tree_init(&self->tabs[i]);
    self->constructed = 0;
    for (i = 0; i < 3; i++)
        tree_clear(&self->tabs[i], mail_pair_destroy);
    self->open_mail_id = 0;
    self->open_type = 0;
    self->mail_count = 0;
    self->unread = 0;
    self->list_complete = 0;
    self->constructed = 1;
    return self;
}

void rodex_destruct(struct Rodex *self)
{
    tree_free(&self->tabs[2], mail_pair_destroy);
    tree_free(&self->tabs[1], mail_pair_destroy);
    tree_free(&self->tabs[0], mail_pair_destroy);
}

static void rodex_destroy(struct Rodex *self)
{
    rodex_destruct(self);
    mem_free(self);
}

unsigned char rodex_create_instance(void)
{
    struct Rodex *box;

    if (g_rodex)
        return 0;
    box = mem_try_alloc(sizeof *box);
    if (box)
        box = rodex_construct(box);
    g_rodex = box;
    if (!box)
        return 0;
    if (box->constructed)
        return 1;
    rodex_destroy(box);
    g_rodex = NULL;
    return 0;
}

void rodex_destroy_instance(void)
{
    if (!g_rodex)
        return;
    rodex_destroy(g_rodex);
    g_rodex = NULL;
}

void rodex_clear(struct Rodex *self)
{
    tree_clear(&self->tabs[0], mail_pair_destroy);
    tree_clear(&self->tabs[1], mail_pair_destroy);
    tree_clear(&self->tabs[2], mail_pair_destroy);
    self->open_mail_id = 0;
    self->open_type = 0;
    self->unread = 0;
}

void rodex_purge_deleted(struct Rodex *self)
{
    int i;

    for (i = 0; i < 3; i++) {
        struct Tree *map = &self->tabs[i];
        struct TreeNode *node = tree_first(map);

        while (node != map->head) {
            if (TREE_VALUE(node, struct MailPair)->mail.deleted == 1)
                node = tree_erase(map, node, mail_pair_destroy);
            else
                node = tree_next(node);
        }
    }
}

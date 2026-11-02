#ifndef GAMEMODE_GUILD_STORAGE_H
#define GAMEMODE_GUILD_STORAGE_H

#include "game/packet_handlers.h"
#include "base.h"

struct GuildStorageLogEntry {
    int id;
    int amount;
    char action;
    char name[24];
    char time[24];
    struct String item_name;
    char identified;
    int item_type;
    int refine;
    int cards[4];
    unsigned char attribute;
    unsigned short grade;
};

struct GuildStorageLogEntry *guild_log_entry_construct(struct GuildStorageLogEntry *entry);
void guild_log_entry_construct_copy(struct GuildStorageLogEntry *where,
                                         const struct GuildStorageLogEntry *from);
struct GuildStorageLogEntry *guild_log_entry_construct_move(struct GuildStorageLogEntry *entry,
                                                                    struct GuildStorageLogEntry *from);
void guild_log_sort(struct GuildStorageLogEntry *first, struct GuildStorageLogEntry *last, int ideal,
                           void *pred);
void **std_make_move_iterator(void **out, void *in);

struct PACKET_ZC_ACK_OPEN_GUILD_STORAGE;
struct PACKET_ZC_ACK_CLOSE_GUILD_STORAGE;
struct PACKET_ZC_ACK_GUILDSTORAGE_LOG;

void guild_log_clear(struct Vec *v);
void guild_log_free(struct Vec *v);

#endif

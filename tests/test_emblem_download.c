#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "contents/guild_mgr.h"
static struct Block
{
    unsigned char *base;
    size_t size;
} blocks[256];
static int allocations, attempts, fail_at;
static int block_index(void *p)
{
    int i;
    for (i = 0; i < 256; ++i)
        if (blocks[i].base && blocks[i].base + 8 == p)
            return i;
    assert(0);
    return -1;
}
static void check_guards(void)
{
    int i, j;
    for (i = 0; i < 256; ++i)
        if (blocks[i].base)
            for (j = 0; j < 8; ++j)
            {
                assert(blocks[i].base[j] == 0xa7);
                assert(blocks[i].base[8 + blocks[i].size + j] == 0xa7);
            }
}
static void *allocate(size_t size)
{
    int i;
    if (++attempts == fail_at)
        return NULL;
    for (i = 0; i < 256 && blocks[i].base; ++i)
    {
    }
    assert(i < 256 && size <= SIZE_MAX - 16);
    blocks[i].base = malloc(size + 16);
    assert(blocks[i].base);
    blocks[i].size = size;
    memset(blocks[i].base, 0xa7, size + 16);
    ++allocations;
    return blocks[i].base + 8;
}
static void release(void *p)
{
    int i;
    if (!p)
        return;
    check_guards();
    i = block_index(p);
    free(blocks[i].base);
    blocks[i].base = NULL;
    --allocations;
}
static void *resize(void *p, size_t size)
{
    void *result = allocate(size);
    if (result && p)
    {
        size_t old_size = blocks[block_index(p)].size;
        memcpy(result, p, size < old_size ? size : old_size);
        release(p);
    }
    return result;
}

#define malloc allocate
#define realloc resize
#define free release
#include "../src/contents/guild_emblem_download.c"
#undef malloc
#undef realloc
#undef free
static unsigned char saved[2048];
static size_t saved_size;
static unsigned saved_guild;
static int saved_version, saves, changes;
void guild_emblem_save(const unsigned char *data, unsigned size, unsigned guild, int version)
{
    assert(size <= sizeof saved);
    if (size)
        memcpy(saved, data, size);
    saved_size = size;
    saved_guild = guild;
    saved_version = version;
    ++saves;
}
void guild_emblem_changed(unsigned guild)
{
    assert(guild == saved_guild);
    ++changes;
}
int main(void)
{
    struct GuildMgr manager = {0};
    unsigned char data[1024], failure_data[2048] = {0};
    size_t i;
    unsigned char *old;
    for (i = 0; i < sizeof data; ++i)
        data[i] = (unsigned char)i;
    assert(!guild_mgr_download_chunk(NULL, 0, 10, 1, NULL, 0));
    assert(!guild_mgr_download_chunk(&manager, 0, 0, 1, NULL, 0));
    assert(!guild_mgr_download_chunk(&manager, 1, 10, 1, data, 1));
    assert(guild_mgr_download_chunk(&manager, 0, 10, 1, NULL, 0));
    assert(guild_mgr_download_chunk(&manager, 1, 10, 1, data, 128));
    old = manager.download.data;
    assert(!guild_mgr_download_chunk(&manager, 1, 20, 1, data, 1));
    assert(!guild_mgr_download_chunk(&manager, 1, 10, 2, data, 1));
    assert(guild_mgr_download_chunk(&manager, 1, 10, 1, data + 128, 128));
    assert(manager.download.data == old);
    assert(guild_mgr_download_chunk(&manager, 1, 10, 1, data + 256, 768));
    assert(manager.download.size == sizeof data &&
           !memcmp(manager.download.data, data, sizeof data));
    assert(guild_mgr_download_chunk(&manager, 2, 10, 1, NULL, 0));
    assert(saved_size == sizeof data && !memcmp(saved, data, sizeof data) && saves == 1 &&
           changes == 1);
    assert(!guild_mgr_download_chunk(&manager, 2, 20, 1, NULL, 0));
    assert(guild_mgr_download_chunk(&manager, 0, 20, -1, NULL, 0));
    assert(manager.download.size == 0 && manager.download.capacity >= sizeof data);
    assert(guild_mgr_download_chunk(&manager, 1, 20, -1, data, 3));
    assert(guild_mgr_download_chunk(&manager, 2, 20, -1, NULL, 0) && saved_size == 3 &&
           saved_version == -1);
    assert(guild_mgr_download_chunk(&manager, 0, 30, 2, NULL, 0));
    attempts = 0;
    fail_at = 1;
    assert(manager.download.capacity + 1 <= sizeof failure_data);
    assert(
        !guild_mgr_download_chunk(&manager, 1, 30, 2, failure_data, manager.download.capacity + 1));
    fail_at = 0;
    assert(!manager.download.guild_id && !manager.download.size);
    assert(!guild_mgr_download_chunk(&manager, 1, 30, 2, data, 3));
    assert(!guild_mgr_download_chunk(&manager, 2, 30, 2, NULL, 0));
    assert(saves == 2 && changes == 2);
    assert(guild_mgr_download_chunk(&manager, 0, 30, 2, NULL, 0));
    assert(!guild_mgr_download_chunk(&manager, 1, 30, 2, data, (size_t)INT_MAX + 1));
    assert(!manager.download.guild_id);
    assert(guild_mgr_download_chunk(&manager, 0, 30, 2, NULL, 0));
    assert(guild_mgr_download_chunk(&manager, 1, 30, 2, NULL, 0));
    assert(guild_mgr_download_chunk(&manager, 2, 30, 2, NULL, 0) && saved_size == 0);
    guild_emblem_download_destroy(&manager.download);
    guild_emblem_download_destroy(&manager.download);
    assert(!allocations && !manager.download.data && !manager.download.capacity);
    puts("emblem downloads: C storage, ordered fragments, guild/version matching, retained "
         "capacity, failed-fragment cancellation, integer bounds and repeat cleanup passed");
    return 0;
}

#include "contents/guild_mgr.h"
#include <stdlib.h>
#include <string.h>

void guild_emblem_download_destroy(struct GuildEmblemDownload *download)
{
    free(download->data);
    memset(download, 0, sizeof *download);
}

int guild_mgr_download_chunk(struct GuildMgr *manager, int part, unsigned guild, int version,
                             const void *data, size_t size)
{
    struct GuildEmblemDownload *download;
    size_t needed, capacity;
    void *grown;
    if (!manager || !guild || (size && !data))
        return 0;
    download = &manager->download;
    switch ((short)part)
    {
    case 0:
        download->guild_id = guild;
        download->version = version;
        download->size = 0;
        return 1;
    case 1:
        if (download->guild_id != guild || download->version != version)
            return 0;
        if (download->size > GUILD_EMBLEM_MAX_SIZE || size > GUILD_EMBLEM_MAX_SIZE - download->size)
            goto failed;
        needed = download->size + size;
        if (needed > download->capacity)
        {
            capacity = download->capacity > GUILD_EMBLEM_MAX_SIZE / 2
                           ? GUILD_EMBLEM_MAX_SIZE
                           : download->capacity * 2;
            if (capacity < 256)
                capacity = 256;
            if (capacity < needed)
                capacity = needed;
            grown = realloc(download->data, capacity);
            if (!grown)
                goto failed;
            download->data = grown;
            download->capacity = capacity;
        }
        if (size)
            memcpy(download->data + download->size, data, size);
        download->size = needed;
        return 1;
    case 2:
        if (download->guild_id != guild || download->version != version)
            return 0;
        guild_emblem_save(download->data, (unsigned)download->size, guild, version);
        guild_emblem_changed(guild);
        return 1;
    default:
        return 0;
    }
failed:
    download->guild_id = 0;
    download->size = 0;
    return 0;
}

#include "contents/guild_emblem_transport.h"
#include "contents/connection_api.h"
#include "contents/guild_mgr.h"
#include "resources/bitmap.h"
#include "resources/bitmap_types.h"
#include <stdlib.h>
#include <string.h>

#pragma pack(push, 1)
struct EmblemRequestPacket {
    unsigned short id;
    int guild, version;
};
struct EmblemUploadPacket {
    unsigned short id, length;
    unsigned char data[];
};
#pragma pack(pop)
SIZE_ASSERT(EmblemRequestPacket, 10);
SIZE_ASSERT(EmblemUploadPacket, 4);

int guild_emblem_request(int guild, int version)
{
    struct RagConnection *connection = rag_connection_instance();
    struct EmblemRequestPacket packet = {0x0b46, guild, version};
    if (!guild || !connection_is_open(rag_connection_transport(connection)))
        return 0;
    return rag_connection_send_packet(connection, sizeof packet, &packet);
}

int guild_emblem_upload(const void *data, size_t size)
{
    struct RagConnection *connection = rag_connection_instance();
    struct EmblemUploadPacket *packet;
    struct GifRes image;
    int valid, sent;
    if (!data || !size || size > GUILD_EMBLEM_MAX_SIZE ||
        !connection_is_open(rag_connection_transport(connection)))
        return 0;
    gif_resource_init(&image);
    valid = gif_res_load_from_buffer(&image, "_emblem.bmp", data, (int)size, 1);
    gif_resource_finish(&image);
    if (!valid)
        return 0;
    packet = malloc(sizeof *packet + size);
    if (!packet)
        return 0;
    packet->id = 0x0153;
    packet->length = (unsigned short)(sizeof *packet + size);
    memcpy(packet->data, data, size);
    sent = rag_connection_send_packet(connection, packet->length, packet);
    free(packet);
    return sent;
}

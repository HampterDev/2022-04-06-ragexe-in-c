#include "ui/window_messages.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "web/service_manager_state.h"
#include "contents/connection_api.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "web/emblem.h"
#include "json/service_json.h"
#include "contents/game_session.h"
#include "core/msg_str.h"

#define HTTP_OK 200
#define EMBLEM_PIECE_MAX 0x3f2
#define PACKET_EMBLEM_DATA 0xb36
#define PACKET_EMBLEM_UPLOADED 0xb46
#define EMBLEM_KIND_END 0
#define EMBLEM_KIND_DATA 1
#define EMBLEM_KIND_START 2
#define UPLOAD_TYPE_DONE 1
#define MSG_EMBLEM_UPLOAD_FAILED 0xe02

#pragma pack(push, 1)
struct EmblemDataPacket {
    unsigned short packet_id;
    short len;
    unsigned short kind;
    int gdid;
    int version;
};
struct EmblemUploadedPacket {
    unsigned short packet_id;
    int gdid;
    int version;
};
#pragma pack(pop)
SIZE_ASSERT(EmblemDataPacket, 14);
SIZE_ASSERT(EmblemUploadedPacket, 10);

void emblem_download_work_done(struct EmblemDownloadWork *self)
{
    emblem_mgr_on_downloaded(g_emblemMgr, self->succeeded, self->status,
        owned_text_view(&self->data), self->gdid, self->version);
}

char emblem_download_work_run(struct EmblemDownloadWork *self)
{
    char aid[0x40], gdid[0x40], version[10];
    char *world;
    const char *body;
    size_t body_size;
    struct HttpPost *post;

    memset(aid, 0, sizeof aid);
    _snprintf_s(aid, sizeof aid, (size_t)-1, "%lu", self->aid);
    memset(gdid, 0, sizeof gdid);
    _snprintf_s(gdid, sizeof gdid, (size_t)-1, "%lu", self->gdid);
    memset(version, 0, sizeof version);
    _snprintf_s(version, sizeof version, (size_t)-1, "%lu", self->version);
    world = http_text_to_utf8(owned_text_cstr(&self->world));
    post = http_post_create(owned_text_cstr(&self->url));

    http_post_add_text(post, "AID", aid);
    http_post_add_text(post, "AuthToken", owned_text_cstr(&self->token));
    http_post_add_text(post, "WorldName", world);
    http_post_add_text(post, "GDID", gdid);
    http_post_add_text(post, "Version", version);

    if (http_post_perform(post) != 0) {
        self->succeeded = 0;
    } else {
        self->status = http_post_status(post);
        body = http_post_body(post, &body_size);
        self->succeeded = owned_text_assign(&self->data, text_view(body, body_size));
    }
    http_post_free(post);
    free(world);
    return 0;
}

void emblem_upload_work_done(struct EmblemUploadWork *self)
{
    emblem_mgr_on_uploaded(g_emblemMgr, self->succeeded, owned_text_view(&self->response), self->gdid);
}

char emblem_upload_work_run(struct EmblemUploadWork *self)
{
    char aid[0x40], gdid[0x40];
    char extension[4];
    char *world, *path, *type, *body;
    struct HttpPost *post;
    size_t i;

    if (self->path.size < 3) {
        self->succeeded = 0;
        return 0;
    }
    memset(aid, 0, sizeof aid);
    _snprintf_s(aid, sizeof aid, (size_t)-1, "%lu", self->aid);
    memset(gdid, 0, sizeof gdid);
    _snprintf_s(gdid, sizeof gdid, (size_t)-1, "%lu", self->gdid);
    world = http_text_to_utf8(owned_text_cstr(&self->world));
    path = http_text_to_utf8(owned_text_cstr(&self->path));
    for (i = 0; i < 3; ++i)
        extension[i] = (char)toupper((unsigned char)self->path.data[self->path.size - 3 + i]);
    extension[3] = 0;
    type = http_text_to_utf8(extension);
    post = http_post_create(owned_text_cstr(&self->url));

    http_post_add_text(post, "AID", aid);
    http_post_add_text(post, "AuthToken", owned_text_cstr(&self->token));
    http_post_add_text(post, "WorldName", world);
    http_post_add_text(post, "GDID", gdid);
    http_post_add_text(post, "ImgType", type);
    http_post_add_file(post, "Img", path);

    if (http_post_perform(post) != 0 || (body = http_text_from_utf8(http_post_body(post, NULL))) == NULL) {
        self->succeeded = 0;
    } else {
        self->succeeded = owned_text_assign(&self->response, text_view_cstr(body));
        free(body);
    }
    http_post_free(post);
    free(type);
    free(path);
    free(world);
    return 0;
}

void emblem_mgr_on_downloaded(struct EmblemMgr *self, int succeeded, int status,
                                         struct TextView data, int gdid, int version)
{
    struct EmblemDataPacket *packet;
    int left, piece, pos;

    (void)self;

    if (succeeded && status == HTTP_OK) {
        packet = malloc(sizeof *packet + EMBLEM_PIECE_MAX);
        if (packet) {
            packet->packet_id = PACKET_EMBLEM_DATA;
            packet->len = sizeof *packet;
            packet->kind = EMBLEM_KIND_START;
            packet->gdid = gdid;
            packet->version = version;
            connection_unread(rag_connection_transport(rag_connection_instance()), sizeof *packet, packet);
            for (left = (int)data.size; left > 0; left = pos) {
                piece = left > EMBLEM_PIECE_MAX ? EMBLEM_PIECE_MAX : left;
                pos = left - piece;
                packet->gdid = gdid;
                packet->kind = EMBLEM_KIND_DATA;
                packet->packet_id = PACKET_EMBLEM_DATA;
                packet->len = (short)(piece + sizeof *packet);
                packet->version = version;
                memcpy_s(packet + 1, packet->len - (int)sizeof *packet, data.data + pos, piece);
                connection_unread(rag_connection_transport(rag_connection_instance()), packet->len, packet);
            }
            packet->packet_id = PACKET_EMBLEM_DATA;
            packet->len = sizeof *packet;
            packet->kind = EMBLEM_KIND_END;
            packet->gdid = gdid;
            packet->version = version;
            connection_unread(rag_connection_transport(rag_connection_instance()), sizeof *packet, packet);
            free(packet);
        }
    }
}

void emblem_mgr_on_uploaded(struct EmblemMgr *self, int succeeded,
                                       struct TextView response, int gdid)
{
    struct JsonTree root;

    struct EmblemUploadedPacket packet;
    char parsed;
    int type, len;

    if (succeeded) {
        json_tree_init(&root);

        parsed = json_parse_text(&root, response.data, response.size) && json_value_is_object(&root);
        if (parsed) {
            type = json_value_as_int(json_value_member(&root, "Type"));
            if (type == UPLOAD_TYPE_DONE) {
                packet.version = json_value_as_int(json_value_member(&root, "version"));
                packet.packet_id = PACKET_EMBLEM_UPLOADED;
                packet.gdid = gdid;
                len = rag_connection_get_packet_length(rag_connection_instance(), PACKET_EMBLEM_UPLOADED);
                rag_connection_send_packet(rag_connection_instance(), len, &packet);
            } else if ((unsigned)(type - 2) <= 2) {
                ui_window_mgr_message_box(&g_windowMgr, msg_str(MSG_EMBLEM_UPLOAD_FAILED), 0, 1, 0, 0,
                                       "메시지", 0x118, 0x78, 0);
            }
            self->upload_blocked = 0;
        }

        json_tree_free(&root);
    }
}

#include "game/replay_actor_events.h"
#include "game/gamemode.h"
#include "replay/replay.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

static size_t text_length(const char *text, size_t capacity)
{
    size_t size = 0;
    if (text)
        while (size < capacity && text[size])
            ++size;
    return size;
}

void game_mode_record_whisper(struct GameMode *mode, const char *receiver, const char *text)
{
    struct ReplayRecorder *recorder = replay_recorder_instance();
    struct ReplayWhisper packet = {{0x8ae, 0x38a, sizeof packet}};
    (void)mode;
    if (recorder->state != REPLAY_STATE_RECORDING || !recorder->info.options[6])
        return;
    if (receiver)
        memcpy(packet.receiver, receiver, text_length(receiver, sizeof packet.receiver));
    if (text)
        memcpy(packet.text, text, text_length(text, sizeof packet.text));
    replay_recorder_write_packet(recorder, sizeof packet, &packet, REPLAY_TIME_NOW, 0);
}

char game_mode_record_mode_msg(struct GameMode *mode, int message, int a, int b, int c)
{
    struct ReplayRecorder *recorder = replay_recorder_instance();
    struct ReplayModeMessage packet = {{0x8ae, 0x389, sizeof packet}, 0x45, 10000, b, c};
    (void)mode;
    if (recorder->state != REPLAY_STATE_RECORDING || message != 0x45 || a != 10000)
        return 0;
    replay_recorder_write_packet(recorder, sizeof packet, &packet, REPLAY_TIME_NOW, 0);
    return !recorder->writeFailed;
}

void game_mode_record_own_chat_room(struct GameMode *mode)
{
    struct ReplayRecorder *recorder = replay_recorder_instance();
    const struct ChatRoomInfo *room = &mode->own_chat_room;
    struct ReplayOwnRoom header = {{0x8ae, 0x388, 0}};
    unsigned char *packet;
    size_t size;
    if (recorder->state != REPLAY_STATE_RECORDING)
        return;
    if (room->title.size > INT16_MAX - sizeof header ||
        room->password.size > INT16_MAX - sizeof header - room->title.size) {
        recorder->writeFailed = 1;
        return;
    }
    size = sizeof header + room->title.size + room->password.size;
    packet = malloc(size);
    if (!packet) {
        recorder->writeFailed = 1;
        return;
    }
    header.header.size = (uint16_t)size;
    header.title_size = (int32_t)room->title.size;
    header.password_size = (int32_t)room->password.size;
    header.type = room->type;
    header.users = room->users;
    header.max_users = room->max_users;
    header.room_id = room->room_id;
    memcpy(packet, &header, sizeof header);
    memcpy(packet + sizeof header, string_cstr(&room->title), room->title.size);
    memcpy(packet + sizeof header + room->title.size, string_cstr(&room->password), room->password.size);
    replay_recorder_write_packet(recorder, (unsigned short)size, packet, REPLAY_TIME_NOW, 0);
    free(packet);
}

static size_t bounded_text_length(const char *text, size_t size)
{
    const char *end = memchr(text, 0, size);
    return end ? (size_t)(end - text) : size;
}

int game_replay_whisper(struct GameMode *self, const void *data, size_t size)
{
    const struct ReplayWhisper *packet = data;
    if (!packet || size != sizeof *packet || packet->header.id != 0x8ae ||
        packet->header.kind != 0x38a || packet->header.size != size)
        return 0;
    string_assign(&self->whisper_target, packet->receiver,
                      bounded_text_length(packet->receiver, sizeof packet->receiver));
    string_assign(&self->whisper_text, packet->text,
                      bounded_text_length(packet->text, sizeof packet->text));
    return 1;
}

int game_replay_chat_room(struct GameMode *self, const void *data, size_t size)
{
    const struct ReplayOwnRoom *packet = data;
    const char *title, *password;
    size_t title_size, password_size;
    if (!packet || size < sizeof *packet || packet->header.id != 0x8ae ||
        packet->header.kind != 0x388 || packet->header.size != size ||
        packet->title_size < 0 || packet->password_size < 0)
        return 0;
    title_size = (size_t)packet->title_size;
    password_size = (size_t)packet->password_size;
    if (title_size > size - sizeof *packet || password_size != size - sizeof *packet - title_size)
        return 0;
    title = (const char *)(packet + 1);
    password = title + title_size;
    string_assign(&self->own_chat_room.title, title, bounded_text_length(title, title_size));
    string_assign(&self->own_chat_room.password, password, bounded_text_length(password, password_size));
    self->own_chat_room.type = packet->type;
    self->own_chat_room.users = packet->users;
    self->own_chat_room.max_users = packet->max_users;
    self->own_chat_room.room_id = packet->room_id;
    return 1;
}

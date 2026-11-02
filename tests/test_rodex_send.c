#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "owned_storage_test_allocator.h"
#include "../src/game/msg_rodex.c"

struct Session g_session1;
struct UIWindowMgr g_windowMgr;
static unsigned sent, closed, reset_items, reset_counts, alerts;
static unsigned char packet_bytes[8192];
static size_t packet_size;

void *mem_alloc(size_t size) { return checked_malloc(size); }
void mem_free(void *memory)
{ checked_free(memory); }

void fatal_error(const char *message) { (void)message; abort(); }

struct RagConnection *rag_connection_instance(void)
{ return (struct RagConnection *)&sent; }
char rag_connection_send_packet(struct RagConnection *connection, int length, const void *bytes)
{
    assert(connection == rag_connection_instance());
    assert(length >= 68 && length <= sizeof packet_bytes);
    memcpy(packet_bytes, bytes, length);
    packet_size = length;
    ++sent;
    return 1;
}
int rag_connection_get_packet_length(struct RagConnection *connection, int id)
{ (void)connection; (void)id; abort(); }
const char *session_character_name(const struct Session *session)
{ assert(session == &g_session1); return "Sender"; }
void session_reset_mail_items(struct Session *session)
{ assert(session == &g_session1); ++reset_items; }
void session_clear_mail_item_counts(struct Session *session)
{ assert(session == &g_session1); ++reset_counts; }
unsigned char ui_window_mgr_post_quit1(struct UIWindowMgr *manager, int id)
{ assert(manager == &g_windowMgr && id == WID_WRITE_MAIL); ++closed; return 1; }
char *msg_str(int id) { assert(id == 0xbd4); return "Rejected"; }
int ui_window_mgr_message_box(struct UIWindowMgr *manager, const char *text, int type,
                          int a, int b, int c, const char *title, int width, int height, int d)
{
    assert(manager == &g_windowMgr && !strcmp(text, "Rejected"));
    (void)type; (void)a; (void)b; (void)c; (void)title; (void)width; (void)height; (void)d;
    ++alerts;
    return 0;
}
struct GameMode *game_current_mode(void) { abort(); }

struct ItemInfo *item_info_copy(struct ItemInfo *a, const struct ItemInfo *b)
{ (void)a; (void)b; abort(); }
struct String *item_info_get_display_name(const struct ItemInfo *item, struct String *out)
{ (void)item; (void)out; abort(); }
void store_search_item_copy(void *a, const void *b) { (void)a; (void)b; abort(); }
void store_search_items_destroy(void *a, void *b) { (void)a; (void)b; abort(); }
char *std_copy_memmove(const char *b, const char *c, char *d)
{  (void)b; (void)c; (void)d; abort(); }

static struct String borrowed(const char *text)
{
    struct String value;
    memset(&value, 0, sizeof value);
    value.size = value.capacity = strlen(text);
    value.data = (char *)text;
    return value;
}
static unsigned u16(size_t offset)
{ unsigned short value; memcpy(&value, packet_bytes + offset, 2); return value; }
static void check_mail(const char *title, const char *body, int zeny)
{
    struct RodexMail mail;
    __int64 amount;
    int receiver;
    unsigned before = sent;
    size_t title_size = strlen(title) + 1, body_size = strlen(body) + 1;
    mail.receiver_char_id = 123456;
    mail.receiver = borrowed("Receiver");
    mail.title = borrowed(title);
    mail.body = borrowed(body);
    mail.zeny = zeny;
    assert(game_mode_msg_send_rodex(&mail) == 0);
    assert(sent == before + 1 && closed == sent && reset_items == sent && reset_counts == sent);
    assert(packet_size == 68 + title_size + body_size);
    assert(u16(0) == CZ_REQ_WRITE_MAIL && u16(2) == packet_size);
    assert(!strcmp((char *)packet_bytes + 4, "Receiver"));
    assert(!strcmp((char *)packet_bytes + 28, "Sender"));
    memcpy(&amount, packet_bytes + 52, 8); assert(amount == zeny);
    assert(u16(60) == title_size && u16(62) == body_size);
    memcpy(&receiver, packet_bytes + 64, 4); assert(receiver == 123456);
    assert(!memcmp(packet_bytes + 68, title, title_size));
    assert(!memcmp(packet_bytes + 68 + title_size, body, body_size));
    assert(!outstanding);
}
int main(void)
{
    char body[5001];
    check_mail("", "", 0);
    check_mail("Title", "Body", 100);
    check_mail("A title stored outside the inline buffer", "Korean bytes: \xb0\xa1\xb3\xaa", -10);
    memset(body, 'x', sizeof body - 1); body[sizeof body - 1] = 0;
    check_mail("Large body", body, 2147483647);
    check_mail("Blocked BAD title", "Bad words keep Their CASE", 0);
    assert(!alerts && !outstanding);
    puts("RODEX send: actual packet header, title/body terminators, inline/heap text, growth, unchanged text and allocation cleanup passed.");
    return 0;
}

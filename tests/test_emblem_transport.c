#define main bitmap_owner_main
#include "test_bitmap_owner.c"
#undef main
#define malloc bitmap_test_malloc
#define free bitmap_test_free
#include "../src/contents/guild_emblem_transport.c"
#undef malloc
#undef free
#define timeGetTime bitmap_test_now
#define platform_ticks_ms bitmap_test_now
#include "../src/contents/guild_emblem_req.c"
#undef timeGetTime
#undef platform_ticks_ms

static int connected = 1, accepts = 1, sends, sent_size;
static unsigned char sent_bytes[GUILD_EMBLEM_MAX_SIZE + 4];
struct RagConnection *rag_connection_instance(void) { return (struct RagConnection *)1; }
struct Connection *rag_connection_transport(struct RagConnection *c)
{ assert(c == (struct RagConnection *)1); return (struct Connection *)2; }
int connection_is_open(const struct Connection *c)
{ assert(c == (struct Connection *)2); return connected; }
char rag_connection_send_packet(struct RagConnection *c, int length, const void *packet)
{
    assert(c == (struct RagConnection *)1 && length > 0 && length <= sizeof sent_bytes);
    ++sends; sent_size = length; memcpy(sent_bytes, packet, length); return (char)accepts;
}
void *mem_try_alloc(size_t size) { return bitmap_test_malloc(size); }

static void assert_upload(const BYTE *data, size_t size)
{
    unsigned short id, length;
    assert(guild_emblem_upload(data, size));
    memcpy(&id, sent_bytes, 2); memcpy(&length, sent_bytes + 2, 2);
    assert(id == 0x0153 && length == size + 4 && sent_size == length);
    assert(!memcmp(sent_bytes + 4, data, size) && !outstanding);
}

int main(void)
{
    BYTE bmp[54 + 24 * 24 * 3] = {0}, gif[sizeof gif_disposal_data];
    struct GuildEmblemReqBundle *queue;
    unsigned short id;
    int guild_id, version, before;
    unsigned n, allocations;
    bmp[0] = 'B'; bmp[1] = 'M'; bmp[2] = sizeof bmp & 255; bmp[3] = sizeof bmp >> 8;
    bmp[10] = 54; bmp[14] = 40; bmp[18] = bmp[22] = 24; bmp[26] = 1; bmp[28] = 24;
    memcpy(gif, gif_disposal_data, sizeof gif);
    gif[6] = gif[8] = 24; gif[7] = gif[9] = 0;
    assert_upload(bmp, sizeof bmp);
    assert_upload(gif, sizeof gif);
    allocation_calls = 0;
    assert_upload(gif, sizeof gif);
    allocations = allocation_calls;
    for (n = 1; n <= allocations; ++n) {
        before = sends; allocation_calls = 0; fail_at = n;
        assert(!guild_emblem_upload(gif, sizeof gif));
        assert(sends == before && !outstanding);
    }
    fail_at = 0;
    before = sends;
    assert(!guild_emblem_upload(NULL, 1));
    assert(!guild_emblem_upload(bmp, 0));
    assert(!guild_emblem_upload(bmp, GUILD_EMBLEM_MAX_SIZE + 1));
    for (n = 0; n < sizeof gif; ++n)
        assert(!guild_emblem_upload(gif, n));
    bmp[18] = 23; assert(!guild_emblem_upload(bmp, sizeof bmp)); bmp[18] = 24;
    assert(sends == before && !outstanding);
    connected = 0;
    assert(!guild_emblem_upload(bmp, sizeof bmp) && !guild_emblem_request(7, 9));
    assert(sends == before);
    connected = 1;
    assert(guild_emblem_request(7, 9));
    memcpy(&id, sent_bytes, 2); memcpy(&guild_id, sent_bytes + 2, 4); memcpy(&version, sent_bytes + 6, 4);
    assert(id == 0x0b46 && sent_size == 10 && guild_id == 7 && version == 9);
    accepts = 0;
    assert(!guild_emblem_upload(bmp, sizeof bmp) && !outstanding);
    queue = emblem_bundle_create(); assert(queue);
    before = sends;
    emblem_bundle_queue(queue, 7, 9);
    emblem_bundle_process(queue);
    assert(sends == before + 1);
    accepts = 1;
    emblem_bundle_process(queue);
    assert(sends == before + 2);
    emblem_bundle_queue(queue, 7, 9);
    emblem_bundle_process(queue);
    assert(sends == before + 2);
    emblem_bundle_destroy(queue);
    assert(!outstanding);
    puts("PASS: actual BMP/GIF validation and wire bytes, malformed/truncated images, every allocation failure, disconnected/rejected sends, queued retry and deduplication; no HTTP linked.");
    return 0;
}

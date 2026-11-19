#include "delivery/delivery.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* The delivery piece alone: two ends in this program and a wire between them that the test
 * drops, repeats, delays and reorders by a seed. The checks are the ones the client
 * repository's plans/delivery.md lists under "What can be shown to hold". */

static int g_checks, g_failed;

#define CHECK(condition)                                                        \
    do {                                                                        \
        g_checks++;                                                             \
        if (!(condition)) {                                                     \
            g_failed++;                                                         \
            if (g_failed <= 20)                                                 \
                printf("FAILED line %d: %s\n", __LINE__, #condition);           \
        }                                                                       \
    } while (0)

static uint32_t g_seed;

static uint32_t rnd(void)
{
    g_seed ^= g_seed << 13;
    g_seed ^= g_seed >> 17;
    g_seed ^= g_seed << 5;
    return g_seed;
}

static struct Delivery *make_end(uint32_t out_messages, uint32_t out_bytes, uint32_t in_bytes, uint32_t rate,
                                 uint32_t now)
{
    struct DeliveryConfig config;
    size_t size;

    config.out_messages = out_messages;
    config.out_bytes = out_bytes;
    config.in_bytes = in_bytes;
    config.bytes_per_second = rate;
    size = delivery_size(&config);
    return size ? delivery_init(malloc(size), size, &config, now) : NULL;
}

static unsigned get16(const unsigned char *p)
{
    return (unsigned)p[0] | (unsigned)p[1] << 8;
}

static uint32_t get32(const unsigned char *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static void put32(unsigned char *p, uint32_t value)
{
    p[0] = (unsigned char)value;
    p[1] = (unsigned char)(value >> 8);
    p[2] = (unsigned char)(value >> 16);
    p[3] = (unsigned char)(value >> 24);
}

/* The messages of a datagram, as the header says they lie. Returns how many, -1 for a datagram
   that is not shaped right. */
struct Parsed {
    int kind;
    unsigned number, size;
    const unsigned char *bytes;
};

static int parse(const unsigned char *datagram, size_t size, struct Parsed *out, int room)
{
    size_t at = 9;
    int count = 0;

    if (size < 9)
        return -1;
    while (at < size) {
        unsigned head = datagram[at] == DELIVERY_REPLACEABLE ? 3u : 5u;

        if (datagram[at] > DELIVERY_ORDERED || size - at < head || count == room)
            return -1;
        out[count].kind = datagram[at];
        out[count].number = head == 5u ? get16(datagram + at + 1) : 0;
        out[count].size = get16(datagram + at + head - 2);
        out[count].bytes = datagram + at + head;
        if (size - at - head < out[count].size)
            return -1;
        at += head + out[count].size;
        count++;
    }
    return count;
}

/* ---- the first exchange, and the bytes of it ---- */

static void test_first_exchange(void)
{
    static const unsigned char ordered[] = { 0xa1, 0xa2, 0xa3 }, single[] = { 0xb1 }, replaceable[] = { 0xc1, 0xc2 };
    static const unsigned char first[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* nothing received, datagram 0 */
        0x02, 0x00, 0x00, 0x03, 0x00, 0xa1, 0xa2, 0xa3,       /* ordered 0 */
        0x00, 0x02, 0x00, 0xc1, 0xc2,                         /* replaceable, priority 0 */
        0x01, 0x00, 0x00, 0x01, 0x00, 0xb1,                   /* single 0, priority 1 */
    };
    static const unsigned char answer[] = { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    struct Delivery *a = make_end(1, 1024, 1200, 0, 1000), *b = make_end(4, 1024, 1200, 0, 1000);
    struct Delivery *c = make_end(4, 1024, 1200, 0, 1000), *e = make_end(4, 1024, 1200, 0, 1000);
    unsigned char datagram[DELIVERY_DATAGRAM_MAX];
    const unsigned char *message;
    size_t size, n;

    CHECK(a && b && c && delivery_check(a) && delivery_check(b));
    CHECK(delivery_make(a, 1000, datagram, sizeof datagram) == 0);
    CHECK(delivery_take(a, &size) == NULL);
    CHECK(delivery_waiting(c) == 0);

    CHECK(delivery_send(c, DELIVERY_ORDERED, 0, 0, 0, ordered, sizeof ordered));
    CHECK(delivery_send(c, DELIVERY_SINGLE, 0, 0, 1, single, sizeof single));
    CHECK(delivery_send(c, DELIVERY_REPLACEABLE, 7, 5, 0, replaceable, sizeof replaceable));
    CHECK(delivery_waiting(c) == 3);
    /* a replaced message waits no more */
    CHECK(delivery_send(c, DELIVERY_REPLACEABLE, 7, 9, 0, replaceable, sizeof replaceable));
    CHECK(delivery_send(c, DELIVERY_REPLACEABLE, 7, 5, 0, replaceable, sizeof replaceable));
    CHECK(delivery_waiting(c) == 3);
    n = delivery_make(c, 1000, datagram, sizeof datagram);
    CHECK(n == sizeof first && memcmp(datagram, first, n) == 0);
    CHECK(delivery_arrived(b, 1001, datagram, n) == 1);
    CHECK(delivery_arrived(b, 1001, datagram, n) == 0); /* the same one again */
    message = delivery_take(b, &size);
    CHECK(message && size == 3 && memcmp(message, ordered, 3) == 0);
    message = delivery_take(b, &size);
    CHECK(message && size == 2 && memcmp(message, replaceable, 2) == 0);
    message = delivery_take(b, &size);
    CHECK(message && size == 1 && memcmp(message, single, 1) == 0);
    CHECK(delivery_take(b, &size) == NULL);
    n = delivery_make(b, 1002, datagram, sizeof datagram);
    CHECK(n == sizeof answer && memcmp(datagram, answer, n) == 0);
    CHECK(delivery_make(b, 1002, datagram, sizeof datagram) == 0);
    CHECK(delivery_arrived(c, 1003, datagram, n) == 1);
    CHECK(delivery_make(c, 1100, datagram, sizeof datagram) == 0); /* all three are done */
    CHECK(delivery_waiting(c) == 0);

    /* an end with room for one message has it again once that one is acknowledged */
    CHECK(delivery_send(a, DELIVERY_SINGLE, 0, 0, 0, single, 1));
    CHECK(!delivery_send(a, DELIVERY_SINGLE, 0, 0, 0, single, 1));
    n = delivery_make(a, 1000, datagram, sizeof datagram);
    CHECK(n == 9 + 5 + 1 && delivery_arrived(e, 1001, datagram, n) == 1);
    n = delivery_make(e, 1002, datagram, sizeof datagram);
    CHECK(n == 9 && delivery_arrived(a, 1003, datagram, n) == 1);
    CHECK(delivery_send(a, DELIVERY_SINGLE, 0, 0, 0, single, 1));
    CHECK(delivery_check(a) && delivery_check(b) && delivery_check(c) && delivery_check(e));
    free(a);
    free(b);
    free(c);
    free(e);
}

/* ---- what is refused ---- */

static void test_refusals(void)
{
    struct DeliveryConfig config = { 8, 4096, 1200, 0 };
    unsigned char bytes[DELIVERY_MESSAGE_MAX + 1] = { 0 }, datagram[DELIVERY_DATAGRAM_MAX + 1] = { 0 };
    unsigned char memory[64];
    struct Delivery *a = make_end(8, 4096, 1200, 0, 0);
    int i;

    CHECK(delivery_size(&config) > 0 && delivery_size(&config) % 4 == 0);
    config.out_messages = 0;
    CHECK(delivery_size(&config) == 0);
    config.out_messages = 4097;
    CHECK(delivery_size(&config) == 0);
    config.out_messages = 8;
    config.out_bytes = DELIVERY_MESSAGE_MAX - 1;
    CHECK(delivery_size(&config) == 0);
    config.out_bytes = 4096;
    config.in_bytes = DELIVERY_DATAGRAM_MAX - 1;
    CHECK(delivery_size(&config) == 0);
    config.in_bytes = 1200;
    config.bytes_per_second = 4000001;
    CHECK(delivery_size(&config) == 0);
    config.bytes_per_second = 0;
    CHECK(delivery_init(memory, sizeof memory, &config, 0) == NULL);
    CHECK(delivery_init(NULL, 1 << 20, &config, 0) == NULL);
    CHECK(delivery_check(NULL) == 0);

    CHECK(!delivery_send(a, 3, 0, 0, 0, bytes, 1));
    CHECK(!delivery_send(a, -1, 0, 0, 0, bytes, 1));
    CHECK(!delivery_send(a, DELIVERY_SINGLE, 0, 0, DELIVERY_PRIORITIES, bytes, 1));
    CHECK(!delivery_send(a, DELIVERY_SINGLE, 0, 0, -1, bytes, 1));
    CHECK(!delivery_send(a, DELIVERY_SINGLE, 0, 0, 0, bytes, 0));
    CHECK(!delivery_send(a, DELIVERY_SINGLE, 0, 0, 0, bytes, DELIVERY_MESSAGE_MAX + 1));
    CHECK(!delivery_send(a, DELIVERY_SINGLE, 0, 0, 0, NULL, 1));
    /* the bytes run out before the slots do */
    for (i = 0; i < 4; i++)
        CHECK(delivery_send(a, DELIVERY_SINGLE, 0, 0, 0, bytes, DELIVERY_MESSAGE_MAX));
    CHECK(!delivery_send(a, DELIVERY_SINGLE, 0, 0, 0, bytes, 1));
    CHECK(delivery_make(a, 0, datagram, DELIVERY_DATAGRAM_MAX - 1) == 0);
    CHECK(delivery_make(a, 0, NULL, DELIVERY_DATAGRAM_MAX) == 0);
    CHECK(delivery_arrived(a, 0, NULL, 9) == 0);
    CHECK(delivery_arrived(a, 0, datagram, 8) == 0);
    CHECK(delivery_arrived(a, 0, datagram, DELIVERY_DATAGRAM_MAX + 1) == 0);
    datagram[0] = 2; /* a flag nobody knows */
    CHECK(delivery_arrived(a, 0, datagram, 9) == 0);
    datagram[0] = 1; /* an acknowledgement of a datagram that was never made */
    CHECK(delivery_arrived(a, 0, datagram, 9) == 0);
    datagram[0] = 0;
    datagram[9] = DELIVERY_SINGLE; /* a message of no bytes */
    CHECK(delivery_arrived(a, 0, datagram, 14) == 0);
    datagram[12] = 5; /* a message longer than the datagram */
    CHECK(delivery_arrived(a, 0, datagram, 14 + 4) == 0);
    datagram[9] = 3; /* a kind nobody knows */
    CHECK(delivery_arrived(a, 0, datagram, 14 + 5) == 0);
    datagram[9] = DELIVERY_SINGLE;
    CHECK(delivery_arrived(a, 0, datagram, 14 + 5) == 1);
    CHECK(delivery_check(a));
    free(a);
}

/* ---- what a replaceable message replaces ---- */

/* The letters of the messages `end` puts into its next datagram, in their order. */
static void letters(struct Delivery *end, uint32_t now, char *out)
{
    unsigned char datagram[DELIVERY_DATAGRAM_MAX];
    struct Parsed messages[32];
    int count = parse(datagram, delivery_make(end, now, datagram, sizeof datagram), messages, 32), i;

    for (i = 0; i < count; i++)
        out[i] = (char)messages[i].bytes[0];
    out[count > 0 ? count : 0] = 0;
}

static void test_replacing(void)
{
    const uint64_t here = ((uint64_t)7 << 32) | 5, other_low = ((uint64_t)7 << 32) | 6, other_high = ((uint64_t)8 << 32) | 5;
    struct Delivery *a = make_end(16, 4096, 1200, 0, 0);
    char said[33];

    /* a later record leaves an earlier one alone; one at the same tick or before takes all
       from its tick on */
    CHECK(delivery_send(a, DELIVERY_REPLACEABLE, here, 10, 0, "a", 1));
    CHECK(delivery_send(a, DELIVERY_REPLACEABLE, here, 20, 0, "b", 1));
    CHECK(delivery_send(a, DELIVERY_REPLACEABLE, here, 30, 0, "c", 1));
    CHECK(delivery_send(a, DELIVERY_REPLACEABLE, here, 20, 0, "d", 1));
    /* other keys, and other kinds with the same key, are not touched */
    CHECK(delivery_send(a, DELIVERY_REPLACEABLE, other_low, 0, 0, "e", 1));
    CHECK(delivery_send(a, DELIVERY_REPLACEABLE, other_high, 0, 0, "f", 1));
    CHECK(delivery_send(a, DELIVERY_SINGLE, here, 99, 0, "g", 1));
    CHECK(delivery_send(a, DELIVERY_ORDERED, here, 99, 0, "h", 1));
    CHECK(delivery_send(a, DELIVERY_REPLACEABLE, here, 20, 0, "i", 1));
    letters(a, 0, said);
    CHECK(strcmp(said, "haefgi") == 0);
    /* one that flies is replaced as well: lost, it is not sent again */
    CHECK(delivery_send(a, DELIVERY_REPLACEABLE, here, 0, 0, "j", 1));
    letters(a, 5000, said);
    CHECK(strcmp(said, "hefgj") == 0);
    /* ticks are compared by their difference: one before the wrap is earlier than one after */
    CHECK(delivery_send(a, DELIVERY_REPLACEABLE, other_low, 0xfffffff0u, 0, "k", 1));
    CHECK(delivery_send(a, DELIVERY_REPLACEABLE, other_low, 0x10u, 0, "l", 1));
    letters(a, 5001, said);
    CHECK(strcmp(said, "kl") == 0);
    CHECK(delivery_send(a, DELIVERY_REPLACEABLE, other_low, 0xfffffff0u, 0, "m", 1));
    letters(a, 20000, said);
    CHECK(strcmp(said, "hfgjm") == 0);
    CHECK(delivery_check(a));
    free(a);
}

/* ---- a world of two ends and a wire ---- */

#define WIRE_FLIGHTS 8192

struct Flight {
    uint32_t due;
    int to;
    size_t size;
    unsigned char bytes[DELIVERY_DATAGRAM_MAX];
};

struct Wire {
    struct Flight *flights;
    int count;
    unsigned loss_pct, repeat_pct;
    uint32_t delay_min, delay_max;
};

static void wire_put(struct Wire *wire, uint32_t now, int to, const unsigned char *bytes, size_t size)
{
    int copies = 1 + (rnd() % 100 < wire->repeat_pct);

    if (rnd() % 100 < wire->loss_pct)
        return;
    while (copies-- && wire->count < WIRE_FLIGHTS) {
        struct Flight *flight = &wire->flights[wire->count++];

        flight->due = now + wire->delay_min + rnd() % (wire->delay_max - wire->delay_min + 1);
        flight->to = to;
        flight->size = size;
        memcpy(flight->bytes, bytes, size);
    }
}

/* What one end sends the other, and what of it arrived. A message's bytes: its kind's letter,
   its fields, and filler that follows from the fields. */
#define KEYS 6
#define TICKS 16
#define SINGLES 60000

struct Record {
    unsigned rev;
    uint32_t tick;
};

struct Records {
    struct Record held[TICKS * 4];
    int count;
};

struct Traffic {
    uint32_t ordered_sent, ordered_got, single_sent, single_got;
    unsigned char *single_times; /* how often each single message arrived */
    unsigned rev[KEYS];
    struct Records truth[KEYS], got[KEYS]; /* the records that count at the sender, the receiver's merge */
    int refused;
};

static size_t compose(unsigned char *out, int letter, uint32_t a, uint32_t b, uint32_t c)
{
    size_t size = 13 + (rnd() % 8 ? rnd() % 40 : rnd() % (DELIVERY_MESSAGE_MAX - 13)), i;

    out[0] = (unsigned char)letter;
    put32(out + 1, a);
    put32(out + 5, b);
    put32(out + 9, c);
    for (i = 13; i < size; i++)
        out[i] = (unsigned char)(a * 31u + b * 7u + c + i);
    return size;
}

static int composed(const unsigned char *bytes, size_t size)
{
    uint32_t a = get32(bytes + 1), b = get32(bytes + 5), c = get32(bytes + 9);
    size_t i;

    for (i = 13; i < size; i++)
        if (bytes[i] != (unsigned char)(a * 31u + b * 7u + c + i))
            return 0;
    return size >= 13;
}

static int rev_newer(unsigned a, unsigned b)
{
    return (int16_t)(uint16_t)(a - b) > 0;
}

/* The sender's side of core-loop's rule 3: a record made removes the earlier ones with the same
   or a later tick. */
static void truth_file(struct Records *records, unsigned rev, uint32_t tick)
{
    int i, kept = 0;

    for (i = 0; i < records->count; i++)
        if ((int32_t)(records->held[i].tick - tick) < 0)
            records->held[kept++] = records->held[i];
    records->held[kept].rev = rev;
    records->held[kept].tick = tick;
    records->count = kept + 1;
}

/* The receiver's side, which must not depend on the order records arrive in (plans/delivery.md,
   part 1). */
static void merge_file(struct Records *records, unsigned rev, uint32_t tick)
{
    int i, kept = 0;

    for (i = 0; i < records->count; i++) {
        const struct Record *held = &records->held[i];

        if (held->rev == rev || (rev_newer(held->rev, rev) && (int32_t)(held->tick - tick) <= 0))
            return;
    }
    for (i = 0; i < records->count; i++)
        if (rev_newer(records->held[i].rev, rev) || (int32_t)(records->held[i].tick - tick) < 0)
            records->held[kept++] = records->held[i];
    CHECK(kept < TICKS * 4);
    records->held[kept].rev = rev;
    records->held[kept].tick = tick;
    records->count = kept + 1;
}

static int same_records(const struct Records *a, const struct Records *b)
{
    int i, j, found;

    if (a->count != b->count)
        return 0;
    for (i = 0; i < a->count; i++) {
        for (found = 0, j = 0; j < b->count; j++)
            found |= a->held[i].rev == b->held[j].rev && a->held[i].tick == b->held[j].tick;
        if (!found)
            return 0;
    }
    return 1;
}

static void send_something(struct Delivery *end, struct Traffic *traffic)
{
    unsigned char bytes[DELIVERY_MESSAGE_MAX];
    unsigned what = rnd() % 10, key;
    uint32_t tick;
    size_t size;

    if (what < 3) {
        size = compose(bytes, 'O', traffic->ordered_sent, 0, 0);
        if (delivery_send(end, DELIVERY_ORDERED, 0, 0, 0, bytes, size))
            traffic->ordered_sent++;
        else
            traffic->refused++;
    } else if (what < 6 && traffic->single_sent < SINGLES) {
        size = compose(bytes, 'S', traffic->single_sent, 0, 0);
        if (delivery_send(end, DELIVERY_SINGLE, 0, 0, (int)(rnd() % DELIVERY_PRIORITIES), bytes, size))
            traffic->single_sent++;
        else
            traffic->refused++;
    } else {
        key = rnd() % KEYS;
        tick = 0xfffffc00u + 100u * (rnd() % TICKS); /* the ticks wrap in the middle */
        size = compose(bytes, 'R', key, ++traffic->rev[key], tick);
        truth_file(&traffic->truth[key], traffic->rev[key] & 0xffffu, tick);
        if (!delivery_send(end, DELIVERY_REPLACEABLE, ((uint64_t)0x12345678u << 32) | key, tick,
                           (int)(rnd() % DELIVERY_PRIORITIES), bytes, size))
            traffic->refused++;
    }
}

static void got_something(struct Traffic *traffic, const unsigned char *bytes, size_t size)
{
    uint32_t a, b, c;

    CHECK(composed(bytes, size));
    if (size < 13)
        return;
    a = get32(bytes + 1);
    b = get32(bytes + 5);
    c = get32(bytes + 9);
    if (bytes[0] == 'O') {
        CHECK(a == traffic->ordered_got);
        traffic->ordered_got++;
    } else if (bytes[0] == 'S') {
        CHECK(a < traffic->single_sent && traffic->single_times[a] == 0);
        if (a < SINGLES && traffic->single_times[a]++ == 0)
            traffic->single_got++;
    } else {
        CHECK(bytes[0] == 'R' && a < KEYS);
        if (a < KEYS)
            merge_file(&traffic->got[a], b & 0xffffu, c);
    }
}

/* No datagram carries a record that a later one had replaced when it was made. */
static void nothing_stale(const struct Traffic *traffic, const unsigned char *datagram, size_t size)
{
    struct Parsed messages[512];
    int count = parse(datagram, size, messages, 512), i, j, counts;

    CHECK(count >= 0);
    for (i = 0; i < count; i++) {
        uint32_t key, rev, tick;

        if (messages[i].kind != DELIVERY_REPLACEABLE || messages[i].size < 13)
            continue;
        key = get32(messages[i].bytes + 1);
        rev = get32(messages[i].bytes + 5) & 0xffffu;
        tick = get32(messages[i].bytes + 9);
        for (counts = 0, j = 0; key < KEYS && j < traffic->truth[key].count; j++)
            counts |= traffic->truth[key].held[j].rev == rev && traffic->truth[key].held[j].tick == tick;
        CHECK(counts);
    }
}

static int all_arrived(const struct Traffic *traffic)
{
    int key;

    if (traffic->ordered_got != traffic->ordered_sent || traffic->single_got != traffic->single_sent)
        return 0;
    for (key = 0; key < KEYS; key++)
        if (!same_records(&traffic->truth[key], &traffic->got[key]))
            return 0;
    return 1;
}

/* Plays `busy_ms` of both ends sending over the wire as given, then lets the wire be quiet
   (nothing lost, nothing sent anew) until everything has arrived. */
static void play_world(uint32_t seed, unsigned loss_pct, unsigned repeat_pct, uint32_t delay_min, uint32_t delay_max,
                       uint32_t busy_ms, unsigned send_pct)
{
    struct Delivery *end[2];
    struct Traffic *traffic = calloc(2, sizeof *traffic); /* traffic[i]: what end i sends */
    struct Wire wire;
    unsigned char datagram[DELIVERY_DATAGRAM_MAX];
    uint32_t start = 0xffffe000u + seed * 977u, now, quiet = 0; /* the clock wraps on the way */
    const unsigned char *message;
    size_t size;
    int i, side, done = 0;

    g_seed = seed * 2654435761u + 1u;
    /* few enough bytes that the gaps among them are closed now and then */
    end[0] = make_end(2048, 1u << 18, 4096, 0, start);
    end[1] = make_end(2048, 1u << 18, 4096, 0, start);
    traffic[0].single_times = calloc(SINGLES, 1);
    traffic[1].single_times = calloc(SINGLES, 1);
    wire.flights = malloc(WIRE_FLIGHTS * sizeof *wire.flights);
    wire.count = 0;
    wire.loss_pct = loss_pct;
    wire.repeat_pct = repeat_pct;
    wire.delay_min = delay_min;
    wire.delay_max = delay_max;

    for (now = start; !done && quiet < 120000; now++) {
        int busy = now - start < busy_ms;

        if (!busy) {
            wire.loss_pct = 0;
            quiet++;
        }
        for (side = 0; busy && side < 2; side++)
            if (rnd() % 100 < send_pct)
                send_something(end[side], &traffic[side]);
        for (i = 0; i < wire.count;) {
            struct Flight *flight = &wire.flights[i];

            if ((int32_t)(now - flight->due) < 0) {
                i++;
                continue;
            }
            delivery_arrived(end[flight->to], now, flight->bytes, flight->size);
            while ((message = delivery_take(end[flight->to], &size)) != NULL)
                got_something(&traffic[!flight->to], message, size);
            wire.flights[i] = wire.flights[--wire.count];
        }
        for (side = 0; side < 2; side++) {
            while ((size = delivery_make(end[side], now, datagram, sizeof datagram)) != 0) {
                nothing_stale(&traffic[side], datagram, size);
                wire_put(&wire, now, !side, datagram, size);
            }
            CHECK(delivery_check(end[side]));
        }
        done = !busy && all_arrived(&traffic[0]) && all_arrived(&traffic[1]);
    }
    CHECK(done);
    CHECK(traffic[0].refused == 0 && traffic[1].refused == 0);
    CHECK(traffic[0].ordered_sent > 0 && traffic[0].single_sent > 0 && traffic[1].ordered_sent > 0);
    if (!done)
        printf("  world %u: ordered %u/%u %u/%u, single %u/%u %u/%u\n", (unsigned)seed, (unsigned)traffic[0].ordered_got,
               (unsigned)traffic[0].ordered_sent, (unsigned)traffic[1].ordered_got, (unsigned)traffic[1].ordered_sent,
               (unsigned)traffic[0].single_got, (unsigned)traffic[0].single_sent, (unsigned)traffic[1].single_got,
               (unsigned)traffic[1].single_sent);
    /* What is still on the wire never arrives. Whatever was not acknowledged is sent again,
       changes nothing that arrived, and then nothing more is said. */
    for (i = 0; i < 4000; i++, now++) {
        for (side = 0; side < 2; side++) {
            while ((size = delivery_make(end[side], now, datagram, sizeof datagram)) != 0) {
                CHECK(delivery_arrived(end[!side], now, datagram, size) == 1);
                while ((message = delivery_take(end[!side], &size)) != NULL)
                    got_something(&traffic[side], message, size);
            }
        }
    }
    CHECK(all_arrived(&traffic[0]) && all_arrived(&traffic[1]));
    for (side = 0; side < 2; side++)
        CHECK(delivery_make(end[side], now + 100000u, datagram, sizeof datagram) == 0);
    free(wire.flights);
    free(traffic[0].single_times);
    free(traffic[1].single_times);
    free(traffic);
    free(end[0]);
    free(end[1]);
}

/* ---- the numbers wrap ---- */

static void test_wraps(void)
{
    struct Delivery *a = make_end(4, 4096, 1200, 0, 0xfffff000u), *b = make_end(4, 4096, 1200, 0, 0xfffff000u);
    unsigned char datagram[DELIVERY_DATAGRAM_MAX], bytes[4];
    const unsigned char *message;
    uint32_t now = 0xfffff000u, i, wrong = 0;
    size_t size, n;

    for (i = 0; i < 140000; i++, now++) {
        int kind = i & 1 ? DELIVERY_ORDERED : DELIVERY_SINGLE;

        put32(bytes, i);
        wrong += !delivery_send(a, kind, 0, 0, 0, bytes, 4);
        n = delivery_make(a, now, datagram, sizeof datagram);
        wrong += n == 0 || delivery_arrived(b, now, datagram, n) != 1;
        message = delivery_take(b, &size);
        wrong += !message || size != 4 || get32(message) != i || delivery_take(b, &size) != NULL;
        n = delivery_make(b, now, datagram, sizeof datagram);
        wrong += n != 9 || delivery_arrived(a, now, datagram, n) != 1;
    }
    CHECK(wrong == 0);
    CHECK(delivery_check(a) && delivery_check(b));
    free(a);
    free(b);
}

/* ---- a lost datagram costs an ordered message one sending ---- */

static void test_one_sending(void)
{
    struct Delivery *a = make_end(8, 4096, 1200, 0, 5000), *b = make_end(8, 4096, 1200, 0, 5000);
    unsigned char datagram[DELIVERY_DATAGRAM_MAX], lost[DELIVERY_DATAGRAM_MAX];
    const unsigned char *message;
    uint32_t now;
    size_t size, n;

    CHECK(delivery_send(a, DELIVERY_ORDERED, 0, 0, 0, "first", 5));
    CHECK(delivery_make(a, 5000, lost, sizeof lost) == 9 + 5 + 5); /* and the wire drops it */
    for (now = 5001; now < 5015; now++)
        CHECK(delivery_make(a, now, datagram, sizeof datagram) == 0);
    n = delivery_make(a, 5015, datagram, sizeof datagram);
    CHECK(n == 9 + 5 + 5);
    /* a second request goes with the first, behind it, and at once */
    CHECK(delivery_send(a, DELIVERY_ORDERED, 0, 0, 0, "second", 6));
    n = delivery_make(a, 5021, datagram, sizeof datagram);
    CHECK(n == 9 + 5 + 5 + 5 + 6);
    CHECK(delivery_arrived(b, 5030, datagram, n) == 1);
    message = delivery_take(b, &size);
    CHECK(message && size == 5 && memcmp(message, "first", 5) == 0);
    message = delivery_take(b, &size);
    CHECK(message && size == 6 && memcmp(message, "second", 6) == 0);
    /* the acknowledgement of the last datagram is one of every datagram the two were in */
    n = delivery_make(b, 5031, datagram, sizeof datagram);
    CHECK(n == 9 && delivery_arrived(a, 5040, datagram, n) == 1);
    for (now = 5041; now < 6000; now++)
        CHECK(delivery_make(a, now, datagram, sizeof datagram) == 0);
    free(a);
    free(b);
}

/* ---- a datagram taken for lost that arrives after all ---- */

static void test_late_after_all(void)
{
    struct Delivery *a = make_end(2, 4096, 1200, 0, 0), *b = make_end(2, 4096, 1200, 0, 0);
    unsigned char early[DELIVERY_DATAGRAM_MAX], again[DELIVERY_DATAGRAM_MAX], answer[DELIVERY_DATAGRAM_MAX];
    size_t size, early_size, again_size = 0, n;
    uint32_t now;
    int singles = 0, replaceables = 0;
    const unsigned char *message;

    CHECK(delivery_send(a, DELIVERY_SINGLE, 0, 0, 0, "s", 1));
    CHECK(delivery_send(a, DELIVERY_REPLACEABLE, 1, 0, 0, "r", 1));
    early_size = delivery_make(a, 0, early, sizeof early);
    CHECK(early_size == 9 + 6 + 4);
    /* nothing comes back: after the first round trip assumed and four times its variance the
       two go out again */
    for (now = 1; now < 2000 && !again_size; now++)
        again_size = delivery_make(a, now, again, sizeof again);
    CHECK(now - 1 == 200 + 4 * 100 + 25 && again_size == early_size);
    CHECK(delivery_arrived(b, now, again, again_size) == 1);
    CHECK(delivery_arrived(b, now, early, early_size) == 1);
    while ((message = delivery_take(b, &size)) != NULL) {
        singles += *message == 's';
        replaceables += *message == 'r';
    }
    CHECK(singles == 1 && replaceables == 2);
    /* the answer names both datagrams, and the two messages are done */
    n = delivery_make(b, now, answer, sizeof answer);
    CHECK(n == 9 && get16(answer + 3) == 1 && get32(answer + 5) == 1);
    CHECK(!delivery_send(a, DELIVERY_SINGLE, 0, 0, 0, "t", 1));
    CHECK(delivery_arrived(a, now + 5, answer, n) == 1);
    CHECK(delivery_send(a, DELIVERY_SINGLE, 0, 0, 0, "t", 1) && delivery_send(a, DELIVERY_SINGLE, 0, 0, 0, "u", 1));
    free(a);
    free(b);
}

/* ---- when a datagram counts as lost ---- */

/* Two ends that have measured a round trip of 100 ms. */
static void warm_up(struct Delivery **a, struct Delivery **b)
{
    unsigned char datagram[DELIVERY_DATAGRAM_MAX];
    size_t size, n;

    *a = make_end(16, 4096, 1200, 0, 0);
    *b = make_end(16, 4096, 1200, 0, 0);
    CHECK(delivery_send(*a, DELIVERY_SINGLE, 0, 0, 0, "w", 1));
    n = delivery_make(*a, 0, datagram, sizeof datagram);
    CHECK(n && delivery_arrived(*b, 50, datagram, n) == 1 && delivery_take(*b, &size) != NULL);
    n = delivery_make(*b, 50, datagram, sizeof datagram);
    CHECK(n == 9 && delivery_arrived(*a, 100, datagram, n) == 1);
}

/* Sends one single message from `a` in a datagram of its own; `b` gets it and answers at
   `answer_ms` unless `drop`. */
static void one_each(struct Delivery *a, struct Delivery *b, uint32_t now, const char *letter, int drop,
                     uint32_t answer_ms)
{
    unsigned char datagram[DELIVERY_DATAGRAM_MAX];
    size_t size, n;

    CHECK(delivery_send(a, DELIVERY_SINGLE, 0, 0, 0, letter, 1));
    n = delivery_make(a, now, datagram, sizeof datagram);
    CHECK(n == 9 + 5 + 1 && datagram[14] == (unsigned char)*letter);
    if (drop)
        return;
    CHECK(delivery_arrived(b, now, datagram, n) == 1 && delivery_take(b, &size) != NULL);
    n = delivery_make(b, now, datagram, sizeof datagram);
    CHECK(n == 9 && delivery_arrived(a, answer_ms, datagram, n) == 1);
}

static void test_loss_rules(void)
{
    unsigned char datagram[DELIVERY_DATAGRAM_MAX], late[DELIVERY_DATAGRAM_MAX];
    struct Delivery *a, *b;
    uint32_t now, at[6];
    size_t size, n, late_size;
    int i;

    /* three later datagrams acknowledged */
    warm_up(&a, &b);
    one_each(a, b, 1000, "x", 1, 0);
    one_each(a, b, 1001, "1", 0, 1005);
    one_each(a, b, 1002, "2", 0, 1006);
    CHECK(delivery_make(a, 1007, datagram, sizeof datagram) == 0);
    one_each(a, b, 1008, "3", 0, 1009);
    n = delivery_make(a, 1009, datagram, sizeof datagram);
    CHECK(n == 9 + 5 + 1 && datagram[14] == 'x');
    free(a);
    free(b);

    /* the same when one answer names the three */
    warm_up(&a, &b);
    one_each(a, b, 1000, "x", 1, 0);
    for (i = 0; i < 3; i++) {
        CHECK(delivery_send(a, DELIVERY_SINGLE, 0, 0, 0, "n", 1));
        n = delivery_make(a, 1001 + (uint32_t)i, datagram, sizeof datagram);
        CHECK(n && delivery_arrived(b, 1001 + (uint32_t)i, datagram, n) == 1);
    }
    n = delivery_make(b, 1004, datagram, sizeof datagram);
    CHECK(n == 9 && delivery_arrived(a, 1005, datagram, n) == 1);
    n = delivery_make(a, 1005, datagram, sizeof datagram);
    CHECK(n == 9 + 5 + 1 && datagram[14] == 'x');
    free(a);
    free(b);

    /* a datagram taken for lost that is acknowledged before what it carried went out again:
       nothing goes out again */
    warm_up(&a, &b);
    CHECK(delivery_send(a, DELIVERY_SINGLE, 0, 0, 0, "x", 1));
    late_size = delivery_make(a, 1000, late, sizeof late);
    CHECK(late_size == 9 + 5 + 1);
    one_each(a, b, 1001, "1", 0, 1002);
    one_each(a, b, 1002, "2", 0, 1003);
    one_each(a, b, 1003, "3", 0, 1004); /* and the first counts as lost */
    CHECK(delivery_arrived(b, 1004, late, late_size) == 1);
    n = delivery_make(b, 1004, datagram, sizeof datagram);
    CHECK(n == 9 && delivery_arrived(a, 1004, datagram, n) == 1);
    for (now = 1004; now < 3000; now++)
        CHECK(delivery_make(a, now, datagram, sizeof datagram) == 0);
    free(a);
    free(b);

    /* a later one acknowledged, and nine eighths of the round trip gone: the round trip is 88
       after the sample of 5 ms (seven eighths of 100 and an eighth of 5), so that is 99 ms */
    warm_up(&a, &b);
    one_each(a, b, 1000, "x", 1, 0);
    one_each(a, b, 1001, "1", 0, 1006);
    for (now = 1006, n = 0; now < 2000 && !n; now++)
        n = delivery_make(a, now, datagram, sizeof datagram);
    CHECK(now - 1 == 1000 + 99 && n == 9 + 5 + 1 && datagram[14] == 'x');
    free(a);
    free(b);

    /* nothing comes back at all: the round trip, four times its variance and the time an
       answer may be kept, 100 + 200 + 25, and twice as long each time after, up to eight
       times */
    warm_up(&a, &b);
    CHECK(delivery_send(a, DELIVERY_SINGLE, 0, 0, 0, "x", 1));
    for (now = 1000, i = 0; now < 40000 && i < 6; now++)
        if (delivery_make(a, now, datagram, sizeof datagram))
            at[i++] = now - 1000;
    CHECK(i == 6 && at[0] == 0 && at[1] == 325 && at[2] == 325 + 650 && at[3] == 325 + 650 + 1300
          && at[4] == 325 + 650 + 1300 + 2600 && at[5] == 325 + 650 + 1300 + 2600 + 2600);
    /* an answer, and the wait is no longer doubled */
    CHECK(delivery_send(a, DELIVERY_SINGLE, 0, 0, 0, "y", 1));
    n = delivery_make(a, 40000, datagram, sizeof datagram);
    CHECK(n && delivery_arrived(b, 40000, datagram, n) == 1);
    while (delivery_take(b, &size) != NULL) {
    }
    n = delivery_make(b, 40000, datagram, sizeof datagram);
    CHECK(n == 9 && delivery_arrived(a, 40010, datagram, n) == 1);
    CHECK(delivery_send(a, DELIVERY_SINGLE, 0, 0, 0, "z", 1));
    for (now = 40010, i = 0; now < 60000 && i < 2; now++)
        if (delivery_make(a, now, datagram, sizeof datagram))
            at[i++] = now;
    /* the average after a sample of 10 ms: (7 * 100 + 10) / 8 = 88, the variance
       (3 * 50 + 90) / 4 = 60 */
    CHECK(i == 2 && at[0] == 40010 && at[1] == 40010 + 88 + 240 + 25);
    free(a);
    free(b);
}

/* ---- more datagrams fly than this end remembers ---- */

static void test_many_flying(void)
{
    struct Delivery *a = make_end(256, 8192, 1200, 0, 0), *b = make_end(4, 4096, 4096, 0, 0);
    unsigned char datagram[DELIVERY_DATAGRAM_MAX], times[200] = { 0 };
    const unsigned char *message;
    uint32_t now;
    size_t size, n;
    int i, got = 0;

    /* two hundred messages within a millisecond, each to go in a datagram of its own, and no
       datagram arrives: 128 fly, and a further one goes only when its time has come */
    for (i = 0; i < 200; i++) {
        unsigned char id = (unsigned char)i;

        CHECK(delivery_send(a, DELIVERY_SINGLE, 0, 0, 0, &id, 1));
        CHECK(delivery_make(a, 0, datagram, sizeof datagram) == (i < 128 ? 9u + 5u + 1u : 0u));
        CHECK(delivery_check(a));
    }
    for (now = 1; now < 20000 && got < 200; now++) {
        while ((n = delivery_make(a, now, datagram, sizeof datagram)) != 0)
            delivery_arrived(b, now, datagram, n);
        while ((message = delivery_take(b, &size)) != NULL) {
            CHECK(size == 1 && message[0] < 200 && times[message[0]]++ == 0);
            got++;
        }
        while ((n = delivery_make(b, now, datagram, sizeof datagram)) != 0)
            delivery_arrived(a, now, datagram, n);
        CHECK(delivery_check(a));
    }
    CHECK(got == 200);
    free(a);
    free(b);

    /* a datagram that flies keeps its place, however many datagrams without messages follow */
    a = make_end(4, 4096, 4096, 0, 0);
    b = make_end(4, 4096, 4096, 0, 0);
    CHECK(delivery_send(a, DELIVERY_SINGLE, 0, 0, 0, "x", 1));
    CHECK(delivery_make(a, 0, datagram, sizeof datagram) == 9 + 5 + 1); /* which is lost */
    for (i = 0; i < 300; i++) {
        CHECK(delivery_send(b, DELIVERY_SINGLE, 0, 0, 0, "y", 1));
        n = delivery_make(b, 0, datagram, sizeof datagram);
        CHECK(n == 9 + 5 + 1 && delivery_arrived(a, 0, datagram, n) == 1 && delivery_take(a, &size) != NULL);
        n = delivery_make(a, 0, datagram, sizeof datagram);
        CHECK(n == 9 && delivery_arrived(b, 0, datagram, n) == 1);
    }
    for (now = 1, got = 0; now < 5000; now++) {
        while ((n = delivery_make(a, now, datagram, sizeof datagram)) != 0)
            delivery_arrived(b, now, datagram, n);
        while ((message = delivery_take(b, &size)) != NULL)
            got += size == 1 && message[0] == 'x';
        while ((n = delivery_make(b, now, datagram, sizeof datagram)) != 0)
            delivery_arrived(a, now, datagram, n);
    }
    CHECK(got == 1 && delivery_check(a) && delivery_check(b));
    free(a);
    free(b);
}

/* ---- an end that hears nothing goes on asking, and is back when it is heard ---- */

static void test_silence(void)
{
    struct Delivery *a = make_end(512, 65536, 4096, 0, 0), *b = make_end(4, 4096, 65536, 0, 0);
    unsigned char datagram[DELIVERY_DATAGRAM_MAX], bytes[5];
    unsigned char *times = calloc(400, 1);
    const unsigned char *message;
    uint32_t now, ordered = 0, single = 0, made = 0, got_ordered = 0, got = 0, back_at = 0;
    size_t size, n;

    for (now = 0; now < 12000 && got < 400; now++) {
        /* a request and a notice every 50 ms for ten seconds, of which nothing arrives */
        if (now < 10000 && now % 50 == 0) {
            bytes[0] = 'o';
            put32(bytes + 1, ordered);
            CHECK(delivery_send(a, DELIVERY_ORDERED, 0, 0, 0, bytes, 5));
            ordered++;
            bytes[0] = 's';
            put32(bytes + 1, single);
            CHECK(delivery_send(a, DELIVERY_SINGLE, 0, 0, 1, bytes, 5));
            single++;
        }
        while ((n = delivery_make(a, now, datagram, sizeof datagram)) != 0) {
            made += now < 10000;
            if (now >= 10000)
                delivery_arrived(b, now, datagram, n);
        }
        while ((message = delivery_take(b, &size)) != NULL) {
            uint32_t number = get32(message + 1);

            if (message[0] == 'o') {
                CHECK(number == got_ordered);
                got_ordered++;
            } else {
                CHECK(number < 200 && times[number]++ == 0);
            }
            if (++got == 400)
                back_at = now;
        }
        while ((n = delivery_make(b, now, datagram, sizeof datagram)) != 0)
            delivery_arrived(a, now, datagram, n);
        CHECK(delivery_check(a));
    }
    /* it went on asking, a datagram every 15 ms and no more, and all 400 are there within 60 ms
       of the first datagram that arrived */
    CHECK(ordered == 200 && single == 200 && made >= 400 && made <= 10000 / 15 + 200);
    CHECK(got == 400 && got_ordered == 200 && back_at - 10000 <= 60);
    free(times);
    free(a);
    free(b);
}

/* ---- a receiver that does not take what arrived ---- */

static void test_receiver_full(void)
{
    struct Delivery *a = make_end(16, 8192, 1200, 0, 0), *b = make_end(16, 8192, 1200, 0, 0);
    unsigned char datagram[DELIVERY_DATAGRAM_MAX], bytes[300];
    unsigned char times[8] = { 0 };
    const unsigned char *message;
    uint32_t now;
    size_t size, n;
    int i, refused = 0, got = 0;

    for (i = 0; i < 8; i++) {
        memset(bytes, i, sizeof bytes);
        CHECK(delivery_send(a, DELIVERY_SINGLE, 0, 0, 0, bytes, sizeof bytes));
    }
    for (now = 0; now < 20000 && got < 8; now++) {
        while ((n = delivery_make(a, now, datagram, sizeof datagram)) != 0)
            refused += !delivery_arrived(b, now, datagram, n);
        /* b looks at what arrived only now and then */
        while (now % 700 == 699 && (message = delivery_take(b, &size)) != NULL) {
            CHECK(size == sizeof bytes && message[0] < 8 && times[message[0]]++ == 0);
            got++;
        }
        while ((n = delivery_make(b, now, datagram, sizeof datagram)) != 0)
            delivery_arrived(a, now, datagram, n);
        CHECK(delivery_check(a) && delivery_check(b));
    }
    CHECK(got == 8 && refused > 0);
    free(a);
    free(b);

    /* room that was taken from the front counts: three messages arrive, two are taken, and
       two more fit that would not fit behind the three */
    a = make_end(16, 8192, 1200, 0, 0);
    b = make_end(16, 8192, 1200, 0, 0);
    for (i = 0; i < 5; i++) {
        memset(bytes, i, sizeof bytes);
        CHECK(delivery_send(a, DELIVERY_SINGLE, 0, 0, 0, bytes, sizeof bytes));
        if (i == 2) {
            n = delivery_make(a, 0, datagram, sizeof datagram);
            CHECK(n == 9 + 3 * 305 && delivery_arrived(b, 0, datagram, n) == 1);
            CHECK(delivery_take(b, &size) != NULL && delivery_take(b, &size) != NULL);
        }
    }
    n = delivery_make(a, 1, datagram, sizeof datagram);
    CHECK(n == 9 + 2 * 305 && delivery_arrived(b, 1, datagram, n) == 1);
    for (i = 2; i < 5; i++) {
        message = delivery_take(b, &size);
        CHECK(message && size == sizeof bytes && message[0] == i && message[299] == i);
    }
    CHECK(delivery_take(b, &size) == NULL && delivery_check(b));
    free(a);
    free(b);
}

/* ---- single messages do not run ahead of what the receiver remembers ---- */

static void test_single_window(void)
{
    struct Delivery *a = make_end(1500, 65536, 1200, 0, 0), *b = make_end(4, 4096, 4096, 0, 0);
    unsigned char datagram[DELIVERY_DATAGRAM_MAX], bytes[2];
    unsigned char *times = calloc(1400, 1);
    struct Parsed messages[512];
    const unsigned char *message;
    unsigned highest = 0;
    uint32_t now;
    size_t size, n;
    int i, count, got = 0;

    for (i = 0; i < 1400; i++) {
        bytes[0] = (unsigned char)i;
        bytes[1] = (unsigned char)(i >> 8);
        CHECK(delivery_send(a, DELIVERY_SINGLE, 0, 0, 0, bytes, 2));
    }
    /* nobody answers for ten seconds */
    for (now = 0; now < 10000; now++) {
        while ((n = delivery_make(a, now, datagram, sizeof datagram)) != 0) {
            count = parse(datagram, n, messages, 512);
            for (i = 0; i < count; i++)
                if (messages[i].number > highest)
                    highest = messages[i].number;
        }
    }
    CHECK(highest == 511);
    for (; now < 60000 && got < 1400; now++) {
        while ((n = delivery_make(a, now, datagram, sizeof datagram)) != 0)
            delivery_arrived(b, now, datagram, n);
        while ((message = delivery_take(b, &size)) != NULL) {
            unsigned id = get16(message);

            CHECK(size == 2 && id < 1400 && times[id]++ == 0);
            got++;
        }
        while ((n = delivery_make(b, now, datagram, sizeof datagram)) != 0)
            delivery_arrived(a, now, datagram, n);
    }
    CHECK(got == 1400 && delivery_check(a) && delivery_check(b));
    free(times);
    free(a);
    free(b);
}

/* ---- what waits is never more than the records that count ---- */

static void test_bounded(void)
{
    struct Delivery *a = make_end(KEYS, 4096, 1200, 0, 0), *b = make_end(4, 4096, 4096, 0, 0);
    unsigned char datagram[DELIVERY_DATAGRAM_MAX], bytes[5];
    uint32_t last[KEYS] = { 0 }, seen[KEYS] = { 0 }, now, i, refused = 0;
    const unsigned char *message;
    size_t size, n;
    int key, same = 1;

    g_seed = 99;
    for (now = 0, i = 1; now < 30000; now++, i++) {
        if (now < 20000) {
            /* a state of which only the newest counts, like the player's own body */
            key = (int)(rnd() % KEYS);
            bytes[0] = (unsigned char)key;
            put32(bytes + 1, i);
            last[key] = i;
            refused += !delivery_send(a, DELIVERY_REPLACEABLE, (uint64_t)key, 0, 0, bytes, sizeof bytes);
        }
        while ((n = delivery_make(a, now, datagram, sizeof datagram)) != 0)
            if (now >= 20000 || rnd() % 100 >= 40)
                delivery_arrived(b, now, datagram, n);
        while ((message = delivery_take(b, &size)) != NULL)
            if (size == 5 && message[0] < KEYS && get32(message + 1) > seen[message[0]])
                seen[message[0]] = get32(message + 1);
        while ((n = delivery_make(b, now, datagram, sizeof datagram)) != 0)
            if (now >= 20000 || rnd() % 100 >= 40)
                delivery_arrived(a, now, datagram, n);
    }
    for (key = 0; key < KEYS; key++)
        same &= last[key] == seen[key];
    CHECK(refused == 0 && same && delivery_check(a));
    free(a);
    free(b);
}

/* ---- the budget ---- */

static void test_budget(void)
{
    const uint32_t rate = 8000, seconds = 10;
    struct Delivery *a = make_end(2048, 1u << 20, 1200, rate, 0), *b = make_end(4, 4096, 4096, 0, 0);
    unsigned char datagram[DELIVERY_DATAGRAM_MAX], bytes[100] = { 0 };
    uint32_t now, spent = 0, window_start = 0, window_spent = 0, most = 0;
    size_t size, n;

    for (now = 0; now < seconds * 1000; now++) {
        delivery_send(a, DELIVERY_SINGLE, 0, 0, 0, bytes, sizeof bytes);
        while ((n = delivery_make(a, now, datagram, sizeof datagram)) != 0) {
            delivery_arrived(b, now, datagram, n);
            if (n > 9) {
                spent += (uint32_t)n;
                window_spent += (uint32_t)n;
            }
        }
        while (delivery_take(b, &size) != NULL) {
        }
        /* the answers carry no message and are outside any budget */
        while ((n = delivery_make(b, now, datagram, sizeof datagram)) != 0)
            delivery_arrived(a, now, datagram, n);
        if (now - window_start == 999) {
            if (window_spent > most)
                most = window_spent;
            window_start = now + 1;
            window_spent = 0;
        }
    }
    /* no second takes more than its share, the burst and the datagram that passes it */
    CHECK(most <= rate + rate / 10 + DELIVERY_DATAGRAM_MAX * 2);
    CHECK(spent <= rate * seconds + DELIVERY_DATAGRAM_MAX * 2 && spent >= rate * seconds * 9 / 10);
    free(a);
    free(b);
    /* five idle seconds save up no more than the burst */
    a = make_end(2048, 1u << 20, 1200, rate, 0);
    b = make_end(4, 4096, 4096, 0, 0);
    for (now = 0; now < 5000; now++)
        CHECK(delivery_make(a, now, datagram, sizeof datagram) == 0);
    for (spent = 0; now < 6000; now++) {
        delivery_send(a, DELIVERY_SINGLE, 0, 0, 0, bytes, sizeof bytes);
        while ((n = delivery_make(a, now, datagram, sizeof datagram)) != 0) {
            delivery_arrived(b, now, datagram, n);
            spent += (uint32_t)n;
        }
        while (delivery_take(b, &size) != NULL) {
        }
        while ((n = delivery_make(b, now, datagram, sizeof datagram)) != 0)
            delivery_arrived(a, now, datagram, n);
    }
    CHECK(spent <= rate + rate / 10 + DELIVERY_DATAGRAM_MAX * 2 && spent >= rate);
    free(a);
    free(b);
}

/* ---- any bytes ---- */

static void test_any_bytes(void)
{
    struct Delivery *a = make_end(64, 16384, 4096, 0, 0), *b = make_end(64, 16384, 4096, 0, 0);
    unsigned char datagram[DELIVERY_DATAGRAM_MAX + 64], bytes[64] = { 0 };
    uint32_t now = 0, round, i;
    size_t size, n;
    int valid = 1;

    g_seed = 4242;
    for (round = 0; round < 60000; round++, now += rnd() % 4) {
        unsigned how = rnd() % 4;

        if (how == 0) {
            /* noise of any length */
            n = rnd() % sizeof datagram;
            for (i = 0; i < n; i++)
                datagram[i] = (unsigned char)rnd();
        } else if (how == 1) {
            /* noise behind a head that passes */
            n = 9 + rnd() % 64;
            for (i = 0; i < n; i++)
                datagram[i] = (unsigned char)rnd();
            datagram[0] &= 1;
        } else {
            /* a datagram of the other end's, with some of it changed */
            delivery_send(b, (int)(rnd() % 3), rnd() % 4, rnd() % 4, (int)(rnd() % DELIVERY_PRIORITIES), bytes,
                          1 + rnd() % sizeof bytes);
            n = delivery_make(b, now, datagram, DELIVERY_DATAGRAM_MAX);
            for (i = rnd() % 3; n && i; i--)
                datagram[rnd() % n] ^= (unsigned char)(1u << rnd() % 8);
            if (n && rnd() % 5 == 0)
                n = rnd() % n;
        }
        delivery_arrived(a, now, datagram, n);
        valid &= delivery_check(a);
        while (delivery_take(a, &size) != NULL)
            valid &= size >= 1 && size <= DELIVERY_MESSAGE_MAX;
        if (rnd() % 3 == 0) {
            delivery_send(a, (int)(rnd() % 3), rnd() % 4, rnd() % 4, 0, bytes, 1 + rnd() % sizeof bytes);
            while ((n = delivery_make(a, now, datagram, DELIVERY_DATAGRAM_MAX)) != 0)
                delivery_arrived(b, now, datagram, n);
            valid &= delivery_check(a) && delivery_check(b);
            while (delivery_take(b, &size) != NULL) {
            }
        }
    }
    CHECK(valid);
    free(a);
    free(b);
}

int main(void)
{
    test_first_exchange();
    test_refusals();
    test_replacing();
    test_one_sending();
    test_loss_rules();
    test_late_after_all();
    test_many_flying();
    test_silence();
    test_receiver_full();
    test_single_window();
    test_bounded();
    test_budget();
    test_wraps();
    test_any_bytes();
    /* seed, lost of a hundred, repeated of a hundred, the delay's range, the busy time, sends a
       millisecond of a hundred */
    play_world(1, 0, 0, 0, 0, 4000, 40);
    play_world(2, 10, 0, 5, 40, 8000, 30);
    play_world(3, 30, 10, 1, 200, 8000, 20);
    play_world(4, 50, 20, 0, 300, 6000, 10);
    play_world(5, 5, 50, 20, 25, 8000, 60);
    play_world(6, 80, 0, 0, 50, 4000, 5);
    if (g_failed) {
        printf("delivery: %d of %d checks FAILED\n", g_failed, g_checks);
        return 1;
    }
    printf("delivery: %d checks passed\n", g_checks);
    return 0;
}

#include "delivery/delivery.h"

/* The datagram's fields before its messages. */
#define HEAD 9u
/* A message's fields before its bytes. */
#define MESSAGE_HEAD 3u
#define NUMBERED_HEAD 5u
/* Datagrams with messages whose fate this end remembers: the newest. While the one whose
   place a further datagram needs still flies, that further one goes only at the resend
   interval, and the old one then counts as lost. So an end that hears nothing goes on asking,
   a datagram at a time, and is back as soon as one is answered. */
#define SENT_RING 128u
/* Numbers of single messages a receiver remembers, and how far the sender lets the newest
   number run ahead of the oldest one not acknowledged: less than what the receiver remembers,
   so a number it no longer knows is one it has had. */
#define SINGLE_SEEN 1024u
#define SINGLE_AHEAD 512u
/* The ordered messages leave at once when one is new, and again this often until
   acknowledged. A datagram that takes the place of one that still flies goes as often. A
   client that looks at its socket 60 times a second sends them again every time it looks. */
#define RESEND_EVERY_MS 15u
/* Later datagrams acknowledged before one counts as lost. */
#define LOSS_AFTER 3u
/* How long the other end may keep an acknowledgement before it sends it. */
#define ACK_WAIT_MS 25u
/* The round trip until one was measured, and the longest one believed. */
#define ROUND_TRIP_START_MS 200u
#define ROUND_TRIP_MAX_MS 60000u
#define TIMEOUT_VARIANCE_MIN_MS 10u
#define TIMEOUT_DOUBLINGS 3u

#define MESSAGES_MAX 4096u
#define BYTES_MAX (16u * 1024u * 1024u)
#define RATE_MAX 4000000u
#define MAGIC 0x56494c44u

enum { OUT_FREE = 0, OUT_WAITING = 1, OUT_FLYING = 2 };
enum { SENT_NONE = 0, SENT_FLYING = 1, SENT_ACKED = 2, SENT_LOST = 3 };

/* A message that is not acknowledged yet. */
struct OutMessage {
    uint32_t key_hi, key_lo, from; /* a replaceable message's */
    uint32_t offset;               /* of its bytes among the out bytes */
    uint16_t size;
    uint16_t number;        /* an ordered message's; a single one's once it was in a datagram */
    uint16_t first_carrier; /* an ordered message's: the first datagram it was in */
    uint16_t carrier;       /* the last datagram it was in */
    uint8_t kind;
    uint8_t priority;
    uint8_t state; /* OUT_WAITING for a datagram, OUT_FLYING in one that is neither acknowledged nor lost */
    uint8_t sent;  /* it has been in a datagram */
};

struct SentDatagram {
    uint32_t sent_ms;
    uint16_t number;
    uint8_t state;
    uint8_t ordered; /* the ordered messages were in it */
};

struct Delivery {
    uint32_t magic;
    struct DeliveryConfig config;
    /* where the parts of the block are, from its start */
    uint32_t messages_at; /* config.out_messages of struct OutMessage */
    uint32_t order_at;    /* the slots of the messages that wait, the longest waiting first */
    uint32_t free_at;     /* the slots that are free */
    uint32_t out_at;      /* config.out_bytes: the messages' bytes, in the order of `order` */
    uint32_t in_at;       /* config.in_bytes: arrived messages, each its size in two bytes and its bytes */

    /* sending */
    uint32_t live;       /* slots in `order` */
    uint32_t free_count; /* slots in `free` */
    uint32_t out_end;    /* the first out byte no message has */
    uint32_t out_live;   /* the bytes of the messages in `order` */
    uint32_t ordered_ms; /* when ordered messages last left */
    uint32_t crowded_ms; /* when a datagram last took the place of one that still flew */
    uint32_t srtt, rttvar, latest_rtt;
    uint32_t budget_ms, budget_part; /* when the budget was last counted up, and the thousandths left over */
    int32_t budget;                  /* bytes that may be sent; a datagram may take it below 0 */
    uint16_t next_number;            /* the next datagram's */
    uint16_t base_number;            /* no datagram before this one is flying */
    uint16_t highest_acked;
    uint16_t single_next;
    uint16_t ordered_next;
    uint8_t any_acked;
    uint8_t rtt_known;
    uint8_t ordered_fresh; /* an ordered message has not been in a datagram and could be */
    uint8_t doublings;     /* of the wait for an acknowledgement, since the last one that said something new */
    struct SentDatagram sent[SENT_RING];

    /* receiving */
    uint32_t recv_bits; /* bit i: the datagram recv_high - 1 - i arrived */
    uint32_t single_seen[SINGLE_SEEN / 32];
    uint32_t in_taken, in_used;
    uint16_t recv_high;
    uint16_t single_high;
    uint16_t ordered_expect;
    uint8_t recv_any;
    uint8_t single_any;
    uint8_t ack_owed; /* a datagram with messages arrived since the last one left */
};

static uint32_t round4(uint32_t value)
{
    return (value + 3u) & ~3u;
}

static struct OutMessage *messages_of(const struct Delivery *d)
{
    return (struct OutMessage *)((unsigned char *)d + d->messages_at);
}

static uint16_t *order_of(const struct Delivery *d)
{
    return (uint16_t *)((unsigned char *)d + d->order_at);
}

static uint16_t *free_of(const struct Delivery *d)
{
    return (uint16_t *)((unsigned char *)d + d->free_at);
}

static unsigned char *out_of(const struct Delivery *d)
{
    return (unsigned char *)d + d->out_at;
}

static unsigned char *in_of(const struct Delivery *d)
{
    return (unsigned char *)d + d->in_at;
}

static unsigned get16(const unsigned char *p)
{
    return (unsigned)p[0] | (unsigned)p[1] << 8;
}

static uint32_t get32(const unsigned char *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static void put16(unsigned char *p, unsigned value)
{
    p[0] = (unsigned char)value;
    p[1] = (unsigned char)(value >> 8);
}

static void put32(unsigned char *p, uint32_t value)
{
    put16(p, value & 0xffffu);
    put16(p + 2, value >> 16);
}

/* Also right for moving bytes down within a block. */
static void copy(unsigned char *to, const unsigned char *from, size_t count)
{
    while (count--)
        *to++ = *from++;
}

/* Whether number `a` comes before `b`, however the numbers have wrapped. */
static int before(uint16_t a, uint16_t b)
{
    return (int16_t)(uint16_t)(a - b) < 0;
}

static int config_fits(const struct DeliveryConfig *config)
{
    return config->out_messages >= 1 && config->out_messages <= MESSAGES_MAX
        && config->out_bytes >= DELIVERY_MESSAGE_MAX && config->out_bytes <= BYTES_MAX
        && config->in_bytes >= DELIVERY_DATAGRAM_MAX && config->in_bytes <= BYTES_MAX
        && config->bytes_per_second <= RATE_MAX;
}

/* Sets where the block's parts are and returns its size. */
static uint32_t lay_out(struct Delivery *d, const struct DeliveryConfig *config)
{
    d->messages_at = round4((uint32_t)sizeof *d);
    d->order_at = d->messages_at + config->out_messages * (uint32_t)sizeof(struct OutMessage);
    d->free_at = d->order_at + round4(config->out_messages * 2u);
    d->out_at = d->free_at + round4(config->out_messages * 2u);
    d->in_at = d->out_at + round4(config->out_bytes);
    return d->in_at + round4(config->in_bytes);
}

size_t delivery_size(const struct DeliveryConfig *config)
{
    struct Delivery layout;

    if (!config || !config_fits(config))
        return 0;
    return lay_out(&layout, config);
}

/* How much of the budget a burst may use: a tenth of a second's, and a datagram at least. */
static int32_t budget_most(const struct Delivery *d)
{
    uint32_t most = d->config.bytes_per_second / 10u;

    return (int32_t)(most > DELIVERY_DATAGRAM_MAX ? most : DELIVERY_DATAGRAM_MAX);
}

struct Delivery *delivery_init(void *memory, size_t size, const struct DeliveryConfig *config, uint32_t now_ms)
{
    struct Delivery *d = memory;
    unsigned char *bytes = memory;
    size_t need = delivery_size(config);
    uint16_t *freed;
    uint32_t i;

    if (!memory || !need || size < need)
        return NULL;
    for (i = 0; i < need; i++)
        bytes[i] = 0;
    lay_out(d, config);
    d->magic = MAGIC;
    d->config = *config;
    d->free_count = config->out_messages;
    freed = free_of(d);
    /* the first message takes slot 0 */
    for (i = 0; i < config->out_messages; i++)
        freed[i] = (uint16_t)(config->out_messages - 1u - i);
    d->ordered_ms = now_ms;
    d->crowded_ms = now_ms;
    d->budget_ms = now_ms;
    d->budget = budget_most(d);
    d->srtt = d->latest_rtt = ROUND_TRIP_START_MS;
    d->rttvar = ROUND_TRIP_START_MS / 2u;
    return d;
}

/* The message at `index` of the order is done with: acknowledged or replaced. */
static void release(struct Delivery *d, uint32_t index)
{
    uint16_t *order = order_of(d);
    uint16_t slot = order[index];
    struct OutMessage *message = messages_of(d) + slot;
    uint32_t i;

    d->out_live -= message->size;
    message->state = OUT_FREE;
    for (i = index + 1; i < d->live; i++)
        order[i - 1] = order[i];
    d->live--;
    free_of(d)[d->free_count++] = slot;
    if (!d->live)
        d->out_end = 0;
}

/* Closes the gaps released messages left among the out bytes. */
static void pack(struct Delivery *d)
{
    struct OutMessage *messages = messages_of(d);
    const uint16_t *order = order_of(d);
    unsigned char *out = out_of(d);
    uint32_t end = 0, i;

    for (i = 0; i < d->live; i++) {
        struct OutMessage *message = messages + order[i];

        if (message->offset != end)
            copy(out + end, out + message->offset, message->size);
        message->offset = end;
        end += message->size;
    }
    d->out_end = end;
}

int delivery_send(struct Delivery *d, int kind, uint64_t key, uint32_t from, int priority, const void *bytes,
                  size_t size)
{
    struct OutMessage *message;
    uint32_t key_hi = (uint32_t)(key >> 32), key_lo = (uint32_t)key;
    uint16_t slot;

    if (kind < DELIVERY_REPLACEABLE || kind > DELIVERY_ORDERED || priority < 0 || priority >= DELIVERY_PRIORITIES
        || !bytes || size < 1 || size > DELIVERY_MESSAGE_MAX)
        return 0;
    if (kind == DELIVERY_REPLACEABLE) {
        const struct OutMessage *messages = messages_of(d);
        const uint16_t *order = order_of(d);
        uint32_t i = d->live;

        while (i--) {
            const struct OutMessage *held = messages + order[i];

            if (held->kind == DELIVERY_REPLACEABLE && held->key_hi == key_hi && held->key_lo == key_lo
                && (int32_t)(held->from - from) >= 0)
                release(d, i);
        }
    }
    if (!d->free_count || d->config.out_bytes - d->out_live < size)
        return 0;
    if (d->config.out_bytes - d->out_end < size)
        pack(d);
    slot = free_of(d)[--d->free_count];
    message = messages_of(d) + slot;
    message->key_hi = key_hi;
    message->key_lo = key_lo;
    message->from = from;
    message->offset = d->out_end;
    message->size = (uint16_t)size;
    message->number = 0;
    message->first_carrier = 0;
    message->carrier = 0;
    message->kind = (uint8_t)kind;
    message->priority = (uint8_t)priority;
    message->state = OUT_WAITING;
    message->sent = 0;
    if (kind == DELIVERY_ORDERED) {
        message->number = d->ordered_next++;
        d->ordered_fresh = 1;
    }
    copy(out_of(d) + d->out_end, bytes, size);
    d->out_end += (uint32_t)size;
    d->out_live += (uint32_t)size;
    order_of(d)[d->live++] = slot;
    return 1;
}

static void round_trip(struct Delivery *d, uint32_t sample)
{
    if (sample < 1)
        sample = 1;
    if (sample > ROUND_TRIP_MAX_MS)
        sample = ROUND_TRIP_MAX_MS;
    d->latest_rtt = sample;
    if (!d->rtt_known) {
        d->rtt_known = 1;
        d->srtt = sample;
        d->rttvar = sample / 2u;
        return;
    }
    d->rttvar = (3u * d->rttvar + (d->srtt > sample ? d->srtt - sample : sample - d->srtt)) / 4u;
    d->srtt = (7u * d->srtt + sample) / 8u;
}

/* How long a datagram may fly without an acknowledgement of anything before it counts as lost. */
static uint32_t timeout(const struct Delivery *d)
{
    uint32_t variance = 4u * d->rttvar;

    if (variance < TIMEOUT_VARIANCE_MIN_MS)
        variance = TIMEOUT_VARIANCE_MIN_MS;
    return (d->srtt + variance + ACK_WAIT_MS) << d->doublings;
}

/* Datagram `number` is lost: what it carried last and is still wanted waits for the next one.
   An ordered message goes out again by its own clock. */
static void lose(struct Delivery *d, uint16_t number)
{
    struct OutMessage *messages = messages_of(d);
    const uint16_t *order = order_of(d);
    uint32_t i;

    for (i = 0; i < d->live; i++) {
        struct OutMessage *message = messages + order[i];

        if (message->state == OUT_FLYING && message->kind != DELIVERY_ORDERED && message->carrier == number)
            message->state = OUT_WAITING;
    }
}

/* Finds the datagrams that are lost by now. */
static void settle(struct Delivery *d, uint32_t now)
{
    uint32_t late = 9u * (d->srtt > d->latest_rtt ? d->srtt : d->latest_rtt) / 8u;
    uint32_t wait = timeout(d);
    int timed_out = 0;
    uint16_t number;

    if (late < 1)
        late = 1;
    for (number = d->base_number; number != d->next_number; number++) {
        struct SentDatagram *sent = &d->sent[number % SENT_RING];
        uint32_t age;

        if (sent->number != number || sent->state != SENT_FLYING)
            continue;
        age = now - sent->sent_ms;
        if (d->any_acked && before(number, d->highest_acked)
            && ((uint16_t)(d->highest_acked - number) >= LOSS_AFTER || age >= late)) {
            sent->state = SENT_LOST;
            lose(d, number);
        } else if (age >= wait) {
            sent->state = SENT_LOST;
            lose(d, number);
            timed_out = 1;
        }
    }
    if (timed_out && d->doublings < TIMEOUT_DOUBLINGS)
        d->doublings++;
    while (d->base_number != d->next_number) {
        const struct SentDatagram *sent = &d->sent[d->base_number % SENT_RING];

        if (sent->number == d->base_number && sent->state == SENT_FLYING)
            break;
        d->base_number++;
    }
}

/* Datagram `number` arrived: so did what it carried. An ordered message was in every datagram
   with ordered messages from its first on: they go from the oldest on, so one that had room
   once has it in every later one. */
static void arrived_there(struct Delivery *d, uint16_t number, int ordered)
{
    const struct OutMessage *messages = messages_of(d);
    const uint16_t *order = order_of(d);
    uint32_t i = d->live;
    int ordered_went = 0;

    while (i--) {
        const struct OutMessage *message = messages + order[i];

        if (!message->sent)
            continue;
        if (message->kind == DELIVERY_ORDERED) {
            if (!ordered || before(number, message->first_carrier))
                continue;
            ordered_went = 1;
        } else if (message->carrier != number) {
            continue;
        }
        release(d, i);
    }
    if (!ordered_went)
        return;
    /* an ordered message that had no room behind the ones that went has it now */
    for (i = 0; i < d->live; i++)
        if (messages[order[i]].kind == DELIVERY_ORDERED && !messages[order[i]].sent)
            d->ordered_fresh = 1;
}

static void acknowledge(struct Delivery *d, uint32_t now, uint16_t ack, uint32_t bits)
{
    int news = 0;
    unsigned i;

    for (i = 0; i <= 32; i++) {
        uint16_t number = (uint16_t)(ack - i);
        struct SentDatagram *sent = &d->sent[number % SENT_RING];

        if (i && !(bits >> (i - 1) & 1u))
            continue;
        /* one taken for lost that arrived after all is as good */
        if (sent->number != number || (sent->state != SENT_FLYING && sent->state != SENT_LOST))
            continue;
        if (i == 0)
            round_trip(d, now - sent->sent_ms);
        sent->state = SENT_ACKED;
        arrived_there(d, number, sent->ordered);
        if (!d->any_acked || before(d->highest_acked, number))
            d->highest_acked = number;
        d->any_acked = 1;
        news = 1;
    }
    if (news)
        d->doublings = 0;
}

/* Whether a single message's number is new, and notes it. */
static int single_new(struct Delivery *d, uint16_t number)
{
    uint32_t bit = number % SINGLE_SEEN;
    int ahead;
    unsigned i;

    if (!d->single_any) {
        d->single_any = 1;
        d->single_high = number;
        d->single_seen[bit / 32] |= 1u << (bit % 32);
        return 1;
    }
    ahead = (int16_t)(uint16_t)(number - d->single_high);
    if (ahead > 0) {
        if ((unsigned)ahead >= SINGLE_SEEN) {
            for (i = 0; i < SINGLE_SEEN / 32; i++)
                d->single_seen[i] = 0;
        } else {
            for (i = 1; i <= (unsigned)ahead; i++) {
                uint32_t gone = (uint16_t)(d->single_high + i) % SINGLE_SEEN;

                d->single_seen[gone / 32] &= ~(1u << (gone % 32));
            }
        }
        d->single_high = number;
        d->single_seen[bit / 32] |= 1u << (bit % 32);
        return 1;
    }
    /* further back than is remembered: the sender does not let that be a new one */
    if ((unsigned)-ahead >= SINGLE_SEEN || d->single_seen[bit / 32] >> (bit % 32) & 1u)
        return 0;
    d->single_seen[bit / 32] |= 1u << (bit % 32);
    return 1;
}

/* Makes room for `need` more bytes of arrived messages, if there is. */
static int in_room(struct Delivery *d, uint32_t need)
{
    unsigned char *in = in_of(d);

    if (d->config.in_bytes - d->in_used >= need)
        return 1;
    copy(in, in + d->in_taken, d->in_used - d->in_taken);
    d->in_used -= d->in_taken;
    d->in_taken = 0;
    return d->config.in_bytes - d->in_used >= need;
}

static void keep(struct Delivery *d, const unsigned char *bytes, unsigned size)
{
    unsigned char *in = in_of(d) + d->in_used;

    put16(in, size);
    copy(in + 2, bytes, size);
    d->in_used += 2u + size;
}

int delivery_arrived(struct Delivery *d, uint32_t now_ms, const unsigned char *bytes, size_t size)
{
    uint16_t number, ack;
    uint32_t at;
    int carries = 0;

    if (!bytes || size < HEAD || size > DELIVERY_DATAGRAM_MAX || (bytes[0] & ~1u))
        return 0;
    number = (uint16_t)get16(bytes + 1);
    ack = (uint16_t)get16(bytes + 3);
    /* an acknowledgement of a datagram this end has not made */
    if ((bytes[0] & 1u) && !before(ack, d->next_number))
        return 0;
    for (at = HEAD; at < size; carries = 1) {
        unsigned kind = bytes[at], head = kind == DELIVERY_REPLACEABLE ? MESSAGE_HEAD : NUMBERED_HEAD, length;

        if (kind > DELIVERY_ORDERED || size - at < head)
            return 0;
        length = get16(bytes + at + head - 2);
        if (length < 1 || length > DELIVERY_MESSAGE_MAX || size - at - head < length)
            return 0;
        at += head + length;
    }
    if (d->recv_any) {
        int ahead = (int16_t)(uint16_t)(number - d->recv_high);

        if (ahead == 0 || ahead < -32 || (ahead < 0 && (d->recv_bits >> (-ahead - 1) & 1u)))
            return 0;
    }
    if (d->in_taken == d->in_used)
        d->in_taken = d->in_used = 0;
    /* every message is shorter in here than in the datagram */
    if (carries && !in_room(d, (uint32_t)size))
        return 0;

    if (!d->recv_any) {
        d->recv_any = 1;
        d->recv_high = number;
        d->recv_bits = 0;
    } else {
        int ahead = (int16_t)(uint16_t)(number - d->recv_high);

        if (ahead > 0) {
            d->recv_bits = ahead > 32 ? 0 : ahead == 32 ? 0x80000000u : d->recv_bits << ahead | 1u << (ahead - 1);
            d->recv_high = number;
        } else {
            d->recv_bits |= 1u << (-ahead - 1);
        }
    }
    if (carries)
        d->ack_owed = 1;
    if (bytes[0] & 1u)
        acknowledge(d, now_ms, ack, get32(bytes + 5));
    for (at = HEAD; at < size;) {
        unsigned kind = bytes[at], head = kind == DELIVERY_REPLACEABLE ? MESSAGE_HEAD : NUMBERED_HEAD;
        unsigned length = get16(bytes + at + head - 2);
        uint16_t message_number = kind == DELIVERY_REPLACEABLE ? 0 : (uint16_t)get16(bytes + at + 1);

        if (kind == DELIVERY_REPLACEABLE || (kind == DELIVERY_SINGLE && single_new(d, message_number))) {
            keep(d, bytes + at + head, length);
        } else if (kind == DELIVERY_ORDERED && message_number == d->ordered_expect) {
            /* a datagram has them from the oldest one not acknowledged on, so none is ahead of
               the one expected unless an earlier one is in front of it */
            keep(d, bytes + at + head, length);
            d->ordered_expect++;
        }
        at += head + length;
    }
    settle(d, now_ms);
    return 1;
}

const unsigned char *delivery_take(struct Delivery *d, size_t *size)
{
    const unsigned char *message = in_of(d) + d->in_taken;

    if (d->in_taken == d->in_used)
        return NULL;
    *size = get16(message);
    d->in_taken += 2u + (uint32_t)*size;
    return message + 2;
}

/* Counts the budget up to now. */
static void earn(struct Delivery *d, uint32_t now)
{
    uint32_t elapsed = now - d->budget_ms, earned;

    d->budget_ms = now;
    if (!d->config.bytes_per_second)
        return;
    if (elapsed > 1000u)
        elapsed = 1000u;
    earned = elapsed * d->config.bytes_per_second + d->budget_part;
    d->budget_part = earned % 1000u;
    d->budget += (int32_t)(earned / 1000u);
    if (d->budget > budget_most(d))
        d->budget = budget_most(d);
}

/* Writes a message into the datagram `number` and notes that it flies in it. */
static uint32_t put_message(struct Delivery *d, struct OutMessage *message, uint16_t number, unsigned char *out,
                            uint32_t at)
{
    out[at++] = message->kind;
    if (message->kind != DELIVERY_REPLACEABLE) {
        put16(out + at, message->number);
        at += 2;
    }
    put16(out + at, message->size);
    at += 2;
    copy(out + at, out_of(d) + message->offset, message->size);
    if (!message->sent) {
        message->sent = 1;
        message->first_carrier = number;
    }
    message->carrier = number;
    message->state = OUT_FLYING;
    return at + message->size;
}

size_t delivery_make(struct Delivery *d, uint32_t now_ms, unsigned char *out, size_t room)
{
    struct OutMessage *messages = messages_of(d);
    const uint16_t *order = order_of(d);
    uint16_t number = d->next_number, single_oldest = d->single_next;
    int spend, ordered_due = 0, other = 0, carries = 0, ordered_put = 0, priority;
    struct SentDatagram *sent = &d->sent[number % SENT_RING];
    uint32_t at = HEAD, i;

    if (!out || room < DELIVERY_DATAGRAM_MAX)
        return 0;
    earn(d, now_ms);
    settle(d, now_ms);
    /* a datagram with messages needs a place in `sent`; one that a flying datagram still has
       it gets only now and then */
    spend = (!d->config.bytes_per_second || d->budget > 0)
        && (sent->state != SENT_FLYING || now_ms - d->crowded_ms >= RESEND_EVERY_MS);
    for (i = 0; i < d->live; i++) {
        const struct OutMessage *message = messages + order[i];

        if (message->kind == DELIVERY_SINGLE && message->sent && before(message->number, single_oldest))
            single_oldest = message->number;
    }
    for (i = 0; i < d->live; i++) {
        const struct OutMessage *message = messages + order[i];

        if (message->kind == DELIVERY_ORDERED)
            ordered_due = d->ordered_fresh || now_ms - d->ordered_ms >= RESEND_EVERY_MS;
        else if (message->state == OUT_WAITING
                 && (message->kind != DELIVERY_SINGLE || message->sent
                     || (uint16_t)(d->single_next - single_oldest) < SINGLE_AHEAD))
            other = 1;
    }
    if (!d->ack_owed && !(spend && (ordered_due || other)))
        return 0;

    if (spend) {
        /* the ordered ones when their time has come, from the oldest on and without a gap */
        for (i = 0; ordered_due && i < d->live; i++) {
            struct OutMessage *message = messages + order[i];

            if (message->kind != DELIVERY_ORDERED)
                continue;
            if (DELIVERY_DATAGRAM_MAX - at < NUMBERED_HEAD + message->size)
                break;
            at = put_message(d, message, number, out, at);
            carries = ordered_put = 1;
        }
        /* then what waits, by priority, the longest waiting first */
        for (priority = 0; priority < DELIVERY_PRIORITIES; priority++) {
            for (i = 0; i < d->live; i++) {
                struct OutMessage *message = messages + order[i];
                uint32_t head = message->kind == DELIVERY_REPLACEABLE ? MESSAGE_HEAD : NUMBERED_HEAD;

                if (message->kind == DELIVERY_ORDERED || message->state != OUT_WAITING
                    || message->priority != priority || DELIVERY_DATAGRAM_MAX - at < head + message->size)
                    continue;
                if (message->kind == DELIVERY_SINGLE && !message->sent) {
                    if ((uint16_t)(d->single_next - single_oldest) >= SINGLE_AHEAD)
                        continue;
                    message->number = d->single_next++;
                }
                at = put_message(d, message, number, out, at);
                carries = 1;
            }
        }
    }

    out[0] = d->recv_any;
    put16(out + 1, number);
    put16(out + 3, d->recv_high);
    put32(out + 5, d->recv_bits);
    if (carries) {
        if (sent->state == SENT_FLYING) {
            lose(d, sent->number);
            d->crowded_ms = now_ms;
        }
        sent->number = number;
        sent->sent_ms = now_ms;
        sent->state = SENT_FLYING;
        sent->ordered = (uint8_t)ordered_put;
    }
    if (ordered_put) {
        d->ordered_ms = now_ms;
        d->ordered_fresh = 0;
    }
    if (carries && d->config.bytes_per_second)
        d->budget -= (int32_t)at;
    d->ack_owed = 0;
    d->next_number++;
    return at;
}

size_t delivery_waiting(const struct Delivery *d)
{
    return d->live;
}

int delivery_check(const struct Delivery *d)
{
    struct Delivery layout;
    const struct OutMessage *messages;
    const uint16_t *order, *freed;
    const unsigned char *in;
    uint32_t i, end = 0, bytes = 0, slots = 0, slot_sum = 0;
    int ordered_any = 0, ordered_unsent = 0;
    uint16_t ordered_last = 0;

    if (!d || d->magic != MAGIC || !config_fits(&d->config))
        return 0;
    lay_out(&layout, &d->config);
    if (layout.messages_at != d->messages_at || layout.order_at != d->order_at || layout.free_at != d->free_at
        || layout.out_at != d->out_at || layout.in_at != d->in_at)
        return 0;
    messages = messages_of(d);
    order = order_of(d);
    freed = free_of(d);
    if (d->live > d->config.out_messages || d->live + d->free_count != d->config.out_messages
        || d->out_end > d->config.out_bytes || d->doublings > TIMEOUT_DOUBLINGS)
        return 0;
    for (i = 0; i < d->live; i++) {
        const struct OutMessage *message;

        if (order[i] >= d->config.out_messages)
            return 0;
        message = messages + order[i];
        if ((message->state != OUT_WAITING && message->state != OUT_FLYING) || message->kind > DELIVERY_ORDERED
            || message->priority >= DELIVERY_PRIORITIES || message->size < 1 || message->size > DELIVERY_MESSAGE_MAX
            || message->offset < end || message->sent > 1 || (message->state == OUT_FLYING && !message->sent))
            return 0;
        end = message->offset + message->size;
        bytes += message->size;
        slots++;
        slot_sum += order[i];
        if (message->kind == DELIVERY_ORDERED) {
            /* in the order of their numbers, the ones that were in a datagram first */
            if (ordered_any && message->number != (uint16_t)(ordered_last + 1))
                return 0;
            if (ordered_unsent && message->sent)
                return 0;
            ordered_any = 1;
            ordered_last = message->number;
            ordered_unsent = !message->sent;
        }
    }
    if (end > d->out_end || bytes != d->out_live || (ordered_any && (uint16_t)(ordered_last + 1) != d->ordered_next))
        return 0;
    for (i = 0; i < d->free_count; i++) {
        if (freed[i] >= d->config.out_messages || messages[freed[i]].state != OUT_FREE)
            return 0;
        slots++;
        slot_sum += freed[i];
    }
    /* every slot once, in the order or free */
    if (slots != d->config.out_messages || slot_sum != d->config.out_messages * (d->config.out_messages - 1u) / 2u)
        return 0;
    /* a flying datagram is at its number's place, and was made from the base on */
    for (i = 0; i < SENT_RING; i++)
        if (d->sent[i].state > SENT_LOST
            || (d->sent[i].state == SENT_FLYING
                && (d->sent[i].number % SENT_RING != i || before(d->sent[i].number, d->base_number)
                    || !before(d->sent[i].number, d->next_number))))
            return 0;
    if (d->in_taken > d->in_used || d->in_used > d->config.in_bytes)
        return 0;
    in = in_of(d);
    for (i = d->in_taken; i < d->in_used;) {
        unsigned length;

        if (d->in_used - i < 2u)
            return 0;
        length = get16(in + i);
        if (length < 1 || length > DELIVERY_MESSAGE_MAX || d->in_used - i - 2u < length)
            return 0;
        i += 2u + length;
    }
    return 1;
}

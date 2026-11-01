#include "contents/agency_internal.h"
#include <stdlib.h>
#include <string.h>

struct AdventurerAgencyMgr *g_adventurerAgency;

static unsigned read16(const unsigned char *p) { return p[0] | (unsigned)p[1] << 8; }
static uint32_t read32(const unsigned char *p) { return read16(p) | (uint32_t)read16(p + 2) << 16; }
static void write16(unsigned char *p, unsigned n) { p[0] = (unsigned char)n; p[1] = (unsigned char)(n >> 8); }
static void write32(unsigned char *p, uint32_t n) { write16(p, n & 65535); write16(p + 2, n >> 16); }

int adventurer_agency_create(void)
{
    if (g_adventurerAgency) return 0;
    g_adventurerAgency = calloc(1, sizeof *g_adventurerAgency);
    if (!g_adventurerAgency) return 0;
    g_adventurerAgency->page = g_adventurerAgency->pages = 1;
    return 1;
}

void adventurer_agency_reset(struct AdventurerAgencyMgr *self)
{
    if (!self) return;
    memset(self->pending, 0, sizeof self->pending);
    party_booking_list_clear(&self->parties);
    self->recruiting = 0;
    self->page = self->pages = 1;
    /* Preserve the sequence across connections so old replies cannot match. */
}

void adventurer_agency_destroy(void)
{
    struct AdventurerAgencyMgr *self = g_adventurerAgency;
    g_adventurerAgency = NULL;
    adventurer_agency_reset(self);
    free(self);
}

int adventurer_agency_is_recruiting(const struct AdventurerAgencyMgr *self)
{ return self && self->recruiting; }

const struct PartyBookingList *adventurer_agency_parties(const struct AdventurerAgencyMgr *self)
{
    static const struct PartyBookingList empty = {0};
    return self ? &self->parties : &empty;
}

void adventurer_agency_cancel_detail(struct AdventurerAgencyMgr *self)
{ if (self) memset(&self->pending[AGENCY_DETAIL], 0, sizeof self->pending[0]); }

static int request(struct AdventurerAgencyMgr *self, int op, int page, uint32_t target,
                   const struct AdvenPartyInfo *info)
{
    unsigned char packet[24 + AGENCY_TEXT_MAX] = {0};
    size_t text_size = info ? info->memo.size : 0;
    unsigned roles = 0;
    int i;
    uint32_t serial;
    if (!self || !agency_transport_open() || page < 1 || page > 65535 || text_size > AGENCY_TEXT_MAX)
        return 0;
    if (info) {
        if (info->min_level < 0 || info->max_level > 999 || info->min_level > info->max_level ||
            info->purpose < 0 || info->purpose > 6 ||
            (text_size && memchr(owned_text_cstr(&info->memo), 0, text_size))) return 0;
        for (i = 0; i < 4; ++i) {
            if (info->roles[i] > 1) return 0;
            roles |= info->roles[i] << i;
        }
    }
    /* Serialize mutations; queries may replace older queries of the same kind. */
    if ((op == AGENCY_REGISTER || op == AGENCY_DELETE) &&
        (self->pending[AGENCY_REGISTER].serial || self->pending[AGENCY_DELETE].serial)) return 0;
    serial = ++self->serial;
    if (!serial) serial = ++self->serial;
    write16(packet, AGENCY_REQUEST); write16(packet + 2, (unsigned)(24 + text_size));
    packet[4] = AGENCY_VERSION; packet[5] = (unsigned char)op;
    write32(packet + 6, serial); write16(packet + 10, page); write32(packet + 12, target);
    write16(packet + 16, info ? info->min_level : 0);
    write16(packet + 18, info ? info->max_level : 999);
    packet[20] = (unsigned char)roles; packet[21] = info ? (unsigned char)info->purpose : 0;
    write16(packet + 22, (unsigned)text_size);
    if (text_size) memcpy(packet + 24, owned_text_cstr(&info->memo), text_size);
    if (!agency_transport_send(packet, 24 + text_size)) return 0;
    self->pending[op].serial = serial;
    self->pending[op].started = agency_clock();
    self->pending[op].target = target;
    if (op == AGENCY_REGISTER || op == AGENCY_DELETE)
        self->pending[AGENCY_OWN].serial = 0;
    return 1;
}

void adventurer_agency_load_my_party(struct AdventurerAgencyMgr *self)
{
    if (self && !self->pending[AGENCY_REGISTER].serial && !self->pending[AGENCY_DELETE].serial)
        request(self, AGENCY_OWN, 1, 0, NULL);
}
int adventurer_agency_load_list(struct AdventurerAgencyMgr *self, int page)
{ return request(self, AGENCY_LIST, page, 0, NULL); }
void adventurer_agency_load_party_info(struct AdventurerAgencyMgr *self, int aid)
{
    if (aid <= 0 || !request(self, AGENCY_DETAIL, 1, (uint32_t)aid, NULL))
        agency_present(self, AGENCY_DETAIL, AGENCY_UNAVAILABLE, NULL);
}
int adventurer_agency_register(struct AdventurerAgencyMgr *self, const struct AdvenPartyInfo *info)
{ return info && request(self, AGENCY_REGISTER, 1, 0, info); }
int adventurer_agency_search(struct AdventurerAgencyMgr *self, const struct AdvenPartyInfo *filter, int page)
{ return filter && request(self, AGENCY_LIST, page, 0, filter); }
void adventurer_agency_cancel_own_booking(struct AdventurerAgencyMgr *self)
{
    if (self && (self->recruiting || self->pending[AGENCY_REGISTER].serial)) {
        /* TCP preserves ordering: a cancellation may follow a pending registration. */
        self->pending[AGENCY_REGISTER].serial = 0;
        if (!request(self, AGENCY_DELETE, 1, 0, NULL))
            agency_present(self, AGENCY_DELETE, AGENCY_UNAVAILABLE, NULL);
    }
}

static int decode_entries(struct PartyBookingList *out, const unsigned char *p, size_t size, unsigned count)
{
    size_t i, used = 0;
    if (count) {
        out->items = calloc(count, sizeof *out->items);
        if (!out->items) return 0;
    }
    out->count = count;
    for (i = 0; i < count; ++i) {
        struct AdvenPartyInfo *ad = &out->items[i];
        unsigned name_size, memo_size, role;
        if (size - used < 16) return 0;
        name_size = p[14]; memo_size = p[15];
        if (!read32(p) || read32(p) > INT32_MAX || !read32(p + 4) || read32(p + 4) > INT32_MAX ||
            read16(p + 8) > read16(p + 10) || read16(p + 10) > 999 || p[12] > 15 || p[13] > 6 ||
            !name_size || name_size > AGENCY_NAME_MAX || memo_size > AGENCY_TEXT_MAX ||
            size - used - 16 < name_size + memo_size || memchr(p + 16, 0, name_size + memo_size)) return 0;
        ad->aid = (int)read32(p); ad->gid = (int)read32(p + 4);
        ad->min_level = read16(p + 8); ad->max_level = read16(p + 10); ad->purpose = p[13];
        for (role = 0; role < 4; ++role) ad->roles[role] = (p[12] >> role) & 1;
        if (!owned_text_assign(&ad->name, text_view((const char *)p + 16, name_size)) ||
            !owned_text_assign(&ad->memo, text_view((const char *)p + 16 + name_size, memo_size))) return 0;
        used += 16 + name_size + memo_size;
        p += 16 + name_size + memo_size;
    }
    return used == size;
}

void adventurer_agency_receive(struct AdventurerAgencyMgr *self, const void *packet, size_t size)
{
    const unsigned char *p = packet;
    struct PartyBookingList data = {0};
    unsigned op, status, count, page, pages;
    if (!self || !p || size < 16 || size > 16 + AGENCY_PAGE_SIZE * (16 + AGENCY_NAME_MAX + AGENCY_TEXT_MAX) ||
        read16(p) != AGENCY_REPLY || read16(p + 2) != size || p[4] != AGENCY_VERSION) return;
    op = p[5]; status = p[10]; count = p[11]; page = read16(p + 12); pages = read16(p + 14);
    if (op < AGENCY_OWN || op > AGENCY_DELETE || !self->pending[op].serial ||
        read32(p + 6) != self->pending[op].serial) return;
    if (status > AGENCY_BUSY || count > AGENCY_PAGE_SIZE || !page || page > pages ||
        (op != AGENCY_LIST && count > 1) || (op >= AGENCY_REGISTER && count) || (status && count) ||
        !decode_entries(&data, p + 16, size - 16, count) ||
        (op == AGENCY_DETAIL && count && (uint32_t)data.items[0].aid != self->pending[op].target)) {
        party_booking_list_clear(&data);
        self->pending[op].serial = 0;
        agency_present(self, op, AGENCY_UNAVAILABLE, NULL);
        return;
    }
    self->pending[op].serial = 0;
    if (status == AGENCY_OK) {
        if (op == AGENCY_OWN) self->recruiting = count != 0;
        if (op == AGENCY_REGISTER) self->recruiting = 1;
        if (op == AGENCY_DELETE) self->recruiting = 0;
        if (op == AGENCY_LIST) {
            party_booking_list_clear(&self->parties);
            self->parties = data;
            memset(&data, 0, sizeof data);
            self->page = page; self->pages = pages;
        }
    }
    agency_present(self, op, status, &data);
    party_booking_list_clear(&data);
}

void adventurer_agency_tick(struct AdventurerAgencyMgr *self)
{
    unsigned op;
    uint32_t now = agency_clock();
    if (!self) return;
    for (op = AGENCY_OWN; op <= AGENCY_DELETE; ++op) {
        if (self->pending[op].serial &&
            (!agency_transport_open() || now - self->pending[op].started >= 10000)) {
            self->pending[op].serial = 0;
            agency_present(self, op, AGENCY_UNAVAILABLE, NULL);
        }
    }
    if (!agency_transport_open()) adventurer_agency_reset(self);
}

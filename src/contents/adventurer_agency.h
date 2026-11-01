#ifndef ADVENTURER_AGENCY_H
#define ADVENTURER_AGENCY_H

#include "contents/party_booking_data.h"
#include <stdint.h>

enum AgencyOperation { AGENCY_OWN = 1, AGENCY_LIST, AGENCY_DETAIL, AGENCY_REGISTER, AGENCY_DELETE };
enum AgencyStatus { AGENCY_OK, AGENCY_INVALID, AGENCY_FORBIDDEN, AGENCY_MISSING, AGENCY_BUSY, AGENCY_UNAVAILABLE };
enum { AGENCY_REQUEST = 0x0bf0, AGENCY_REPLY = 0x0bf1, AGENCY_VERSION = 1,
       AGENCY_TEXT_MAX = 150, AGENCY_NAME_MAX = 23, AGENCY_PAGE_SIZE = 10 };

struct AdventurerAgencyMgr;
extern struct AdventurerAgencyMgr *g_adventurerAgency;
int adventurer_agency_create(void);
void adventurer_agency_destroy(void);
void adventurer_agency_reset(struct AdventurerAgencyMgr *self);
void adventurer_agency_tick(struct AdventurerAgencyMgr *self);
void adventurer_agency_cancel_detail(struct AdventurerAgencyMgr *self);
int adventurer_agency_is_recruiting(const struct AdventurerAgencyMgr *self);
const struct PartyBookingList *adventurer_agency_parties(const struct AdventurerAgencyMgr *self);
void adventurer_agency_load_my_party(struct AdventurerAgencyMgr *self);
int adventurer_agency_load_list(struct AdventurerAgencyMgr *self, int page);
void adventurer_agency_load_party_info(struct AdventurerAgencyMgr *self, int aid);
int adventurer_agency_register(struct AdventurerAgencyMgr *self, const struct AdvenPartyInfo *info);
int adventurer_agency_search(struct AdventurerAgencyMgr *self, const struct AdvenPartyInfo *filter, int page);
void adventurer_agency_cancel_own_booking(struct AdventurerAgencyMgr *self);
void adventurer_agency_receive(struct AdventurerAgencyMgr *self, const void *packet, size_t size);

#endif

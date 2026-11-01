#ifndef AGENCY_INTERNAL_H
#define AGENCY_INTERNAL_H
#include "contents/adventurer_agency.h"

struct AgencyPending { uint32_t serial, started, target; };
struct AdventurerAgencyMgr {
    int recruiting, page, pages;
    uint32_t serial;
    struct AgencyPending pending[6];
    struct PartyBookingList parties;
};

int agency_transport_open(void);
int agency_transport_send(const void *packet, size_t size);
uint32_t agency_clock(void);
void agency_present(struct AdventurerAgencyMgr *self, int op, int status,
                    const struct PartyBookingList *data);
#endif

#ifndef ASYNCWORK_SERVICE_MANAGER_STATE_H
#define ASYNCWORK_SERVICE_MANAGER_STATE_H

#include "web/service_managers.h"
#include "core/owned_text.h"
#include "web/party_booking_data.h"

struct AccountUserDataMgr {
    int aid;
    struct OwnedText world;
    char loaded;
    int pending_saves;
    struct OwnedText save_url, load_url;
};

struct CharUserDataMgr {
    int aid, gid;
    struct OwnedText world;
    char loaded;
    int pending_saves;
    struct OwnedText save_url, load_url;
};

struct AdventurerAgencyMgr {
    char recruiting;
    int stop_master_aid;
    struct PartyBookingList parties;
    int pending;
};

struct EmblemMgr {
    struct OwnedText upload_url, download_url;
    int upload_blocked;
};

struct TwitterDataMgr {
    struct OwnedText upload_url, auth_url;
};

#endif

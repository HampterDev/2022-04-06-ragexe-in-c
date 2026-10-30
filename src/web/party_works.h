#include "core/owned_text.h"
#include "web/service_managers.h"
#ifndef ASYNCWORK_PARTY_WORKS_H
#define ASYNCWORK_PARTY_WORKS_H

#include "web/agency.h"

struct PartyChangeMasterWork {
    struct AsyncWork base;
    int aid;
    int gid;
    int to_aid;
    int to_gid;
    struct OwnedText name;
    struct OwnedText to_name;
    struct OwnedText token;
    struct OwnedText world;
    struct OwnedText response;
    struct OwnedText url;
    int succeeded;
};

struct MyPartyLoadWork {
    struct AsyncWork base;
    int aid;
    int gid;
    struct OwnedText token;
    struct OwnedText world;
    struct OwnedText response;
    struct OwnedText url;
    int succeeded;
};

struct PartyInfoLoadWork {
    struct AsyncWork base;
    int aid;
    int gid;
    struct OwnedText world;
    struct OwnedText url;
    struct OwnedText response;
    struct OwnedText token;
    int query_aid;
    int succeeded;
};

struct PartyListLoadWork {
    struct AsyncWork base;
    int aid;
    int gid;
    struct OwnedText world;
    struct OwnedText url;
    int page;
    struct OwnedText response;
    struct OwnedText token;
    struct AdvenPartyInfo info;
    int succeeded;
    int mode;
};

struct PartyRegisterWork {
    struct AsyncWork base;
    int aid;
    int gid;
    struct OwnedText token;
    struct OwnedText world;
    struct OwnedText url;
    struct AdvenPartyInfo info;
    int succeeded;
    struct OwnedText response;
};

struct PartyStopRecruitingWork {
    struct AsyncWork base;
    int aid;
    int master_aid;
    struct OwnedText token;
    struct OwnedText world;
    struct OwnedText response;
    struct OwnedText url;
    int succeeded;
};

#define PARTY_WORK_FUNCTIONS(prefix, T)                                                                   \
    struct T *prefix##_work_construct(struct T *self);                                           \
    void prefix##_work_destruct(struct T *self);                                                 \
    char prefix##_work_run(struct T *self);                                                      \
    void prefix##_work_done(struct T *self);
PARTY_WORK_FUNCTIONS(party_change_master, PartyChangeMasterWork)
PARTY_WORK_FUNCTIONS(my_party_load, MyPartyLoadWork)
PARTY_WORK_FUNCTIONS(party_info_load, PartyInfoLoadWork)
PARTY_WORK_FUNCTIONS(party_list_load, PartyListLoadWork)
PARTY_WORK_FUNCTIONS(party_register, PartyRegisterWork)
PARTY_WORK_FUNCTIONS(party_stop_recruiting, PartyStopRecruitingWork)
#undef PARTY_WORK_FUNCTIONS

#endif

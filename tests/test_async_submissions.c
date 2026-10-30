#include "contents/mcstore_work.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "web/asyncwork.h"
#include "web/work_manager_internal.h"
#include "web/work_names.h"
#include "web/account_data.h"
#include "web/char_user_data.h"
#include "web/emblem.h"
#include "web/party_works.h"
#include "contents/mcstore_backup.h"
#include "contents/twitter.h"

static int live, allocations, fail_acquire, fail_submit, submitted, manager_cookie;
static int payload_attempts, fail_payload_allocation, cancelled, reuse_record, caller_live;
static char input[8][256], expected[8][256];
static struct TextView views[8];
static struct AdvenPartyInfo booking;
static void *record;
static struct AsyncWorkMgr *manager(void) { return (struct AsyncWorkMgr *)&manager_cookie; }

void *mem_alloc(size_t size)
{
    unsigned char *block;
    assert(size <= SIZE_MAX - 64);
    block = malloc(size + 64);
    assert(block);
    memset(block, 0xa7, size + 64);
    *(size_t *)block = size;
    ++live; ++allocations;
    return block + 32;
}

void mem_free(void *pointer)
{
    size_t size;
    unsigned char *block;
    size_t i;
    if (!pointer) return;
    block = (unsigned char *)pointer - 32;
    assert(live > 0);
    size = *(size_t *)block;
    for (i = sizeof(size_t); i < 32; ++i) assert(block[i] == 0xa7);
    for (i = 0; i < 32; ++i) assert(block[32 + size + i] == 0xa7);
    --live;
    free(block);
}

static void *payload_malloc(size_t size)
{
    ++payload_attempts;
    if (payload_attempts == fail_payload_allocation) return NULL;
    return mem_alloc(size);
}
static void payload_free(void *pointer)
{
    if (pointer)
        mem_free(pointer);
}
#define malloc payload_malloc
#define free payload_free
#include "../src/core/owned_text.c"
#undef malloc
#undef free

static void check_text(const struct OwnedText *value, unsigned input_index)
{
    const char *data = owned_text_cstr(value);
    assert(value->size == views[input_index].size);
    assert(data != views[input_index].data);
    assert(!memcmp(data, expected[input_index], value->size));
    assert(!data[value->size]);
}

static void check_booking(const struct AdvenPartyInfo *value)
{
    assert(value->roles[0] == 1 && value->roles[1] == 0 && value->roles[2] == 1 && value->roles[3] == 1);
    assert(value->min_level == 37 && value->max_level == 99 && value->purpose == 3);
    assert(value->aid == 17 && value->gid == 29);
    check_text(&value->memo, 6);
    check_text(&value->name, 7);
}

struct RequestCase {
    const char **name;
    size_t size;
    void (*initialize)(void *);
    void (*dispose)(void *);
    void (*verify)(const void *);
    int (*submit)(void);
};
static const struct RequestCase *test_case;

static void release_record(void)
{
    test_case->dispose(record);
    mem_free(record);
    record = NULL;
}

struct AsyncWork *async_work_mgr_acquire(struct AsyncWorkMgr *self, const char *name)
{
    assert(self == manager() && name == *test_case->name);
    if (reuse_record) {
        assert(record && !fail_acquire);
        reuse_record = 0;
        return record;
    }
    assert(!record);
    if (fail_acquire) return NULL;
    record = mem_alloc(test_case->size);
    memset(record, 0, test_case->size);
    test_case->initialize(record);
    return record;
}

int async_work_mgr_submit(struct AsyncWorkMgr *self, struct AsyncWork *work)
{
    assert(self == manager() && (void *)work == record);
    test_case->verify(record);
    ++submitted;
    if (fail_submit) {
        release_record();
        return 0;
    }
    return 1;
}

void async_work_mgr_cancel(struct AsyncWorkMgr *self, struct AsyncWork *work)
{
    assert(self == manager() && (void *)work == record);
    assert(live == caller_live + 1);
    ++cancelled;
    reuse_record = 1;
}

static void initialize_account_data_load(void *value) { account_data_load_work_construct(value); }
static void dispose_account_data_load(void *value) { account_data_load_work_destruct(value); }
static int submit_account_data_load(void)
{
    return async_work_mgr_account_data_load(manager(), 101, views[0], views[1]);
}
static void verify_account_data_load(const void *value)
{
    const struct AccountDataLoadWork *work = value;
    check_text(&work->world, 0);
    check_text(&work->url, 1);
    assert(work->aid == 101);
    assert(!work->succeeded);
}

static void initialize_account_data_save(void *value) { account_data_save_work_construct(value); }
static void dispose_account_data_save(void *value) { account_data_save_work_destruct(value); }
static int submit_account_data_save(void)
{
    return async_work_mgr_account_data_save(manager(), 101, views[0], views[1], views[2], views[3]);
}
static void verify_account_data_save(const void *value)
{
    const struct AccountDataSaveWork *work = value;
    check_text(&work->world, 0);
    check_text(&work->url, 1);
    check_text(&work->token, 2);
    check_text(&work->data, 3);
    assert(work->aid == 101);
    assert(!work->succeeded);
}

static void initialize_char_data_load(void *value) { char_data_load_work_construct(value); }
static void dispose_char_data_load(void *value) { char_data_load_work_destruct(value); }
static int submit_char_data_load(void)
{
    return async_work_mgr_char_data_load(manager(), 101, 102, views[0], views[1], views[2]);
}
static void verify_char_data_load(const void *value)
{
    const struct CharDataLoadWork *work = value;
    check_text(&work->world, 0);
    check_text(&work->url, 1);
    check_text(&work->token, 2);
    assert(work->aid == 101);
    assert(work->gid == 102);
    assert(!work->succeeded);
}

static void initialize_char_data_save(void *value) { char_data_save_work_construct(value); }
static void dispose_char_data_save(void *value) { char_data_save_work_destruct(value); }
static int submit_char_data_save(void)
{
    return async_work_mgr_char_data_save(manager(), 101, 102, views[0], views[1], views[2], views[3]);
}
static void verify_char_data_save(const void *value)
{
    const struct CharDataSaveWork *work = value;
    check_text(&work->world, 0);
    check_text(&work->url, 1);
    check_text(&work->token, 2);
    check_text(&work->data, 3);
    assert(work->aid == 101);
    assert(work->gid == 102);
    assert(!work->succeeded);
}

static void initialize_emblem_download(void *value) { emblem_download_work_construct(value); }
static void dispose_emblem_download(void *value) { emblem_download_work_destruct(value); }
static int submit_emblem_download(void)
{
    return async_work_mgr_emblem_download(manager(), views[0], 102, views[1], views[2], 105, 106);
}
static void verify_emblem_download(const void *value)
{
    const struct EmblemDownloadWork *work = value;
    check_text(&work->url, 0);
    check_text(&work->token, 1);
    check_text(&work->world, 2);
    assert(work->aid == 102);
    assert(work->gdid == 105);
    assert(work->version == 106);
    assert(!work->succeeded);
}

static void initialize_emblem_upload(void *value) { emblem_upload_work_construct(value); }
static void dispose_emblem_upload(void *value) { emblem_upload_work_destruct(value); }
static int submit_emblem_upload(void)
{
    return async_work_mgr_emblem_upload(manager(), views[0], 102, views[1], views[2], 105, views[3]);
}
static void verify_emblem_upload(const void *value)
{
    const struct EmblemUploadWork *work = value;
    check_text(&work->url, 0);
    check_text(&work->token, 1);
    check_text(&work->world, 2);
    check_text(&work->path, 3);
    assert(work->aid == 102);
    assert(work->gdid == 105);
    assert(!work->succeeded);
}

static void initialize_my_party_load(void *value) { my_party_load_work_construct(value); }
static void dispose_my_party_load(void *value) { my_party_load_work_destruct(value); }
static int submit_my_party_load(void)
{
    return async_work_mgr_my_party_load(manager(), 101, 102, views[0], views[1], views[2]);
}
static void verify_my_party_load(const void *value)
{
    const struct MyPartyLoadWork *work = value;
    check_text(&work->world, 0);
    check_text(&work->url, 1);
    check_text(&work->token, 2);
    assert(work->aid == 101);
    assert(work->gid == 102);
    assert(!work->succeeded);
}

static void initialize_party_info_load(void *value) { party_info_load_work_construct(value); }
static void dispose_party_info_load(void *value) { party_info_load_work_destruct(value); }
static int submit_party_info_load(void)
{
    return async_work_mgr_party_info_load(manager(), 101, 102, views[0], views[1], views[2], 106);
}
static void verify_party_info_load(const void *value)
{
    const struct PartyInfoLoadWork *work = value;
    check_text(&work->world, 0);
    check_text(&work->url, 1);
    check_text(&work->token, 2);
    assert(work->aid == 101);
    assert(work->gid == 102);
    assert(work->query_aid == 106);
    assert(!work->succeeded);
}

static void initialize_party_list_load(void *value) { party_list_load_work_construct(value); }
static void dispose_party_list_load(void *value) { party_list_load_work_destruct(value); }
static int submit_party_list_load(void)
{
    return async_work_mgr_party_list_load(manager(), 101, 102, views[0], views[1], views[2], 106, &booking, 108);
}
static void verify_party_list_load(const void *value)
{
    const struct PartyListLoadWork *work = value;
    check_text(&work->world, 0);
    check_text(&work->url, 1);
    check_text(&work->token, 2);
    assert(work->aid == 101);
    assert(work->gid == 102);
    assert(work->page == 108);
    assert(work->mode == 106);
    check_booking(&work->info);
    assert(!work->succeeded);
}

static void initialize_mcstore_load(void *value) { mcstore_load_work_construct(value); }
static void dispose_mcstore_load(void *value) { mcstore_load_work_destruct(value); }
static int submit_mcstore_load(void)
{
    return async_work_mgr_mcstore_load(manager(), 101, 102, views[0], views[1], views[2], 106);
}
static void verify_mcstore_load(const void *value)
{
    const struct McStoreLoadWork *work = value;
    check_text(&work->world, 0);
    check_text(&work->url, 1);
    check_text(&work->token, 2);
    assert(work->aid == 101);
    assert(work->gid == 102);
    assert(work->type == 106);
    assert(!work->succeeded);
}

static void initialize_mcstore_save(void *value) { mcstore_save_work_construct(value); }
static void dispose_mcstore_save(void *value) { mcstore_save_work_destruct(value); }
static int submit_mcstore_save(void)
{
    return async_work_mgr_mcstore_save(manager(), 101, 102, views[0], views[1], views[2], views[3], 107);
}
static void verify_mcstore_save(const void *value)
{
    const struct McStoreSaveWork *work = value;
    check_text(&work->world, 0);
    check_text(&work->url, 1);
    check_text(&work->token, 2);
    check_text(&work->data, 3);
    assert(work->aid == 101);
    assert(work->gid == 102);
    assert(work->type == 107);
    assert(!work->succeeded);
}

static void initialize_party_register(void *value) { party_register_work_construct(value); }
static void dispose_party_register(void *value) { party_register_work_destruct(value); }
static int submit_party_register(void)
{
    return async_work_mgr_party_register(manager(), 101, 102, views[0], views[1], views[2], &booking);
}
static void verify_party_register(const void *value)
{
    const struct PartyRegisterWork *work = value;
    check_text(&work->world, 0);
    check_text(&work->url, 1);
    check_text(&work->token, 2);
    assert(work->aid == 101);
    assert(work->gid == 102);
    check_booking(&work->info);
    assert(!work->succeeded);
}

static void initialize_party_stop_recruiting(void *value) { party_stop_recruiting_work_construct(value); }
static void dispose_party_stop_recruiting(void *value) { party_stop_recruiting_work_destruct(value); }
static int submit_party_stop_recruiting(void)
{
    return async_work_mgr_party_stop_recruiting(manager(), 101, views[0], views[1], views[2], 105);
}
static void verify_party_stop_recruiting(const void *value)
{
    const struct PartyStopRecruitingWork *work = value;
    check_text(&work->world, 0);
    check_text(&work->url, 1);
    check_text(&work->token, 2);
    assert(work->aid == 101);
    assert(work->master_aid == 105);
    assert(!work->succeeded);
}

static void initialize_twitter_send(void *value) { twitter_send_work_construct(value); }
static void dispose_twitter_send(void *value) { twitter_send_work_destruct(value); }
static int submit_twitter_send(void)
{
    return async_work_mgr_twitter_send(manager(), 101, views[0], views[1], views[2], views[3]);
}
static void verify_twitter_send(const void *value)
{
    const struct TwitterSendWork *work = value;
    check_text(&work->url, 1);
    check_text(&work->tweet, 2);
    check_text(&work->image, 3);
    check_text(&work->token, 0);
    assert(work->aid == 101);
    assert(!work->succeeded);
}

static const struct RequestCase cases[] = {
    {&g_accountDataLoadWorkName, sizeof(struct AccountDataLoadWork), initialize_account_data_load, dispose_account_data_load, verify_account_data_load, submit_account_data_load},
    {&g_accountDataSaveWorkName, sizeof(struct AccountDataSaveWork), initialize_account_data_save, dispose_account_data_save, verify_account_data_save, submit_account_data_save},
    {&g_charDataLoadWorkName, sizeof(struct CharDataLoadWork), initialize_char_data_load, dispose_char_data_load, verify_char_data_load, submit_char_data_load},
    {&g_charDataSaveWorkName, sizeof(struct CharDataSaveWork), initialize_char_data_save, dispose_char_data_save, verify_char_data_save, submit_char_data_save},
    {&g_emblemDownloadWorkName, sizeof(struct EmblemDownloadWork), initialize_emblem_download, dispose_emblem_download, verify_emblem_download, submit_emblem_download},
    {&g_emblemUploadWorkName, sizeof(struct EmblemUploadWork), initialize_emblem_upload, dispose_emblem_upload, verify_emblem_upload, submit_emblem_upload},
    {&g_myPartyLoadWorkName, sizeof(struct MyPartyLoadWork), initialize_my_party_load, dispose_my_party_load, verify_my_party_load, submit_my_party_load},
    {&g_partyInfoLoadWorkName, sizeof(struct PartyInfoLoadWork), initialize_party_info_load, dispose_party_info_load, verify_party_info_load, submit_party_info_load},
    {&g_partyListLoadWorkName, sizeof(struct PartyListLoadWork), initialize_party_list_load, dispose_party_list_load, verify_party_list_load, submit_party_list_load},
    {&g_mcStoreLoadWorkName, sizeof(struct McStoreLoadWork), initialize_mcstore_load, dispose_mcstore_load, verify_mcstore_load, submit_mcstore_load},
    {&g_mcStoreSaveWorkName, sizeof(struct McStoreSaveWork), initialize_mcstore_save, dispose_mcstore_save, verify_mcstore_save, submit_mcstore_save},
    {&g_partyRegisterWorkName, sizeof(struct PartyRegisterWork), initialize_party_register, dispose_party_register, verify_party_register, submit_party_register},
    {&g_partyStopRecruitingWorkName, sizeof(struct PartyStopRecruitingWork), initialize_party_stop_recruiting, dispose_party_stop_recruiting, verify_party_stop_recruiting, submit_party_stop_recruiting},
    {&g_twitterSendWorkName, sizeof(struct TwitterSendWork), initialize_twitter_send, dispose_twitter_send, verify_twitter_send, submit_twitter_send},
};

static void prepare_inputs(size_t size)
{
    size_t i, j;
    assert(!record && !live);
    fail_payload_allocation = payload_attempts = 0;
    for (i = 0; i < 8; ++i) {
        for (j = 0; j < sizeof input[i]; ++j)
            input[i][j] = (char)('A' + (i + j) % 26);
        if (size > 1) input[i][size / 2] = 0;
        memcpy(expected[i], input[i], sizeof input[i]);
        views[i] = text_view(input[i], size);
    }
    party_booking_init(&booking);
    booking.roles[0] = booking.roles[2] = booking.roles[3] = 1;
    booking.min_level = 37; booking.max_level = 99; booking.purpose = 3;
    booking.aid = 17; booking.gid = 29;
    assert(owned_text_assign(&booking.memo, views[6]));
    assert(owned_text_assign(&booking.name, views[7]));
    caller_live = live;
    payload_attempts = submitted = cancelled = 0;
    fail_acquire = fail_submit = reuse_record = 0;
}

static void release_inputs_and_check_copy(int success)
{
    assert(!memcmp(input, expected, sizeof input));
    check_booking(&booking);
    party_booking_clear(&booking);
    memset(input, 0xa5, sizeof input);
    if (success) {
        assert(record);
        test_case->verify(record);
        release_record();
    }
    assert(!record && !live);
}

static void test_text_and_booking(void)
{
    struct OwnedText text = {0};
    struct AdvenPartyInfo original, copy;
    const char bytes[] = {'a', 0, 'b', 'c'};
    char *saved;
    int fail;
    assert(owned_text_assign(&text, text_view(bytes, sizeof bytes)));
    assert(text.size == 4 && !memcmp(text.data, bytes, 4) && !text.data[4]);
    assert(owned_text_assign(&text, text_view(text.data + 1, 3)));
    assert(text.size == 3 && !memcmp(text.data, bytes + 1, 3));
    saved = text.data;
    assert(!owned_text_assign(&text, text_view("", SIZE_MAX)) && saved == text.data);
    payload_attempts = 0; fail_payload_allocation = 1;
    assert(!owned_text_assign(&text, text_view_cstr("new")) && saved == text.data);
    fail_payload_allocation = 0;
    assert(owned_text_assign(&text, text_view("", 0)) && !text.data && !text.size);
    assert(owned_text_concat(&text, text_view(bytes, sizeof bytes), text_view_cstr("tail")));
    assert(text.size == 8 && !memcmp(text.data, "a\0bctail", 8) && !text.data[8]);
    assert(owned_text_concat(&text, text_view(text.data + 1, 3), owned_text_view(&text)));
    assert(text.size == 11 && !memcmp(text.data, "\0bca\0bctail", 11));
    saved = text.data;
    assert(!owned_text_concat(&text, text_view("", SIZE_MAX), text_view("", 0)));
    assert(!owned_text_concat(&text, text_view("", 0), text_view("", SIZE_MAX)));
    assert(!owned_text_concat(&text, text_view("", SIZE_MAX - 1), text_view("", 1)));
    assert(text.data == saved);
    payload_attempts = 0; fail_payload_allocation = 1;
    assert(!owned_text_concat(&text, owned_text_view(&text), text_view_cstr("x")));
    assert(text.data == saved);
    fail_payload_allocation = 0;
    assert(owned_text_concat(&text, text_view("", 0), text_view("", 0)) && !text.data && !text.size);
    owned_text_clear(&text); owned_text_clear(&text);
    party_booking_init(&original); party_booking_init(&copy);
    assert(owned_text_assign(&original.memo, text_view(bytes, sizeof bytes)));
    assert(owned_text_assign(&original.name, text_view_cstr("name")));
    original.aid = 42;
    assert(owned_text_assign(&copy.name, text_view_cstr("old")));
    copy.aid = 7;
    for (fail = 1; fail <= 2; ++fail) {
        payload_attempts = 0; fail_payload_allocation = fail;
        assert(!party_booking_assign(&copy, &original));
        assert(copy.aid == 7 && !strcmp(owned_text_cstr(&copy.name), "old") && !copy.memo.size);
    }
    fail_payload_allocation = 0;
    assert(party_booking_assign(&copy, &original));
    assert(party_booking_assign(&copy, &copy));
    party_booking_clear(&original);
    assert(copy.aid == 42 && !memcmp(copy.memo.data, bytes, sizeof bytes));
    party_booking_clear(&copy); party_booking_clear(&copy);
    assert(!live);
}

int main(void)
{
    static const size_t sizes[] = {0, 1, 15, 16, 17, 127};
    size_t length, case_index;
    int outcome, failure, allocation_count = 0, failure_cases = 0;
    test_text_and_booking();
    for (length = 0; length < sizeof sizes / sizeof sizes[0]; ++length) {
        for (case_index = 0; case_index < sizeof cases / sizeof cases[0]; ++case_index) {
            test_case = &cases[case_index];
            for (outcome = 0; outcome < 3; ++outcome) {
                int prior_allocations;
                prepare_inputs(sizes[length]);
                fail_acquire = outcome == 0;
                fail_submit = outcome == 1;
                prior_allocations = allocations;
                assert(test_case->submit() == (outcome == 2));
                assert(submitted == !fail_acquire && !cancelled);
                if (fail_acquire) assert(allocations == prior_allocations);
                if (outcome == 2) allocation_count = payload_attempts;
                release_inputs_and_check_copy(outcome == 2);
            }
            for (failure = 1; failure <= allocation_count; ++failure) {
                prepare_inputs(sizes[length]);
                fail_payload_allocation = failure;
                assert(!test_case->submit());
                assert(!submitted && cancelled == 1);
                assert(!memcmp(input, expected, sizeof input));
                check_booking(&booking);
                fail_payload_allocation = payload_attempts = 0;
                assert(test_case->submit());
                assert(submitted == 1 && !reuse_record);
                release_inputs_and_check_copy(1);
                ++failure_cases;
            }
        }
    }
    puts("Async submissions: all 14 real payloads, borrowed inputs, six counted lengths, embedded nulls, acquisition/enqueue failures, caller cleanup and queued copies passed");
    printf("C payload ownership: %d allocation failures, cancellation/reuse, guarded cleanup, aliased text and transactional booking copies passed\n", failure_cases);
    return 0;
}

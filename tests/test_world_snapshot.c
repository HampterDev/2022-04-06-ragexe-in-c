#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "game/world_snapshot.h"
#include "game/gamemode.h"
#include "game/ground_item_state.h"
#include "game/ground_item_packets.h"
#include "game/packet_handlers.h"
#include "game/skill_unit_packets.h"
#include "effects/skills/skill_state.h"
#include "game/objects.h"
#include "replay/snapshot.h"
#include "fixtures/world_snapshots_wire.h"

static int attempts, fail_at, allocations;
static void *resize(void *old, size_t size)
{
    void *result;
    if (++attempts == fail_at) return NULL;
    result = realloc(old, size);
    assert(result);
    if (!old) ++allocations;
    return result;
}
static void release(void *p) { if (p) { --allocations; free(p); } }
#define realloc resize
#define free release
#include "../src/replay/snapshot.c"
#undef realloc
#undef free
#include "../src/game/world_snapshot.c"
#include "../src/game/cell_coordinates.c"

static struct PacketGroundItemEntry ground_calls[128];
static struct PACKET_ZC_SKILL_ENTRY skill_calls[128];
static struct PACKET_ZC_SKILL_ENTRY2 text_call;
static int grounds, skills, texts;
int game_actor_get_job(struct GameActor *actor) { return actor->job; }
void game_mode_item_entry(struct GameMode *mode, const struct PacketGroundItemEntry *packet)
{ assert(mode->world && grounds < 128); ground_calls[grounds++] = *packet; }
void game_mode_zc_skill_entry(struct GameMode *mode, struct PACKET_ZC_SKILL_ENTRY *packet)
{ assert(mode->world && skills < 128); skill_calls[skills++] = *packet; packet->visible = 0; }
void game_mode_zc_skill_entry2(struct GameMode *mode, struct PACKET_ZC_SKILL_ENTRY2 *packet)
{ assert(mode->world); text_call = *packet; ++texts; packet->visible = 0; }

static struct GameMode mode;
static struct World world;
static struct C3dAttr attr;
static struct Item items[64];
static char item_name[] = "anything";
static struct Skill units[64];
static struct ListNode ih, sh;
static struct { struct ListNode links; void *value; } in[64], sn[64];
static void lists(size_t count)
{
    size_t i;
    world.items.head = &ih; world.skills.head = &sh;
    ih.next = count ? &in[0].links : &ih; sh.next = count ? &sn[0].links : &sh;
    for (i = 0; i < count; ++i) {
        in[i].value = &items[i]; sn[i].value = &units[i];
        in[i].links.next = i + 1 == count ? &ih : &in[i+1].links;
        sn[i].links.next = i + 1 == count ? &sh : &sn[i+1].links;
    }
}
static void setup(void)
{
    mode.world = &world; world.attr = &attr;
    attr.width = 101; attr.height = 99; attr.zoom = 5;
    lists(2);
    items[0].aid = 0x12345678; items[0].item_id_short = 501; items[0].identified = 1;
    items[0].base.pos.x = -2.0f; items[0].base.pos.z = 7.8125f; items[0].count = 12;
    items[1].aid = -2; items[1].item_id_short = 65535;
    items[1].base.pos.x = -251.0f; items[1].base.pos.z = -250.0f; items[1].count = 32767;
    items[0].name.data = item_name; items[0].name.size = items[0].name.capacity = 8;
    units[0].unit_aid = 0x11223344; units[0].creator_aid = 0x12345678; units[0].base.job = 0x8b;
    units[0].base.pos.x = -2.0f; units[0].base.pos.z = 7.8125f;
    units[1].unit_aid = -2; units[1].creator_aid = -3; units[1].base.job = 0x126;
    units[1].base.pos.x = -251.0f; units[1].base.pos.z = -250.0f;
    mode.npc_dialog_open = 0x12345678;
}

typedef int (*Save)(struct GameMode *, struct ReplayBuffer *);
typedef int (*Load)(struct GameMode *, const void *, size_t);
struct Family { Save save; Load load; const unsigned char *wire; size_t size; unsigned short begin, end, tag; size_t width; };
static const struct Family families[] = {
    {game_ground_items_save,game_ground_items_load,expected_ground,sizeof expected_ground,0x3a98,0x3a99,0xcf,19},
    {game_skill_units_save,game_skill_units_load,expected_skills,sizeof expected_skills,0x4268,0x4269,0xcd,16},
    {game_npc_dialog_save,game_npc_dialog_load,expected_npc,sizeof expected_npc,0x36b0,0x36b1,0x36bb,4}
};
static int read_calls, read_failure, truncated_chunk, empty_chunks;
int replay_read_chunk(struct ReplayRecorder *recorder, int slot, struct ReplayBuffer *out)
{
    const int slots[]={0xf,0x10,0x16};
    int index=read_calls++;
    size_t size;
    assert(recorder && index<3 && slot==slots[index] && !out->data);
    if (read_failure==index+1) return 0;
    if (empty_chunks & (1<<index)) return 1;
    size=families[index].size-(truncated_chunk==index+1);
    out->data=resize(NULL,size);out->size=size;assert(out->data);
    memcpy(out->data,families[index].wire,size);return 1;
}
static void unchanged(void) { assert(!grounds && !skills && !texts && mode.npc_dialog_open == -99); }
static void reset_calls(void) { grounds = skills = texts = 0; mode.npc_dialog_open = -99; }
static void bad(Load load, const void *data, size_t size)
{ reset_calls(); assert(load(&mode,data,size) == -1); unchanged(); }

static void formats(void)
{
    size_t f,i;
    for (f=0;f<3;++f) {
        const struct Family *family = &families[f];
        struct ReplayBuffer out={0};
        unsigned char data[256];
        setup(); assert(family->save(&mode,&out));
        assert(out.size == family->size && !memcmp(out.data,family->wire,out.size));
        replay_buffer_destroy(&out);
        for (i=0;i<family->size;++i) bad(family->load,family->wire,i);
        bad(family->load,NULL,family->size);
        bad(family->load,family->wire,(size_t)INT_MAX+1);
        memcpy(data,family->wire,family->size);
        data[family->size-4]=1; bad(family->load,data,family->size);
        memcpy(data,family->wire,family->size);
        data[2]=1; bad(family->load,data,family->size);
        memcpy(data,family->wire,family->size);
        data[0]^=1; bad(family->load,data,family->size);
        for (i=0;i<=family->width+1;++i) if (i!=family->width) {
            struct SnapshotWriter writer={0};
            snapshot_write(&writer,family->begin,NULL,0);
            snapshot_write(&writer,family->tag,family->wire+12,family->width);
            memset(data,0,sizeof data);
            snapshot_write(&writer,family->tag,data,i);
            snapshot_write(&writer,family->end,NULL,0);
            assert(snapshot_finish(&writer,&out));
            bad(family->load,out.data,out.size); replay_buffer_destroy(&out);
        }
        memcpy(data,family->wire,family->size); memset(data+8,0xff,4);
        bad(family->load,data,family->size);
        {
            struct SnapshotWriter writer={0};
            snapshot_write(&writer,family->begin,NULL,0);
            snapshot_write(&writer,0x7777,"abc",3);
            snapshot_write(&writer,family->end,NULL,0);
            assert(snapshot_finish(&writer,&out)); reset_calls();
            assert(family->load(&mode,out.data,out.size)==(int)out.size); unchanged();
            replay_buffer_destroy(&out);
            snapshot_write(&writer,family->begin,NULL,0);
            snapshot_write(&writer,family->begin,NULL,0);
            snapshot_write(&writer,family->end,NULL,0);
            assert(snapshot_finish(&writer,&out)); bad(family->load,out.data,out.size);
            replay_buffer_destroy(&out);
        }
        memcpy(data,family->wire,family->size); memset(data+family->size,0xff,6);
        reset_calls(); assert(family->load(&mode,data,family->size+6)==(int)family->size);
        assert(!memcmp(data,family->wire,family->size));
        assert(f==0 ? grounds==2 && ground_calls[0].count==12 && ground_calls[1].count==32767 :
               f==1 ? skills==2 && skill_calls[0].visible==1 && skill_calls[1].job==0x26 :
                      mode.npc_dialog_open==0x12345678);
        assert(!family->save(NULL,&out)); assert(family->load(NULL,data,family->size)==-1);
    }
}
static void text_and_flags(void)
{
    struct SnapshotWriter writer={0};
    struct ReplayBuffer out={0};
    struct PACKET_ZC_SKILL_ENTRY2 entry={0};
    size_t i;
    int32_t flag=INT_MIN;
    memset(entry.text,'x',sizeof entry.text); entry.has_text=1; entry.visible=1; entry.aid=42;
    snapshot_write(&writer,0x4268,NULL,0);
    snapshot_write(&writer,0xce,&entry,sizeof entry);
    snapshot_write(&writer,0x4269,NULL,0);
    assert(snapshot_finish(&writer,&out));
    for (i=0;i<out.size;++i) bad(game_skill_units_load,out.data,i);
    reset_calls(); assert(game_skill_units_load(&mode,out.data,out.size)==(int)out.size);
    assert(texts==1 && !skills && !memcmp(&text_call,&entry,sizeof entry));
    assert(!memcmp(out.data+12,&entry,sizeof entry)); replay_buffer_destroy(&out);
    for (i=0;i<=sizeof entry+1;++i) if (i!=sizeof entry) {
        unsigned char bytes[sizeof entry+1]={0};
        snapshot_write(&writer,0x4268,NULL,0); snapshot_write(&writer,0xce,bytes,i);
        snapshot_write(&writer,0x4269,NULL,0); assert(snapshot_finish(&writer,&out));
        bad(game_skill_units_load,out.data,out.size); replay_buffer_destroy(&out);
    }
    snapshot_write(&writer,0x36b0,NULL,0); snapshot_write(&writer,0x36bb,&flag,sizeof flag);
    flag=INT_MAX; snapshot_write(&writer,0x36bb,&flag,sizeof flag);
    snapshot_write(&writer,0x36b1,NULL,0); assert(snapshot_finish(&writer,&out));
    assert(game_npc_dialog_load(&mode,out.data,out.size)==(int)out.size);
    assert(mode.npc_dialog_open==INT_MAX); replay_buffer_destroy(&out);
}
static void failures(void)
{
    size_t f;
    for (f=0;f<3;++f) {
        struct ReplayBuffer out={0};
        unsigned char *old;
        int n,total;
        setup(); lists(64); attempts=0;
        assert(families[f].save(&mode,&out)); total=attempts; replay_buffer_destroy(&out);
        for (n=1;n<=total;++n) {
            out.data=resize(NULL,3); out.size=3; memcpy(out.data,"old",3);old=out.data;
            attempts=0;fail_at=n; assert(!families[f].save(&mode,&out)); fail_at=0;
            assert(out.data==old && out.size==3 && !memcmp(out.data,"old",3));
            assert(allocations==1); replay_buffer_destroy(&out); replay_buffer_destroy(&out);
            assert(!allocations);
        }
    }
    setup();
    for (f=0;f<2;++f) {
        struct ReplayBuffer out={0};
        float *x = f ? &units[0].base.pos.x : &items[0].base.pos.x;
        const float invalid[] = {NAN,INFINITY,-INFINITY,2147483648.0f*5.0f,-2147483648.0f*10.0f};
        size_t j;
        for (j=0;j<sizeof invalid/sizeof invalid[0];++j) {
            *x=invalid[j]; assert(!families[f].save(&mode,&out)); assert(!out.data && !allocations);
        }
        *x=-2;attr.zoom=0;assert(!families[f].save(&mode,&out));attr.zoom=5;
        attr.width=0;assert(!families[f].save(&mode,&out));attr.width=101;
        world.attr=NULL;assert(!families[f].save(&mode,&out));world.attr=&attr;
        if (f) sn[0].value=NULL;else in[0].value=NULL;
        assert(!families[f].save(&mode,&out)); setup();
        lists(0);world.attr=NULL;
        assert(families[f].save(&mode,&out) && out.size==12); replay_buffer_destroy(&out);
        setup();
    }
}
static void restoration(void)
{
    struct ReplayRecorder recorder={0};
    int i,invalid;
    setup();reset_calls();read_calls=0;
    assert(game_world_snapshot_restore(&mode,&recorder));
    assert(read_calls==3 && grounds==2 && skills==2 && mode.npc_dialog_open==0x12345678 && !allocations);
    for(invalid=0;invalid<2;++invalid) for(i=1;i<=3;++i) {
        setup();reset_calls();read_calls=0;
        read_failure=invalid ? 0 : i;truncated_chunk=invalid ? i : 0;
        assert(!game_world_snapshot_restore(&mode,&recorder));
        assert(read_calls==i && grounds==(i>1 ? 2 : 0) && skills==(i>2 ? 2 : 0));
        assert(mode.npc_dialog_open==-99 && !allocations);
    }
    read_failure=truncated_chunk=0;
    for(i=0;i<8;++i) {
        setup();reset_calls();read_calls=0;empty_chunks=i;
        assert(game_world_snapshot_restore(&mode,&recorder));
        assert(read_calls==3 && grounds==((i&1) ? 0 : 2) && skills==((i&2) ? 0 : 2));
        assert(mode.npc_dialog_open==((i&4) ? -99 : 0x12345678) && !allocations);
    }
    assert(!game_world_snapshot_restore(NULL,&recorder));
    assert(!game_world_snapshot_restore(&mode,NULL));
    mode.world=NULL;assert(!game_world_snapshot_restore(&mode,&recorder));
}
int main(void)
{
    formats();text_and_flags();failures();restoration();assert(!allocations);
    puts("world snapshots: frozen wire bytes, stored quantities, real coordinates, bounded records, deferred handlers, immutable input and allocation rollback passed");
    puts("world restore: empty components retain state; malformed or unreadable chunks stop replay restoration and release buffers");
    return 0;
}

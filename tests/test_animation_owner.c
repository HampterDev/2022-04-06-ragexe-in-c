#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "fileio/file.h"
#include "fileio/file_api.h"
#include "owned_storage_test_allocator.h"
static void *checked_calloc(size_t count, size_t size)
{
    void *data;
    assert(!size || count <= SIZE_MAX / size);
    data = checked_malloc(count * size);
    if (data) memset(data, 0, count * size);
    return data;
}
#define malloc checked_malloc
#define calloc checked_calloc
#define free checked_free
#include "../src/resources/act.c"
#undef malloc
#undef calloc
#undef free

struct GameFileMgr g_fileMgr;
static BYTE *input;
static int input_size, present = 1, borrowed, sprite_present = 1;
static unsigned sprite_queries, prefixes, failure_points;
static struct SprRes sprite;
static struct SprImg fixture_image;
struct Res *res_construct(struct Res *base)
{ memset(base,0,sizeof *base); base->typeIndex = -1; return base; }
void error_msg(const char *text, int flags) { assert(text && !flags); }
struct ResMgr *g_resMgr(void) { return (struct ResMgr *)(void *)&sprite; }
void *res_mgr_get(struct ResMgr *manager, const char *name)
{
    assert(manager == g_resMgr() && !strcmp(name,"fixture.spr")); ++sprite_queries;
    return sprite_present ? &sprite : NULL;
}
BYTE *game_file_mgr_get_data(struct GameFileMgr *manager, const char *name, int *size, BYTE disk)
{
    assert(manager == &g_fileMgr && name && !disk && !borrowed);
    if (!present) { *size = 0; return NULL; }
    *size = input_size; ++borrowed; return input;
}
void game_file_mgr_release_data(struct GameFileMgr *manager, void *data)
{ assert(manager == &g_fileMgr && data == input && borrowed == 1); --borrowed; }
BYTE game_file_mgr_is_file_exist(struct GameFileMgr *manager, const char *name, BYTE disk)
{ (void)manager; (void)name; (void)disk; return (BYTE)present; }
static void set_input(const void *data, size_t size)
{ assert(size <= INT_MAX && !borrowed); input = (BYTE *)data; input_size = (int)size; }
static int load(struct Res *base, const void *data, size_t size)
{ set_input(data,size); sprite_queries = 0; return resource_load(base,"fixture.act"); }
static void clean(void) { assert(!outstanding && !borrowed); }
static void put16(BYTE *data, unsigned value) { uint16_t x = (uint16_t)value; memcpy(data,&x,2); }
static void put32(BYTE *data, int32_t value) { memcpy(data,&value,4); }
static void putfloat(BYTE *data, float value) { memcpy(data,&value,4); }
static void setup_sprite(unsigned half)
{
    memset(&sprite,0,sizeof sprite); memset(&fixture_image,0,sizeof fixture_image);
    fixture_image.width = (short)(5 / (1+(half&1))); fixture_image.height = (short)(7 / (1+(half>>1)));
    fixture_image.widthHalved = (short)(half&1); fixture_image.heightHalved = (short)(half>>1);
    sprite.images[0].items = &fixture_image; sprite.images[0].count = 1; sprite_present = 1;
}
struct Locations { size_t motion_count, clip_count, clip, attachment_count, event_count, delay; };
static size_t make_fixture(BYTE *data, unsigned version, unsigned clips, struct Locations *where)
{
    size_t at = 16; unsigned m,c;
    memset(data,0,16); put16(data,0x4341); put16(data+2,version); put16(data+4,2);
    where->motion_count = at; put32(data+at,2); at += 4;
    for (m=0;m<2;++m) {
        int i;
        for (i=0;i<8;++i) { put32(data+at,(i+1)*(m+1)); at+=4; }
        if (!m) where->clip_count=at;
        put32(data+at,(int32_t)clips);at+=4;
        for (c=0;c<clips;++c) {
            size_t bytes=version<0x200?16:version<0x204?32:version<0x205?36:44;
            if (!m&&!c) where->clip=at;
            memset(data+at,0,bytes);put32(data+at,10);put32(data+at+4,20);put32(data+at+8,c?-1:0);put32(data+at+12,1);
            if (version>=0x200) {
                data[at+16]=10;data[at+17]=20;data[at+18]=30;data[at+19]=40;putfloat(data+at+20,2);
                if (version>=0x204) { putfloat(data+at+24,4);put32(data+at+28,90); }
                else put32(data+at+24,90);
                if (version>=0x205) { put32(data+at+36,5);put32(data+at+40,7); }
            }
            at+=bytes;
        }
        if (version>=0x200) { put32(data+at,0);at+=4; }
        if (version>=0x203) {
            if (!m)where->attachment_count=at;
            put32(data+at,2);at+=4;
            for(c=0;c<2;++c) {put32(data+at,99);put32(data+at+4,11+(int)c);put32(data+at+8,-13-(int)c);put32(data+at+12,9);at+=16;}
        }
    }
    put32(data+at,0);at+=4;
    if(version>=0x201) {where->event_count=at;put32(data+at,1);at+=4;memset(data+at,0,40);memcpy(data+at,"sound.wav",9);at+=40;}
    if(version>=0x202) {where->delay=at;putfloat(data+at,3.5f);putfloat(data+at+4,4.25f);at+=8;}
    return at;
}
static void verify_fixture(struct Res *base, unsigned version, unsigned half, unsigned clips)
{
    struct ActRes *resource=(struct ActRes*)base;
    struct SpriteAction *action=act_res_get_action(resource,0);
    struct Motion *m=act_res_get_motion(resource,0,0);
    struct SprClip *clip=motion_get_clip(m,0);
    int width=version>=0x205?5:fixture_image.width*(fixture_image.widthHalved?2:1);
    int height=version>=0x205?7:fixture_image.height*(fixture_image.heightHalved?2:1);
    (void)half;
    assert(animation_action_count(resource)==2 && action->motion_count==2 && m==action->motions);
    assert(actres_frame_count(resource,0)==2 && actres_frame_count(resource,1)==1 && actres_frame_count(resource,9)==1);
    assert(act_res_get_motion(resource,0,999)==m && act_res_get_motion(resource,9,1)->clip_count==0);
    assert(act_res_get_motion(resource,9,1)->eventId==-1);
    assert(m->clip_count==clips && m->range1.left==1 && m->range2.bottom==8);
    assert(clip->sprIndex==0 && clip->flags==1 && clip->clipType==0);
    if(version<0x200) {assert(clip->x==10 && clip->y==20 && clip->zoomX==1 && clip->color[0]==255);}
    else {
        assert(clip->color[0]==10&&clip->color[3]==40&&clip->angle==90&&clip->zoomX==2);
        assert(clip->zoomY==(version<0x204?2.0f:4.0f));
        assert(clip->x==(int)((10.0f-(width/2+width%2)*2.0f)/2.0f));
        assert(clip->y==(int)((20.0f-(height/2+height%2)*clip->zoomY)/clip->zoomY));
    }
    assert(motion_get_clip(m,clips)->sprIndex==-1);
    assert(m->eventId==(version>=0x201?0:-1));
    assert(m->attachment_count==(version>=0x203?2u:0u));
    if(version>=0x203) {assert(m->attachments[1].x==12&&m->attachments[1].y==-14&&m->attachments[1].attr==9);}
    assert(!animation_attachment(m,99)&&!animation_attachment(NULL,0));
    assert(act_res_get_delay(resource,0)==(version>=0x202?3.5f:4.0f));
    assert(act_res_get_delay(resource,1)==(version>=0x202?4.25f:4.0f));
    assert(act_res_get_delay(resource,9)==4.0f);
    assert(!strcmp(act_res_get_event_name(resource,0),version>=0x201?"sound.wav":""));
    assert(!*act_res_get_event_name(resource,999));
}
static void test_versions(void)
{
    const unsigned versions[]={0,0xff,0x100,0x200,0x201,0x202,0x203,0x204,0x205};
    BYTE data[1024]; struct Locations loc; unsigned v,h;
    for(v=0;v<sizeof versions/sizeof *versions;++v)for(h=0;h<4;++h){
        struct Res *base=animation_resource_create();size_t size=make_fixture(data,versions[v],2,&loc);
        setup_sprite(h);assert(load(base,data,size));verify_fixture(base,versions[v],h,2);
        assert(sprite_queries==((versions[v]>=0x200&&versions[v]<0x205)?1u:0u));
        data[size++]=0xaa;assert(load(base,data,size));verify_fixture(base,versions[v],h,2);
        resource_destroy(base);clean();
    }
    assert(!animation_action_count(NULL)&&actres_frame_count(NULL,0)==1);
    assert(motion_get_clip(NULL,0)->sprIndex==-1&&act_res_get_motion(NULL,0,0)->eventId==-1);
}
static void test_prefixes_and_allocations(void)
{
    const unsigned versions[]={0xff,0x100,0x200,0x201,0x202,0x203,0x204,0x205};
    BYTE data[1024]; SYSTEM_INFO sys; DWORD old; BYTE *pages,*guard; unsigned v;
    GetSystemInfo(&sys);pages=VirtualAlloc(NULL,sys.dwPageSize*2,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);assert(pages);
    guard=pages+sys.dwPageSize;assert(VirtualProtect(guard,sys.dwPageSize,PAGE_NOACCESS,&old));setup_sprite(0);
    fail_at=allocation_calls+1;assert(!animation_resource_create());fail_at=0;clean();
    for(v=0;v<sizeof versions/sizeof *versions;++v){
        struct Locations loc;size_t size=make_fixture(data,versions[v],2,&loc),n;
        struct Res *base=animation_resource_create();struct ActRes saved;unsigned held,start,count,point;
        start=allocation_calls;assert(load(base,data,size));count=allocation_calls-start;saved=*(struct ActRes*)base;held=outstanding;
        for(n=0;n<size;++n){
            memcpy(guard-n,data,n);
            if(versions[v]>=0x202&&n>=loc.delay){
                assert(load(base,guard-n,n));
                assert(act_res_get_delay((struct ActRes*)base,0)==(n>=loc.delay+4?3.5f:4.0f));
                assert(act_res_get_delay((struct ActRes*)base,1)==4.0f);
                continue;
            }
            assert(!load(base,guard-n,n));
            assert(!memcmp(base,&saved,sizeof saved)&&outstanding==held&&!borrowed);++prefixes;
        }
        assert(load(base,data,size));saved=*(struct ActRes*)base;held=outstanding;
        for(point=1;point<=count;++point){
            fail_at=allocation_calls+point;assert(!load(base,data,size));fail_at=0;
            assert(!memcmp(base,&saved,sizeof saved)&&outstanding==held&&!borrowed);++failure_points;
        }
        verify_fixture(base,versions[v],0,2);resource_destroy(base);clean();
    }
    assert(VirtualFree(pages,0,MEM_RELEASE));
}
static void test_malformed_and_replacement(void)
{
    BYTE data[1024],bad[1024];struct Locations loc;size_t size=make_fixture(data,0x205,2,&loc);
    struct Res *base=animation_resource_create();struct ActRes saved;unsigned i,held;
    struct SprClip *clip;
    size_t offsets[]={0,2,4,0,0,0,0,0,0,0};
    int32_t values[]={0,0x206,0xffff,-1,INT_MAX,-1,INT_MAX,-1,INT_MAX,-1};
    offsets[3]=offsets[4]=loc.motion_count;offsets[5]=offsets[6]=loc.clip_count;
    offsets[7]=offsets[8]=loc.attachment_count;offsets[9]=loc.event_count;
    setup_sprite(0);assert(load(base,data,size));saved=*(struct ActRes*)base;held=outstanding;
    for(i=0;i<sizeof offsets/sizeof *offsets;++i){
        memcpy(bad,data,size);if(i<3)put16(bad+offsets[i],values[i]);else put32(bad+offsets[i],values[i]);
        assert(!load(base,bad,size)&&!memcmp(base,&saved,sizeof saved)&&outstanding==held);
    }
    present=0;assert(!resource_load(base,"missing.act"));present=1;assert(!resource_load(base,NULL));
    set_input(data,size);input_size=-1;assert(!resource_load(base,"negative.act"));assert(!memcmp(base,&saved,sizeof saved));
    memcpy(bad,data,size);putfloat(bad+loc.clip+20,0);assert(load(base,bad,size));assert(motion_get_clip(act_res_get_motion((struct ActRes*)base,0,0),0)->x==INT_MIN);
    memcpy(bad,data,size);put32(bad+loc.clip+20,0x7fc00000);assert(load(base,bad,size));
    clip=motion_get_clip(act_res_get_motion((struct ActRes*)base,0,0),0);assert(isnan(clip->zoomX)&&clip->x==INT_MIN&&clip->y==1);
    memcpy(bad,data,size);put32(bad+loc.clip+36,-5);assert(load(base,bad,size));
    assert(motion_get_clip(act_res_get_motion((struct ActRes*)base,0,0),0)->x==8);
    memcpy(bad,data,size);putfloat(bad+loc.clip+20,1e-30f);assert(load(base,bad,size));
    assert(motion_get_clip(act_res_get_motion((struct ActRes*)base,0,0),0)->x==INT_MIN);
    memcpy(bad,data,size);putfloat(bad+loc.delay,0);put32(bad+loc.delay+4,0x7fc00000);assert(load(base,bad,size));
    assert(act_res_get_delay((struct ActRes*)base,0)==0.0f&&isnan(act_res_get_delay((struct ActRes*)base,1)));
    memcpy(bad,data,size);putfloat(bad+loc.delay,-2);assert(load(base,bad,size));assert(act_res_get_delay((struct ActRes*)base,0)==-2.0f);
    memcpy(bad,data,size);put32(bad+loc.clip+8,-2);assert(load(base,bad,size));
    assert(motion_get_clip(act_res_get_motion((struct ActRes*)base,0,0),0)->x==(int)((10.0f-3*2.0f)/2.0f));
    memcpy(bad,data,size);put32(bad+loc.clip+8,-858993460);putfloat(bad+loc.clip+20,0);assert(load(base,bad,size));
    assert(motion_get_clip(act_res_get_motion((struct ActRes*)base,0,0),0)->sprIndex==-858993460);
    memcpy(bad,data,size);memset(bad+loc.event_count+4,'x',40);assert(load(base,bad,size));assert(strlen(act_res_get_event_name((struct ActRes*)base,0))==40);
    assert(load(base,data,size));animation_data_clear(&((struct ActRes*)base)->data);animation_data_clear(&((struct ActRes*)base)->data);
    assert(outstanding==1);assert(load(base,data,size));verify_fixture(base,0x205,0,2);resource_destroy(base);clean();
}
static void test_missing_sprite_and_large_motion(void)
{
    BYTE *data=malloc(100000);struct Locations loc;size_t size;struct Res *base=animation_resource_create();struct SprClip *clip;
    assert(data);setup_sprite(0);size=make_fixture(data,0x204,2,&loc);sprite_present=0;
    assert(load(base,data,size));clip=motion_get_clip(act_res_get_motion((struct ActRes*)base,0,0),0);assert(clip->x==10&&clip->y==20);
    sprite_present=1;fixture_image.width=0;assert(load(base,data,size));assert(motion_get_clip(act_res_get_motion((struct ActRes*)base,0,0),0)->x==10);
    setup_sprite(0);size=make_fixture(data,0x205,1001,&loc);assert(load(base,data,size));verify_fixture(base,0x205,0,1001);
    resource_destroy(base);free(data);clean();
}

static uint32_t read_u32(FILE *file)
{
    uint32_t value; assert(fread(&value,4,1,file)==1); return value;
}
struct Expected { const BYTE *at, *end; };
static const BYTE *expected_take(struct Expected *input, size_t size)
{
    const BYTE *value=input->at; assert(size <= (size_t)(input->end-input->at));input->at+=size;return value;
}
static uint32_t expected_u32(struct Expected *input)
{ uint32_t value;memcpy(&value,expected_take(input,4),4);return value; }
static unsigned compare_expected(struct ActRes *resource,const BYTE *bytes,size_t size)
{
    struct Expected expected={bytes,bytes+size};size_t a,m,c;unsigned frames=0;
    uint32_t count=expected_u32(&expected);assert(animation_action_count(resource)==count);
    for(a=0;a<count;++a){
        struct SpriteAction *action=act_res_get_action(resource,(unsigned)a);
        uint32_t motions=expected_u32(&expected);float delay;
        memcpy(&delay,expected_take(&expected,4),4);assert(action->motion_count==motions&&action->delay==delay);
        assert(actres_frame_count(resource,(unsigned)a)==(motions?motions:1));
        for(m=0;m<motions;++m){
            struct Motion *motion=act_res_get_motion(resource,(unsigned)a,(unsigned)m);
            uint32_t clips,attach;int32_t event;
            assert(!memcmp(&motion->range1,expected_take(&expected,16),16));
            assert(!memcmp(&motion->range2,expected_take(&expected,16),16));
            clips=expected_u32(&expected);assert(motion->clip_count==clips);
            for(c=0;c<clips;++c){
                const BYTE *record=expected_take(&expected,36);struct SprClip *clip=motion_get_clip(motion,(unsigned)c);
                if(memcmp(clip,record,36)){fprintf(stderr,"clip mismatch: action %zu frame %zu clip %zu\n",a,m,c);abort();}
            }
            memcpy(&event,expected_take(&expected,4),4);assert(motion->eventId==event);
            attach=expected_u32(&expected);assert(motion->attachment_count==attach);
            for(c=0;c<attach;++c)assert(!memcmp(animation_attachment(motion,c),expected_take(&expected,12),12));
            ++frames;
        }
    }
    count=expected_u32(&expected);assert(resource->data.event_count==count);
    for(a=0;a<count;++a){
        const BYTE *name=expected_take(&expected,40);assert(!memcmp(act_res_get_event_name(resource,(unsigned)a),name,40));
        assert(act_res_get_event_name(resource,(unsigned)a)[40]==0);
    }
    assert(expected.at==expected.end);return frames;
}
static void corpus(const char *name)
{
    FILE *file=fopen(name,"rb");uint32_t size;unsigned files=0;unsigned long long frames=0;
    assert(file&&sizeof(struct SprClip)==36&&sizeof(struct AttachPointInfo)==12);
    while(fread(&size,4,1,file)==1){
        BYTE *data=malloc(size),*expected[4]={0};uint32_t expected_sizes[4]={0},counts[2],variants;unsigned kind,i,h;
        uint16_t (*dimensions[2])[2]={0};
        assert(data&&fread(data,1,size,file)==size);memset(&sprite,0,sizeof sprite);sprite_present=1;
        for(kind=0;kind<2;++kind){
            counts[kind]=read_u32(file);
            if(counts[kind]){
                dimensions[kind]=malloc((size_t)counts[kind]*4);sprite.images[kind].items=calloc(counts[kind],sizeof(struct SprImg));
                assert(dimensions[kind]&&sprite.images[kind].items&&fread(dimensions[kind],4,counts[kind],file)==counts[kind]);
                sprite.images[kind].count=counts[kind];
            }
        }
        variants=read_u32(file);assert(variants==1||variants==4);
        for(i=0;i<variants;++i){expected_sizes[i]=read_u32(file);expected[i]=malloc(expected_sizes[i]);assert(expected[i]&&fread(expected[i],1,expected_sizes[i],file)==expected_sizes[i]);}
        for(h=0;h<4;++h){
            struct Res *base=animation_resource_create();unsigned long long actual;
            for(kind=0;kind<2;++kind)for(i=0;i<counts[kind];++i){
                struct SprImg *image=&sprite.images[kind].items[i];
                image->width=(short)(dimensions[kind][i][0]/(1+(h&1)));image->height=(short)(dimensions[kind][i][1]/(1+(h>>1)));
                image->widthHalved=(short)(h&1);image->heightHalved=(short)(h>>1);
            }
            if(!load(base,data,size)){fprintf(stderr,"load failed: record %u half %u\n",files,h);abort();}
            actual=compare_expected((struct ActRes*)base,expected[variants==1?0:h],expected_sizes[variants==1?0:h]);
            if(!h)frames+=actual;
            resource_destroy(base);clean();
        }
        for(kind=0;kind<2;++kind){free(dimensions[kind]);free(sprite.images[kind].items);}
        for(i=0;i<variants;++i)free(expected[i]);free(data);++files;
    }
    assert(feof(file)&&!ferror(file));fclose(file);
    printf("ACT corpus: %u files, %llu frames, four half-size settings match independent records.\n",files,frames);
}

int main(int argc,char **argv)
{
    if(argc==2){corpus(argv[1]);return 0;}
    test_versions();test_prefixes_and_allocations();test_malformed_and_replacement();test_missing_sprite_and_large_motion();
    printf("ACT owner: nine versions, four half-size settings, %u guarded prefixes, %u allocation failures, sparse images, fixed event names and transactional replacement pass.\n",prefixes,failure_points);
    return 0;
}

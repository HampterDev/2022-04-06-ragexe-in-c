#define TEST_RESOURCE_LOAD
#define TEST_RENDER_NODES
#define TEST_MODEL_ACTORS
#include "model_mesh_test_support.h"
#include "resources/model_res.h"
#include "render/texture_cache.h"
#include "render/keyframes.h"
#include "model_owner_test_support.h"

struct TexCache g_textureCache;
static struct Texture texture;
static unsigned locks, releases, closes, destructs, errors;
static int open_failure, bad_texture, parent_case, bad_root;
static int first_key_offset, blocks_offset, names_offset, nodes_offset, texture_offset;
static const char *root_name = "root";
static const char *movie_name = "영화.bik";

void *mem_alloc(size_t bytes)
{
    unsigned failure = fail_at;
    void *p;
    fail_at = 0;
    p = checked_malloc(bytes);
    fail_at = failure;
    --allocation_calls;
    return p;
}
void mem_free(void *p)
{ checked_free(p); }

void fatal_error(const char *message) { (void)message; abort(); }
struct Res *res_construct(struct Res *res) { memset(res, 0, sizeof *res); return res; }
void res_destruct(struct Res *res) { (void)res; }
void error_msg(const char *message, int flags) { assert(message && !flags); ++errors; }

struct File *file_construct(struct File *file) { memset(file, 0, sizeof *file); return file; }
BYTE file_open(struct File *file, const char *name, int kind, BYTE a, BYTE b)
{
    assert(name && kind == 0 && a == 1 && b == 0);
    assert(!strcmp(name, "모델.rsm"));
    if (open_failure) return 0;
    *file = stream(used);
    return 1;
}
void file_close(struct File *file) { (void)file; ++closes; }
void file_destruct(struct File *file) { (void)file; ++destructs; }
struct Texture *texture_cache_get(struct TexCache *cache, const char *name, int a, int b)
{
    assert(cache == &g_textureCache && name && !a && !b);
    return &texture;
}
void texture_lock(struct Texture *value) { assert(value == &texture); ++locks; }
void texture_unlock(struct Texture *value) { assert(value == &texture && locks); --locks; }
void ui_tex_cache_release(struct TexCache *cache, struct Texture *value)
{ assert(cache == &g_textureCache && value == &texture); ++releases; }

static void name_record(const char *name, int fixed)
{
    char bytes[40] = {0};
    size_t size = strlen(name);
    if (fixed) {
        assert(size <= sizeof bytes);
        memcpy(bytes, name, size);
        put(bytes, sizeof bytes);
    } else {
        integer((int)size);
        put(name, (int)size);
    }
}

static void uv_fixture(void)
{
    const struct KeyFloat old[] = {{0,100}, {10,105}};
    const struct KeyFloat offset[] = {{0,0}, {10,1}, {20,2}};
    const struct KeyFloat scale[] = {{0,1}, {20,3}};
    const struct KeyFloat single = {3,9};
    struct KeyFloat skipped[33] = {0};
    integer(4);
    integer(2); integer(2);
    integer(0); integer(2); put(old, sizeof old);
    integer(4); integer(1); put(&single, sizeof single);
    integer(-1); integer(1);
    integer(8); integer(33); put(skipped, sizeof skipped);
    integer(0); integer(2);
    integer(1); integer(1); put(&single, sizeof single);
    integer(2); integer(0);
    integer(2); integer(4);
    integer(0); integer(2); put(offset, 2 * sizeof *offset);
    integer(0); integer(1); put(offset + 2, sizeof *offset);
    integer(3); integer(2); put(scale, sizeof scale);
    integer(9); integer(1); put(&single, sizeof single);
}

static void animation_fixture(int major, int minor)
{
    const struct KeyQuat rotations[] = {{0,0,0,0,1}, {10,0,0,0,1}};
    const struct KeyVec scales[] = {{0,1,1,1,0}, {10,2,2,2,0}};
    const struct KeyVec positions[] = {{0,0,0,0,0}, {10,10,0,0,0}};
    if (major == 2) { integer(2); put(scales, sizeof scales); }
    integer(2); put(rotations, sizeof rotations);
    if (major == 2) {
        integer(2); put(positions, sizeof positions);
        if (minor >= 3) uv_fixture();
    }
}

static void resource_fixture(int major, int minor)
{
    unsigned char mesh[4096], version[2] = {(unsigned char)major, (unsigned char)minor};
    unsigned char alpha = 173, reserved[16] = {0};
    float identity[12] = {1,0,0, 0,1,0, 0,0,1, 0,0,0};
    float placement[10] = {0,0,0, 0, 0,0,1, 1,1,1}, fps = 30;
    int mesh_size, node, texture_index;
    fixture(major, minor, 1, 3);
    mesh_size = used; memcpy(mesh, input, (size_t)used);
    used = 0; put("GRSM", 4); put(version, 2); integer(120); integer(1);
    if (major == 2 || minor >= 4) put(&alpha, 1);
    if (major == 1) put(reserved, sizeof reserved); else put(&fps, sizeof fps);
    names_offset = used;
    if (major == 1 || minor < 3) { integer(1); name_record(movie_name, major == 1); }
    if (major == 2) integer(1);
    name_record(bad_root ? "absent" : root_name, major == 1);
    nodes_offset = used; integer(2);
    for (node = 0; node < 2; ++node) {
        name_record(node ? "child" : root_name, major == 1);
        name_record(node ? (parent_case == 1 ? "absent" : root_name) :
                    (parent_case == 2 ? "child" : ""), major == 1);
        integer(3);
        for (texture_index = 0; texture_index < 3; ++texture_index) {
            if (major == 2 && minor >= 3) name_record(movie_name, 0); else {
                if (!node && !texture_index) texture_offset = used;
                integer(bad_texture ? -1 : 0);
            }
        }
        put(identity, sizeof identity);
        if (major == 1) put(placement, sizeof placement);
        put(mesh, mesh_size);
        if (!node) first_key_offset = used;
        animation_fixture(major, minor);
    }
    blocks_offset = used;
    integer(major == 1 && minor < 2 ? 0 : 2);
    if (major == 2 || minor >= 2) {
        struct VolumeBox box = {0};
        int i;
        for (i = 0; i < 2; ++i) {
            box.size[0] = 5; box.size[1] = 7; box.size[2] = 9;
            box.pos[0] = -3; box.rot[2] = 0.5f; box.flags = i;
            if (major == 2) put(&box, sizeof box);
            else { put(&box, 0x24); if (minor >= 3) integer(i); }
        }
    }
}

static void verify_contents(struct C3dModelRes *model, int major, int minor)
{
    struct ModelResNode *root = model_res_find_node(model, root_name);
    struct ModelResNode *child = model_res_find_node(model, "child");
    assert(root && child && model->nodeCount == 2 && model->mainNodes.size == 1);
    assert(root->mesh && child->mesh && locks == 6);
    assert(root->textureCount == 3 && child->textureCount == 3);
    if (major == 1) {
        struct ModelResNodeV1 *v1 = (struct ModelResNodeV1 *)root;
        assert(v1->rotKeyCount == 2 && v1->rotKeys[1].w == 1);
    } else {
        struct ModelResNodeV2 *v2 = (struct ModelResNodeV2 *)root;
        assert(v2->scaleKeyCount == 2 && v2->scaleKeys[1].x == 2);
        assert(v2->posKeyCount == 2 && v2->posKeys[1].x == 10);
        assert(v2->rotKeyCount == 2 && v2->rotKeys[1].w == 1);
        if (minor >= 3) {
            float uv[5] = {0,0,1,1,0};
            const struct UvKeyTrack *track = v2->uvKeys.tracks;
            assert(v2->uvKeys.count == 3);
            assert(track[0].texture == -1 && track[1].texture == 0 && track[2].texture == 2);
            assert(!track[0].channels[0].count && !track[2].channels[4].count);
            assert(track[2].channels[0].count == 3);
            key_uv_channels_eval(track[2].channels, 5, uv);
            assert(uv[0] == .5f && uv[1] == 0 && uv[2] == 1 && uv[3] == 1.5f && uv[4] == 0);
        }
    }
    assert(!strcmp(owned_text_cstr(&model->fileName), "모델.rsm"));
    assert(!strcmp(owned_text_cstr(&root->name), root_name));
    if (parent_case == 1) assert(major == 2 && !child->parent && !root->childCount);
    else assert(child->parent == root && root->childCount == 1 && root->children[0] == child);
    assert(model->videoNames.size == (major == 1 ? 1u : 0u));
    if (major == 1) assert(!strcmp(owned_text_cstr(&model->videoNames.items[0]), movie_name));
    if (major == 1 && minor < 2) assert(!model->blockCount);
    else {
        assert(model->blockCount == 2 && model->blocks[0].pos[0] == -3);
        assert(model->blocks[0].size[0] == (major == 1 && minor >= 3 ? 8 : 5));
        assert(model->blocks[0].size[2] == (major == 1 && minor >= 3 ? 12 : 9));
        assert(model->blocks[1].size[0] == 5 && model->blocks[1].rot[2] == 0.5f);
        assert(model->blocks[1].flags == (major == 1 && minor < 3 ? 0 : 1));
    }
}

static void check_invalid(int major)
{
    struct C3dModelRes model = {0};
    model_res_init(&model);
    assert(!(major == 1 ? model_resource_load_v1(&model, "모델.rsm") :
                         model_resource_load_v2(&model, "모델.rsm")));
    assert(!model.nodeCount && !model.nodes && !model.fileName.data);
    assert(!outstanding && !locks);
}

static void check_short_boxes(int major, int cut)
{
    struct C3dModelRes model = {0};
    size_t boxes = cut < blocks_offset + 4 ? 0 : 2;
    model_res_init(&model);
    assert(major == 1 ? model_resource_load_v1(&model, "모델.rsm") :
                        model_resource_load_v2(&model, "모델.rsm"));
    assert(model.nodeCount == 2 && model.blockCount == boxes);
    if (boxes)
        assert(model.blocks[1].size[0] == 0 || model.blocks[1].size[0] == 3 || model.blocks[1].size[0] == 5);
    model_res_clear(&model);
    assert(!outstanding && !locks);
}

static void malformed_and_reload(void)
{
    int major, minor, cut, complete, value;
    for (major = 1; major <= 2; ++major) {
        struct Res *resource;
        struct C3dModelRes *model;
        minor = major == 1 ? 5 : 3;
        resource_fixture(major, minor);
        allocation_calls = 0; fail_at = 1;
        assert(!(major == 1 ? model_resource_v1_create() : model_resource_v2_create()));
        fail_at = 0;
        resource = major == 1 ? model_resource_v1_create() : model_resource_v2_create();
        assert(resource && resource->ops && outstanding == 1);
        model = (struct C3dModelRes *)resource;
        assert(resource->ops->load(resource, "모델.rsm"));
        verify_contents(model, major, minor);
        allocation_calls = 0; fail_at = 1;
        assert(!resource->ops->load(resource, owned_text_cstr(&model->fileName)));
        fail_at = 0;
        verify_contents(model, major, minor);
        assert(resource->ops->load(resource, owned_text_cstr(&model->fileName)));
        verify_contents(model, major, minor);
        resource->ops->destroy(resource);
        assert(!locks && !outstanding);

        parent_case = 2; resource_fixture(major, minor); check_invalid(major);
        parent_case = 1; resource_fixture(major, minor);
        if (major == 1) check_invalid(major);
        else {
            struct C3dModelRes detached = {0};
            assert(model_resource_load_v2(&detached, "모델.rsm"));
            verify_contents(&detached, major, minor); model_res_clear(&detached);
        }
        parent_case = 0;
        resource_fixture(major, minor);
        complete = used;
        for (cut = 0; cut < complete; ++cut) {
            used = cut;
            if (cut >= blocks_offset) check_short_boxes(major, cut);
            else check_invalid(major);
        }
        used = complete;
        open_failure = 1; check_invalid(major); open_failure = 0;
        input[4] = 7; check_invalid(major);
        resource_fixture(major, minor); input[5] = 99; check_invalid(major);
        resource_fixture(major, minor); value = -1; memcpy(input + nodes_offset, &value, 4); check_invalid(major);
        resource_fixture(major, minor); memcpy(input + names_offset, &value, 4); check_invalid(major);
    }
    bad_texture = 1; resource_fixture(1, 5); check_invalid(1);
    resource_fixture(2, 2); check_invalid(2); bad_texture = 0;
    resource_fixture(2, 2); value = 1; memcpy(input + texture_offset, &value, 4); check_invalid(2);
    bad_root = 1; resource_fixture(2, 3); check_invalid(2); bad_root = 0;
    assert(!model_res_node_create(0) && !model_res_node_create(3));
    assert(!outstanding && !locks);
}

static void runtime_ownership(void)
{
    int version;
    for (version = 1; version <= 2; ++version) {
        struct C3dModelRes model = {0};
        unsigned failure, total = 0, resources;
        int minor = version == 1 ? 5 : 3;
        resource_fixture(version, minor);
        assert(version == 1 ? model_resource_load_v1(&model, "모델.rsm") :
                              model_resource_load_v2(&model, "모델.rsm"));
        resources = outstanding;
        for (failure = 0; failure <= total; ++failure) {
            struct C3dActor *actor;
            int ok = 0;
            allocation_calls = 0; fail_at = failure;
            actor = model_actor_create(version);
            if (actor && c3d_actor_add_volume_boxes(actor, model.blocks, model.blockCount))
                ok = actor->ops->create(actor, &model);
            fail_at = 0;
            if (!failure) {
                struct C3dNode *root, *child;
                unsigned before;
                assert(ok);
                total = allocation_calls;
                root = version == 1 ? ((struct C3dActorV1 *)actor)->root : ((struct C3dActorV2 *)actor)->nodes[0];
                assert(root && root->childCount == 1);
                child = root->children[0];
                assert(!strcmp(root->name, root_name) && !strcmp(child->name, "child") && child->parent == root);
                assert(root->actor == actor && child->actor == actor);
                assert(root->resource == model.nodes[0] && root->mesh == model.nodes[0]->mesh);
                assert(root->faceInfo && root->lightInts && child->faceInfo && child->lightInts);
                assert(actor->volumeBoxCount == model.blockCount && actor->volumeBoxes != model.blocks);
                before = allocation_calls;
                actor->ops->animate(actor, version == 1 ? 5 : 500);
                assert(allocation_calls == before);
                if (version == 2) {
                    const struct UvTransform *uv = uv_xform_find((struct C3dNodeV2 *)root, 2);
                    assert(uv && uv->offset[0] == 1.5f && uv->scale[1] == 2.5f);
                }
                assert(c3d_actor_add_volume_boxes(actor, actor->volumeBoxes, 1));
                assert(actor->volumeBoxCount == model.blockCount + 1);
                assert(!memcmp(actor->volumeBoxes, actor->volumeBoxes + model.blockCount,
                               sizeof *actor->volumeBoxes));
            } else assert(!ok);
            if (actor) actor->ops->destroy(actor);
            assert(outstanding == resources && locks == 6);
            verify_contents(&model, version, minor);
        }
        model_res_clear(&model);
        assert(!outstanding && !locks);
    }
    assert(!model_actor_create(0) && !model_actor_create(3));
}

static void atomic_key_readers(void)
{
    int kind, cut, complete;
    for (kind = 0; kind < 3; ++kind) {
        struct KeyVec *vectors = NULL;
        struct KeyQuat *rotations = NULL;
        struct UvAnimation uv = {0};
        size_t count = 0;
        void *original;
        unsigned resources, failure, total;
        struct File file;
        used = 0;
        if (kind == 2) uv_fixture();
        else {
            const struct KeyVec keys[] = {{0,1,2,3,0}, {10,4,5,6,0}};
            integer(2); put(keys, sizeof keys);
        }
        complete = used; file = stream(complete); allocation_calls = 0;
        assert(kind == 0 ? key_vec_read(&file, &vectors, &count) :
               kind == 1 ? key_quat_read(&file, &rotations, &count) : key_uv_read(&file, &uv));
        total = allocation_calls;
        original = kind == 0 ? (void *)vectors : kind == 1 ? (void *)rotations : (void *)uv.tracks;
        resources = outstanding;
        for (cut = 0; cut < complete; ++cut) {
            file = stream(cut);
            assert(!(kind == 0 ? key_vec_read(&file, &vectors, &count) :
                     kind == 1 ? key_quat_read(&file, &rotations, &count) : key_uv_read(&file, &uv)));
            assert(outstanding == resources);
        }
        for (failure = 1; failure <= total; ++failure) {
            file = stream(complete); allocation_calls = 0; fail_at = failure;
            assert(!(kind == 0 ? key_vec_read(&file, &vectors, &count) :
                     kind == 1 ? key_quat_read(&file, &rotations, &count) : key_uv_read(&file, &uv)));
            fail_at = 0;
            assert(outstanding == resources);
        }
        for (cut = 0; cut < 2; ++cut) {
            int invalid = cut ? INT_MAX : -1;
            memcpy(input, &invalid, sizeof invalid); file = stream(complete);
            assert(!(kind == 0 ? key_vec_read(&file, &vectors, &count) :
                     kind == 1 ? key_quat_read(&file, &rotations, &count) : key_uv_read(&file, &uv)));
            assert(outstanding == resources);
        }
        assert(original == (kind == 0 ? (void *)vectors : kind == 1 ? (void *)rotations : (void *)uv.tracks));
        if (kind == 2) assert(uv.count == 3 && uv.tracks[2].channels[0].count == 3);
        else assert(count == 2 && (kind == 0 ? vectors[1].x : rotations[1].x) == 4);
        used = 0; integer(0); file = stream(used);
        assert(kind == 0 ? key_vec_read(&file, &vectors, &count) :
               kind == 1 ? key_quat_read(&file, &rotations, &count) : key_uv_read(&file, &uv));
        assert(!vectors && !rotations && !count && !uv.tracks && !uv.count && !outstanding);
    }
}

int main(void)
{
    int major, minor;
    for (major = 1; major <= 2; ++major) {
        for (minor = 1; minor <= (major == 1 ? 5 : 3); ++minor) {
            unsigned failure = 0, total = 1;
            resource_fixture(major, minor);
            do {
                struct C3dModelRes model = {0};
                unsigned before_closes = closes, before_destructs = destructs;
                int result;
                model_res_init(&model);
                allocation_calls = 0; fail_at = failure;
                result = major == 1 ? model_resource_load_v1(&model, "모델.rsm") : model_resource_load_v2(&model, "모델.rsm");
                fail_at = 0;
                assert(closes == before_closes + (failure == 1 ? 0 : 1));
                assert(destructs == before_destructs + (failure == 1 ? 0 : 1));
                if (!failure) {
                    assert(result);
                    verify_contents(&model, major, minor);
                    total = allocation_calls;
                } else {
                    assert(!result && !model.nodeCount && !model.mainNodes.size && !locks);
                }
                model_res_clear(&model);
                res_destruct(&model.res);
                assert(!outstanding && !locks);
            } while (++failure <= total);
        }
    }
    malformed_and_reload();
    atomic_key_readers();
    runtime_ownership();
    root_name = "a_model_node_name_much_longer_than_the_old_forty_byte_stack_buffer_한국어";
    {
        struct C3dModelRes model = {0};
        resource_fixture(2, 3);
        assert(model_resource_load_v2(&model, "모델.rsm"));
        verify_contents(&model, 2, 3); model_res_clear(&model);
        assert(!outstanding && !locks);
    }
    puts("model resource loading: both real loaders release partial nodes, earlier nodes, texture locks, names, arrays and files at every C owner allocation failure");
    return 0;
}

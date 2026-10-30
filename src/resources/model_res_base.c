#include <stdlib.h>
#include <string.h>
#include "resources/model_res.h"

const char g_model_res_not_a_model[] = "모델파일이 아닙니다. 형식을 다시한번 확인해주세요.";

void model_res_init(struct C3dModelRes *model)
{
    memset(model, 0, sizeof *model);
    res_construct(&model->res);
    model->alpha = 255;
}

void model_res_clear(struct C3dModelRes *model)
{
    size_t i;
    for (i = 0; i < model->nodeCount; ++i)
        if (model->nodes[i]) model->nodes[i]->destroy(model->nodes[i]);
    free(model->nodes);
    model->nodes = NULL;
    model->nodeCount = 0;
    text_array_clear(&model->mainNodes);
    free(model->blocks);
    model->blocks = NULL;
    model->blockCount = 0;
    owned_text_clear(&model->fileName);
    text_array_clear(&model->videoNames);
}

static struct Res *model_resource_create(const struct ResourceOps *ops)
{
    struct C3dModelRes *model = calloc(1, sizeof *model);
    if (!model) return NULL;
    model_res_init(model);
    model->res.ops = ops;
    return &model->res;
}

struct Res *model_resource_v1_create(void) { return model_resource_create(&model_v1_resource_ops); }
struct Res *model_resource_v2_create(void) { return model_resource_create(&model_v2_resource_ops); }

static void model_resource_destroy(struct Res *resource)
{
    struct C3dModelRes *model = (struct C3dModelRes *)resource;
    model_res_clear(model);
    res_destruct(resource);
    free(model);
}

static int model_v1_resource_load(struct Res *resource, const char *name)
{ return model_resource_load_v1((struct C3dModelRes *)resource, name); }
static int model_v2_resource_load(struct Res *resource, const char *name)
{ return model_resource_load_v2((struct C3dModelRes *)resource, name); }

const struct ResourceOps model_v1_resource_ops = {
    .destroy = model_resource_destroy, .load = model_v1_resource_load,
};
const struct ResourceOps model_v2_resource_ops = {
    .destroy = model_resource_destroy, .load = model_v2_resource_load,
};

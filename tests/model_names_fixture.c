#include "resources/model_res.h"

static struct C3dModelRes resource;
static struct OwnedText names[] = {{"first", 5}, {"second", 6}};

struct C3dModelRes *test_model_names(void)
{
    resource.mainNodes.items = names; resource.mainNodes.size = 2;
    resource.fps = 20; resource.frameCount = 30; resource.shadeType = 2;
    return &resource;
}

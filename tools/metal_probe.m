#include "metal_probe.h"
#include <stdlib.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

struct MetalProbe {
    CAMetalLayer *layer;
    id<MTLDevice> device;
    id<MTLCommandQueue> queue;
};

struct MetalProbe *metal_probe_create(void *layer)
{
    struct MetalProbe *probe = calloc(1, sizeof *probe);
    if (!probe || !layer) {
        free(probe);
        return NULL;
    }
    probe->layer = [(CAMetalLayer *)layer retain];
    probe->device = MTLCreateSystemDefaultDevice();
    probe->queue = [probe->device newCommandQueue];
    if (!probe->queue) {
        metal_probe_destroy(probe);
        return NULL;
    }
    probe->layer.device = probe->device;
    probe->layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    return probe;
}

int metal_probe_draw(struct MetalProbe *probe, int width, int height)
{
    @autoreleasepool {
        probe->layer.drawableSize = CGSizeMake(width, height);
        id<CAMetalDrawable> drawable = [probe->layer nextDrawable];
        if (!drawable) return 0;
        MTLRenderPassDescriptor *pass = [MTLRenderPassDescriptor renderPassDescriptor];
        pass.colorAttachments[0].texture = drawable.texture;
        pass.colorAttachments[0].loadAction = MTLLoadActionClear;
        pass.colorAttachments[0].storeAction = MTLStoreActionStore;
        pass.colorAttachments[0].clearColor = MTLClearColorMake(0.08, 0.16, 0.24, 1.0);
        id<MTLCommandBuffer> commands = [probe->queue commandBuffer];
        id<MTLRenderCommandEncoder> encoder = [commands renderCommandEncoderWithDescriptor:pass];
        if (!encoder) return 0;
        [encoder endEncoding];
        [commands presentDrawable:drawable];
        [commands commit];
        [commands waitUntilCompleted];
        return commands.status == MTLCommandBufferStatusCompleted;
    }
}

void metal_probe_destroy(struct MetalProbe *probe)
{
    if (!probe) return;
    [probe->queue release];
    [probe->layer release];
    [probe->device release];
    free(probe);
}

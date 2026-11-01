#ifndef TOOLS_METAL_PROBE_H
#define TOOLS_METAL_PROBE_H

struct MetalProbe;
struct MetalProbe *metal_probe_create(void *layer);
int metal_probe_draw(struct MetalProbe *probe, int width, int height);
void metal_probe_destroy(struct MetalProbe *probe);

#endif

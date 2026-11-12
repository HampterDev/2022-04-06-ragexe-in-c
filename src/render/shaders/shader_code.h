#ifndef RENDER_SHADERS_SHADER_CODE_H
#define RENDER_SHADERS_SHADER_CODE_H

#include <stddef.h>

/* One entry point of a shader as tools/shaders compiled it (the <name>.h beside each <name>.hlsl):
   SPIR-V for Vulkan, MSL for Metal (entry points main and main0), and the resources it declares,
   which SDL_GPUShaderCreateInfo wants counted. */
#define GPU_SHADER_VERTEX 0
#define GPU_SHADER_FRAGMENT 1

struct GpuShaderCode {
    const unsigned char *spirv;
    size_t spirvSize;
    const unsigned char *msl;
    size_t mslSize;
    int stage;
    unsigned samplers, storageTextures, storageBuffers, uniformBuffers;
};

#endif

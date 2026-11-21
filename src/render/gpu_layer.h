#ifndef RENDER_GPU_LAYER_H
#define RENDER_GPU_LAYER_H

/* The graphics device's seam for the layers that record into its frames (render/gpu_device.c): a
   layer keeps GPU objects and draws of its own, which the device knows only through the hooks it
   registers (struct GpuLayer) and the draws it adds to the device's passes (struct
   GpuLayerDraw). The static world is one (render/gfx_world.h, render/gpu_world.c). Through this
   header a layer gets the SDL device, makes its shaders, pipelines and compute pipelines, grows and
   retires GPU objects as the device does, stages bytes among the frame's texels, which the frame's
   copy pass reads, and adds draws to the current pass in the current mode; the device calls its
   hooks at fixed points of a frame and of the device's life. Everything here but the render
   thread's hooks runs on the thread that draws. */
#include "render/gfx.h"
#include "render/shaders/shader_code.h"
#include <SDL3/SDL.h>
#include <stddef.h>
#include <stdint.h>

/* The format of every colour texture of the device's: what it samples and what a pass draws into. */
#define GPU_TARGET_FORMAT SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM
/* A target's textures are a pixel wider and taller than the target: every pass's viewport is the
   target moved by half a pixel, (0.5, 0.5, width, height), where Direct3D 9's pixel centres fall on
   the target's, and a viewport must lie within its texture (SDL's GPU API, Metal). */
#define GPU_MARGIN 1

/* The records the shaders read (src/render/shaders/records.hlsli): a draw's state, its stages and
   its colours, among the frame's records (gpu_records) or pushed as a draw's uniform. */
struct GpuDraw {
    uint32_t mode[4];
    float target[4];
    uint32_t stages[4];
    float fog[4];
    float fogColour[4];
    float colour[4];
    float uvScale[4], uvLimit[4];
    uint32_t mesh[4];
};

/* ---- the device ---- */

/* The SDL device, NULL while there is none; and the device's fixed objects while there is one: the
   depth buffers' format, the texture of one white texel a stage without a texture reads, the array
   of one layer a draw binds where its stage 0 reads no layer of an atlas, the nearest-texel sampler
   clamped to the edge. */
SDL_GPUDevice *gpu_dev(void);
SDL_GPUTextureFormat gpu_depth_format(void);
SDL_GPUTexture *gpu_white(void);
SDL_GPUTexture *gpu_no_atlas(void);
SDL_GPUSampler *gpu_point_clamp(void);
/* The sampler a stage reads with, by its texture addressing and filter (render/draw_modes.h, enum
   DrawAddress and enum DrawFilter): stage 1, the lightmap's, filters linearly whatever the filter. */
SDL_GPUSampler *gpu_stage_sampler(int address, int filter, int stage);
/* The screen's size; 0 without a screen. */
int gpu_screen_size(int *w, int *h);

/* A shader, a compute pipeline, a graphics pipeline, made with the device; NULL where it cannot be
   (SDL_GetError says why). The device lets go of the pipelines it made when it goes; the layer lets
   go of its shaders and compute pipelines. A pipeline: its shaders, its vertex input, and its state:
   blending by the D3DBLEND factors src and dst (enum DrawFactor), depth (enum DrawDepth), cull
   (GPU_CULL_*), colour mask (GPU_MASK_*), whether it draws into R32_FLOAT, with a depth target, its
   samples and alpha to coverage, its colour targets (1 or 2: the second written in red alone where
   `back`, not at all otherwise). Pipelines whose descriptions are the same are one: zero a
   description whole before filling it. */
enum { GPU_CULL_NONE = 1, GPU_CULL_CW, GPU_CULL_CCW };
enum { GPU_MASK_RGBA, GPU_MASK_RGB, GPU_MASK_NONE };
struct GpuPipe {
    SDL_GPUShader *vs, *fs;
    SDL_GPUVertexBufferDescription buffers[2];
    SDL_GPUVertexAttribute attrs[8];
    uint32_t nbuffers, nattrs;
    int blend, src, dst, depth, cull, mask, r32f, hasDepth, samples, a2c, targets, back;
};
SDL_GPUShader *gpu_shader(const struct GpuShaderCode *code);
SDL_GPUComputePipeline *gpu_compute(const struct GpuComputeCode *code);
SDL_GPUGraphicsPipeline *gpu_pipeline(struct GpuPipe *p);
/* What a mode (not an effect mode) makes of a pipeline that draws into the screen, not flat: its
   blending, depth, cull (none), colour mask, samples and alpha to coverage. */
void gpu_pipe_mode(struct GpuPipe *p, const struct DrawModeDesc *m);

/* A texture of the device's made while no command buffer is being recorded (SDL submits one of its
   own for it); a colour texture of GPU_TARGET_FORMAT, w by h with `levels` levels. */
SDL_GPUTexture *gpu_make_texture(const SDL_GPUTextureCreateInfo *info);
SDL_GPUTexture *gpu_new_texture(int w, int h, int levels, SDL_GPUTextureUsageFlags usage);
/* A texture or buffer given up while the frames handed over may still use it: released once the
   next frame handed over has been submitted. */
void gpu_retire(SDL_GPUTexture *texture, SDL_GPUBuffer *buffer);
/* A command buffer of the thread that draws, recorded while no other is (NULL where none can be
   had), and its submission, with a fence where `fence` is given; 0 where it could not be submitted. */
SDL_GPUCommandBuffer *gpu_commands(void);
int gpu_submit(SDL_GPUCommandBuffer *cmd, SDL_GPUFence **fence);
/* Hands what was recorded over and waits until it and every frame before has been submitted, so
   that a command buffer of gpu_commands reaches the GPU after them; 0 where it did not. */
int gpu_flush(void);
/* A level of a layer of a texture (w by h at the top, `levels` levels), max(1, w >> level) by
   max(1, h >> level) texels of B8G8R8A8 into out; 0, or -1 where there is no such level or the
   download failed. */
int gpu_read_level(SDL_GPUTexture *texture, uint32_t layer, uint32_t levels, int level, uint32_t w, uint32_t h,
                   void *out);
/* Every level of `scratch` (w by h, `levels` levels) into layer `layer` of `array`: how a layer of an
   array takes the levels SDL made for a texture of its own (SDL_GenerateMipmapsForGPUTexture). */
void gpu_copy_levels(SDL_GPUCopyPass *copy, SDL_GPUTexture *scratch, SDL_GPUTexture *array, uint32_t layer,
                     uint32_t w, uint32_t h, uint32_t levels);

/* ---- bytes ---- */

/* Bytes written one after the other: on the CPU, or (`staged`, the device's own) in a transfer
   buffer mapped while the frame is recorded. */
struct GpuBytes {
    unsigned char *data;
    size_t size, capacity;
    int staged;
    SDL_GPUTransferBuffer *buffer;
    int quiet;  /* frames in a row that needed a quarter of its transfer buffer at most */
};
/* `size` bytes at an `align`-aligned offset of b (align a power of two), zeroed (gpu_bytes_add) or
   left for the caller to write whole (gpu_bytes_take, the padding before them zeroed): their offset,
   or -1 without memory. b->data may move. */
long gpu_bytes_add(struct GpuBytes *b, size_t size, size_t align);
long gpu_bytes_take(struct GpuBytes *b, size_t size, size_t align);
/* An array of `size`-byte items grown to hold `need`, doubling from 64; 0 without memory. */
int gpu_grow(void **items, size_t *capacity, size_t need, size_t size);
/* The frame's texels: what its copy pass uploads from (the copy hook's `texels`), staged now. */
struct GpuBytes *gpu_texels(void);
/* The frame's draw records, which the shaders read (records.hlsli); a record among those of b
   (zeroed: *out, NULL without memory) and its index. */
struct GpuBytes *gpu_records(void);
uint32_t gpu_add_record_to(struct GpuBytes *b, struct GpuDraw **out);

/* ---- surfaces ---- */

/* A texture's CPU pixels as the client gave them (gfx_create_texture): its size, levels, format
   (render/gfx_format.h) and level 0 in that format's layout. 0 for a surface that is no texture. */
struct GpuTexels {
    int width, height, levels, fmt;
    const unsigned char *pixels;
};
int gpu_texture_texels(gfx_surface *s, struct GpuTexels *out);
/* A reference to a surface, which gfx_release lets go. */
void gpu_surface_ref(gfx_surface *s);

/* ---- drawing, on the thread that draws ---- */

/* The number of the frame being recorded, which every frame handed over makes one more. */
unsigned gpu_frame_number(void);
/* The client's quads and Granny's draws given so far, drawn: a layer's draw comes after them. */
void gpu_end_batch(void);
/* What draws are drawn with now (render/gfx.h): the mode, the fog, whether the current target is
   the screen. */
int gpu_mode(void);
const struct GfxFog *gpu_fog(void);
int gpu_into_screen(void);
/* The pass the next draw goes into, the current target's (NULL without memory), and its target's
   size. */
struct GpuStep;
struct GpuStep *gpu_pass(void);
void gpu_pass_size(const struct GpuStep *pass, int *w, int *h);
/* A stage's texture as the current mode reads it, uploaded where the CPU changed it, with its
   sampler (white where the stage has none). */
void gpu_bind_stage(SDL_GPUTextureSamplerBinding *bind, int stage);
/* A draw record's state in the current mode and fog into a pass: with `textures`, its stages'
   textures' scale and border as set (gfx_set_texture), otherwise none; with `vertexFog`, the fog's
   parameters as the quads' vertex fog takes them (render/shaders/quad.hlsli). */
void gpu_fill_record(struct GpuDraw *r, const struct GpuStep *pass, int textures, int vertexFog);
/* A layer's draw: its pipeline, the textures and samplers its fragment shader reads, and what the
   layer keeps of it for its draw hook, in its own words. A draw without a pipeline is not drawn. */
#define GPU_LAYER_DRAW_WORDS 12
struct GpuLayerDraw {
    SDL_GPUGraphicsPipeline *pipeline;
    SDL_GPUTextureSamplerBinding bind[3];
    int program;  /* the layer's own numbering: a draw of another program binds its buffers anew */
    uint32_t words[GPU_LAYER_DRAW_WORDS];
};
/* A draw of layer `layer` added to the pass, zeroed but for its pipeline and program; NULL without
   memory or a pipeline. */
struct GpuLayerDraw *gpu_add_draw(struct GpuStep *pass, int layer, int program, SDL_GPUGraphicsPipeline *pipeline);
/* The current target drawn into (gfx_content_version). */
void gpu_drawn(void);
/* The layer's part of the frame being recorded (struct GpuLayer, partSize). */
void *gpu_part(int layer);

/* ---- a pass, on the render thread ---- */

/* A render pass as the device records it, which a layer's draw hook draws into. */
struct GpuPass {
    SDL_GPUCommandBuffer *cmd;
    SDL_GPURenderPass *pass;
    SDL_GPUBuffer *records;  /* the frame's draw records */
};
/* The draw's pipeline bound where another is; 1 where the draw's program (of its layer) is not the
   one bound before, which the layer then binds its buffers for, and the textures bound are
   forgotten. */
int gpu_pass_use(struct GpuPass *pass, const struct GpuLayerDraw *draw);
/* The fragment shader's first n textures and samplers bound, where they are not bound already. */
void gpu_pass_samplers(struct GpuPass *pass, const SDL_GPUTextureSamplerBinding *bind, int n);

/* ---- layers ---- */

/* How a frame went (the retired hook): drawn; not handed over whole, so that nothing of it went to
   the render thread; or not submitted by the render thread. */
enum { GPU_FRAME_DRAWN, GPU_FRAME_DROPPED, GPU_FRAME_UNSUBMITTED };

/* A layer's hooks, each optional. `part` is the layer's part of a frame packet, partSize bytes,
   zeroed when the layer is added and kept with the packet: the main thread writes the part of the
   frame it records, the render thread reads (and writes) the part of the frame it records, never
   both at once.
   - made: the device was made, its fixed objects are there, its render thread is not yet; 0 fails
     the device (lost is called, the device goes). pipelines: the screen's samples and depth are
     known (gfx_create_zbuffer); 0 fails it. lost: the device goes; it is still there, with every
     frame finished, and what the layer retires now is released with the device.
   - begin: a frame is begun into the packet `part`. pending: whether the layer has something of
     its own to take up, which a flush with nothing recorded does not skip. prepare: a flush begins,
     every frame before it taken back: the layer's uploads and staging for the frame. hand_over:
     what the frame's passes read of the layer, into `part`; 0 drops the frame. flushed: every
     layer has handed the frame over, which goes now (`presented`: it ends the frame, gfx_present).
     retired: a frame back from the render thread, how it went (GPU_FRAME_*), its number
     (gpu_frame_number when it was recorded).
   - the render thread's: copy, in the frame's copy pass after the device's streams, from the
     frame's texels; compute, after the copy pass and the textures' mipmaps, before the frame's
     passes; draw, each of the layer's draws in a pass, in the order they were added. */
struct GpuLayer {
    size_t partSize;
    int (*made)(void);
    int (*pipelines)(void);
    void (*lost)(void);
    void (*begin)(void *part);
    int (*pending)(void);
    void (*prepare)(void);
    int (*hand_over)(void *part);
    void (*flushed)(int presented);
    void (*retired)(void *part, int how, unsigned number);
    void (*copy)(void *part, SDL_GPUCopyPass *copy, SDL_GPUTransferBuffer *texels);
    void (*compute)(void *part, SDL_GPUCommandBuffer *cmd);
    void (*draw)(void *part, struct GpuPass *pass, const struct GpuLayerDraw *draw);
};

/* Adds a layer before the device is made (gfx_create_screen); a layer added again is the same one.
   Answers its number, which its draws and gpu_part name, or -1 (no room, no memory, a device made
   already). */
#define GPU_LAYERS_MAX 4
int gpu_add_layer(const struct GpuLayer *layer);

#endif

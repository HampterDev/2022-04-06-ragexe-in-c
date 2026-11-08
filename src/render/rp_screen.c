#include "render/device.h"
#include "render/render_api.h"
#include <math.h>
#include "render/rp_pool.h"
#include "world/world_imports.h"

float g_spriteDepthScale = 0.159999996f;

float g_spriteRowDepthStep = 0.159999996f;


float g_spriteScaleDivisor = 1.34000003f;

void renderer_restore_frame(struct Renderer *self)
{
    (void)self;

    c3d_device_restore_frame(&g_3dDevice);
}

void renderer_set_projection(struct Renderer *self, int width, int height)
{
    int half_w = width / 2, half_h = height / 2;
    float aspect, t_half, t_far;

    self->height = height;
    self->centreX = half_w;
    self->width = width;
    self->centreY = half_h;
    aspect = (float)height / (float)width;
    self->aspect = aspect;
    t_half = (float)tan(0.13089966773986816);
    self->proj[1] = (float)half_h / t_half;
    t_far = (float)tan(1.4398963451385498);
    self->halfH = half_h;
    self->halfW = half_w;
    self->proj[3] = t_far;
    self->proj[2] = t_far * aspect;
    self->proj[0] = (float)half_w / t_half * aspect;
    self->scaleX = (float)width / 640.0f;
    self->scaleY = (float)height / 480.0f;
    g_spriteScaleDivisor = (float)height * 1.34f / 768.0f;
    g_spriteRowDepthStep = g_spriteDepthScale / self->scaleY;
}

void renderer_clear_frame(struct Renderer *self, int flag)
{
    (void)self;

    if (flag)
        c3d_device_clear(&g_3dDevice, 0xff000000);
    else
        c3d_device_clear_z(&g_3dDevice);
}

void renderer_clear_scene(struct Renderer *self)
{
    DWORD colour = self->clearColor;

    if (self->fogEnable == 0)
        c3d_device_clear(&g_3dDevice, colour);
    else
        c3d_device_clear(&g_3dDevice, rp_fog_blend(colour));
}

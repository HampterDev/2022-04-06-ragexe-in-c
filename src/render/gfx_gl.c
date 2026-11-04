#ifndef _WIN32
#include "render/gfx.h"
#include "render/gfx_batch.h"
#include "render/device.h"
#include "render/display_devices.h"
#include "render/present_fit.h"
#include "diag/perf.h"
#include <SDL3/SDL.h>
#if defined(__ANDROID__) || defined(RAGE_GLES)
/* OpenGL ES 3.0 (Android, or RAGE_GLES to try it elsewhere): no BGRA uploads, reads or vertex
   colours, no reversed 16-bit layouts and no border clamping, so those convert on the CPU or fall
   back; everything else is shared. */
#define GFX_GLES 1
#include <GLES3/gl3.h>
#ifndef APIENTRY
#define APIENTRY GL_APIENTRY
#endif
#else
#include <SDL3/SDL_opengl.h>
#endif
#include <stdlib.h>
#include <string.h>

#ifdef GFX_GLES
#define GFX_PIXEL_FORMAT GL_RGBA
#define GFX_COLOUR_SIZE 4
#define GL_CLEAR_DEPTH(X) X(void, glClearDepthf, (GLfloat depth))
#else
#define GFX_PIXEL_FORMAT GL_BGRA
#define GFX_COLOUR_SIZE GL_BGRA
#define GL_CLEAR_DEPTH(X) X(void, glClearDepth, (GLdouble depth))
#endif

#define GL_FUNCTIONS(X) \
    X(void, glGenTextures, (GLsizei n, GLuint *textures)) \
    X(void, glDeleteTextures, (GLsizei n, const GLuint *textures)) \
    X(void, glBindTexture, (GLenum target, GLuint texture)) \
    X(void, glTexImage2D, (GLenum target, GLint level, GLint internal, GLsizei width, GLsizei height, GLint border, \
                           GLenum format, GLenum type, const void *pixels)) \
    X(void, glTexSubImage2D, (GLenum target, GLint level, GLint x, GLint y, GLsizei width, GLsizei height, \
                              GLenum format, GLenum type, const void *pixels)) \
    X(void, glTexParameteri, (GLenum target, GLenum name, GLint value)) \
    X(void, glPixelStorei, (GLenum name, GLint value)) \
    X(void, glReadPixels, (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void *pixels)) \
    X(void, glViewport, (GLint x, GLint y, GLsizei width, GLsizei height)) \
    X(void, glScissor, (GLint x, GLint y, GLsizei width, GLsizei height)) \
    X(void, glEnable, (GLenum cap)) \
    X(void, glDisable, (GLenum cap)) \
    X(void, glClear, (GLbitfield mask)) \
    X(void, glClearColor, (GLfloat r, GLfloat g, GLfloat b, GLfloat a)) \
    GL_CLEAR_DEPTH(X) \
    X(void, glDepthFunc, (GLenum func)) \
    X(void, glDepthMask, (GLboolean flag)) \
    X(void, glColorMask, (GLboolean r, GLboolean g, GLboolean b, GLboolean a)) \
    X(void, glBlendFunc, (GLenum src, GLenum dst)) \
    X(void, glCullFace, (GLenum mode)) \
    X(void, glFrontFace, (GLenum mode)) \
    X(void, glDrawArrays, (GLenum mode, GLint first, GLsizei count)) \
    X(void, glDrawElements, (GLenum mode, GLsizei count, GLenum type, const void *indices)) \
    X(void, glGetIntegerv, (GLenum name, GLint *value)) \
    X(GLenum, glGetError, (void)) \
    X(void, glActiveTexture, (GLenum texture)) \
    X(void, glGenBuffers, (GLsizei n, GLuint *buffers)) \
    X(void, glDeleteBuffers, (GLsizei n, const GLuint *buffers)) \
    X(void, glBindBuffer, (GLenum target, GLuint buffer)) \
    X(void, glBufferData, (GLenum target, GLsizeiptr size, const void *data, GLenum usage)) \
    X(void, glGenVertexArrays, (GLsizei n, GLuint *arrays)) \
    X(void, glBindVertexArray, (GLuint array)) \
    X(void, glDeleteVertexArrays, (GLsizei n, const GLuint *arrays)) \
    X(void, glEnableVertexAttribArray, (GLuint index)) \
    X(void, glVertexAttribPointer, (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, \
                                    const void *pointer)) \
    X(GLuint, glCreateShader, (GLenum type)) \
    X(void, glShaderSource, (GLuint shader, GLsizei count, const GLchar *const *source, const GLint *length)) \
    X(void, glCompileShader, (GLuint shader)) \
    X(void, glGetShaderiv, (GLuint shader, GLenum name, GLint *value)) \
    X(void, glGetShaderInfoLog, (GLuint shader, GLsizei size, GLsizei *length, GLchar *log)) \
    X(void, glDeleteShader, (GLuint shader)) \
    X(GLuint, glCreateProgram, (void)) \
    X(void, glAttachShader, (GLuint program, GLuint shader)) \
    X(void, glLinkProgram, (GLuint program)) \
    X(void, glGetProgramiv, (GLuint program, GLenum name, GLint *value)) \
    X(void, glGetProgramInfoLog, (GLuint program, GLsizei size, GLsizei *length, GLchar *log)) \
    X(void, glDeleteProgram, (GLuint program)) \
    X(void, glUseProgram, (GLuint program)) \
    X(GLint, glGetUniformLocation, (GLuint program, const GLchar *name)) \
    X(void, glUniform1i, (GLint location, GLint value)) \
    X(void, glUniform1f, (GLint location, GLfloat value)) \
    X(void, glUniform2f, (GLint location, GLfloat a, GLfloat b)) \
    X(void, glUniform4f, (GLint location, GLfloat a, GLfloat b, GLfloat c, GLfloat d)) \
    X(void, glUniform4i, (GLint location, GLint a, GLint b, GLint c, GLint d)) \
    X(void, glGenFramebuffers, (GLsizei n, GLuint *framebuffers)) \
    X(void, glDeleteFramebuffers, (GLsizei n, const GLuint *framebuffers)) \
    X(void, glBindFramebuffer, (GLenum target, GLuint framebuffer)) \
    X(void, glFramebufferTexture2D, (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)) \
    X(void, glFramebufferRenderbuffer, (GLenum target, GLenum attachment, GLenum rbtarget, GLuint renderbuffer)) \
    X(GLenum, glCheckFramebufferStatus, (GLenum target)) \
    X(void, glGenRenderbuffers, (GLsizei n, GLuint *renderbuffers)) \
    X(void, glDeleteRenderbuffers, (GLsizei n, const GLuint *renderbuffers)) \
    X(void, glBindRenderbuffer, (GLenum target, GLuint renderbuffer)) \
    X(void, glRenderbufferStorage, (GLenum target, GLenum format, GLsizei width, GLsizei height)) \
    X(void, glBlitFramebuffer, (GLint sx0, GLint sy0, GLint sx1, GLint sy1, GLint dx0, GLint dy0, GLint dx1, GLint dy1, \
                                GLbitfield mask, GLenum filter)) \
    X(void, glGenSamplers, (GLsizei n, GLuint *samplers)) \
    X(void, glDeleteSamplers, (GLsizei n, const GLuint *samplers)) \
    X(void, glBindSampler, (GLuint unit, GLuint sampler)) \
    X(void, glSamplerParameteri, (GLuint sampler, GLenum name, GLint value)) \
    X(void, glSamplerParameterfv, (GLuint sampler, GLenum name, const GLfloat *value))

#define GL_DECLARE(ret, name, params) static ret (APIENTRY *p_##name) params;
GL_FUNCTIONS(GL_DECLARE)
#undef GL_DECLARE

enum { K_SCREEN, K_TARGET, K_SAVE, K_SYSMEM, K_TEXTURE, K_RTTEX, K_ZBUFFER };

#define CLIENT_SURFACE_KEY 0x00ff00ffu
#define MAX_LEVELS 16
#define N_RS 256
#define N_TSS 33

struct gfx_surface {
    int kind, refs;
    int width, height;
    int fmt, bpp;
    unsigned char *pixels[MAX_LEVELS];
    int levels;
    int dirty;
    int lockLevel;
    GLuint tex, fbo, depth;
    DWORD key;
    struct gfx_surface *prev, *next;
    unsigned version;
};

struct gfx_vbuffer {
    int unused;
};

enum {
    U_VIEWPORT, U_STAGE0, U_STAGE0B, U_STAGE1, U_STAGE1B, U_FLAGS, U_ALPHAREF, U_FOGCOLOR, U_TFACTOR,
    U_BUMPMAT, U_TEX0, U_TEX1, U_COUNT
};

static const char *const uniform_names[U_COUNT] = {
    "u_viewport", "u_stage0", "u_stage0b", "u_stage1", "u_stage1b", "u_flags", "u_alpharef", "u_fogcolor",
    "u_tfactor", "u_bumpmat", "u_tex0", "u_tex1"
};

static struct {
    SDL_Window *window;
    SDL_GLContext context;
    SDL_Mutex *lock;
    GLuint program, vao, vbo, ibo, samplers[2];
    GLint uniforms[U_COUNT];
    struct gfx_surface *handles;
    GLuint *dead_textures;
    size_t dead_count, dead_capacity;
    struct gfx_surface *target, *mainTarget, *depthSurface;
    struct gfx_surface *stageTex[8];
    DWORD rs[N_RS];
    DWORD tss[8][N_TSS];
    DWORD samp[2][5];
    int stateDirty, samplerDirty;
    int inScene, mipmaps, vsync;
    int boundTarget;
} gl;

static unsigned content_tick;
static struct PresentCover present_cover;

void gfx_gl_set_cover(const struct PresentCover *cover)
{
    present_cover = *cover;
}

#ifdef GFX_GLES
static const char shader_header[] = "#version 300 es\nprecision highp float;\nprecision highp int;\n#define GFX_GLES 1\n";
#else
static const char shader_header[] = "#version 330 core\n";
#endif

static const char vertex_source[] =
    "layout(location = 0) in vec4 a_pos;\n"
    "layout(location = 1) in vec4 a_diffuse;\n"
    "layout(location = 2) in vec4 a_specular;\n"
    "layout(location = 3) in vec2 a_uv0;\n"
    "layout(location = 4) in vec2 a_uv1;\n"
    "uniform vec2 u_viewport;\n"
    "out vec4 v_diffuse;\n"
    "out vec4 v_specular;\n"
    "out vec2 v_uv0;\n"
    "out vec2 v_uv1;\n"
    "void main()\n"
    "{\n"
    "    float w = a_pos.w > 0.0 ? 1.0 / a_pos.w : 1.0;\n"
    "    vec2 pixel = a_pos.xy + vec2(0.5 - 1.0 / 128.0);\n"
    "    vec2 ndc = vec2(pixel.x / u_viewport.x * 2.0 - 1.0, 1.0 - pixel.y / u_viewport.y * 2.0);\n"
    "    gl_Position = vec4(ndc * w, (a_pos.z * 2.0 - 1.0) * w, w);\n"
    "#ifdef GFX_GLES\n"
    "    v_diffuse = a_diffuse.bgra;\n"
    "    v_specular = a_specular.bgra;\n"
    "#else\n"
    "    v_diffuse = a_diffuse;\n"
    "    v_specular = a_specular;\n"
    "#endif\n"
    "    v_uv0 = a_uv0;\n"
    "    v_uv1 = a_uv1;\n"
    "}\n";

static const char fragment_source[] =
    "in vec4 v_diffuse;\n"
    "in vec4 v_specular;\n"
    "in vec2 v_uv0;\n"
    "in vec2 v_uv1;\n"
    "uniform sampler2D u_tex0;\n"
    "uniform sampler2D u_tex1;\n"
    "uniform ivec4 u_stage0;\n"
    "uniform ivec4 u_stage0b;\n"
    "uniform ivec4 u_stage1;\n"
    "uniform ivec4 u_stage1b;\n"
    "uniform ivec4 u_flags;\n"
    "uniform float u_alpharef;\n"
    "uniform vec4 u_fogcolor;\n"
    "uniform vec4 u_tfactor;\n"
    "uniform vec4 u_bumpmat;\n"
    "out vec4 o_color;\n"
    "vec4 arg(int a, vec4 cur, vec4 tex)\n"
    "{\n"
    "    int sel = a & 15;\n"
    "    vec4 r = sel == 0 ? v_diffuse : sel == 1 ? cur : sel == 2 ? tex : sel == 3 ? u_tfactor : sel == 4 ? v_specular : cur;\n"
    "    if ((a & 32) != 0) r = vec4(r.a);\n"
    "    if ((a & 16) != 0) r = vec4(1.0) - r;\n"
    "    return r;\n"
    "}\n"
    "vec4 combine(int op, vec4 a1, vec4 a2, vec4 cur, vec4 tex)\n"
    "{\n"
    "    if (op == 2) return a1;\n"
    "    if (op == 3) return a2;\n"
    "    if (op == 4) return a1 * a2;\n"
    "    if (op == 5) return a1 * a2 * 2.0;\n"
    "    if (op == 6) return a1 * a2 * 4.0;\n"
    "    if (op == 7) return a1 + a2;\n"
    "    if (op == 8) return a1 + a2 - 0.5;\n"
    "    if (op == 9) return (a1 + a2 - 0.5) * 2.0;\n"
    "    if (op == 10) return a1 - a2;\n"
    "    if (op == 11) return a1 + a2 - a1 * a2;\n"
    "    if (op == 12) return mix(a2, a1, v_diffuse.a);\n"
    "    if (op == 13) return mix(a2, a1, tex.a);\n"
    "    if (op == 14) return mix(a2, a1, u_tfactor.a);\n"
    "    if (op == 15) return a1 + a2 * (1.0 - tex.a);\n"
    "    if (op == 16) return mix(a2, a1, cur.a);\n"
    "    if (op == 18) return vec4(a1.rgb + a1.a * a2.rgb, a1.a);\n"
    "    if (op == 19) return vec4(a1.rgb * a2.rgb + a1.a, a1.a);\n"
    "    if (op == 24) return vec4(clamp(dot(a1.rgb - 0.5, a2.rgb - 0.5) * 4.0, 0.0, 1.0));\n"
    "    return cur;\n"
    "}\n"
    "vec4 stage(ivec4 s, ivec4 sb, vec4 cur, vec4 tex)\n"
    "{\n"
    "    vec4 c = combine(s.x, arg(s.y, cur, tex), arg(s.z, cur, tex), cur, tex);\n"
    "    float a = s.w == 1 ? cur.a : combine(s.w, arg(sb.x, cur, tex), arg(sb.y, cur, tex), cur, tex).a;\n"
    "    return clamp(vec4(c.rgb, a), 0.0, 1.0);\n"
    "}\n"
    "vec2 uv0, uv1;\n"
    "vec2 coords(int index, int flip)\n"
    "{\n"
    "    vec2 uv = (index & 7) == 1 ? uv1 : uv0;\n"
    "    if (flip != 0) uv.y = 1.0 - uv.y;\n"
    "    return uv;\n"
    "}\n"
    "void main()\n"
    "{\n"
    "    uv0 = v_uv0 - (dFdx(v_uv0) - dFdy(v_uv0)) * (1.0 / 128.0);\n"
    "    uv1 = v_uv1 - (dFdx(v_uv1) - dFdy(v_uv1)) * (1.0 / 128.0);\n"
    "    vec4 cur = v_diffuse;\n"
    "    vec2 bump = vec2(0.0);\n"
    "    if (u_stage0.x != 1) {\n"
    "        vec4 tex0 = u_stage0b.w != 0 ? texture(u_tex0, coords(u_stage0b.z, u_flags.w & 1)) : vec4(1.0);\n"
    "        if (u_stage0.x == 22 || u_stage0.x == 23) {\n"
    "            bump = vec2(u_bumpmat.x * tex0.r + u_bumpmat.z * tex0.g, u_bumpmat.y * tex0.r + u_bumpmat.w * tex0.g);\n"
    "        } else {\n"
    "            cur = stage(u_stage0, u_stage0b, cur, tex0);\n"
    "        }\n"
    "        if (u_stage1.x != 1) {\n"
    "            vec4 tex1 = u_stage1b.w != 0 ? texture(u_tex1, coords(u_stage1b.z, u_flags.w & 2) + bump) : vec4(1.0);\n"
    "            cur = stage(u_stage1, u_stage1b, cur, tex1);\n"
    "        }\n"
    "    }\n"
    "    if (u_flags.z != 0) cur.rgb = mix(u_fogcolor.rgb, cur.rgb, v_specular.a);\n"
    "    if (u_flags.x != 0) {\n"
    "        float a = floor(cur.a * 255.0 + 0.5);\n"
    "        int f = u_flags.y;\n"
    "        bool pass = f == 8 || (f == 2 && a < u_alpharef) || (f == 3 && a == u_alpharef) ||\n"
    "                    (f == 4 && a <= u_alpharef) || (f == 5 && a > u_alpharef) ||\n"
    "                    (f == 6 && a != u_alpharef) || (f == 7 && a >= u_alpharef);\n"
    "        if (!pass) discard;\n"
    "    }\n"
    "    o_color = cur;\n"
    "}\n";

static int load_functions(void)
{
#define GL_LOAD_PROC(ret, name, params) \
    if (!(p_##name = (ret (APIENTRY *) params)SDL_GL_GetProcAddress(#name))) return 0;
    GL_FUNCTIONS(GL_LOAD_PROC)
#undef GL_LOAD_PROC
    return 1;
}

static GLuint compile_shader(GLenum kind, const char *source)
{
    GLuint shader = p_glCreateShader(kind);
    GLint ok = 0;
    char log[1024];
    const GLchar *parts[2];
    parts[0] = shader_header;
    parts[1] = source;
    p_glShaderSource(shader, 2, parts, NULL);
    p_glCompileShader(shader);
    p_glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        p_glGetShaderInfoLog(shader, sizeof log, NULL, log);
        SDL_Log("gfx: shader compile failed: %s", log);
        p_glDeleteShader(shader);
        return 0;
    }
    return shader;
}

static int build_program(void)
{
    GLuint vs = compile_shader(GL_VERTEX_SHADER, vertex_source);
    GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fragment_source);
    GLint ok = 0;
    char log[1024];
    int i;
    if (!vs || !fs)
        return 0;
    gl.program = p_glCreateProgram();
    p_glAttachShader(gl.program, vs);
    p_glAttachShader(gl.program, fs);
    p_glLinkProgram(gl.program);
    p_glDeleteShader(vs);
    p_glDeleteShader(fs);
    p_glGetProgramiv(gl.program, GL_LINK_STATUS, &ok);
    if (!ok) {
        p_glGetProgramInfoLog(gl.program, sizeof log, NULL, log);
        SDL_Log("gfx: program link failed: %s", log);
        return 0;
    }
    for (i = 0; i < U_COUNT; i++)
        gl.uniforms[i] = p_glGetUniformLocation(gl.program, uniform_names[i]);
    p_glUseProgram(gl.program);
    p_glUniform1i(gl.uniforms[U_TEX0], 0);
    p_glUniform1i(gl.uniforms[U_TEX1], 1);
    return 1;
}

static int format_bpp(int fmt)
{
    return fmt == GFX_FMT_8888 || fmt < 0 ? 32 : 16;
}

static size_t level_bytes(const struct gfx_surface *s, int level)
{
    int w = s->width >> level, h = s->height >> level;
    if (w < 1) w = 1;
    if (h < 1) h = 1;
    return (size_t)w * h * (s->bpp >> 3);
}

static void lock_handles(void)
{
    if (gl.lock)
        SDL_LockMutex(gl.lock);
}

static void unlock_handles(void)
{
    if (gl.lock)
        SDL_UnlockMutex(gl.lock);
}

static struct gfx_surface *new_handle(int kind, int w, int h, int fmt)
{
    struct gfx_surface *s = calloc(1, sizeof *s);
    if (!s)
        return NULL;
    s->kind = kind;
    s->refs = 1;
    s->width = w;
    s->height = h;
    s->fmt = fmt;
    s->bpp = format_bpp(fmt);
    s->levels = 1;
    s->lockLevel = -1;
    lock_handles();
    s->version = ++content_tick;
    s->next = gl.handles;
    if (gl.handles)
        gl.handles->prev = s;
    gl.handles = s;
    unlock_handles();
    return s;
}

static int alloc_pixels(struct gfx_surface *s)
{
    int i;
    for (i = 0; i < s->levels; i++) {
        s->pixels[i] = calloc(1, level_bytes(s, i));
        if (!s->pixels[i])
            return 0;
    }
    return 1;
}

static void forget_texture(GLuint name)
{
    GLuint *grown;
    if (!name)
        return;
    lock_handles();
    if (gl.dead_count == gl.dead_capacity) {
        size_t capacity = gl.dead_capacity ? gl.dead_capacity * 2 : 64;
        grown = realloc(gl.dead_textures, capacity * sizeof *grown);
        if (grown) {
            gl.dead_textures = grown;
            gl.dead_capacity = capacity;
        }
    }
    if (gl.dead_count < gl.dead_capacity)
        gl.dead_textures[gl.dead_count++] = name;
    unlock_handles();
}

static void collect_dead_textures(void)
{
    lock_handles();
    if (gl.dead_count && gl.context) {
        p_glDeleteTextures((GLsizei)gl.dead_count, gl.dead_textures);
        gl.dead_count = 0;
    }
    unlock_handles();
}

static void drop_gpu(struct gfx_surface *s)
{
    if (s->fbo && gl.context)
        p_glDeleteFramebuffers(1, &s->fbo);
    if (s->depth && gl.context)
        p_glDeleteRenderbuffers(1, &s->depth);
    s->fbo = 0;
    s->depth = 0;
    forget_texture(s->tex);
    s->tex = 0;
    s->dirty = 1;
}

static void free_handle(struct gfx_surface *s)
{
    int i;
    for (i = 0; i < 8; i++)
        if (gl.stageTex[i] == s) {
            batch_flush();
            gl.stageTex[i] = NULL;
            gl.stateDirty = 1;
        }
    if (gl.mainTarget == s)
        gl.mainTarget = NULL;
    if (gl.depthSurface == s)
        gl.depthSurface = NULL;
    if (gl.target == s)
        gl.target = gl.mainTarget;
    if (s->kind == K_SCREEN || s->kind == K_TARGET) {
        int shared = 0;
        struct gfx_surface *o;
        for (o = gl.handles; o; o = o->next)
            if (o != s && (o->kind == K_SCREEN || o->kind == K_TARGET) && o->fbo == s->fbo)
                shared = 1;
        if (shared)
            s->fbo = s->tex = s->depth = 0;
    }
    if (s->kind == K_ZBUFFER)
        s->depth = 0;
    drop_gpu(s);
    for (i = 0; i < MAX_LEVELS; i++)
        free(s->pixels[i]);
    lock_handles();
    if (s->prev)
        s->prev->next = s->next;
    else
        gl.handles = s->next;
    if (s->next)
        s->next->prev = s->prev;
    unlock_handles();
    free(s);
}

void gfx_addref(gfx_surface *s)
{
    if (s)
        s->refs++;
}

void gfx_release(gfx_surface *s)
{
    if (s && --s->refs <= 0)
        free_handle(s);
}

static int is_main_target(const struct gfx_surface *s)
{
    return s && (s->kind == K_SCREEN || s->kind == K_TARGET);
}

static int is_gpu_target(const struct gfx_surface *s)
{
    return s && (s->kind == K_SCREEN || s->kind == K_TARGET || s->kind == K_SAVE || s->kind == K_RTTEX);
}

static GLuint new_texture(int w, int h, GLint internal, GLenum format, GLenum type)
{
    GLuint tex = 0;
    p_glGenTextures(1, &tex);
    p_glBindTexture(GL_TEXTURE_2D, tex);
    p_glTexImage2D(GL_TEXTURE_2D, 0, internal, w, h, 0, format, type, NULL);
    p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    return tex;
}

static int make_target(struct gfx_surface *s, int withDepth)
{
    s->tex = new_texture(s->width, s->height, GL_RGBA8, GFX_PIXEL_FORMAT, GL_UNSIGNED_BYTE);
    p_glGenFramebuffers(1, &s->fbo);
    p_glBindFramebuffer(GL_FRAMEBUFFER, s->fbo);
    p_glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, s->tex, 0);
    if (withDepth) {
        p_glGenRenderbuffers(1, &s->depth);
        p_glBindRenderbuffer(GL_RENDERBUFFER, s->depth);
        p_glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, s->width, s->height);
        p_glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, s->depth);
    }
    gl.boundTarget = -1;
    s->dirty = 0;
    return p_glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
}

static void bind_target(struct gfx_surface *s)
{
    if (!s || !s->fbo)
        return;
    if (gl.boundTarget != (int)s->fbo) {
        p_glBindFramebuffer(GL_FRAMEBUFFER, s->fbo);
        p_glViewport(0, 0, s->width, s->height);
        p_glUniform2f(gl.uniforms[U_VIEWPORT], (float)s->width, (float)s->height);
        gl.boundTarget = (int)s->fbo;
    }
}

#ifdef GFX_GLES
/* The client's A8R8G8B8 pixels (bytes B, G, R, A) as the R, G, B, A bytes ES takes. */
static DWORD gl_rgba(DWORD argb)
{
    return (argb & 0xff00ff00u) | (argb >> 16 & 0xff) | (argb & 0xff) << 16;
}

/* Widened by bit replication, as desktop drivers widen 5- and 6-bit channels. */
static DWORD expand5(DWORD v)
{
    v &= 31;
    return v << 3 | v >> 2;
}

static unsigned char *rgba_copy(const struct gfx_surface *s, int level, int w, int h)
{
    size_t n = (size_t)w * h, i;
    DWORD *out = malloc(n * 4);
    if (!out)
        return NULL;
    if (s->fmt == GFX_FMT_1555 || s->fmt == GFX_FMT_4444 || s->fmt == GFX_FMT_565) {
        const WORD *in = (const WORD *)s->pixels[level];
        for (i = 0; i < n; i++) {
            DWORD v = in[i], a, r, g, b;
            if (s->fmt == GFX_FMT_565) {
                a = 255;
                r = expand5(v >> 11);
                g = (v >> 5 & 63) << 2 | (v >> 9 & 3);
                b = expand5(v);
            } else if (s->fmt == GFX_FMT_1555) {
                a = v & 0x8000 ? 255 : 0;
                r = expand5(v >> 10);
                g = expand5(v >> 5);
                b = expand5(v);
            } else {
                a = (v >> 12 & 15) * 17;
                r = (v >> 8 & 15) * 17;
                g = (v >> 4 & 15) * 17;
                b = (v & 15) * 17;
            }
            out[i] = a << 24 | b << 16 | g << 8 | r;
        }
    } else {
        const DWORD *in = (const DWORD *)s->pixels[level];
        for (i = 0; i < n; i++)
            out[i] = gl_rgba(in[i]);
    }
    return (unsigned char *)out;
}
#endif

static void upload_sysmem(struct gfx_surface *s)
{
    int n = s->width * s->height, i;
    DWORD *staging = malloc((size_t)n * 4);
    const DWORD *src = (const DWORD *)s->pixels[0];
    if (!staging)
        return;
    for (i = 0; i < n; i++)
        staging[i] = src[i] == s->key ? (src[i] & 0xffffff) : (src[i] | 0xff000000);
#ifdef GFX_GLES
    for (i = 0; i < n; i++)
        staging[i] = gl_rgba(staging[i]);
#endif
    p_glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    p_glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    p_glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, s->width, s->height, 0, GFX_PIXEL_FORMAT, GL_UNSIGNED_BYTE, staging);
    free(staging);
}

static void upload_texture(struct gfx_surface *s)
{
    int level;
    GLint internal = GL_RGBA8;
    GLenum format = GFX_PIXEL_FORMAT, type = GL_UNSIGNED_BYTE;
    int convert = 0;
    switch (s->fmt) {
#ifdef GFX_GLES
    case GFX_FMT_1555: case GFX_FMT_4444: case GFX_FMT_565: convert = 1; break;
#else
    case GFX_FMT_1555: type = GL_UNSIGNED_SHORT_1_5_5_5_REV; break;
    case GFX_FMT_4444: type = GL_UNSIGNED_SHORT_4_4_4_4_REV; break;
    case GFX_FMT_565:  format = GL_RGB; type = GL_UNSIGNED_SHORT_5_6_5; internal = GL_RGB8; break;
#endif
    case GFX_FMT_BUMP: format = GL_RG; type = GL_BYTE; internal = GL_RG8_SNORM; break;
    default:
#ifdef GFX_GLES
        convert = 1;
#endif
        break;
    }
    p_glPixelStorei(GL_UNPACK_ALIGNMENT, s->bpp == 32 || convert ? 4 : 2);
    p_glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    for (level = 0; level < s->levels; level++) {
        int w = s->width >> level, h = s->height >> level;
        w = w < 1 ? 1 : w;
        h = h < 1 ? 1 : h;
#ifdef GFX_GLES
        if (convert) {
            unsigned char *rgba = rgba_copy(s, level, w, h);
            p_glTexImage2D(GL_TEXTURE_2D, level, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
            free(rgba);
            continue;
        }
#endif
        p_glTexImage2D(GL_TEXTURE_2D, level, internal, w, h, 0, format, type, s->pixels[level]);
    }
    p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, s->levels - 1);
}

static GLuint texture_of(struct gfx_surface *s)
{
    if (!s)
        return 0;
    if (s->kind == K_SYSMEM || s->kind == K_TEXTURE) {
        if (!s->tex) {
            p_glGenTextures(1, &s->tex);
            s->dirty = 1;
        }
        if (s->dirty) {
            p_glBindTexture(GL_TEXTURE_2D, s->tex);
            if (s->kind == K_SYSMEM)
                upload_sysmem(s);
            else
                upload_texture(s);
            s->dirty = 0;
        }
    }
    return s->tex;
}

static GLenum gl_compare(DWORD f)
{
    switch (f) {
    case 1: return GL_NEVER;
    case 2: return GL_LESS;
    case 3: return GL_EQUAL;
    case 4: return GL_LEQUAL;
    case 5: return GL_GREATER;
    case 6: return GL_NOTEQUAL;
    case 7: return GL_GEQUAL;
    default: return GL_ALWAYS;
    }
}

static GLenum gl_blend(DWORD b)
{
    switch (b) {
    case 1: return GL_ZERO;
    case 2: return GL_ONE;
    case 3: return GL_SRC_COLOR;
    case 4: return GL_ONE_MINUS_SRC_COLOR;
    case 5: return GL_SRC_ALPHA;
    case 6: return GL_ONE_MINUS_SRC_ALPHA;
    case 7: return GL_DST_ALPHA;
    case 8: return GL_ONE_MINUS_DST_ALPHA;
    case 9: return GL_DST_COLOR;
    case 10: return GL_ONE_MINUS_DST_COLOR;
    case 11: return GL_SRC_ALPHA_SATURATE;
    default: return GL_ONE;
    }
}

static GLint gl_address(DWORD a)
{
    switch (a) {
    case GFX_ADDRESS_CLAMP: return GL_CLAMP_TO_EDGE;
#ifdef GFX_GLES
    case GFX_ADDRESS_BORDER: return GL_CLAMP_TO_EDGE;
#else
    case GFX_ADDRESS_BORDER: return GL_CLAMP_TO_BORDER;
#endif
    case 2: return GL_MIRRORED_REPEAT;
    default: return GL_REPEAT;
    }
}

enum { SAMP_ADDRESS, SAMP_BORDER, SAMP_MAG, SAMP_MIN, SAMP_MIP };

static void apply_samplers(void)
{
    int i;
    for (i = 0; i < 2; i++) {
        GLuint sm = gl.samplers[i];
        float border[4];
        DWORD b = gl.samp[i][SAMP_BORDER];
        int linearMin = gl.samp[i][SAMP_MIN] == GFX_FILTER_LINEAR;
        int mip = gl.samp[i][SAMP_MIP];
        struct gfx_surface *t = gl.stageTex[i];
        GLint minFilter;
        if (!t || t->levels <= 1 || mip == GFX_MIPFILTER_NONE || mip == 0)
            minFilter = linearMin ? GL_LINEAR : GL_NEAREST;
        else if (mip == GFX_MIPFILTER_LINEAR)
            minFilter = linearMin ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_LINEAR;
        else
            minFilter = linearMin ? GL_LINEAR_MIPMAP_NEAREST : GL_NEAREST_MIPMAP_NEAREST;
        p_glSamplerParameteri(sm, GL_TEXTURE_WRAP_S, gl_address(gl.samp[i][SAMP_ADDRESS]));
        p_glSamplerParameteri(sm, GL_TEXTURE_WRAP_T, gl_address(gl.samp[i][SAMP_ADDRESS]));
        p_glSamplerParameteri(sm, GL_TEXTURE_MAG_FILTER, gl.samp[i][SAMP_MAG] == GFX_FILTER_LINEAR ? GL_LINEAR : GL_NEAREST);
        p_glSamplerParameteri(sm, GL_TEXTURE_MIN_FILTER, minFilter);
        border[0] = ((b >> 16) & 0xff) / 255.0f;
        border[1] = ((b >> 8) & 0xff) / 255.0f;
        border[2] = (b & 0xff) / 255.0f;
        border[3] = ((b >> 24) & 0xff) / 255.0f;
#ifndef GFX_GLES
        p_glSamplerParameterfv(sm, GL_TEXTURE_BORDER_COLOR, border);
#else
        (void)border;
#endif
    }
    gl.samplerDirty = 0;
}

static float dword_float(DWORD v)
{
    float f;
    memcpy(&f, &v, sizeof f);
    return f;
}

static void colour_uniform(int which, DWORD c)
{
    p_glUniform4f(gl.uniforms[which], ((c >> 16) & 0xff) / 255.0f, ((c >> 8) & 0xff) / 255.0f, (c & 0xff) / 255.0f,
                  ((c >> 24) & 0xff) / 255.0f);
}

static void apply_state(void)
{
    int i, flip = 0;
    DWORD cull = gl.rs[GFX_RS_CULLMODE];
    for (i = 0; i < 2; i++) {
        struct gfx_surface *t = gl.stageTex[i];
        p_glActiveTexture(GL_TEXTURE0 + i);
        p_glBindTexture(GL_TEXTURE_2D, texture_of(t));
        if (t && is_gpu_target(t))
            flip |= 1 << i;
    }
    p_glActiveTexture(GL_TEXTURE0);
    p_glUniform4i(gl.uniforms[U_STAGE0], (int)gl.tss[0][GFX_TSS_COLOROP], (int)gl.tss[0][GFX_TSS_COLORARG1],
                  (int)gl.tss[0][GFX_TSS_COLORARG2], (int)gl.tss[0][GFX_TSS_ALPHAOP]);
    p_glUniform4i(gl.uniforms[U_STAGE0B], (int)gl.tss[0][GFX_TSS_ALPHAARG1], (int)gl.tss[0][GFX_TSS_ALPHAARG2],
                  (int)gl.tss[0][GFX_TSS_TEXCOORDINDEX], gl.stageTex[0] != NULL);
    p_glUniform4i(gl.uniforms[U_STAGE1], (int)gl.tss[1][GFX_TSS_COLOROP], (int)gl.tss[1][GFX_TSS_COLORARG1],
                  (int)gl.tss[1][GFX_TSS_COLORARG2], (int)gl.tss[1][GFX_TSS_ALPHAOP]);
    p_glUniform4i(gl.uniforms[U_STAGE1B], (int)gl.tss[1][GFX_TSS_ALPHAARG1], (int)gl.tss[1][GFX_TSS_ALPHAARG2],
                  (int)gl.tss[1][GFX_TSS_TEXCOORDINDEX], gl.stageTex[1] != NULL);
    p_glUniform4i(gl.uniforms[U_FLAGS], gl.rs[GFX_RS_ALPHATESTENABLE] != 0, (int)gl.rs[GFX_RS_ALPHAFUNC],
                  gl.rs[GFX_RS_FOGENABLE] != 0, flip);
    p_glUniform1f(gl.uniforms[U_ALPHAREF], (float)(gl.rs[GFX_RS_ALPHAREF] & 0xff));
    colour_uniform(U_FOGCOLOR, gl.rs[GFX_RS_FOGCOLOR]);
    colour_uniform(U_TFACTOR, gl.rs[60]);
    p_glUniform4f(gl.uniforms[U_BUMPMAT], dword_float(gl.tss[0][7]), dword_float(gl.tss[0][8]),
                  dword_float(gl.tss[0][9]), dword_float(gl.tss[0][10]));
    if (gl.rs[GFX_RS_ALPHABLENDENABLE]) {
        p_glEnable(GL_BLEND);
        p_glBlendFunc(gl_blend(gl.rs[GFX_RS_SRCBLEND]), gl_blend(gl.rs[GFX_RS_DESTBLEND]));
    } else {
        p_glDisable(GL_BLEND);
    }
    if (gl.rs[GFX_RS_ZENABLE]) {
        p_glEnable(GL_DEPTH_TEST);
        p_glDepthFunc(gl_compare(gl.rs[GFX_RS_ZFUNC]));
    } else {
        p_glDisable(GL_DEPTH_TEST);
    }
    p_glDepthMask(gl.rs[GFX_RS_ZWRITEENABLE] ? GL_TRUE : GL_FALSE);
    if (cull == GFX_CULL_NONE) {
        p_glDisable(GL_CULL_FACE);
    } else {
        p_glEnable(GL_CULL_FACE);
        p_glCullFace(cull == GFX_CULL_CW ? GL_BACK : GL_FRONT);
    }
    if (gl.samplerDirty)
        apply_samplers();
    gl.stateDirty = 0;
}

static void reset_state(void)
{
    int i;
    memset(gl.rs, 0, sizeof gl.rs);
    memset(gl.tss, 0, sizeof gl.tss);
    gl.rs[GFX_RS_ZENABLE] = 1;
    gl.rs[GFX_RS_ZWRITEENABLE] = 1;
    gl.rs[GFX_RS_SRCBLEND] = GFX_BLEND_ONE;
    gl.rs[GFX_RS_DESTBLEND] = 1;
    gl.rs[GFX_RS_CULLMODE] = 3;
    gl.rs[GFX_RS_ZFUNC] = GFX_CMP_LESSEQUAL;
    gl.rs[GFX_RS_ALPHAFUNC] = 8;
    gl.rs[60] = 0xffffffff;
    for (i = 0; i < 8; i++) {
        gl.tss[i][GFX_TSS_COLOROP] = i ? GFX_TOP_DISABLE : GFX_TOP_MODULATE;
        gl.tss[i][GFX_TSS_COLORARG1] = GFX_TA_TEXTURE;
        gl.tss[i][GFX_TSS_COLORARG2] = GFX_TA_CURRENT;
        gl.tss[i][GFX_TSS_ALPHAOP] = i ? GFX_TOP_DISABLE : GFX_TOP_SELECTARG1;
        gl.tss[i][GFX_TSS_ALPHAARG1] = GFX_TA_TEXTURE;
        gl.tss[i][GFX_TSS_ALPHAARG2] = GFX_TA_CURRENT;
        gl.tss[i][GFX_TSS_TEXCOORDINDEX] = (DWORD)i;
    }
    for (i = 0; i < 2; i++) {
        gl.samp[i][SAMP_ADDRESS] = GFX_ADDRESS_WRAP;
        gl.samp[i][SAMP_BORDER] = 0;
        gl.samp[i][SAMP_MAG] = 1;
        gl.samp[i][SAMP_MIN] = 1;
        gl.samp[i][SAMP_MIP] = GFX_MIPFILTER_NONE;
    }
    memset(gl.stageTex, 0, sizeof gl.stageTex);
    gl.stateDirty = 1;
    gl.samplerDirty = 1;
}

static GLenum gl_prim(int prim)
{
    switch (prim) {
    case GFX_PT_POINTLIST: return GL_POINTS;
    case GFX_PT_LINELIST: return GL_LINES;
    case GFX_PT_LINESTRIP: return GL_LINE_STRIP;
    case GFX_PT_TRIANGLESTRIP: return GL_TRIANGLE_STRIP;
    case GFX_PT_TRIANGLEFAN: return GL_TRIANGLE_FAN;
    default: return GL_TRIANGLES;
    }
}

static void raw_draw(int prim, int vertexKind, const void *verts, int nverts, const WORD *indices, int nindices)
{
    GLsizei stride = vertexKind == GFX_VTX_TEX2 ? 40 : 32;
    if (!gl.context || !gl.target || nverts <= 0)
        return;
    g_perf.draws++;
    g_perf.prims += perf_prim_count(prim, nindices ? nindices : nverts);
    bind_target(gl.target);
    if (gl.stateDirty)
        apply_state();
    p_glBindBuffer(GL_ARRAY_BUFFER, gl.vbo);
    p_glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)nverts * stride, verts, GL_STREAM_DRAW);
    p_glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, stride, (const void *)(uintptr_t)0);
    p_glVertexAttribPointer(1, GFX_COLOUR_SIZE, GL_UNSIGNED_BYTE, GL_TRUE, stride, (const void *)(uintptr_t)16);
    p_glVertexAttribPointer(2, GFX_COLOUR_SIZE, GL_UNSIGNED_BYTE, GL_TRUE, stride, (const void *)(uintptr_t)20);
    p_glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, stride, (const void *)(uintptr_t)24);
    p_glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, stride, (const void *)(uintptr_t)(vertexKind == GFX_VTX_TEX2 ? 32 : 24));
    if (nindices) {
        p_glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl.ibo);
        p_glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)nindices * 2, indices, GL_STREAM_DRAW);
        p_glDrawElements(gl_prim(prim), nindices, GL_UNSIGNED_SHORT, NULL);
    } else {
        p_glDrawArrays(gl_prim(prim), 0, nverts);
    }
    if (gl.target)
        gl.target->version = ++content_tick;
}

void gfx_draw(int prim, int vertexKind, const void *verts, int nverts, const WORD *indices, int nindices)
{
    g_perf.drawCalls++;
    if (batch_add(prim, vertexKind, verts, nverts, indices, nindices))
        return;
    batch_flush();
    raw_draw(prim, vertexKind, verts, nverts, indices, nindices);
}

void gfx_draw_vbuffer(int prim, gfx_vbuffer *vb, DWORD start, DWORD count, const WORD *indices, DWORD nindices)
{
    (void)prim;
    (void)vb;
    (void)start;
    (void)count;
    (void)indices;
    (void)nindices;
}

static void draw_textured_quad(struct gfx_surface *dst, const RECT *dr, struct gfx_surface *src, const RECT *sr, int keyed)
{
    struct { float x, y, z, rhw; DWORD diffuse, specular; float u, v; } v[4];
    DWORD rs[N_RS], tss0[N_TSS], tss1[N_TSS], samp0[5];
    struct gfx_surface *stage0 = gl.stageTex[0], *stage1 = gl.stageTex[1], *prevTarget = gl.target;
    RECT d, s;
    float tw = (float)src->width, th = (float)src->height;
    int i;
    if (sr) s = *sr; else { s.left = 0; s.top = 0; s.right = src->width; s.bottom = src->height; }
    if (dr) d = *dr; else { d.left = 0; d.top = 0; d.right = dst->width; d.bottom = dst->height; }
    for (i = 0; i < 4; i++) {
        v[i].z = 0.0f;
        v[i].rhw = 1.0f;
        v[i].diffuse = 0xffffffff;
        v[i].specular = 0;
    }
    v[0].x = (float)d.left - 0.5f;  v[0].y = (float)d.top - 0.5f;    v[0].u = s.left / tw;  v[0].v = s.top / th;
    v[1].x = (float)d.right - 0.5f; v[1].y = (float)d.top - 0.5f;    v[1].u = s.right / tw; v[1].v = s.top / th;
    v[2].x = (float)d.left - 0.5f;  v[2].y = (float)d.bottom - 0.5f; v[2].u = s.left / tw;  v[2].v = s.bottom / th;
    v[3].x = (float)d.right - 0.5f; v[3].y = (float)d.bottom - 0.5f; v[3].u = s.right / tw; v[3].v = s.bottom / th;
    batch_flush();
    g_perf.blitQuads++;
    memcpy(rs, gl.rs, sizeof rs);
    memcpy(tss0, gl.tss[0], sizeof tss0);
    memcpy(tss1, gl.tss[1], sizeof tss1);
    memcpy(samp0, gl.samp[0], sizeof samp0);
    gl.rs[GFX_RS_ALPHATESTENABLE] = keyed;
    gl.rs[GFX_RS_ALPHABLENDENABLE] = 0;
    gl.rs[GFX_RS_ZENABLE] = 0;
    gl.rs[GFX_RS_ZWRITEENABLE] = 0;
    gl.rs[GFX_RS_FOGENABLE] = 0;
    gl.rs[GFX_RS_CULLMODE] = GFX_CULL_NONE;
    gl.rs[GFX_RS_ALPHAREF] = 1;
    gl.rs[GFX_RS_ALPHAFUNC] = GFX_CMP_GREATEREQUAL;
    gl.tss[0][GFX_TSS_COLOROP] = GFX_TOP_SELECTARG1;
    gl.tss[0][GFX_TSS_COLORARG1] = GFX_TA_TEXTURE;
    gl.tss[0][GFX_TSS_ALPHAOP] = GFX_TOP_SELECTARG1;
    gl.tss[0][GFX_TSS_ALPHAARG1] = GFX_TA_TEXTURE;
    gl.tss[0][GFX_TSS_TEXCOORDINDEX] = 0;
    gl.tss[1][GFX_TSS_COLOROP] = GFX_TOP_DISABLE;
    gl.tss[1][GFX_TSS_ALPHAOP] = GFX_TOP_DISABLE;
    gl.samp[0][SAMP_ADDRESS] = GFX_ADDRESS_CLAMP;
    gl.samp[0][SAMP_MAG] = 1;
    gl.samp[0][SAMP_MIN] = 1;
    gl.samp[0][SAMP_MIP] = GFX_MIPFILTER_NONE;
    gl.stageTex[0] = src;
    gl.stageTex[1] = NULL;
    gl.stateDirty = gl.samplerDirty = 1;
    gl.target = dst;
    raw_draw(GFX_PT_TRIANGLESTRIP, GFX_VTX_TEX1, v, 4, NULL, 0);
    memcpy(gl.rs, rs, sizeof rs);
    memcpy(gl.tss[0], tss0, sizeof tss0);
    memcpy(gl.tss[1], tss1, sizeof tss1);
    memcpy(gl.samp[0], samp0, sizeof samp0);
    gl.stageTex[0] = stage0;
    gl.stageTex[1] = stage1;
    gl.stateDirty = gl.samplerDirty = 1;
    gl.target = prevTarget;
}

static void read_target(struct gfx_surface *s, unsigned char *out)
{
    int y, row = s->width * 4;
    unsigned char *flip = malloc((size_t)row * s->height);
    if (!flip)
        return;
    batch_flush();
    p_glBindFramebuffer(GL_READ_FRAMEBUFFER, s->fbo);
    p_glPixelStorei(GL_PACK_ALIGNMENT, 4);
    p_glReadPixels(0, 0, s->width, s->height, GFX_PIXEL_FORMAT, GL_UNSIGNED_BYTE, flip);
    p_glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    for (y = 0; y < s->height; y++)
        memcpy(out + (size_t)y * row, flip + (size_t)(s->height - 1 - y) * row, row);
#ifdef GFX_GLES
    for (y = 0; y < s->width * s->height; y++)
        ((DWORD *)out)[y] = gl_rgba(((DWORD *)out)[y]);
#endif
    free(flip);
    gl.boundTarget = -1;
}

static void write_target(struct gfx_surface *s, const unsigned char *in)
{
    int y, row = s->width * 4;
    unsigned char *flip = malloc((size_t)row * s->height);
    if (!flip)
        return;
    for (y = 0; y < s->height; y++) {
        unsigned char *dst = flip + (size_t)(s->height - 1 - y) * row;
        int x;
        memcpy(dst, in + (size_t)y * row, row);
        for (x = 0; x < s->width; x++) {
            dst[x * 4 + 3] = 0xff;
#ifdef GFX_GLES
            ((DWORD *)dst)[x] = gl_rgba(((DWORD *)dst)[x]);
#endif
        }
    }
    p_glBindTexture(GL_TEXTURE_2D, s->tex);
    p_glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    p_glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    p_glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, s->width, s->height, GFX_PIXEL_FORMAT, GL_UNSIGNED_BYTE, flip);
    free(flip);
    gl.stateDirty = 1;
}

static int choose_vsync(void)
{
    const char *env = SDL_getenv("RAGEXE_VSYNC");
    return !(env && env[0] == '0' && !env[1]);
}

int gfx_open(struct C3dDevice *d, const GUID *adapter, int flags)
{
    (void)adapter;
    (void)flags;
    gl.window = d->hwnd;
    if (!gl.window)
        return GFX_E_DDRAW_CREATE;
    if (!gl.lock)
        gl.lock = SDL_CreateMutex();
    d->dd = gl.window;
    d->desktopBpp = 32;
    return 0;
}

static int ensure_context(void)
{
    if (gl.context)
        return 1;
    gl.context = SDL_GL_CreateContext(gl.window);
    if (!gl.context) {
        SDL_Log("gfx: SDL_GL_CreateContext: %s", SDL_GetError());
        return 0;
    }
    SDL_GL_MakeCurrent(gl.window, gl.context);
    if (!load_functions() || !build_program())
        return 0;
    p_glDisable(GL_DITHER);
    p_glGenVertexArrays(1, &gl.vao);
    p_glBindVertexArray(gl.vao);
    p_glGenBuffers(1, &gl.vbo);
    p_glGenBuffers(1, &gl.ibo);
    p_glBindBuffer(GL_ARRAY_BUFFER, gl.vbo);
    p_glEnableVertexAttribArray(0);
    p_glEnableVertexAttribArray(1);
    p_glEnableVertexAttribArray(2);
    p_glEnableVertexAttribArray(3);
    p_glEnableVertexAttribArray(4);
    p_glGenSamplers(2, gl.samplers);
    p_glBindSampler(0, gl.samplers[0]);
    p_glBindSampler(1, gl.samplers[1]);
    gl.vsync = choose_vsync();
    SDL_GL_SetSwapInterval(gl.vsync);
    gl.boundTarget = -1;
    return 1;
}

static void fill_surface_desc(struct SurfaceDesc *sd, int w, int h, int refresh)
{
    memset(sd, 0, sizeof(*sd));
    sd->size = sizeof(*sd);
    sd->flags = 0x1 | 0x2 | 0x4 | 0x8 | 0x1000 | 0x40000;
    sd->height = h;
    sd->width = w;
    sd->pitch = w * 4;
    sd->refreshRate = refresh;
    sd->pfSize = 0x20;
    sd->pfFlags = 0x40;
    sd->bitCount = 32;
    sd->rMask = 0xff0000;
    sd->gMask = 0xff00;
    sd->bMask = 0xff;
}

int gfx_create_screen(struct C3dDevice *d, const struct SurfaceDesc *mode, int flags)
{
    struct gfx_surface *scene, *save;
    (void)mode;
    if (!gl.window || !ensure_context())
        return GFX_E_PRIMARY;
    reset_state();
    batch_init(raw_draw);
    scene = new_handle(K_SCREEN, d->width, d->height, -1);
    if (!scene || !make_target(scene, 1))
        return GFX_E_PRIMARY;
    d->primary = scene;
    if (flags & 2) {
        d->back = new_handle(K_TARGET, d->width, d->height, -1);
        if (!d->back)
            return GFX_E_PRIMARY;
        d->back->fbo = scene->fbo;
        d->back->tex = scene->tex;
        d->back->depth = scene->depth;
    }
    d->target = (flags & 2) ? d->back : d->primary;
    d->target->refs++;
    gl.mainTarget = d->target;
    gl.target = d->target;
    fill_surface_desc(&d->primaryDesc, d->width, d->height, 0);
    d->bpp = 32;
    d->rMask = 0xff0000;
    d->gMask = 0xff00;
    d->bMask = 0xff;
    d->aMask = 0;
    save = new_handle(K_SAVE, d->width, d->height, -1);
    if (!save || !make_target(save, 0))
        return GFX_E_PRIMARY;
    d->save = save;
    d->dev = gl.context;
    bind_target(gl.target);
    return 0;
}

int gfx_create_3d(struct C3dDevice *d, const GUID *device, int flags)
{
    (void)device;
    (void)flags;
    if (!gl.context)
        return GFX_E_DEVICE_CREATE;
    d->d3d = gl.context;
    d->zbufferCaps = 0;
    memset(d->zformat, 0, sizeof(d->zformat));
    return 0;
}

int gfx_create_zbuffer(struct C3dDevice *d)
{
    struct gfx_surface *z;
    if (!gl.mainTarget)
        return GFX_E_ZBUFFER;
    z = new_handle(K_ZBUFFER, d->width, d->height, -1);
    if (!z)
        return GFX_E_ZBUFFER;
    z->depth = gl.mainTarget->depth;
    d->zbuffer = z;
    gl.depthSurface = z;
    return 0;
}

int gfx_get_caps(struct C3dDevice *d, struct gfx_caps *out)
{
    GLint maxSize = 4096;
    (void)d;
    if (!gl.context)
        return -1;
    p_glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxSize);
    out->zbufferless = 0;
    out->vertexFog = 1;
    out->mipNearest = 1;
    out->bumpEnv = 1;
    out->bumpEnvLuminance = 1;
    out->waterBump = 1;
    out->minTexW = 1;
    out->minTexH = 1;
    out->maxTexW = (DWORD)maxSize;
    out->maxTexH = (DWORD)maxSize;
    out->texAspect = 0;
    out->guardLeft = -32768.0f;
    out->guardTop = -32768.0f;
    out->guardRight = 32767.0f;
    out->guardBottom = 32767.0f;
    out->bltStretch = 1;
    return 0;
}

void gfx_default_states(struct C3dDevice *d)
{
    static const struct { int state; DWORD value; } render_states[] = {
        { GFX_RS_CULLMODE, GFX_CULL_NONE },
        { GFX_RS_ZFUNC, GFX_CMP_LESSEQUAL },
        { GFX_RS_ALPHAFUNC, GFX_CMP_GREATEREQUAL },
        { GFX_RS_ALPHAREF, 0xf0 },
        { GFX_RS_ZENABLE, TRUE },
        { GFX_RS_ZWRITEENABLE, TRUE },
        { GFX_RS_SRCBLEND, GFX_BLEND_SRCALPHA },
        { GFX_RS_DESTBLEND, GFX_BLEND_INVSRCALPHA },
    };
    static const struct { int stage, state; DWORD value; } stage_states[] = {
        { 0, GFX_TSS_TEXCOORDINDEX, 0 },
        { 0, GFX_TSS_COLOROP, GFX_TOP_MODULATE },
        { 0, GFX_TSS_COLORARG1, GFX_TA_TEXTURE },
        { 0, GFX_TSS_COLORARG2, GFX_TA_DIFFUSE },
        { 0, GFX_TSS_ALPHAOP, GFX_TOP_MODULATE },
        { 0, GFX_TSS_ALPHAARG1, GFX_TA_TEXTURE },
        { 0, GFX_TSS_ALPHAARG2, GFX_TA_DIFFUSE },
        { 1, GFX_TSS_TEXCOORDINDEX, 1 },
        { 1, GFX_TSS_COLORARG1, GFX_TA_TEXTURE },
        { 1, GFX_TSS_COLOROP, GFX_TOP_MODULATEALPHA_ADDCOLOR },
        { 1, GFX_TSS_COLORARG2, GFX_TA_CURRENT },
        { 1, GFX_TSS_ALPHAOP, GFX_TOP_DISABLE },
        { 2, GFX_TSS_COLOROP, GFX_TOP_DISABLE },
        { 2, GFX_TSS_ALPHAOP, GFX_TOP_DISABLE },
        { 0, GFX_TSS_MINFILTER, GFX_FILTER_LINEAR },
        { 0, GFX_TSS_MAGFILTER, GFX_FILTER_LINEAR },
        { 1, GFX_TSS_MINFILTER, GFX_FILTER_LINEAR },
        { 1, GFX_TSS_MAGFILTER, GFX_FILTER_LINEAR },
        { 0, GFX_TSS_ADDRESS, GFX_ADDRESS_CLAMP },
        { 1, GFX_TSS_ADDRESS, GFX_ADDRESS_BORDER },
        { 1, GFX_TSS_BORDERCOLOR, 0xff000000 },
    };
    size_t i;
    (void)d;
    for (i = 0; i < sizeof render_states / sizeof render_states[0]; i++)
        gfx_set_render_state(render_states[i].state, render_states[i].value);
    for (i = 0; i < sizeof stage_states / sizeof stage_states[0]; i++)
        gfx_set_stage_state(stage_states[i].stage, stage_states[i].state, stage_states[i].value);
}

int gfx_close(struct C3dDevice *d)
{
    struct gfx_surface *s;
    batch_drop();
    if (d->target)  { gfx_release(d->target);  d->target = NULL; }
    if (d->back)    { gfx_release(d->back);    d->back = NULL; }
    if (d->primary) { gfx_release(d->primary); d->primary = NULL; }
    if (d->zbuffer) { gfx_release(d->zbuffer); d->zbuffer = NULL; }
    if (d->save)    { gfx_release(d->save);    d->save = NULL; }
    gl.mainTarget = gl.target = gl.depthSurface = NULL;
    for (s = gl.handles; s; s = s->next)
        drop_gpu(s);
    collect_dead_textures();
    if (gl.context) {
        p_glDeleteSamplers(2, gl.samplers);
        p_glDeleteBuffers(1, &gl.vbo);
        p_glDeleteBuffers(1, &gl.ibo);
        p_glDeleteVertexArrays(1, &gl.vao);
        p_glDeleteProgram(gl.program);
        SDL_GL_DestroyContext(gl.context);
        gl.context = NULL;
    }
    d->dev = NULL;
    d->d3d = NULL;
    d->dd = NULL;
    return 0;
}

void gfx_flip_to_gdi(struct C3dDevice *d, int unlockFirst)
{
    (void)d;
    (void)unlockFirst;
    batch_flush();
}

/* The cover in window points, in the drawable's pixels. */
static struct PresentCover cover_in_pixels(int w, int h)
{
    struct PresentCover pixels = present_cover;
    int points_w = 0, points_h = 0;
    SDL_GetWindowSize(gl.window, &points_w, &points_h);
    if (points_w > 0 && points_w != w) {
        pixels.left = (int)((long long)pixels.left * w / points_w);
        pixels.right = (int)((long long)pixels.right * w / points_w);
    }
    if (points_h > 0 && points_h != h) {
        pixels.top = (int)((long long)pixels.top * h / points_h);
        pixels.bottom = (int)((long long)pixels.bottom * h / points_h);
        pixels.lift = (int)((long long)pixels.lift * h / points_h);
    }
    return pixels;
}

int gfx_present(struct C3dDevice *d, int vsync, int whole)
{
    int w = 0, h = 0, fx, fy, fw, fh;
    struct gfx_surface *scene = gl.mainTarget;
    struct PresentCover cover;
    (void)d;
    (void)vsync;
    (void)whole;
    batch_flush();
    if (!gl.context || !scene)
        return -1;
    SDL_GetWindowSizeInPixels(gl.window, &w, &h);
    cover = cover_in_pixels(w, h);
    present_fit(w, h, scene->width, scene->height, &cover, &fx, &fy, &fw, &fh);
    p_glBindFramebuffer(GL_READ_FRAMEBUFFER, scene->fbo);
    p_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    if (fx || fy || fw != w || fh != h) {
        p_glDisable(GL_SCISSOR_TEST);
        p_glViewport(0, 0, w, h);
        p_glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        p_glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        p_glClear(GL_COLOR_BUFFER_BIT);
        gl.stateDirty = 1;
    }
    fy = h - fy - fh;
    p_glBlitFramebuffer(0, 0, scene->width, scene->height, fx, fy, fx + fw, fy + fh, GL_COLOR_BUFFER_BIT,
                        (fw == scene->width && fh == scene->height) ? GL_NEAREST : GL_LINEAR);
    SDL_GL_SwapWindow(gl.window);
    p_glBindFramebuffer(GL_FRAMEBUFFER, scene->fbo);
    p_glViewport(0, 0, scene->width, scene->height);
    gl.boundTarget = (int)scene->fbo;
    collect_dead_textures();
    return 0;
}

int gfx_restore_all(struct C3dDevice *d)
{
    (void)d;
    return gl.context ? 0 : -1;
}

void gfx_evict_managed_textures(struct C3dDevice *d)
{
    (void)d;
}

static void resize_target(struct gfx_surface *s, int w, int h, int withDepth)
{
    if (!s)
        return;
    drop_gpu(s);
    s->width = w;
    s->height = h;
    make_target(s, withDepth);
    s->version = ++content_tick;
}

int gfx_set_mode(struct C3dDevice *d, const GUID *adapter, int w, int h, int fullscreen)
{
    struct gfx_surface *s;
    (void)adapter;
    (void)fullscreen;
    if (!gl.context || w <= 0 || h <= 0)
        return -1;
    batch_flush();
    if (d->primary)
        resize_target(d->primary, w, h, 1);
    for (s = gl.handles; s; s = s->next) {
        if (s->kind == K_TARGET && d->primary) {
            s->fbo = d->primary->fbo;
            s->tex = d->primary->tex;
            s->depth = d->primary->depth;
            s->width = w;
            s->height = h;
        } else if (s->kind == K_ZBUFFER && d->primary) {
            s->depth = d->primary->depth;
            s->width = w;
            s->height = h;
        }
    }
    if (d->save)
        resize_target(d->save, w, h, 0);
    gl.boundTarget = -1;
    fill_surface_desc(&d->primaryDesc, w, h, 0);
    return 0;
}

/* One adapter, the GL context's: which display it shows on is the window's, so no adapter names one. */
PlatformDisplay gfx_adapter_monitor(const GUID *adapter)
{
    (void)adapter;
    return 0;
}

void gfx_set_mipmaps(int on)
{
    gl.mipmaps = on;
}

static const GUID gl_device_guid = { 0x3ec2a7e0, 0x5b2f, 0x4f62, { 'o', 'p', 'e', 'n', 'g', 'l', '3', '3' } };

void gfx_enum_adapter(const GUID *adapter, const char *desc, PlatformDisplay monitor)
{
    static const struct { unsigned w, h; } windowed[] = {
        { 640, 480 }, { 800, 600 }, { 1024, 768 }, { 1280, 720 }, { 1280, 1024 }, { 1366, 768 },
        { 1600, 900 }, { 1920, 1080 }, { 2560, 1440 }
    };
    struct DisplayModeList modes = {0};
    SDL_DisplayID display = monitor ? monitor : SDL_GetPrimaryDisplay();
    const SDL_DisplayMode *desktop = display ? SDL_GetDesktopDisplayMode(display) : NULL;
    SDL_DisplayMode **list;
    int count = 0, i;
    unsigned maxW = desktop ? (unsigned)desktop->w : 1920, maxH = desktop ? (unsigned)desktop->h : 1080;
    for (i = 0; i < (int)(sizeof windowed / sizeof windowed[0]); i++)
        if (windowed[i].w <= maxW && windowed[i].h <= maxH)
            display_modes_append(&modes, windowed[i].w, windowed[i].h, 32);
    list = display ? SDL_GetFullscreenDisplayModes(display, &count) : NULL;
    for (i = 0; list && i < count; i++) {
        size_t k;
        int dup = 0;
        for (k = 0; k < modes.count; k++)
            if (modes.items[k].width == (unsigned)list[i]->w && modes.items[k].height == (unsigned)list[i]->h)
                dup = 1;
        if (!dup)
            display_modes_append(&modes, (unsigned)list[i]->w, (unsigned)list[i]->h, 32);
    }
    SDL_free(list);
    display_modes_sort(&modes);
    display_devices_add(desc, adapter, desc, &gl_device_guid, 1, DISPLAY_DEPTH_32, &modes);
    display_modes_destroy(&modes);
}

int gfx_ddraw_enumerate(GfxAdapterCallback callback, void *ctx, DWORD flags)
{
    char desc[] = "OpenGL";
    char name[] = "display";
    (void)flags;
    callback(NULL, desc, name, ctx, 0);
    return 0;
}

gfx_surface *gfx_create_surface(int w, int h)
{
    struct gfx_surface *s;
    g_perf.surfCreated++;
    s = new_handle(K_SYSMEM, w, h, -1);
    if (!s)
        return NULL;
    if (!alloc_pixels(s)) {
        gfx_release(s);
        return NULL;
    }
    s->key = CLIENT_SURFACE_KEY;
    s->dirty = 1;
    return s;
}

int gfx_texture_format_ok(int fmt)
{
    return fmt == GFX_FMT_1555 || fmt == GFX_FMT_4444 || fmt == GFX_FMT_565 || fmt == GFX_FMT_8888
        || fmt == GFX_FMT_BUMP;
}

static int mip_levels(int w, int h)
{
    int levels = 1, most = 1, n;
    if (w > 2) {
        for (;;) {
            if ((1 << levels) >= h)
                break;
            levels++;
            n = 1 << levels;
            if (n >= w)
                break;
        }
    }
    for (n = w > h ? w : h; n > 1; n >>= 1)
        most++;
    if (levels > most)
        levels = most;
    return levels < MAX_LEVELS ? levels : MAX_LEVELS;
}

gfx_surface *gfx_create_texture(int w, int h, int fmt)
{
    struct gfx_surface *s;
    g_perf.texCreated++;
    if (!gfx_texture_format_ok(fmt) || w <= 0 || h <= 0)
        return NULL;
    s = new_handle(K_TEXTURE, w, h, fmt);
    if (!s)
        return NULL;
    s->levels = (gl.mipmaps && fmt != GFX_FMT_BUMP) ? mip_levels(w, h) : 1;
    if (!alloc_pixels(s)) {
        gfx_release(s);
        return NULL;
    }
    s->dirty = 1;
    return s;
}

gfx_surface *gfx_create_render_target(int w, int h)
{
    struct gfx_surface *s;
    if (!gl.context)
        return NULL;
    s = new_handle(K_RTTEX, w, h, GFX_FMT_8888);
    if (!s)
        return NULL;
    if (!make_target(s, 0)) {
        gfx_release(s);
        return NULL;
    }
    return s;
}

int gfx_get_size(gfx_surface *s, int *w, int *h)
{
    *w = s->width;
    *h = s->height;
    return 0;
}

int gfx_lock(gfx_surface *s, const RECT *r, int flags, struct gfx_lock *out)
{
    int bytes;
    if (flags & GFX_LOCK_UNSIZED)
        return -1;
    g_perf.locks++;
    if (is_gpu_target(s)) {
        if (!s->pixels[0]) {
            s->pixels[0] = malloc((size_t)s->width * s->height * 4);
            if (!s->pixels[0])
                return -1;
        }
        g_perf.targetLocks++;
        read_target(s, s->pixels[0]);
        s->lockLevel = 0;
    } else if (!s->pixels[0]) {
        return -1;
    }
    bytes = s->bpp >> 3;
    out->bits = s->pixels[0] + (r ? (size_t)r->top * s->width * bytes + (size_t)r->left * bytes : 0);
    out->pitch = s->width * bytes;
    out->width = s->width;
    out->height = s->height;
    out->bpp = s->bpp;
    return 0;
}

int gfx_unlock(gfx_surface *s)
{
    s->version = ++content_tick;
    if (is_gpu_target(s)) {
        if (s->lockLevel == 0 && s->pixels[0])
            write_target(s, s->pixels[0]);
        s->lockLevel = -1;
    } else {
        s->dirty = 1;
    }
    return 0;
}

static DWORD read_pixel(const unsigned char *p, int bpp)
{
    DWORD v;
    if (bpp == 32) {
        memcpy(&v, p, 4);
        return v;
    }
    return (DWORD)p[0] | (DWORD)p[1] << 8;
}

static void write_pixel(unsigned char *p, int bpp, DWORD v)
{
    if (bpp == 32)
        memcpy(p, &v, 4);
    else {
        p[0] = (unsigned char)v;
        p[1] = (unsigned char)(v >> 8);
    }
}

static DWORD unpack(int fmt, DWORD p)
{
    DWORD r, g, b, a;
    switch (fmt) {
    case GFX_FMT_565:
        r = (p >> 11) & 0x1f; g = (p >> 5) & 0x3f; b = p & 0x1f;
        return 0xff000000 | ((r << 3 | r >> 2) << 16) | ((g << 2 | g >> 4) << 8) | (b << 3 | b >> 2);
    case GFX_FMT_1555:
        a = (p & 0x8000) ? 0xff : 0;
        r = (p >> 10) & 0x1f; g = (p >> 5) & 0x1f; b = p & 0x1f;
        return (a << 24) | ((r << 3 | r >> 2) << 16) | ((g << 3 | g >> 2) << 8) | (b << 3 | b >> 2);
    case GFX_FMT_4444:
        a = (p >> 12) & 0xf; r = (p >> 8) & 0xf; g = (p >> 4) & 0xf; b = p & 0xf;
        return (a * 17 << 24) | (r * 17 << 16) | (g * 17 << 8) | (b * 17);
    default:
        return p;
    }
}

static DWORD pack(int fmt, DWORD argb)
{
    DWORD a = argb >> 24, r = (argb >> 16) & 0xff, g = (argb >> 8) & 0xff, b = argb & 0xff;
    switch (fmt) {
    case GFX_FMT_565:  return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
    case GFX_FMT_1555: return ((a >> 7) << 15) | ((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3);
    case GFX_FMT_4444: return ((a >> 4) << 12) | ((r >> 4) << 8) | ((g >> 4) << 4) | (b >> 4);
    default:           return argb;
    }
}

static int cpu_blit(struct gfx_surface *dst, const RECT *dr, struct gfx_surface *src, const unsigned char *srcPixels,
                    const RECT *sr)
{
    RECT d, s;
    int sbpp = src->bpp, dbpp = dst->bpp, same = src->fmt == dst->fmt || (src->bpp == 32 && dst->bpp == 32);
    int x, y, dw, dh, sw, sh;
    if (sr) s = *sr; else { s.left = 0; s.top = 0; s.right = src->width; s.bottom = src->height; }
    if (dr) d = *dr; else { d.left = 0; d.top = 0; d.right = dst->width; d.bottom = dst->height; }
    dw = d.right - d.left;
    dh = d.bottom - d.top;
    sw = s.right - s.left;
    sh = s.bottom - s.top;
    if (dw <= 0 || dh <= 0 || sw <= 0 || sh <= 0 || !dst->pixels[0] || !srcPixels)
        return 0;
    g_perf.cpuBlits++;
    for (y = 0; y < dh; y++) {
        const unsigned char *srow = srcPixels + (size_t)(s.top + (dh == sh ? y : y * sh / dh)) * src->width * (sbpp >> 3);
        unsigned char *drow = dst->pixels[0] + (size_t)(d.top + y) * dst->width * (dbpp >> 3);
        for (x = 0; x < dw; x++) {
            int sx = s.left + (dw == sw ? x : x * sw / dw);
            DWORD p = read_pixel(srow + sx * (sbpp >> 3), sbpp);
            if (!same)
                p = pack(dst->fmt, unpack(src->fmt, p));
            write_pixel(drow + (d.left + x) * (dbpp >> 3), dbpp, p);
        }
    }
    return 0;
}

int gfx_blit(gfx_surface *dst, const RECT *dr, gfx_surface *src, const RECT *sr, int flags)
{
    (void)flags;
    batch_flush();
    g_perf.blits++;
    dst->version = ++content_tick;
    if (dst == src || (is_gpu_target(dst) && is_gpu_target(src) && dst->fbo == src->fbo))
        return 0;
    if (is_gpu_target(dst) && is_gpu_target(src)) {
        RECT d, s;
        if (sr) s = *sr; else { s.left = 0; s.top = 0; s.right = src->width; s.bottom = src->height; }
        if (dr) d = *dr; else { d.left = 0; d.top = 0; d.right = dst->width; d.bottom = dst->height; }
        p_glBindFramebuffer(GL_READ_FRAMEBUFFER, src->fbo);
        p_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dst->fbo);
        p_glBlitFramebuffer(s.left, src->height - s.bottom, s.right, src->height - s.top,
                            d.left, dst->height - d.bottom, d.right, dst->height - d.top, GL_COLOR_BUFFER_BIT, GL_NEAREST);
        p_glBindFramebuffer(GL_FRAMEBUFFER, 0);
        gl.boundTarget = -1;
        return 0;
    }
    if (is_gpu_target(dst)) {
        if (!src->pixels[0])
            return -1;
        draw_textured_quad(dst, dr, src, sr, 0);
        return 0;
    }
    if (is_gpu_target(src)) {
        unsigned char *pixels = malloc((size_t)src->width * src->height * 4);
        int r;
        if (!pixels)
            return -1;
        read_target(src, pixels);
        r = cpu_blit(dst, dr, src, pixels, sr);
        free(pixels);
        dst->dirty = 1;
        return r;
    }
    dst->dirty = 1;
    return cpu_blit(dst, dr, src, src->pixels[0], sr);
}

int gfx_refresh_mips(gfx_surface *tex, const RECT *r)
{
    RECT rc, top;
    int level, bytes, pw, ph, x, y;
    if (!tex || tex->levels <= 1 || tex->fmt == GFX_FMT_BUMP || !tex->pixels[0])
        return 0;
    bytes = tex->bpp >> 3;
    if (r) top = *r; else { top.left = 0; top.top = 0; top.right = tex->width; top.bottom = tex->height; }
    pw = top.right - top.left;
    ph = top.bottom - top.top;
    if (pw <= 0 || ph <= 0)
        return -1;
    rc = top;
    for (level = 1; level < tex->levels; level++) {
        int w, h, lw = tex->width >> level;
        if (lw < 1)
            lw = 1;
        rc.left >>= 1;
        rc.right >>= 1;
        rc.top >>= 1;
        rc.bottom >>= 1;
        if (rc.left == rc.right || rc.top == rc.bottom)
            continue;
        w = rc.right - rc.left;
        h = rc.bottom - rc.top;
        for (y = 0; y < h; y++) {
            unsigned char *d = tex->pixels[level] + ((size_t)(rc.top + y) * lw + rc.left) * bytes;
            const unsigned char *srow = tex->pixels[0]
                + ((size_t)(top.top + (2 * y + 1) * ph / (2 * h)) * tex->width + top.left) * bytes;
            for (x = 0; x < w; x++)
                memcpy(d + x * bytes, srow + (size_t)((2 * x + 1) * pw / (2 * w)) * bytes, bytes);
        }
    }
    tex->dirty = 1;
    return 0;
}

int gfx_is_lost(gfx_surface *s)
{
    (void)s;
    return 0;
}

void gfx_restore(gfx_surface *s)
{
    s->version = ++content_tick;
}

int gfx_get_dc(gfx_surface *s, HDC *out)
{
    (void)s;
    *out = NULL;
    return -1;
}

void gfx_release_dc(gfx_surface *s, HDC dc)
{
    (void)s;
    (void)dc;
}

int gfx_begin_scene(void)
{
    if (!gl.context)
        return -1;
    gl.inScene = 1;
    return 0;
}

void gfx_end_scene(void)
{
    batch_flush();
    gl.inScene = 0;
}

int gfx_clear(int flags, DWORD colour, float z, const RECT *r)
{
    GLbitfield bits = 0;
    struct gfx_surface *t = gl.target;
    batch_flush();
    if (!gl.context || !t)
        return -1;
    bind_target(t);
    if (r) {
        p_glEnable(GL_SCISSOR_TEST);
        p_glScissor(r->left, t->height - r->bottom, r->right - r->left, r->bottom - r->top);
    }
    if (flags & GFX_CLEAR_TARGET) {
        p_glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        p_glClearColor(((colour >> 16) & 0xff) / 255.0f, ((colour >> 8) & 0xff) / 255.0f, (colour & 0xff) / 255.0f,
                       ((colour >> 24) & 0xff) / 255.0f);
        bits |= GL_COLOR_BUFFER_BIT;
    }
    if ((flags & GFX_CLEAR_ZBUFFER) && t->depth) {
        p_glDepthMask(GL_TRUE);
#ifdef GFX_GLES
        p_glClearDepthf(z);
#else
        p_glClearDepth(z);
#endif
        bits |= GL_DEPTH_BUFFER_BIT;
    }
    if (bits)
        p_glClear(bits);
    if (r)
        p_glDisable(GL_SCISSOR_TEST);
    gl.stateDirty = 1;
    t->version = ++content_tick;
    return 0;
}

gfx_surface *gfx_get_render_target(void)
{
    if (!gl.target)
        return NULL;
    gl.target->refs++;
    return gl.target;
}

int gfx_set_render_target(gfx_surface *s)
{
    batch_flush();
    if (!gl.context || !s || !s->fbo)
        return -1;
    gl.target = s;
    bind_target(s);
    return 0;
}

void gfx_set_render_state(int state, DWORD value)
{
    if ((unsigned)state >= N_RS || gl.rs[state] == value)
        return;
    batch_flush();
    gl.rs[state] = value;
    gl.stateDirty = 1;
    g_perf.statesSent++;
}

int gfx_get_render_state(int state, DWORD *out)
{
    if ((unsigned)state >= N_RS)
        return -1;
    *out = state == GFX_RS_TEXTUREPERSPECTIVE || state == GFX_RS_COLORKEYENABLE ? TRUE : gl.rs[state];
    return 0;
}

static void set_sampler(int stage, int which, DWORD value)
{
    if (stage >= 2 || gl.samp[stage][which] == value)
        return;
    batch_flush();
    gl.samp[stage][which] = value;
    gl.samplerDirty = 1;
    gl.stateDirty = 1;
}

void gfx_set_stage_state(int stage, int state, DWORD value)
{
    if ((unsigned)stage >= 8)
        return;
    switch (state) {
    case GFX_TSS_ADDRESS:     set_sampler(stage, SAMP_ADDRESS, value); break;
    case GFX_TSS_BORDERCOLOR: set_sampler(stage, SAMP_BORDER, value); break;
    case GFX_TSS_MAGFILTER:   set_sampler(stage, SAMP_MAG, value); break;
    case GFX_TSS_MINFILTER:   set_sampler(stage, SAMP_MIN, value); break;
    case GFX_TSS_MIPFILTER:   set_sampler(stage, SAMP_MIP, value); break;
    default:
        if ((unsigned)state < N_TSS && gl.tss[stage][state] != value) {
            batch_flush();
            gl.tss[stage][state] = value;
            gl.stateDirty = 1;
        }
        break;
    }
}

void gfx_set_texture(int stage, gfx_surface *s)
{
    if ((unsigned)stage >= 8 || gl.stageTex[stage] == s)
        return;
    batch_flush();
    g_perf.texSets++;
    gl.stageTex[stage] = s;
    gl.stateDirty = 1;
    if (stage < 2)
        gl.samplerDirty = 1;
}

void gfx_set_transform(int which, const struct GfxMatrix *m)
{
    (void)which;
    (void)m;
}

void gfx_set_light(const float *dir, const float *diffuse, const float *ambient)
{
    (void)dir;
    (void)diffuse;
    (void)ambient;
}

unsigned gfx_content_version(gfx_surface *s)
{
    return s ? s->version : 0;
}

struct gfx_caps g_gfxCaps;
#endif

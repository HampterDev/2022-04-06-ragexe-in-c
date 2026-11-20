/* tools/shaders: compiles the renderer's HLSL (src/render/shaders/<name>.hlsl) for SDL's GPU API
   and writes <name>.h beside each source: per entry point the SPIR-V (Vulkan 1.0) and the MSL
   (Metal), as byte arrays, with the resource counts SDL_GPUShaderCreateInfo needs. The header's
   comment records hashes (FNV-1a, 64 bits, over text with LF line ends) of the source and its
   includes, of this tool's own source, which holds the flags and the binding rules, and of the
   output after the comment, and the versions of the compilers. tools/shaders/check.py (build.bat
   check and the test suite) fails when a source or this tool changed since, or the output was
   edited.

   usage: shaders <dxc> <spirv-val> <tools/shaders/shaders.c> <work directory> <sdk version> <file.hlsl>...

   An entry point vs_main is a vertex shader, fs_main a fragment shader, cs_main a compute shader,
   which a file holds alone. dxc compiles each to SPIR-V with HLSL registers that follow SDL's rules,
   which land in the descriptor sets SDL's Vulkan driver binds:
   - vertex and fragment shaders (SDL_CreateGPUShader in SDL_gpu.h): vertex resources in space 0
     and its uniforms in space 1, fragment ones in spaces 2 and 3; sampled textures, then storage
     textures, then storage buffers;
   - compute shaders (SDL_CreateGPUComputePipeline): space 0 sampled textures, then read-only
     storage textures, then read-only storage buffers; space 1 read-write storage textures, then
     read-write storage buffers; space 2 uniforms.
   A texture and its sampler share one binding as [[vk::combinedImageSampler]]. A read-only storage
   texture, of either kind of shader, is a texture without a sampler (Texture2D, read with Load or
   []), since SDL's Vulkan driver binds it as a sampled image; only a compute shader's read-write
   storage textures are storage images (RWTexture2D). SPIRV-Cross's C
   API turns the SPIR-V into MSL with the indices SDL's Metal driver binds: [[texture(n)]] and
   [[sampler(n)]] in the order above, [[buffer(n)]] the uniform buffers first and then the storage
   buffers in the order above. The tool fails when a resource is not where those rules put it.
   Entry points are main in SPIR-V and main0 in MSL. */
#define _CRT_SECURE_NO_WARNINGS
#include <spirv_cross/spirv_cross_c.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DXC_FLAGS "-spirv -fspv-target-env=vulkan1.0 -fspv-entrypoint-name=main -O3 -Zpr -WX"
#define MSL_FLAGS "MSL 2.1"
#define MAX_SOURCE (1 << 20)

static const char *g_dxc, *g_val, *g_work, *g_sdk;
static char g_dxcVersion[256];
static uint64_t g_toolHash;

static void fail(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    fputs("shaders: ", stderr);
    vfprintf(stderr, fmt, ap);
    fputc('\n', stderr);
    va_end(ap);
    exit(1);
}

/* Runs a command line through the shell; the whole line is quoted once more, as cmd /c takes
   it. */
static int run(const char *fmt, ...)
{
    char line[4096], quoted[4200];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(line, sizeof line, fmt, ap);
    va_end(ap);
#ifdef _WIN32
    snprintf(quoted, sizeof quoted, "\"%s\"", line);
#else
    snprintf(quoted, sizeof quoted, "%s", line);
#endif
    return system(quoted);
}

static unsigned char *read_file(const char *path, size_t *size)
{
    FILE *f = fopen(path, "rb");
    unsigned char *data;
    long n;
    if (!f)
        return NULL;
    fseek(f, 0, SEEK_END);
    n = ftell(f);
    fseek(f, 0, SEEK_SET);
    data = malloc((size_t)n + 1);
    if (!data || fread(data, 1, (size_t)n, f) != (size_t)n)
        fail("cannot read %s", path);
    fclose(f);
    data[n] = 0;
    *size = (size_t)n;
    return data;
}

/* FNV-1a, 64 bits, over text with its line ends made LF (tools/shaders/check.py does the same). */
static uint64_t fnv(uint64_t h, const unsigned char *p, size_t n)
{
    size_t i;
    for (i = 0; i < n; i++) {
        if (p[i] == '\r' && i + 1 < n && p[i + 1] == '\n')
            continue;
        h = (h ^ p[i]) * 0x100000001b3ull;
    }
    return h;
}

/* The source's hash: the file, then each file it includes with #include "name", named from the
   directory of the file that includes it as dxc names it, depth first, in the order they appear,
   each path once. */
static uint64_t hash_source(const char *dir, const char *name, uint64_t h, char seen[][1024], int *nseen)
{
    char path[1024], subdir[1024];
    unsigned char *text;
    size_t size;
    const char *p, *slash;
    int i;
    snprintf(path, sizeof path, "%s/%s", dir, name);
    for (i = 0; i < *nseen; i++)
        if (!strcmp(seen[i], path))
            return h;
    if (*nseen >= 32)
        fail("too many includes");
    snprintf(seen[(*nseen)++], 1024, "%s", path);
    slash = strrchr(path, '/');
    snprintf(subdir, sizeof subdir, "%.*s", (int)(slash - path), path);
    text = read_file(path, &size);
    if (!text)
        fail("cannot read %s", path);
    h = fnv(h, text, size);
    for (p = (const char *)text; (p = strstr(p, "#include \"")) != NULL;) {
        char inc[128];
        const char *end;
        p += 10;
        end = strchr(p, '"');
        if (!end || end - p >= (long)sizeof inc)
            fail("%s: bad #include", path);
        memcpy(inc, p, (size_t)(end - p));
        inc[end - p] = 0;
        h = hash_source(subdir, inc, h, seen, nseen);
        p = end;
    }
    free(text);
    return h;
}

typedef struct {
    unsigned set, binding;
    spvc_variable_id id;
    const char *name;
} Res;

typedef struct {
    Res r[32];
    unsigned n;
} List;

static spvc_compiler g_comp;

static void collect(spvc_resources res, spvc_resource_type type, List *out)
{
    const spvc_reflected_resource *list;
    size_t count, i;
    out->n = 0;
    spvc_resources_get_resource_list_for_type(res, type, &list, &count);
    for (i = 0; i < count; i++) {
        Res *r;
        if (out->n >= 32)
            fail("too many resources");
        r = &out->r[out->n++];
        r->id = list[i].id;
        r->name = list[i].name;
        r->set = spvc_compiler_get_decoration(g_comp, list[i].id, SpvDecorationDescriptorSet);
        r->binding = spvc_compiler_get_decoration(g_comp, list[i].id, SpvDecorationBinding);
    }
}

/* The entries of a list in one set, by binding. */
static unsigned in_set(const List *l, unsigned set, Res *o)
{
    unsigned n = 0, i, j;
    for (i = 0; i < l->n; i++)
        if (l->r[i].set == set)
            o[n++] = l->r[i];
    for (i = 1; i < n; i++)
        for (j = i; j > 0 && o[j - 1].binding > o[j].binding; j--) {
            Res t = o[j];
            o[j] = o[j - 1];
            o[j - 1] = t;
        }
    return n;
}

static unsigned not_in(const List *l, unsigned a, unsigned b)
{
    unsigned i, n = 0;
    for (i = 0; i < l->n; i++)
        if (l->r[i].set != a && l->r[i].set != b)
            n++;
    return n;
}

static void place(const char *what, SpvExecutionModel stage, const Res *r, unsigned binding, unsigned buffer,
                  unsigned texture, unsigned sampler)
{
    spvc_msl_resource_binding_2 b;
    if (r->binding != binding)
        fail("%s: %s '%s' is at set %u binding %u, where SDL expects binding %u", what, "resource", r->name, r->set,
             r->binding, binding);
    spvc_msl_resource_binding_init_2(&b);
    b.stage = stage;
    b.desc_set = r->set;
    b.binding = r->binding;
    b.count = 1;
    b.msl_buffer = buffer;
    b.msl_texture = texture;
    b.msl_sampler = sampler;
    spvc_compiler_msl_add_resource_binding_2(g_comp, &b);
}

#define VERTEX 0
#define FRAGMENT 1
#define COMPUTE 2

static const char *const g_suffix[3] = {"vs", "fs", "cs"};
static const char *const g_profile[3] = {"vs_6_0", "ps_6_0", "cs_6_0"};

struct Compiled {
    unsigned char *spirv;
    size_t spirvSize;
    char *msl;
    /* storageTextures and storageBuffers: a compute shader's read-only ones */
    unsigned samplers, storageTextures, storageBuffers, uniformBuffers;
    unsigned readwriteStorageTextures, readwriteStorageBuffers, threads[3];
};

/* A vertex or fragment shader's resources (the comment at the top), its own in `set` and its
   uniforms in set + 1. */
static void place_graphics(const char *file, const char *entry, SpvExecutionModel stage, unsigned set,
                           const List *sampled, const List *storageTextures, const List *storageImages,
                           const List *storageBuffers, const List *uniforms, struct Compiled *out)
{
    const unsigned NONE = ~0u;
    Res a[32];
    unsigned i, n, textures;
    if (storageImages->n)
        fail("%s (%s): a storage image, where SDL binds a storage texture as a sampled image (declare it "
             "Texture2D, without a sampler)", file, entry);
    if (not_in(sampled, set, set) || not_in(storageTextures, set, set) || not_in(storageBuffers, set, set)
        || not_in(uniforms, set + 1, set + 1))
        fail("%s (%s): a resource outside the sets SDL binds for this stage (%u and %u)", file, entry, set, set + 1);

    /* sampled textures 0..n-1, storage textures after them, storage buffers after those; MSL
       numbers textures and samplers alike, and buffers uniforms first */
    n = in_set(sampled, set, a);
    for (i = 0; i < n; i++)
        place(file, stage, &a[i], i, NONE, i, i);
    out->samplers = n;
    textures = n;
    n = in_set(storageTextures, set, a);
    for (i = 0; i < n; i++)
        place(file, stage, &a[i], textures + i, NONE, textures + i, NONE);
    out->storageTextures = n;
    textures += n;
    n = in_set(uniforms, set + 1, a);
    for (i = 0; i < n; i++)
        place(file, stage, &a[i], i, i, NONE, NONE);
    out->uniformBuffers = n;
    n = in_set(storageBuffers, set, a);
    for (i = 0; i < n; i++)
        place(file, stage, &a[i], textures + i, out->uniformBuffers + i, NONE, NONE);
    out->storageBuffers = n;
}

/* A compute shader's resources (the comment at the top). */
static void place_compute(const char *file, SpvExecutionModel stage, const List *sampled,
                          const List *storageTextures, const List *storageImages, const List *storageBuffers,
                          const List *uniforms, struct Compiled *out)
{
    const unsigned NONE = ~0u;
    Res a[32];
    unsigned i, n, binding, textures, buffers;
    if (not_in(storageImages, 1, 1))
        fail("%s (cs_main): a storage image outside set 1, where SDL binds a read-only storage texture as a "
             "sampled image (declare it Texture2D, without a sampler)", file);
    if (not_in(sampled, 0, 0) || not_in(storageTextures, 0, 0) || not_in(storageBuffers, 0, 1)
        || not_in(uniforms, 2, 2))
        fail("%s (cs_main): a resource outside the sets SDL binds for compute (0, 1 and 2)", file);
    n = in_set(uniforms, 2, a);
    for (i = 0; i < n; i++)
        place(file, stage, &a[i], i, i, NONE, NONE);
    out->uniformBuffers = buffers = n;
    n = in_set(sampled, 0, a);
    for (i = 0; i < n; i++)
        place(file, stage, &a[i], i, NONE, i, i);
    out->samplers = textures = binding = n;
    n = in_set(storageTextures, 0, a);
    for (i = 0; i < n; i++)
        place(file, stage, &a[i], binding + i, NONE, textures + i, NONE);
    out->storageTextures = n;
    textures += n;
    binding += n;
    n = in_set(storageBuffers, 0, a);
    for (i = 0; i < n; i++)
        place(file, stage, &a[i], binding + i, buffers + i, NONE, NONE);
    out->storageBuffers = n;
    buffers += n;
    n = in_set(storageImages, 1, a);
    for (i = 0; i < n; i++)
        place(file, stage, &a[i], i, NONE, textures + i, NONE);
    out->readwriteStorageTextures = binding = n;
    n = in_set(storageBuffers, 1, a);
    for (i = 0; i < n; i++)
        place(file, stage, &a[i], binding + i, buffers + i, NONE, NONE);
    out->readwriteStorageBuffers = n;
    for (i = 0; i < 3; i++)
        out->threads[i] = spvc_compiler_get_execution_mode_argument_by_index(g_comp, SpvExecutionModeLocalSize, i);
}

static void compile_stage(const char *dir, const char *file, const char *base, const char *entry, int kind,
                          struct Compiled *out)
{
    char spv[1024];
    spvc_context ctx;
    spvc_parsed_ir ir;
    spvc_compiler_options opt;
    spvc_resources res;
    SpvExecutionModel stage;
    List sampled, images, samplers, storageImages, storageBuffers, uniforms;
    const char *msl;
    snprintf(spv, sizeof spv, "%s/%s_%s.spv", g_work, base, g_suffix[kind]);
    if (run("\"%s\" %s -T %s -E %s -Fo \"%s\" \"%s/%s\"", g_dxc, DXC_FLAGS, g_profile[kind], entry, spv, dir, file))
        fail("dxc failed on %s (%s)", file, entry);
    if (run("\"%s\" --target-env vulkan1.0 \"%s\"", g_val, spv))
        fail("spirv-val rejects %s (%s)", file, entry);
    out->spirv = read_file(spv, &out->spirvSize);
    if (!out->spirv || out->spirvSize % 4)
        fail("cannot read %s", spv);

    spvc_context_create(&ctx);
    if (spvc_context_parse_spirv(ctx, (const SpvId *)out->spirv, out->spirvSize / 4, &ir) != SPVC_SUCCESS
        || spvc_context_create_compiler(ctx, SPVC_BACKEND_MSL, ir, SPVC_CAPTURE_MODE_TAKE_OWNERSHIP, &g_comp)
               != SPVC_SUCCESS)
        fail("%s: %s", file, spvc_context_get_last_error_string(ctx));
    spvc_compiler_create_compiler_options(g_comp, &opt);
    spvc_compiler_options_set_uint(opt, SPVC_COMPILER_OPTION_MSL_VERSION, SPVC_MAKE_MSL_VERSION(2, 1, 0));
    spvc_compiler_install_compiler_options(g_comp, opt);
    stage = spvc_compiler_get_execution_model(g_comp);

    spvc_compiler_create_shader_resources(g_comp, &res);
    collect(res, SPVC_RESOURCE_TYPE_SAMPLED_IMAGE, &sampled);
    collect(res, SPVC_RESOURCE_TYPE_SEPARATE_IMAGE, &images);
    collect(res, SPVC_RESOURCE_TYPE_SEPARATE_SAMPLERS, &samplers);
    collect(res, SPVC_RESOURCE_TYPE_STORAGE_IMAGE, &storageImages);
    collect(res, SPVC_RESOURCE_TYPE_STORAGE_BUFFER, &storageBuffers);
    collect(res, SPVC_RESOURCE_TYPE_UNIFORM_BUFFER, &uniforms);
    if (samplers.n)
        fail("%s (%s): a sampler without its texture in [[vk::combinedImageSampler]]", file, entry);
    if (kind == COMPUTE)
        place_compute(file, stage, &sampled, &images, &storageImages, &storageBuffers, &uniforms, out);
    else
        place_graphics(file, entry, stage, kind == FRAGMENT ? 2 : 0, &sampled, &images, &storageImages,
                       &storageBuffers, &uniforms, out);
    if (spvc_compiler_compile(g_comp, &msl) != SPVC_SUCCESS)
        fail("%s (%s): %s", file, entry, spvc_context_get_last_error_string(ctx));
    out->msl = malloc(strlen(msl) + 1);
    if (!out->msl)
        fail("out of memory");
    strcpy(out->msl, msl);
    spvc_context_destroy(ctx);
}

static void write_bytes(FILE *f, const char *name, const unsigned char *p, size_t n)
{
    size_t i;
    fprintf(f, "static const unsigned char %s[%zu] = {", name, n);
    for (i = 0; i < n; i++)
        fprintf(f, "%s0x%02x,", i % 16 ? " " : "\n    ", p[i]);
    fprintf(f, "\n};\n");
}

static void write_stage(FILE *f, const char *base, int kind, const struct Compiled *c)
{
    static const char *const stages[3] = {"GPU_SHADER_VERTEX", "GPU_SHADER_FRAGMENT", "GPU_SHADER_COMPUTE"};
    char spirv[160], msl[160];
    snprintf(spirv, sizeof spirv, "%s_%s_spirv", base, g_suffix[kind]);
    snprintf(msl, sizeof msl, "%s_%s_msl", base, g_suffix[kind]);
    write_bytes(f, spirv, c->spirv, c->spirvSize);
    write_bytes(f, msl, (const unsigned char *)c->msl, strlen(c->msl));
    if (kind == COMPUTE)
        fprintf(f,
                "static const struct GpuComputeCode %s_%s = {\n    {%s, sizeof %s, %s, sizeof %s, %s,\n"
                "    %u, %u, %u, %u},\n    %u, %u, {%u, %u, %u}\n};\n",
                base, g_suffix[kind], spirv, spirv, msl, msl, stages[kind], c->samplers, c->storageTextures,
                c->storageBuffers, c->uniformBuffers, c->readwriteStorageTextures, c->readwriteStorageBuffers,
                c->threads[0], c->threads[1], c->threads[2]);
    else
        fprintf(f,
                "static const struct GpuShaderCode %s_%s = {\n    %s, sizeof %s, %s, sizeof %s, %s,\n"
                "    %u, %u, %u, %u\n};\n",
                base, g_suffix[kind], spirv, spirv, msl, msl, stages[kind], c->samplers, c->storageTextures,
                c->storageBuffers, c->uniformBuffers);
}

static void tool_versions(void)
{
    char path[1024];
    FILE *f;
    size_t n;
    snprintf(path, sizeof path, "%s/dxc_version.txt", g_work);
    if (run("\"%s\" --version > \"%s\"", g_dxc, path))
        fail("cannot run %s", g_dxc);
    f = fopen(path, "rb");
    if (!f)
        fail("cannot read %s", path);
    n = fread(g_dxcVersion, 1, sizeof g_dxcVersion - 1, f);
    fclose(f);
    g_dxcVersion[n] = 0;
    while (n && (g_dxcVersion[n - 1] == '\n' || g_dxcVersion[n - 1] == '\r'))
        g_dxcVersion[--n] = 0;
}

static void generate(const char *source)
{
    char dir[1024], file[256], base[128], out[1200], tools[512], seen[32][1024], hex[17];
    const char *slash = strrchr(source, '/'), *back = strrchr(source, '\\'), *dot;
    unsigned char *text, *body, *mark;
    size_t size;
    uint64_t h;
    int nseen = 0, vs, fs, cs;
    unsigned major, minor, patch;
    struct Compiled vsc = {0}, fsc = {0}, csc = {0};
    FILE *f;
    if (back > slash)
        slash = back;
    if (slash) {
        snprintf(dir, sizeof dir, "%.*s", (int)(slash - source), source);
        snprintf(file, sizeof file, "%s", slash + 1);
    } else {
        snprintf(dir, sizeof dir, ".");
        snprintf(file, sizeof file, "%s", source);
    }
    dot = strrchr(file, '.');
    if (!dot || strcmp(dot, ".hlsl"))
        fail("%s is not a .hlsl file", source);
    snprintf(base, sizeof base, "%.*s", (int)(dot - file), file);
    text = read_file(source, &size);
    if (!text)
        fail("cannot read %s", source);
    vs = strstr((const char *)text, " vs_main(") != NULL;
    fs = strstr((const char *)text, " fs_main(") != NULL;
    cs = strstr((const char *)text, " cs_main(") != NULL;
    free(text);
    if (!vs && !fs && !cs)
        fail("%s has none of vs_main, fs_main and cs_main", source);
    if (cs && (vs || fs))
        fail("%s: a compute shader's file holds it alone", source);
    h = hash_source(dir, file, 0xcbf29ce484222325ull, seen, &nseen);
    spvc_get_version(&major, &minor, &patch);
    snprintf(tools, sizeof tools, "dxc %s; SPIRV-Cross C API %u.%u.%u (Vulkan SDK %s)", g_dxcVersion, major, minor,
             patch, g_sdk);
    if (vs)
        compile_stage(dir, file, base, "vs_main", VERTEX, &vsc);
    if (fs)
        compile_stage(dir, file, base, "fs_main", FRAGMENT, &fsc);
    if (cs)
        compile_stage(dir, file, base, "cs_main", COMPUTE, &csc);

    snprintf(out, sizeof out, "%s/%s.h", dir, base);
    f = fopen(out, "wb");
    if (!f)
        fail("cannot write %s", out);
    fprintf(f, "/* Generated from %s by tools/shaders (tools/shaders/build.bat); do not edit.\n", file);
    fprintf(f, "   source %016llx\n", (unsigned long long)h);
    fprintf(f, "   tool %016llx\n", (unsigned long long)g_toolHash);
    fprintf(f, "   tools: %s\n", tools);
    fprintf(f, "   flags: %s; %s\n", DXC_FLAGS, MSL_FLAGS);
    fprintf(f, "   output %016llx */\n", 0ull);
    if (vs)
        write_stage(f, base, VERTEX, &vsc);
    if (fs)
        write_stage(f, base, FRAGMENT, &fsc);
    if (cs)
        write_stage(f, base, COMPUTE, &csc);
    if (fclose(f))
        fail("cannot write %s", out);
    /* the output's hash, over what follows the comment, in place of the zeros */
    text = read_file(out, &size);
    if (!text)
        fail("cannot read %s", out);
    body = (unsigned char *)strstr((const char *)text, "*/\n");
    mark = (unsigned char *)strstr((const char *)text, "   output ");
    if (!body || !mark || mark > body)
        fail("%s: no output line", out);
    body += 3;
    snprintf(hex, sizeof hex, "%016llx", (unsigned long long)fnv(0xcbf29ce484222325ull, body, size - (size_t)(body - text)));
    memcpy(mark + 10, hex, 16);
    f = fopen(out, "wb");
    if (!f || fwrite(text, 1, size, f) != size || fclose(f))
        fail("cannot write %s", out);
    free(text);
    printf("%s: %s%s%s%s\n", out, vs ? "vertex" : "", vs && fs ? " and " : "", fs ? "fragment" : "", cs ? "compute" : "");
    free(vsc.spirv);
    free(vsc.msl);
    free(fsc.spirv);
    free(fsc.msl);
    free(csc.spirv);
    free(csc.msl);
}

int main(int argc, char **argv)
{
    int i;
    unsigned char *tool;
    size_t size;
    if (argc < 7) {
        fprintf(stderr, "usage: shaders <dxc> <spirv-val> <tools/shaders/shaders.c> <work directory> <sdk version> "
                        "<file.hlsl>...\n");
        return 2;
    }
    g_dxc = argv[1];
    g_val = argv[2];
    tool = read_file(argv[3], &size);
    if (!tool)
        fail("cannot read %s", argv[3]);
    g_toolHash = fnv(0xcbf29ce484222325ull, tool, size);
    free(tool);
    g_work = argv[4];
    g_sdk = argv[5];
    tool_versions();
    for (i = 6; i < argc; i++)
        generate(argv[i]);
    return 0;
}

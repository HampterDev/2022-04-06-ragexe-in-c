# One renderer on SDL's GPU API

Status: phases 0 and 1 are done, and so is the window that resizes and goes fullscreen at native
resolution. Phases 2 and 3 are done on Windows. On Linux their unit tests and scene harness have run
in WSL with Mesa's lavapipe, but no one has played on it; the macOS and Android clients have not
been built or run on it, and no Metal compiler has seen its shaders. Phases 4 to 7 are to do. File
references drift as the code changes. Sizes are judgement, not measurement.

## Goal

- One renderer on every platform, on SDL 3's GPU API (Vulkan on Windows, Linux and Android, Metal on
  macOS), with one SDL window on every platform. The Direct3D 9 and GL backends
  (`render/gfx_d3d9.c`, `render/gfx_gl.c`) are deleted. There is no backend switch, no fallback
  renderer, and no capability check that picks between two ways of drawing the same thing.
- The CPU talks to the GPU a few times a frame instead of once per draw: static geometry and textures
  stay on the GPU, the frame's changing data goes up in one upload, draws come in few, large batches,
  and nothing is read back while playing. The GPU culls the world and fits the shadows itself.
- The picture stays as the Direct3D 9 build draws it, shadows included. Linux and macOS gain the
  shadows and the anti-aliasing their GL backend does not have.
- The builds: `build.bat` for 64-bit Windows, and the CMake builds for Linux, macOS and Android.

## Ground rules

- **No fallbacks, no second path.** Each phase ends with exactly one way to draw each thing, and the
  code it replaces is deleted in the same phase.
- **A reference client, not a reference code path.** `build/ref/ragexe.exe` and
  `build/ref/ragexe_scenes.exe` are built from the last commit that draws with Direct3D 9
  (`build/ref/COMMIT.txt`) and kept outside the tree. Every phase compares scene captures against
  that binary's. It is never linked, switched to or shipped.
- **Every commit builds and passes** `build.bat check` and `tests\build.bat` (`AGENTS.md`). Each phase
  ends with the scene set, the hill scenes and the gate scene captured against the reference, and
  with the frame counters recorded.
- **custom only.** This is a deliberate change: each phase adds its line to `README.md` and its
  entry to `docs/custom-review.md`. Render and window fixes `main` makes to the Direct3D 9 and Win32
  code have nowhere to land here; `docs/upstream/README.md` records them as not needed, with the
  reason.

## Where the renderer stands

- **One backend behind `render/gfx.h`:** `render/gfx_gpu.c` on SDL's GPU API, Vulkan or Metal, on
  every platform (phase 2). It records a frame and submits it in one command buffer with one upload,
  through the shaders of `src/render/shaders` that `tools/shaders` compiles offline.
- **Draw modes, not render states** (phase 3). `render/draw_modes.c` names each way the client
  draws: blending, depth test and write, alpha test, fog, the texture stages' combination and their
  sampling, as the Direct3D 9 client set them for each list. The device makes every mode's
  pipelines when it starts; the effect lists' quads carry their own D3DBLEND factors and
  texture-stage preset, and their modes have a pipeline for each pair of the screen's blend
  factors. One quad shader runs the stage combinations, chosen by the draw's record. Fog is
  computed in the vertex shaders from view depth. Pre-transformed vertices (`GFX_VTX_TEX1/TEX2`,
  xyz-rhw) carry most of what is drawn.
- **Already static on the GPU:** the ground (one mesh per 32 by 32 cells, `world/ground_draw.c`) and
  the RSM models (one mesh per node, `world/model_node_render.c`). Per frame each visible block or
  node still queues one draw per (texture, lightmap) or (texture, two-sided) run, each a draw of
  its own with its own record.
- **Built on the CPU every frame:** actor sprites (projected on the CPU, `actor/sprite_quad.c`),
  Granny actors (skinned, lit and projected per vertex, `actor/granny.c`), water (heights animated
  per tick, corners projected), particles, STR effects, effect primitives, damage numbers and every
  2D quad. The shadow cascades are fitted on the CPU to per-leaf boxes of the ground and models
  (`renderer_shadow_frame`, `view_shadow_boxes`), and the CPU picks the casters
  (`view_cast_shadows`).
- **How it reaches the device:** 19 queues (`render/draw_queue.h`), flushed in a fixed order by
  `renderer_draw_lists`, each in its draw mode (`render/crenderer.c`). Non-mesh draws go through
  `render/gfx_batch.c` (8,192 vertices), flushed on any texture change or change of what they are
  drawn with into a draw of the recorded frame.
- **GPU-to-CPU traffic:** the screen wobble locks the back buffer every frame it runs
  (`render/cframe.c`); UI windows that composite render to a target and copy it back
  (`render/render_clients.c`); a startup probe locks a target (`render/c3ddevice.c`); screenshots
  and scene captures lock the target. Each lock submits the frame so far and waits for the target's
  download.
- **Textures:** 16-bit 4444/1555 converted on the CPU (`render/csurface.c`) and widened to
  B8G8R8A8 when they go up, magenta keyed to alpha 0, mips made on the GPU; sprite atlas pages
  decoded from palette RLE on the CPU (`render/rp_atlas.c`); UI canvases re-uploaded as 256-pixel
  tiles when they change. One texture per surface; no texture arrays yet.
- **Measured:** each scene capture writes its frame's counters (`tests/scenes/README.md`).
  `prontera_crowd` makes 1,174 draws (1,954 calls) and `camera_level_view` 3,345, as with Direct3D
  9; `docs/custom-review.md` has the baseline and phase 2's CPU times.

## The floor

The client uses two of SDL's drivers. SDL 3.4's `SDL_gpu.h` states what they need:

- **Vulkan** (Windows, Linux, Android): 1.0 with `VK_KHR_swapchain`, `VK_KHR_maintenance1` and a few
  features (`independentBlend`, `imageCubeArray`, `sampleRateShading`, `samplerAnisotropy`); the
  client turns off `depthClamp`, `shaderClipDistance` and `drawIndirectFirstInstance`, which SDL asks
  for unless told otherwise, since it uses none of them. On
  Windows that means NVIDIA and AMD cards of the last decade and Intel graphics from Skylake (2015);
  Intel's Haswell and Broadwell have no Vulkan driver on Windows.
- **Metal** (macOS): macOS 10.14 or newer, Apple Silicon or an Intel "Mac2 family" GPU.

Steam's October 2026 survey puts 90.6% of systems on DirectX 12-class GPUs and cards at DirectX 11
and below at 0.6% (it does not report Vulkan); Haswell's HD 4600 is 0.16%. Android needs Vulkan.
Nothing below this floor is supported by a second path.

## Target design

### Device, window, present

- One SDL window on every platform (`core/app_sdl.c`, `platform/window_sdl.c`), with SDL's input,
  text input and IME.
- One `SDL_GPUDevice`, created with the one shader format of the platform's driver (SPIR-V for
  Vulkan, MSL for Metal), so SDL has exactly one driver to choose. SDL prefers the high-performance
  GPU; there is no choice of card in the client.
- The game draws at the window's size in pixels, whatever it is: the window can be resized and
  maximised, and fullscreen is a borderless window at the native resolution of the display it is
  on. No display-mode switches and no device loss and reset.
- Anti-aliasing: a multisampled colour and depth target resolved at the end of the 3D pass;
  transparency anti-aliasing through the pipeline's alpha-to-coverage instead of the driver FOURCC
  hacks of the Direct3D 9 backend.

### Shaders

- HLSL sources under `src/render/shaders/`, one file per program, compiled offline with the Vulkan
  SDK's `dxc` (to SPIR-V for Vulkan 1.0, the version SDL's Vulkan driver creates) and `spirv-cross`
  (SPIR-V to MSL). A small tool turns the results into C arrays committed beside the sources, each
  stamped with a hash of its source, the tools' versions and their flags; `build.bat check` fails
  when a source changed and its arrays did not. Builds never need a shader compiler, and
  `d3dcompiler_47.dll` is no longer loaded. The chain has been tried on a vertex, a fragment and a
  compute shader with `dxc` 1.8.0.5123:
  - HLSL registers that follow SDL's rules land in the descriptor sets SDL's Vulkan driver expects
    without `vk::binding`; a sampled texture and its sampler take `[[vk::combinedImageSampler]]`,
    one descriptor, as SDL binds them. The output passes `spirv-val` and is byte-identical from run
    to run, with no paths in it.
  - `spirv-cross`'s command line cannot give every resource the Metal index SDL's rules ask for, so
    the tool sets them through SPIRV-Cross's C API (`spvc_compiler_msl_add_resource_binding_2`) and
    reports the resource counts `SDL_GPUShaderCreateInfo` needs. Entry points are `main` in SPIR-V
    and `main0` in MSL.
  - The SPIR-V runs in every Windows and Linux test and capture; the MSL has not met a Metal
    compiler.
- Per-draw data comes through an instance-rate vertex attribute holding the draw's index, never
  through `SV_InstanceID`: SDL warns that `first_instance` does not reach built-in IDs the same way
  on every API (`SDL_GPUIndirectDrawCommand` in `SDL_gpu.h`).

### Pipelines instead of render states

- The fixed-function state machine goes: no `GFX_RS_*`, no `GFX_TSS_*`, no `HRS`/`HTSS`. Each of
  the 19 queues stands for one combination of blend, depth, cull and alpha test; each becomes one
  pipeline (a few queues need two, such as two-pass alpha), created once at start. Texture stage
  combinations become shader code: the ground's lightmap stage and the multitexture particles' 22
  stage states. The three effect queues take each quad's blending and texture-stage preset from
  its effect file, as Direct3D 9's states did, so they have a pipeline for each pair of blend
  factors and the preset is chosen in the shader.
- Fog is computed in the shaders from view depth for everything, as the mesh shaders already do.

### Memory and uploads

- **Textures** are RGBA8 everywhere. The 16-bit look is kept by quantising to 4444 or 1555 on the
  CPU at load before widening. The magenta key stays at upload. Mips are generated on the GPU
  (`SDL_GenerateMipmapsForGPUTexture`). No texture is locked or read back by the CPU; images the CPU
  reads (`render/cdcbitmap.c`) are read from their CPU copy.
- **One upload per frame.** Everything the CPU writes for a frame (dynamic vertices, instance data,
  per-draw constants, changed texture tiles) goes into one transfer buffer that cycles, copied in
  one copy pass at the start of the frame's command buffer.
- **Only the main thread talks to the GPU.** The model loader thread (`world/models.c`) creates and
  uploads textures today (`texture_cache_get` to `gfx_create_texture` and `texture_upload_pixels`),
  which Direct3D 9's `MULTITHREADED` device serialises. SDL documents no thread safety for creating
  resources and filling transfer buffers, so the loader only prepares pixels on the CPU, as the GL
  backend does, and the main thread places them in their texture array layer in the frame's upload.
- **Static data is resident.** A map's ground and model meshes share one vertex and one index
  buffer, filled when the map loads and as streamed models arrive. Map textures and lightmap pages
  live in texture arrays by size class, so a draw picks its texture by layer instead of by binding.

### The static world, GPU-driven

- At load, every (block or node, texture run) of the ground and the models becomes a draw record in
  a storage buffer: its index range, texture layer, lightmap layer, flags, bounds and transform
  index.
- Each frame a compute pass culls the records against the view and against each shadow cascade and
  writes indirect draw commands; the passes draw them with one indirect call per pipeline. SDL has
  no draw whose count the GPU writes (`draw_count` is the CPU's), so the pass writes one command per
  record and culls a record by giving it no instances. The CPU no longer walks the scene graph's
  leaves to queue mesh draws.
- Animated model nodes write their matrices into a per-frame transform buffer; static nodes'
  matrices stay put.
- Water becomes a static mesh per map whose waves the vertex shader animates from the time.

### Shadows, fitted on the GPU

The look stays: four 2048 by 2048 cascades along the map's light, the same soft edges, bias, slant
and offset, the ground's baked shadows replaced where the maps reach, and the shadows' reach of four
times the camera's distance. What changes is who fits them. A CPU fit can only fit to the geometry
it is given; the GPU fits to what the frame shows (sample distribution shadow maps):

1. A depth-only pass draws the receivers, the ground and the models, from the culled draw records.
   The main pass then reuses that depth, so it shades each pixel once.
2. A compute pass reduces that depth to the nearest and farthest depth on screen, splits the range
   into four slices, and reduces each slice's pixels to their extent across the light.
3. A compute pass turns those into the four cascade matrices, rounding each square up and snapping
   its centre to whole texels so edges do not crawl, and takes the light's depth range from the
   map's bounds. The matrices go into a buffer that the cull pass, the shadow pass and the
   receiving shaders read; the CPU never sees them.
4. The cull pass picks each cascade's casters from the draw records; the shadow pass draws them
   into the four layers of one depth-array texture.

Because the fit comes from the pixels themselves, no visible receiver can fall outside its cascade,
and the squares are as tight as the screen allows, so texels are as small as today's or smaller.
Deleted then: `renderer_shadow_frame`'s fit, `ShadowBox`, `view_shadow_boxes`, the leaves'
`groundMin` and `groundMax`, `view_cast_shadows`, `ShadowReach` and `renderer_shadow_reaches`, with
the tests that cover them.

### Sprites, actors and effects

- Actor sprites, damage numbers, item and screen sprites, particles and STR effect quads become
  instances: a world anchor (or a screen position), size, offsets, atlas layer and rectangle, colour
  and depth parameters. The vertex shader projects them, carrying over the sprite depth rules now in
  `actor/sprite_quad.c` and `render/rp_project.c` (one perspective per billboard, the linear row
  depth, the feet lift). Sprite atlas pages become one texture array.
- Granny actors are skinned in the vertex shader from bone matrices in the frame's upload.
- What the CPU still builds per frame is a list of instances and a few matrices, uploaded once.

### 2D, UI and text

- UI canvases stay CPU-drawn and are uploaded as changed tiles in the frame's upload. Windows that
  composite render into a target texture and draw that texture; nothing is copied back.
- Text never touches the frame: it draws into textures like other UI text, with FreeType on every
  platform.
- The screen wobble becomes a pass that samples the scene's colour texture with the wobble's
  offsets.

### No read-backs

Screenshots and scene captures download the finished frame (`SDL_DownloadFromGPUTexture`) when
asked; the scene harness waits for that download, and renders without a swapchain, so captures need
no visible window at all. Nothing else reads from the GPU: the lockable back buffer, the copy swap,
`renderer_lock_target`, `targetBits`, the startup texel probe and every `gfx_lock` on a target are
deleted.

### A frame

1. Copy pass: the frame's upload.
2. Compute: cull the draw records for the view.
3. Depth pass: the receivers.
4. Compute: fit the cascades from that depth, then cull the draw records for each cascade.
5. Shadow pass: four layers, indirect draws of the casters.
6. Main pass (multisampled, on the depth pass's depth): indirect draws of the ground and models,
   then water, then the instanced sprites and effects in the queues' order, resolved.
7. The wobble pass when it runs, then the 2D pass onto the swapchain, then present.

## Phases

Each phase ends deleting what it replaced, with captures against the reference and the counters
recorded.

### Phase 0: groundwork (done)

- **Counters** (`6b176290`): each scene capture writes `<name>.counters.json` with its frame's
  draws, draw calls, primitives, states sent and skipped, texture binds, vertex and texture bytes
  uploaded and the CPU time in `renderer_draw_scene` and `view_on_render`.
- **One 64-bit Windows client** (`02fc9e5e`), with the C runtime linked statically. Programs that
  load 32-bit code of the original stay 32-bit: the oracle tests and `tools/granny_export`; the
  CMake `-A Win32` configuration still builds the Lua fixture with 4-byte sizes. Granny models come
  from the glTF files `tools\granny_export` writes into the game folder.
- **SDL** (`02fc9e5e`): built with video and the GPU API with Vulkan, and without OpenGL. SDL builds
  its Direct3D 12 driver whenever the Windows SDK has `dxgi1_6.h` and has no switch for it; it is
  linked but never created.
- **The reference** (`build/ref`, from `02fc9e5e`): the 12 captures of the scene set with their
  counters, and 16 shadow views (Prontera's south gate, the far steep view, and the hilliest spots of
  `manuk`, `hu_fild01`, `ve_fild05` and `man_fild03` from three cameras), all with `RAGEXE_MSAA=1`.
- **The test suite** (`c7f55cdb`): one definition of every test in `tests/CMakeLists.txt`, built
  with Ninja and run with CTest on every core, headless and silent; a full run takes about 20
  seconds when little changed.

### Phase 1: Windows on the SDL window (done)

- `094cf898`: the Windows client runs on `core/app_sdl.c` and `platform/window_sdl.c` like the
  others, with Direct3D 9 drawing into the SDL window's `HWND`. The Win32 window and message loop
  are gone. Captures match the reference exactly.
- Still to be tried by hand: Hangul composition and the letter hotkeys in Hangul mode, AltGr on a
  German or French layout.

### The window: resizable, fullscreen at native resolution (done)

- Fullscreen is a borderless window at the native resolution of the display it is on, in pixels,
  on every platform; exclusive fullscreen, display-mode switches, adapter and mode enumeration and
  the choice of graphics card are gone. The resolution picker is disabled and shows the native size
  in fullscreen, and offers the sizes that fit the display in windowed mode.
- The window can be dragged to any size from 1024 by 664 and maximised; the game draws at its size
  in pixels on scaled displays too, and the last size and maximised state are kept.
- Still to be tried by hand: fullscreen on a second display and on one with a different scale,
  Alt+Tab, a change of desktop resolution while running, and maximise and restore.

### Phase 2: one backend behind today's API (large)

- `render/gfx_gpu.c` implements `render/gfx.h` as it stands on the GPU API: a pipeline cache keyed
  by the current render states, one shader that does what the GL backend's fixed-function shader
  does, the Direct3D 9 mesh and shadow shaders ported, draws recorded into a frame list and
  replayed after one upload, targets as render passes, captures and the remaining target locks
  served by downloads, and the shader tool with its hash check in `build.bat check`.
- Delete `gfx_d3d9.c`, `gfx_gl.c`, `test_gfx_gl`, the d3dcompiler loading, `d3d9.lib`, the GL
  attributes in `app_sdl.c` and `gfx_gl_set_cover`, and the bump-map texture path, which nothing
  calls (`texture_create_bump`, `bump_caps_check`, `GFX_FMT_BUMP` and the backends'
  bump-environment stage ops). Update the tests that stub `gfx_*`.
- Done when the Windows, Linux and macOS clients play on it and captures match the reference within
  3 per channel with `RAGEXE_MSAA=1`, apart from changes the review entry explains. Linux and macOS
  gain shadows and anti-aliasing here.

### Phase 3: pipelines and a modern API (done)

- The render-state and stage-state machine is replaced by draw modes (`render/draw_modes.c`): 28
  modes, 21 for the lists (the ground with and without its lightmap, both halves of two-pass alpha)
  and one each for the interface's composites, the frame-time overlay, the texel probe, blits and
  three kinds of clear, with their pipelines made once, when the screen's samples and depth are
  known. `gfx.h` lost `GFX_RS_*`,
  `GFX_TSS_*`, `GFX_TOP_*`, `GFX_TA_*`, the comparison, address and filter numbers,
  `gfx_set_render_state`, `gfx_get_render_state`, `gfx_set_stage_state` and `gfx_default_states`;
  `render/crenderer.c` its `HRS` and `HTSS` and `renderer_set_blend_stages`.
  The effect presets are combinations in the quad shader, chosen per quad, and match Direct3D 9's
  stages within a step (`test_gfx_gpu`).
- Fog is computed in the vertex shaders, the quads' to the bit as the CPU computed it, through a
  table of the rhw at which each step begins (`render/vertex_fog.c`); the per-vertex fog alpha on
  the CPU and the vertices' specular colour are gone.
- FreeType draws text on every platform: `render/text_gdi.c`, `render/fonts.c`, the GDI+ helper
  library they needed and the `freetype` build switch are gone. On Windows the font finder reads
  the fonts the registry lists and chooses, sizes and hints each face as GDI did, which
  `test_font_system` compares with GDI on the machine's own fonts.
- With the modes and the fog the scene set, `window_resize`, the hill scenes and the gate scene
  captured the same pixels as phase 2, with the same draws. With FreeType's text the scene set is
  within a step of the Direct3D 9 client built with FreeType, and its text differs from GDI's in
  3 to 16 pixels a scene; the hill and gate views differ from phase 2's in their text alone, and
  five of them from the reference along shadow edges, as phase 2's did (`docs/custom-review.md`).

### Phase 4: no read-backs (small to medium)

- The wobble pass, compositing into target textures, captures by download without a swapchain.
- Delete `gfx_lock` on targets, `renderer_lock_target`, `targetBits`/`targetPitch`, the startup
  texel probe and the lockable back buffer.

### Phase 5: the static world and its shadows on the GPU (large)

- Shared map buffers, texture arrays, draw records, the compute cull for the view, the depth pass,
  the cascade fit and the cascade culls in compute, indirect draws, the transform buffer, water as a
  mesh.
- Delete the per-leaf mesh queuing in `view_on_render`, the CPU shadow fit and caster choice (the
  list under *Shadows, fitted on the GPU*), the `GROUND_MESH` and `SHADOW_CASTERS` queues, and the
  per-tick CPU water heights.
- Done when the scene set, the hill scenes and the gate scene show the same shadows as the
  reference, edge for edge where the texels allow, and the flat and steep far views from 2,500 to
  10,000 keep theirs.

### Phase 6: sprites, actors and effects as instances (large)

- Instanced sprites, damage numbers, particles and STR quads projected in the vertex shader; the
  sprite atlas as a texture array; Granny skinning in the vertex shader.
- Delete the CPU projection of these (`sprite_quad.c` corners, `renderer_project_point` for
  anchors, Granny's per-vertex projection and lighting), `gfx_batch.c` and the pre-transformed
  vertex kinds.

### Phase 7: measure and settle (small)

- Record the counters again on the same scenes and write the result into the review entry. Remove
  what the phases left unused.

## Decisions

- **One SDL window on every platform**, so every platform shares one window, input and IME path.
- **No 32-bit client.** Only programs that load 32-bit code of the original are 32-bit.
- **Vulkan on Windows, Linux and Android; Metal on macOS.** One driver path for three platforms and
  two shader formats (SPIR-V and MSL). Switching Windows to Direct3D 12 later would cost a shader
  format and a device flag, not renderer code.
- **Shaders are compiled offline and committed** with a hash check, so no build needs a shader
  compiler (the Android cross build cannot run host tools it builds).
- **Fullscreen is borderless at native resolution**, with no display-mode switches and no choice of
  graphics card in the client (Windows' per-app graphics settings choose the card).
- **The window resizes and maximises**, and the game draws at its size in pixels.
- **Shadows are fitted on the GPU** from what the frame shows, with today's look.
- **Text is FreeType's on every platform.**
- **Android needs Vulkan.**

## Risks

- **SDL's GPU API is young.** Driver-specific bugs surface on old integrated GPUs first. The scene
  harness only runs where there is a GPU, and CI has none (scene captures need game data and stay
  off CI anyway).
- **Matching the reference.** Pre-transformed vertices must keep Direct3D 9's pixel centres and
  top-left fill rule, as the GL backend does (`docs/portability.md`); GPU mips and RGBA8 sampling of
  quantised texels differ from CPU mips by small amounts. Each phase's review entry accounts for the
  pixels that change.
- **The GPU shadow fit moves when the picture does.** It follows what is on screen, so a character
  walking close past the camera can change a cascade's square from one frame to the next; the
  rounding and texel snapping must hold the edges still, and phase 5 checks it with the moving
  captures.
- **Threads.** The loader thread's texture creation moves to the main thread's upload (see *Memory
  and uploads*); SDL's drivers lock creation internally, but the API does not promise it, so the
  plan does not lean on it.
- **Upstream picks.** Reconstruction fixes to `main`'s Win32 and Direct3D 9 code no longer apply
  here; each is recorded as not needed, and fixes above `gfx.h` and the window layer still pick as
  before.
- **Tests that stub `gfx_*`.** About 14 of them change with each API change in phases 2 to 4.

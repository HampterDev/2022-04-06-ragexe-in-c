# One renderer on SDL's GPU API

Status: plan, written 2026-11-11 on `gpu-world` at `0dcd5df8`; decisions taken with Patrick the same
day (see the end). Nothing of it is implemented. File and line references are from that commit and
will drift. Sizes are judgement, not measurement.

## Goal

- One renderer on every platform, on SDL 3's GPU API (Vulkan on Windows, Linux and Android, Metal on
  macOS), with one SDL window on every platform. The
  Direct3D 9 and GL backends (`render/gfx_d3d9.c`, `render/gfx_gl.c`) and the Win32 window
  (`core/app_win32.c`, `core/wndproc.c`) are deleted. There is no backend switch, no fallback
  renderer, and no capability check that picks between two ways of drawing the same thing.
- The CPU talks to the GPU a few times a frame instead of once per draw: static geometry and textures
  stay on the GPU, the frame's changing data goes up in one upload, draws come in few, large batches,
  and nothing is read back while playing. The GPU culls the world and fits the shadows itself.
- The picture stays as the Direct3D 9 build draws it today, shadows included (their look is settled).
  Linux and macOS gain the shadows and the anti-aliasing their GL backend never had.
- The builds: `build.bat` for 64-bit Windows only, and the CMake builds for Linux, macOS and
  Android. The 32-bit Windows client goes.

## Ground rules

- **No fallbacks, no second path.** Each phase ends with exactly one way to draw each thing, and the
  code it replaces is deleted in the same phase. Nothing is kept "in case".
- **A reference client, not a reference code path.** Before the renderer changes, build `ragexe.exe`
  and `ragexe_scenes.exe` (x64) from the last commit that draws with Direct3D 9 and keep them outside
  the tree (`build/ref/`). Every phase compares scene captures against captures from that binary. It
  is never linked, switched to or shipped.
- **Every commit builds and passes** `build.bat check` and `tests\build.bat` (AGENTS.md). Each phase
  ends with the scene set, the hill scenes and the gate scene captured against the reference, and
  with the frame counters below recorded.
- **custom only.** This is a deliberate change: each phase adds its line to `README.md` and its
  entry to `docs/custom-review.md`. Render and window fixes that `main` makes to the Direct3D 9 and
  Win32 code later have nowhere to land here; `docs/upstream/README.md` records them as not needed,
  with the reason.

## Where the renderer is today

Facts from the code at `0dcd5df8`.

- **Two backends behind `render/gfx.h`** (342 lines): Direct3D 9 (`gfx_d3d9.c`, 2,847 lines) on
  Windows, GL 3.3 / GLES 3.0 (`gfx_gl.c`, 2,119 lines) elsewhere, chosen at build time
  (`gfx_gl.c:1` is `#ifndef _WIN32`; CMake drops `gfx_d3d9.c` through `tools/posix_excluded.txt`).
  No code outside them calls Direct3D 9 or GL.
- **The API is Direct3D 9's fixed function.** `GFX_RS_*` and `GFX_TSS_*` carry Direct3D 9's own
  numbers (`gfx_d3d9.c:17-31` asserts it); GL emulates texture stages, alpha test and fog in one
  fragment shader (`gfx_gl.c:196-333`). Pre-transformed vertices (`GFX_VTX_TEX1/TEX2`, xyz-rhw) carry
  most of what is drawn. DirectDraw and GDI leftovers remain: `SurfaceDesc`, `gfx_ddraw_enumerate`,
  `gfx_flip_to_gdi`, `gfx_get_dc`, `gfx_is_lost`, device loss and reset (`gfx_d3d9.c:1134-1439`).
- **Already static on the GPU:** the ground (one mesh per 32 by 32 cells, `ground_draw.c`) and the
  RSM models (one mesh per node, `model_node_render.c`). Per frame each visible block or node still
  queues one draw per (texture, lightmap) or (texture, two-sided) run, and on Direct3D 9 each mesh
  draw uploads about 13 vertex and 7 pixel shader constants and gets, sets and restores 20 sampler
  states (`gfx_d3d9.c:2620-2741`).
- **Built on the CPU every frame:** actor sprites (projected on the CPU, `sprite_quad.c:209-275`),
  Granny actors (skinned, lit and projected per vertex, `granny.c:335-353`), water (heights animated
  per tick, corners projected), particles, STR effects, effect primitives, damage numbers, and every
  2D quad. Fog alpha is computed per queued vertex (`rp_pool.c:363-369`). The shadow cascades are
  fitted on the CPU to per-leaf boxes of the ground and models (`renderer_shadow_frame`,
  `view_shadow_boxes`), and the CPU picks the casters (`view_cast_shadows`).
- **How it reaches the device:** 19 queues (`draw_queue.h:8-29`), flushed in a fixed order by
  `renderer_draw_lists` with 78 render-state and 127 stage-state calls in `crenderer.c`. Non-mesh
  draws go through `gfx_batch.c` (8,192 vertices), flushed on any texture or state change: on
  Direct3D 9 as `DrawIndexedPrimitiveUP`, on GL as `glBufferData` per flush.
- **GPU-to-CPU traffic:** the screen wobble locks the back buffer every frame it runs
  (`cframe.c:32-65`); UI windows that composite render to a target and copy it back
  (`render_clients.c:29-78`); the login serial text draws with GDI on the back buffer
  (`cframe.c:86-96`); FreeType text locks the target (`text_freetype.c:151`); a startup probe locks a
  target (`c3ddevice.c:365-445`); screenshots and scene captures lock the target. The back buffer is
  created lockable with a copy swap (`gfx_d3d9.c:879-883`).
- **Textures:** 16-bit 4444/1555 converted on the CPU, magenta keyed to alpha 0 at upload, mips
  built on the CPU (`csurface.c:395-523`); sprite atlas pages decoded from palette RLE on the CPU
  (`rp_atlas.c`); UI canvases re-uploaded as 256-pixel tiles when they change.
- **Windows and builds:** Windows has its own Win32 window and message loop (`core/app_win32.c`,
  `core/wndproc.c`); Linux, macOS and Android use SDL 3 (`core/app_sdl.c`). The x86 `build.bat` links
  `granny2.lib`, `mss32.lib` and `cps.lib` from `lib/` and no SDL; x64 links an audio-only SDL built
  with `-DSDL_VIDEO=OFF -DSDL_GPU=OFF` (`vendor/build_vendor.bat:26-31, 67-70`). Shaders are source
  strings compiled at run time (`D3DCompile` from `d3dcompiler_47.dll`, GLSL on GL).
- **Tests:** the whole `tests\build.bat` suite builds 32-bit (`tools/find_vcvars.bat` picks
  `vcvars32.bat`). The oracle tests (`tests/oracle/`) load the original 32-bit executable into their
  own process, so they can only ever be 32-bit; they do not touch rendering. `test_gfx_gl` (CMake)
  drives the GL backend; about 14 tests stub `gfx_*` with today's signatures; the scene harness
  captures by locking the target (`scene_driver.c:480-519`).
- **No measured draw counts exist.** `g_perf` counts draws, states, texture sets and locks
  (`diag/perf.h:35-60`) but only shows them with `RAGEXE_OVERLAY=1`.

## The floor

The client uses two of SDL's drivers. SDL 3.4's `SDL_gpu.h` states what they need:

- **Vulkan** (Windows, Linux, Android): 1.0 with `VK_KHR_swapchain`, `VK_KHR_maintenance1` and a few
  features (`independentBlend`, `imageCubeArray`, `depthClamp`, `shaderClipDistance`,
  `drawIndirectFirstInstance`, `sampleRateShading`), some of which can be dropped for Android. On
  Windows that means NVIDIA and AMD cards of the last decade and Intel graphics from Skylake (2015);
  Intel's Haswell and Broadwell have no Vulkan driver on Windows.
- **Metal** (macOS): macOS 10.14 or newer, Apple Silicon or an Intel "Mac2 family" GPU.

Steam's October 2026 survey does not report Vulkan; it puts 90.6% of systems on DirectX 12-class
GPUs and cards at DirectX 11 and below at 0.6%. Haswell's HD 4600 is 0.16%, Windows 7 0.06%.
Android loses its GLES 3.0 path and needs Vulkan. This floor is the decision; nothing below it is
supported by a second path.

## Target design

### Device, window, present

- One SDL window on every platform (`core/app_sdl.c`, `platform/window_sdl.c`), with SDL's input,
  text input and IME; the Win32 window and message loop are deleted.
- One `SDL_GPUDevice`, created with the one shader format of the platform's driver (SPIR-V for
  Vulkan, MSL for Metal), so SDL has exactly one driver to choose.
- Fullscreen is a borderless window at the desktop's size; the game's resolution is drawn
  offscreen and scaled to it (the GL backend's `present_fit` letterboxing already does this). No
  exclusive mode switches, no adapter enumeration, no device loss and reset.
- Anti-aliasing: a multisampled colour and depth target resolved at the end of the 3D pass;
  transparency anti-aliasing through the pipeline's alpha-to-coverage instead of the driver
  FOURCC hacks (`gfx_d3d9.c:607-657`).

### Shaders

- HLSL sources under `src/render/shaders/`, one file per program, compiled offline with
  SDL_shadercross (DXC and SPIRV-Cross inside) to SPIR-V and MSL. A small tool turns the results
  into C arrays committed beside
  the sources, each stamped with a hash of its source; `build.bat check` fails when a source changed
  and its arrays did not. Builds never need a shader compiler, and `d3dcompiler_47.dll` is no longer
  loaded.
- Per-draw data comes through an instance-rate vertex attribute holding the draw's index, never
  through `SV_InstanceID`: SDL warns that `first_instance` does not reach built-in IDs the same way
  on every API (`SDL_GPUIndirectDrawCommand` in `SDL_gpu.h`).

### Pipelines instead of render states

- The fixed-function state machine goes: no `GFX_RS_*`, no `GFX_TSS_*`, no `HRS`/`HTSS`. Each of
  the 19 queues already stands for one combination of blend, depth, cull and alpha test; each
  becomes one pipeline (a few queues need two, such as two-pass alpha), created once at start.
  Texture stage combinations become shader code: the ground's lightmap stage and the multitexture
  particles' 22 stage states.
- Fog is computed in the shaders from view depth for everything, as the mesh shaders already do.

### Memory and uploads

- **Textures** are RGBA8 everywhere. The 16-bit look is kept by quantising to 4444 or 1555 on the
  CPU at load, as today, before widening. The magenta key stays at upload. Mips are generated on
  the GPU (`SDL_GenerateMipmapsForGPUTexture`). No texture is locked or read back by the CPU; images
  the CPU reads (`cdcbitmap.c:55, 114`) are read from their CPU copy.
- **One upload per frame.** Everything the CPU writes for a frame (dynamic vertices, instance data,
  per-draw constants, changed texture tiles) goes into one transfer buffer that cycles, copied in
  one copy pass at the start of the frame's command buffer.
- **Static data is resident.** A map's ground and model meshes share one vertex and one index
  buffer, filled when the map loads and as streamed models arrive. Map textures and lightmap pages
  live in texture arrays by size class, so a draw picks its texture by layer instead of by binding.

### The static world, GPU-driven

- At load, every (block or node, texture run) of the ground and the models becomes a draw record in
  a storage buffer: its index range, texture layer, lightmap layer, flags, bounds and transform
  index.
- Each frame a compute pass culls the records against the view and against each shadow cascade and
  writes indirect draw commands; the passes draw them with one indirect call per pipeline. The CPU
  no longer walks the scene graph's leaves to queue mesh draws.
- Animated model nodes write their matrices into a per-frame transform buffer; static nodes'
  matrices stay put.
- Water becomes a static mesh per map whose waves the vertex shader animates from the time.

### Shadows, fitted on the GPU

The look stays: four 2048 by 2048 cascades along the map's light, the same soft edges, bias, slant
and offset, the ground's baked shadows replaced where the maps reach, and the shadows' reach of four
times the camera's distance. What changes is who fits them. Today's CPU fit is exact for the boxes
it is given, but it fits to where geometry could be; the GPU can fit to what the frame actually
shows, which is how shadow maps are fitted in renderers of this generation (sample distribution
shadow maps):

1. A depth-only pass draws the receivers, the ground and the models, from the culled draw records.
   The main pass then reuses that depth, so it shades each pixel once.
2. A compute pass reduces that depth to the nearest and farthest depth on screen, splits the range
   into four slices as today, and reduces each slice's pixels to their extent across the light.
3. A compute pass turns those into the four cascade matrices, rounding each square up and snapping
   its centre to whole texels as today so edges do not crawl, and takes the light's depth range
   from the map's bounds. The matrices go into a buffer that the cull pass, the shadow pass and
   the receiving shaders read; the CPU never sees them.
4. The cull pass picks each cascade's casters from the draw records; the shadow pass draws them
   into the four layers of one depth-array texture.

Because the fit comes from the pixels themselves, no visible receiver can fall outside its cascade
(the three shadow bugs of 2026-11-09 to 11 were all receivers the CPU fit did not foresee), and the
squares are as tight as the screen allows, so texels are as small as today's or smaller. Deleted
then: `renderer_shadow_frame`'s fit, `ShadowBox`, `view_shadow_boxes`, the leaves' `groundMin` and
`groundMax`, `view_cast_shadows`, `ShadowReach` and `renderer_shadow_reaches`, with the tests that
covered them.

### Sprites, actors and effects

- Actor sprites, damage numbers, item and screen sprites, particles and STR effect quads become
  instances: a world anchor (or a screen position), size, offsets, atlas layer and rectangle,
  colour and depth parameters. The vertex shader projects them, carrying over the sprite depth rules
  now in `sprite_quad.c` and `rp_project.c` (one perspective per billboard, the linear row depth,
  the feet lift). Sprite atlas pages become one texture array.
- Granny actors are skinned in the vertex shader from bone matrices in the frame's upload, instead
  of on the CPU per vertex.
- What the CPU still builds per frame is a list of instances and a few matrices, uploaded once.

### 2D, UI and text

- UI canvases stay CPU-drawn and are uploaded as changed tiles in the frame's upload. Windows that
  composite render into a target texture and draw that texture; nothing is copied back.
- Text never touches the frame: the GDI text on the back buffer (`cframe.c:86-96`) and the FreeType
  target lock (`text_freetype.c:151`) draw into textures like other UI text.
- The screen wobble becomes a pass that samples the scene's colour texture with the wobble's
  offsets (`cframe.c:32-65` today edits locked rows).

### No read-backs

Screenshots and scene captures download the finished frame (`SDL_DownloadFromGPUTexture`) when
asked; the scene harness waits for that download. Nothing else reads from the GPU: the lockable back
buffer, the copy swap, `renderer_lock_target`, `targetBits`, the startup texel probe and every
`gfx_lock` on a target are deleted.

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

### Phase 0: groundwork (small to medium)

- Drop the 32-bit client: `build.bat` builds x64 only, and the x86 link line goes with
  `granny2.lib`, `mss32.lib` and `cps.lib` from `lib/` and whatever code exists only for them. The
  unit tests move to x64, the way the client is built; `tests/oracle/` stays 32-bit because it
  loads the original executable, and is the only 32-bit build left. Fix what x64 shows in the
  tests. Update `docs/portability.md`, which names the x86 client the reference, and the CI matrix.
- Build SDL 3.4 with video and the GPU API in `vendor/build_vendor.bat`, at the pinned commit
  `cmake/SDL3.cmake` already uses.
- Check on real hardware what the plan rests on: the shader tool chain produces every format from
  one HLSL file; texture creation from the model-streaming thread follows SDL's thread rules (the
  Direct3D 9 device is created `MULTITHREADED`, `gfx_d3d9.c:835`, which suggests other threads use
  it).
- Build the reference binaries into `build/ref/` and capture their goldens.
- Make the counters measurable without the overlay: draws, pipeline or state changes, texture
  binds, bytes uploaded and CPU time in `renderer_draw_scene` and `view_on_render`, logged by the
  scene harness per capture. Record the baseline.

### Phase 1: Windows on the SDL window (medium)

- The Windows client runs on `core/app_sdl.c` and `platform/window_sdl.c` like the others, with
  Direct3D 9 still drawing into the SDL window's `HWND`; the renderer does not change, so captures
  must match the reference exactly.
- Delete `core/app_win32.c`, `core/wndproc.c` and the Win32 window code in `platform/` that only
  they used. The scene harness on Windows replaces SDL's input like the CMake scene target does.
- Done when Patrick has played it on Windows with Hangul text input, resizing, alt-tab and
  fullscreen, since SDL's IME on Windows replaces the client's own.

### Phase 2: one backend behind today's API (large)

- `render/gfx_gpu.c` implements `render/gfx.h` as it stands on the GPU API: a pipeline cache keyed
  by the current render states, one shader that does what the GL backend's fixed-function shader
  does, the Direct3D 9 mesh and shadow shaders ported, draws recorded into a frame list and
  replayed after one upload, targets as render passes, captures and the remaining target locks
  served by downloads.
- Delete `gfx_d3d9.c`, `gfx_gl.c`, `test_gfx_gl`, the d3dcompiler loading, `ddraw.lib d3d9.lib
  dxguid.lib`, the GL attributes in `app_sdl.c` and `gfx_gl_set_cover`. Delete the bump-map
  texture path as well (`texture_create_bump`, `bump_caps_check`, `GFX_FMT_BUMP` and the backends'
  bump-environment stage ops): nothing has called it since the tree's first commit. Update the
  tests that stub `gfx_*`.
- Done when the Windows, Linux and macOS clients play on it and captures match the reference within
  the tolerance the GL backend's cross-platform captures use (3 per channel, `RAGEXE_MSAA=1`), apart
  from changes explained in the review entry. Linux and macOS gain shadows and MSAA here.

### Phase 3: pipelines and a modern API (medium)

- Replace the render-state and stage-state machine with pipelines per queue and shaders per stage
  combination. `gfx.h` loses `GFX_RS_*`, `GFX_TSS_*`, `GFX_TOP_*`, `GFX_TA_*`, the transform and
  light calls, `SurfaceDesc`, the DirectDraw enumeration, modes and adapters, `flip_to_gdi`,
  `get_dc`, `is_lost`, `restore_all` and `evict_managed_textures`.
- Fog moves to the shaders; the per-vertex fog alpha on the CPU goes.
- Fullscreen becomes borderless with scaling.

### Phase 4: no read-backs (small to medium)

- The wobble pass, compositing into target textures, text into textures, captures by download.
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

- Record the counters from phase 0 again on the same scenes and write the result into the review
  entry. Remove what the phases left unused.

## Decisions

Taken with Patrick on 2026-11-11:

- **Windows moves onto SDL's window** (phase 1), so every platform shares one window, input and
  IME path.
- **No 32-bit client.** Only the oracle tests stay 32-bit, because they load the original
  executable.
- **Shadows are fitted on the GPU** from what the frame shows (phase 5), with today's look.
- **Vulkan on Windows**, as on Linux and Android; Metal on macOS. SDL runs on the driver the client
  ships shaders for, and the client's own code is the same with either Windows driver. Against
  Direct3D 12, whose runtime ships with Windows and whose drivers are the most used on laptops with
  Intel graphics, Vulkan leaves two shader formats (SPIR-V and MSL) instead of three and one driver
  path for three platforms, the one Linux and the Steam Deck run; it gives up Intel's Haswell and
  Broadwell, below the floor anyway, and Microsoft's PIX (RenderDoc works with both). Switching
  later costs a shader format and a device flag, not renderer code.
- **Shaders are compiled offline and committed** with a source-hash check, rather than needing a
  shader compiler in every build (the Android cross build cannot run host tools it builds, which is
  why `cmake/EffectData.cmake` runs a CMake script there instead of its `bin2c` tool).
- **Fullscreen without mode switches:** borderless at the desktop's size, scaled.
- **Android needs Vulkan** once the GLES path goes.

Not in this plan, but a uniformity candidate: Windows draws text with GDI (`text_gdi.c`), every
other platform with FreeType (`text_freetype.c`, already a `build.bat freetype` variant). Moving
Windows to FreeType would remove the last per-platform text path, and would change how text looks
on Windows.

## Risks

- **SDL's GPU API is young.** Driver-specific bugs surface on old integrated GPUs first; the scene
  harness only runs where there is a GPU, and CI has none (scene captures need game data and stay
  off CI anyway).
- **IME.** SDL's text input replaces the client's own Win32 IME handling; Hangul composition on
  Windows has to be checked by hand (phase 1), as it still is on macOS.
- **Matching the reference.** Pre-transformed vertices must keep Direct3D 9's pixel centres and
  top-left fill rule, as the GL backend already does (`docs/portability.md:392-410`); GPU mips and
  RGBA8 sampling of quantised texels will differ from CPU mips by small amounts. Each phase's review
  entry accounts for the pixels that change.
- **The GPU shadow fit moves when the picture does.** It follows what is on screen, so a character
  walking close past the camera can change a cascade's square from one frame to the next; the
  rounding and texel snapping that keep today's edges still must hold them still, and phase 5
  checks it with the moving captures.
- **Threads.** If loader threads create textures or meshes today, they will have to hand them to
  the render thread or follow SDL's rules for concurrent resource creation.
- **Upstream picks.** After phases 1 and 2, reconstruction fixes to `main`'s Win32 and Direct3D 9
  code no longer apply here; each is recorded as not needed, and fixes above `gfx.h` and the window
  layer still pick as before.
- **Tests that stub `gfx_*`.** About 14 of them change with each API change in phases 2 to 4.

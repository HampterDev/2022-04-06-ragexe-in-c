# One renderer on SDL's GPU API

Status: phases 0 and 1 are done, and so is the window that resizes and goes fullscreen at native
resolution. Phases 2 to 8 are done on Windows: phase 5 draws the static world from records the GPU
culls and fits its shadows and culls their casters on the GPU, phase 6 draws sprites, effects and
the UI as instances and skins Granny's actors on the GPU, phase 7 holds every shader to a
tolerance of the CPU's floats instead of their bits, sends a texture's changed rectangles alone and
measured the result on the load scenes, and phase 8 records and submits each frame on a render
thread of the device's own. On Linux, in WSL, the client and its tests build with GCC
and pass, the device's own tests of phases 2 to 5 and of phase 6's instances among them on Mesa's
lavapipe (Granny's skinning and phase 7's shaders have not run there), and the scene harness of
phases 2 to 4 has run there, but no one has played on it. The Android client of phase 2 builds for
both ABIs and draws the login screen on the x86_64 emulator (the host's GPU through gfxstream, and
SwiftShader); no map has been drawn on Android. The macOS client has not been built or run on it,
and no Metal compiler has seen its shaders. The CPU's own searches that phase 7's profile found (the
sprite atlas's walk, robes looked for on disk, Lua per layer, effects' owners found by walking the
lists) are fixed on the line, and the load scenes were measured again (*Where the renderer stands*).
Phase 9 is next.
File references drift as the code changes. Sizes are judgement, not measurement.

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

- **One backend behind `render/gfx.h`:** `render/gpu_device.c` on SDL's GPU API, Vulkan or Metal,
  on every platform (phase 2), with the static world a layer of it (`render/gpu_layer.h`,
  `render/gpu_world.c`). A frame goes to the GPU in one command buffer with one upload, through
  the shaders of `src/render/shaders` that `tools/shaders` compiles offline.
- **A render thread of the device's own** (phase 8). The main thread records a frame into a packet,
  one of a ring of four (`render/frame_ring.c`), its records written once into transfer buffers of
  the packet's own, and at the frame's end hands the packet over whole, with every GPU object its
  passes use as they are then; the render thread (`render/render_thread.c`) records it into the
  command buffer and submits it while the main thread records the next. A GPU object the main
  thread gives up goes once the next frame handed over is submitted. The main thread presents: it
  takes the window's picture while the render thread finishes the frame before, then puts the
  screen, as that frame left it, into it, so a frame is shown at the next present. Command buffers
  are recorded one at a time, the render thread's and the main thread's, as SDL moves memory at a
  submit (*Risks*, "Threads").
- **Draw modes, not render states** (phase 3). `render/draw_modes.c` names each way the client
  draws: blending, depth test and write, alpha test, fog, the texture stages' combination and their
  sampling, as the Direct3D 9 client set them for each list. The device makes every mode's
  pipelines when it starts; the effect lists' quads carry their own D3DBLEND factors and
  texture-stage preset, and their modes have a pipeline for each pair of the screen's blend
  factors. One quad shader runs the stage combinations, chosen by the draw's record. Fog is
  computed in the vertex shaders from view depth.
- **Quads and triangles are instances** (phase 6, first part). Everything the client draws but the
  static world and Granny's actors is a record of the frame's quads (`render/quads.c`), from which
  the quad shader's vertex shader makes the vertices the CPU made before, within the tolerance of
  *Decisions* (`render/shaders/instances.hlsli`): a sprite's from its quad's rows, rhw and turn; a
  particle's, an effect primitive's triangle's and a ground cell tile's by projecting its points as
  `renderer_project_vertex` did; everything else, which the client builds on the screen (the UI,
  STR effects around their anchors, the screen-space effect primitives, the cooldown pies), from
  the client's own vertices, copied once into the frame's upload.
- **Granny's actors skinned on the GPU** (phase 6, second part). A model's meshes stay on the GPU
  in a vertex and an index buffer of their own, put up when a draw first takes the model; each frame
  an actor gives one pose (its bones' matrices, its meshes' matrix into the view, its light) among
  the frame's quads, and the skin shader skins, lights and projects every vertex as the CPU did
  (`render/shaders/skin.hlsl`, `render/skin_layout.c`); one mesh of several actors is one instanced
  draw.
- **The static world on the GPU** (phase 5, first half): a map's ground blocks and RSM model nodes
  in one vertex and one index buffer, their textures and lightmap pages in texture arrays of one
  size and number of mipmap levels each, a draw record per (block or node, texture run) and a
  transform slot per model and node (`render/static_world.c`, `render/world_layout.c`,
  `world/world_gpu.c`, `gfx_world_*`); the meshes and textures go up through a stream of their own,
  a bounded transfer buffer at a time. Each frame a compute pass computes every slot's view as the
  CPU computed a node's matrix, and two more cull the records against the view and
  compact each run of records with one texture array, lightmap array and cull mode into one
  indirect draw. A model's pose is the one its first tick of the frame left; a model that animates
  and of which the view, or the box whatever can shade the view lies in, keeps a record is ticked
  though the walk does not reach it. The CPU culls the alpha-blended models' records itself and
  queues them among the sprites as the walk reaches their models. The water's cells stay on the
  GPU, raised by the frame's waves in the vertex shader. Every shader reads at most the four storage
  buffers a stage that every Vulkan device takes, what it needs packed into them.
- **Shadows fitted on the GPU** (phase 5, second half): a single-sample pass draws the receivers the
  view's cull kept into two targets of view depth, three compute passes fit the four cascades to
  them with the range and the squares kept from frame to frame (`render/shadow_fit.c` is their
  reference), the cull pass picks each cascade's casters from the records into indirect draws, and
  the shadow pass draws them into the four layers of one array. The CPU gives the light, the view
  and the box the world's draw records lie in, and never sees the cascades. Only the static world
  casts and receives these shadows: Granny's actors, like the sprites and effects, neither cast nor
  receive them.
- **Built on the CPU every frame:** the quads of actor sprites (projected on the CPU,
  `actors/sprite_quad.c`, their vertices made on the GPU), Granny actors' poses (their bones posed by
  the playback and their light, `actors/granny.c`; one vertex of one monster's mesh is still skinned
  on the CPU, where its effect starts), the water's key among the sprites (one per cell in view), the
  particles', effect primitives' and ground cell tiles' points (projected on the GPU, the CPU
  computing their rhw for the lists' order, and the particles' and tiles' depth, as the sprites'
  rows' depth, which the depth test compares by its bits), STR effects and the screen-space effect
  primitives (built on the screen around anchors the CPU projects), damage numbers and every 2D
  quad. The CPU still walks the scene graph's leaves the view reaches, for the water's keys, the
  actors, and the models' ticks and the poses and alpha-blended records it takes from them for the
  device.
- **How it reaches the device:** 18 queues (`render/draw_queue.h`), flushed in a fixed order by
  `renderer_draw_lists`, each in its draw mode (`render/crenderer.c`), with the static world's
  passes drawn before the opaque lists' quads and the ground's after them. Instances are batched
  into one draw while what they are drawn with stays the same (the textures, or layers of one
  array).
- **No read-backs** (phase 4). Nothing comes back from the GPU while playing: the screen wobble
  is a step of the frame that copies the screen and draws it again with its rows moved
  (`gfx_wobble`), from a table the CPU computes with the original's random numbers
  (`render/screen_wobble.c`); windows that composite draw into a render target that their quad reads
  (`render/render_clients.c`); the texel offset is the constant half a texel. Only screenshots and
  scene captures read the screen, when asked (`gfx_read_frame`), each a submit and a wait for its
  download. The device makes a swapchain only for a window that has been shown, so the scene
  harness has none.
- **Textures:** 16-bit 4444/1555 converted on the CPU (`render/csurface.c`) and widened to
  B8G8R8A8 when they go up, magenta keyed to alpha 0, mips made on the GPU; sprite atlas pages
  decoded from palette RLE on the CPU (`render/rp_atlas.c`) and kept as layers of texture arrays of
  their size, placed by the main thread, only the cells written going up; UI canvases re-uploaded as
  256-pixel tiles when they change. The static world's textures and lightmap pages go into the
  layers of texture arrays of its own, one per size and number of mipmap levels, made for the map;
  every other surface is a texture of its own.
- **Measured:** each scene capture writes its frame's counters (`tests/scenes/README.md`).
  `prontera_crowd` made 1,174 draws (1,954 calls) and `camera_level_view` 3,345 with Direct3D 9;
  they make 617 and 578, of them 322 and 321 for the shadows (the depth pass and the casters, an
  indirect draw a run in each cascade). `docs/custom-review.md` has the baseline and each phase's
  CPU times. The load scenes (`tests/scenes/load.py`, `RAGEXE_MSAA=4`, three rounds by turns, a GTX
  970 at 1,600 by 900), the median frame in ms of this client, of the line before phase 8 (with the
  CPU fixes) and of Direct3D 9, and the parts of this client's frame that the next phases take
  from. The rows marked * are from the rounds of `docs/custom-review.md`, "The shadow passes made
  cheaper", for this client and its parts, and of "The frame recorded on a render thread" for the
  line and Direct3D 9; every other row is from the latter's rounds:

  | scene | this client | the line | Direct3D 9 | draw scene | present | texels |
  |---|---|---|---|---|---|---|
  | `woe_emperium` | 18.18 | 20.23 | 27.04 | 5.28 | 0.02 | 0.22 |
  | `woe_choke` | 15.31 | 16.95 | 22.80 | 4.55 | 0.02 | 0.20 |
  | `arrive_castle` | 14.62 | 16.27 | 20.51 | 5.03 | 0.02 | 0.55 |
  | `bombers_ifrit` | 10.74 | 11.60 | 15.02 | 3.58 | 0.01 | 0.13 |
  | `arrive_prontera` | 3.54 | 3.93 | 12.25 | 0.91 | 0.01 | 0.16 |
  | `town_prontera` * | 3.21 | 3.65 | 12.06 | 0.81 | 0.01 | 0.07 |
  | `mvp_randgris` * | 5.19 | 5.53 | 4.08 | 0.81 | 1.81 | 0.04 |
  | `snow_lutie` | 3.38 | 3.38 | 7.01 | 0.43 | 1.77 | 0.03 |
  | `mvp_baphomet` * | 5.29 | 5.54 | 4.70 | 0.89 | 1.78 | 0.04 |
  | `aoe_orc_dungeon` * | 2.48 | 2.78 | 2.32 | 0.16 | 1.87 | 0.00 |

  `texels` is the textures' pixels widened to B8G8R8A8 as they go up (`texels_ms`), which Direct3D 9
  did not do. The frame is about seven tenths of Direct3D 9's in the crowds and under a third of it
  in Prontera.
  Presenting no longer records the frame on the main thread; in Randgris's, Baphomet's and the Orc
  Dungeon's fights it is the wait for the GPU. There and in Prontera's street the frame lasts as long
  as the GPU's work on it, whose shadows' passes take 1.35 to 1.81 ms (*Risks*), and only in the
  three fights is Direct3D 9's frame the shorter. The first
  frame of an arrival takes 230 ms in the castle and 138 in Prontera (Direct3D 9: 212 and 130),
  most of it handling the packets of the crowd (133 and 99 ms), 17 and 12 ms widening texels.

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
- The shaders are plain arithmetic, held to a tolerance of the CPU code they replaced by their
  tests, and their pictures to a budget (*Decisions*).
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
  per-draw constants, changed texture tiles) is written once into the transfer buffers of the
  frame's packet, mapped while the frame is recorded, copied in one copy pass at the start of the
  frame's command buffer, and made again smaller after a while where they grew for a burst. A
  packet is filled again only once the GPU has finished the frame it held. The static world's
  meshes and textures, hundreds of MB on a map's first
  frame, go up through a stream of their own instead: a transfer buffer of 32 MB (or the largest
  texture) filled a batch at a time, each batch in a command buffer of its own that is waited for
  before the next, the textures' pixels widened straight from their surfaces.
- **The main thread makes, the render thread records.** The model loader thread (`world/models.c`)
  creates and fills textures (`texture_cache_get` to `gfx_create_texture` and
  `texture_upload_pixels`) as pixels on the CPU only; the main thread makes every GPU object and
  stages the frame's uploads, and the device's render thread records and submits the frames. SDL
  documents no thread safety for making objects while another thread records; its Vulkan and Metal
  drivers lock what the two share (*Risks*).
- **Static data is resident.** A map's ground and model meshes share one vertex and one index
  buffer, filled when the map loads and as streamed models arrive, each mesh behind the ones placed
  with its 16-bit indices and a base vertex. Map textures and lightmap pages live in texture arrays,
  one per exact size and number of mipmap levels (a texture in a larger layer would filter, wrap and
  mip differently), each made with the layers the map's first frame places and grown to the power
  of two at or above the layers where more come, so a draw picks its texture by layer instead of by
  binding. Over the 943 maps of the game's
  data there are 41 sizes; 256 by 256 holds 74 % of the textures and every lightmap page; a map
  needs 8 arrays at the median and 23 at most, 338 layers of one size at most (two arrays at
  Vulkan's minimum of 256 layers), and 33 MB of RGBA8 with mips on average, 471 MB at most.

### The static world, GPU-driven

- At load, every (block or node, texture run) of the ground and the models becomes a draw record in
  a storage buffer: its index range and base vertex, texture and lightmap (array and layer), pass,
  cull mode, flags, bounds, transform slot, texture-coordinate matrix slot (RSM2 nodes), today's sort
  keys and, for alpha, the key's point.
- Each frame a compute pass culls the records against the view and against each shadow cascade and
  writes indirect draw commands; the passes draw them with one indirect call for each run of
  records that share a pipeline, texture array and lightmap array, the records sorted by pass, then
  today's texture order, then cull mode and place in the index buffer (SDL binds at most 16 sampled
  textures a stage, and a map can need 23 arrays). Sorting by array before the texture order would
  make fewer calls, but would turn round pairs of coplanar faces of two textures that today draws
  in texture order (up to 10 a map). SDL has no draw whose count the GPU writes (`draw_count` is
  the CPU's), so the cull compacts each run: a group of threads takes the run's records in order,
  keeps those in view, gives each its place in the frame's index buffer of them, one after the
  other, and writes the run's one command with the count it kept; a second pass, a group of
  threads to each 64 indices of a record (fixed with the records), copies them, the base vertex
  added, to the record's place where it is kept and leaves at once where it is not, so that a long
  run's copy (the ground's runs hold the most) is spread over the GPU and not left to one group,
  and no thread searches for its index. Since indirect draws have no first instance to give, each
  vertex carries its record's number. The CPU no longer walks the scene graph's leaves to queue mesh
  draws.
- Alpha-blended model draws stay among the sprites and effects, sorted back to front as today: the
  CPU culls and keys them from their records when the walk reaches their model, in the order of its
  nodes and their runs, so that draws at one depth keep today's order.
- Animated model nodes (the node or an ancestor has more than one key in a channel, whatever the
  model's animation says when it is built: a skill's model is set going later) write their matrices
  into a per-frame transform buffer; static nodes' matrices stay put. A model's pose is taken when
  the frame first draws it, as today drew it after its first tick; a model the walk did not reach
  whose moving part the view holds, or whose moving part can shade the view, is ticked first, so
  that no part is drawn, nor casts its shadow, in a pose the frame did not tick. A compute pass
  multiplies each slot's matrix into its parent's view as the CPU did, so that the vertices land on
  the pixels they did.
- The water's cells stay on the GPU, one quad each, and the vertex shader raises their corners by
  the frame's waves as the CPU did each frame. The water is drawn among the sprites, one
  cell after another in the order of their depth, so the CPU still keys the cells it sees; drawing
  them all at once would change the picture wherever water and sprites overlap.

### Shadows, fitted on the GPU

The look stays: four 2048 by 2048 cascades along the map's light, the same soft edges, bias, slant
and offset, the ground's baked shadows replaced where the maps reach, and the shadows' reach of four
times the camera's distance. What changed is who fits them. A CPU fit can only fit to the geometry
it is given; the GPU fits to what the frame shows (sample distribution shadow maps):

1. A single-sample pass draws the receivers, the ground and the models, from the records the view's
   cull kept, into two R32F targets of view depth: the nearest receiver of every pass, and the
   nearest of the opaque passes and the ground, so that each pixel's receivers lie between the two
   (the ground shows through an alpha-blended face). It cannot be the main pass's depth: the fit must
   come before the main pass, which shades with the cascades, and with multisampling on SDL can
   neither resolve a depth target nor let a compute pass read a multisampled texture. The
   multisampled main pass keeps its own depth.
2. A compute pass reduces those to the nearest and farthest depth on screen within the shadows'
   reach; a second cuts a range kept from frame to frame into four slices that grow geometrically
   and reduces each slice's pixels to their extent across the light. Both take the screen a tile
   of 32 by 32 pixels a group of 64 threads, each thread reducing its 16 pixels in its registers
   and the group its threads' in shared memory, so that the fit's state takes a few atomics a tile;
   the second cuts the slices once a group.
3. A third turns those into the four cascades' matrices, rounding each square up and snapping its
   centre to whole texels, with the light's depth range from the box the world's own draw records
   lie in (the animated ones in the frame's pose). Snapping holds edges still only while a square
   keeps its size, and a fit to the pixels alone moves its slices and resizes its squares whenever
   the receivers on screen change, far more often than a fit to boxes. So both are kept from the
   frames before, in a buffer kept on the GPU and reset with the world: the range keeps its far end
   while it lies within a tenth beyond the farthest receiver and its near end while the nearest lies
   within a quarter beyond it; a square keeps its size while it holds its receivers and is at most
   a fifth larger than they need or no larger than its slice of the view, and when its receivers
   outgrow it, it grows to a fifth more than they need. The matrices, the boxes' planes and the
   count of cascades that receive go into a buffer that the cull pass, the shadow pass and the
   receiving shaders read; the CPU never sees them. A frame where nothing in reach receives keeps
   the ground's baked shadows, as does one whose shadow passes could not be recorded; a device that
   cannot make the fit's pipelines or buffers draws without shadows.
4. The cull pass picks each cascade's casters from the draw records, a group of threads to each run
   of casters in each cascade, and compacts them into a section of the frame's index buffer of
   their own; the shadow pass draws them into the four layers of one array, an indirect draw a run.
   The casters are ordered by texture array and in draw order within each, and a run is a stretch
   of that order whose texels one alpha test decides: the models' textured records, which drop the
   texels the opaque lists' alpha test drops, are drawn by a pipeline whose fragment shader can
   discard, and the ground and the records without a texture by one that cannot, which keeps the
   GPU's early depth test for them. The order is the draws' own whichever pipeline draws a run, so
   the maps are the same to the bit. A shader that can discard turns that test off for every
   fragment it draws, so the models' textured casters keep most of the shadow pass's time
   (*Risks*, "The shadows' GPU time").

`render/shadow_fit.c` is the fit's reference in C, which the tests hold the shaders to: the range,
the range kept and, given the same split, the extents, the squares and the texels their centres lie
on to the bit, the rest within the rounding Vulkan allows a division or a root. Because the fit
comes from the pixels themselves, no receiving pixel centre can fall outside its cascade. In every
motion measured on Prontera's street and fountain, at its gate and on two hilly fields (walking,
turning, zooming, tilting, earthquakes) the squares resize no more often than the CPU fit's did,
most far less often, and never while the camera walks or an earthquake shakes it
(`docs/custom-review.md`, "Shadows fitted on the GPU"). Against the CPU fit it replaced, over the 23
frames of the shadow views, the far views and three scenes (25 million receiving pixels), a pixel's
texel is 10 % smaller on average, up to 29 % on the hills and at the gate up close, where the boxes
reached far beyond what the screen shows; on Prontera's street with the camera at its closest and
flattest the median pixel keeps its texel, as the fit's geometric slices from the nearest receiver
end close to where the CPU's blend from the near plane ended. Only in the far views is a view's
median texel larger, by 3 to 5 %: the range's far end and the squares are kept a little ahead of
the receivers, so that they stay while the camera moves. The CPU still works out, without the
cascades, where a caster could shade the view, so that
a model out of view whose shadow can fall into it is ticked (`renderer_shadow_reach`).

### Sprites, actors and effects

- Actor sprites, damage numbers, item and screen sprites, particles and STR effect quads become
  instances whose vertices the vertex shader makes as the CPU made them. The sprites'
  projection stays on the CPU, which needs their screen quad anyway: picking builds its rectangles
  from it, name tags and balloons take the anchor, and callers add depth tweaks
  (`render_object_clip_to_quad`, `renderer_project_point`); a sprite's instance is that quad.
  Particles hand over their points, which the shader projects; STR effects are built on the screen
  around one projected anchor and drawn from their vertices. Sprite atlas pages become layers of
  texture arrays.
- Granny actors are skinned in the vertex shader from bone matrices in the frame's upload; bone
  and mesh matrices and the light vector stay on the CPU.
- What the CPU decides from its own floats stays its own: the lists' order (the sort keys), which
  draws are flat, and the depths a figure's layers and what stands with it tie at (the sprites'
  rows, the particles' and ground tiles' corners), so that the GPU's rounding (*Shaders*) moves a
  vertex by a hair but never reorders a list nor turns a figure's layers about.
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

The main thread records frame n into its packet. At its end it waits for the render thread to
finish frame n - 1, sends the static world's new meshes and textures up through their stream, makes
the frame's buffers large enough, puts the screen (frame n - 1) into the window's picture and hands
frame n over; then it records frame n + 1. The render thread waits for the GPU to finish frame
n - 2, records frame n into one command buffer and submits it:

1. Copy pass: the frame's upload, from the packet's transfer buffers.
2. Compute: every slot's view, then cull the draw records for the view, compacting each run into
   one indirect draw.
3. Depth pass (single-sample, view depth): the receivers the view's cull kept.
4. Compute: fit the cascades from that depth, then cull the draw records for each cascade.
5. Shadow pass: four layers, indirect draws of the casters.
6. Main pass (multisampled, with its own depth): indirect draws of the ground and models, then
   water, then the alpha-blended models, instanced sprites and effects in the queues' order,
   resolved.
7. The wobble pass when it runs, then the 2D pass onto the screen, which the main thread shows at
   the next present.

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

- `render/gpu_device.c` implements `render/gfx.h` as it stands on the GPU API: a pipeline cache keyed
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

### Phase 4: no read-backs (done)

- The screen wobble is a pass: a copy of the screen so far, then one triangle over the whole screen
  whose pixels read the copy from the row and shift a table gives (`src/render/shaders/rows.hlsl`),
  every sample of a pixel alike, so a multisampled screen resolves to the same texels. The CPU makes
  the table with the same `rand()` calls, in the same order, as the original's row loop
  (`render/screen_wobble.c`, the library `screen-wobble`, tested against that loop). The windows
  that composite draw into a render target their quad reads (`texture_create_target`). The texel
  probe is gone (`TEXEL_CENTRE`). Screenshots and scene captures download the screen
  (`gfx_read_frame`). The window is claimed, and gets its swapchain, when a frame is first presented
  while it is shown, not minimized and of a size, and again at the next frame where SDL did not take
  it; the scene harness hides its window on every platform, so it runs without a swapchain. SDL's
  GPU drivers need no window at all, only a video driver that makes Vulkan surfaces or Metal views;
  the harness keeps the client's window, hidden, because the client reads its size and the resize
  scene changes it.
- Deleted: `gfx_lock` on targets with its lock copies and write-back, `renderer_lock_target`,
  `targetBits`, `targetPitch` and the renderer's `bpp`, the texel probe and its draw mode, the save
  surface's kind (`K_SAVE`) and the pipelines for the screen's copy (`DRAW_INTO_COPY`), copies from
  a target into a texture with their download when the texture was locked (`gpuNewer`), and the
  overlay's target-lock timing and counter, the back buffer's handle that shared the screen's
  texture and the startup flag that asked for it, and the device's fields from Direct3D 9 that
  nothing read. The fade's saved frame is an ordinary render target.
- The scene set, `window_resize`, the wobble's nine captures, the hill scenes and the gate scene are
  phase 3's to the pixel, without and with multisampling, with the same draws; a screenshot is phase
  3's JPEG byte for byte.

### Phase 5: the static world and its shadows on the GPU (done)

- Shared map buffers, texture arrays, draw records, the compute cull for the view, the depth pass,
  the cascade fit and the cascade culls in compute, indirect draws, the transform buffer, water as a
  mesh.
- The first half: the static world as *The static world, GPU-driven* describes it, but for the
  cascades' culls: shared buffers with 32-bit indices, texture arrays filled and mipmapped by layer
  on the main thread through the stream, records, transform slots computed on the GPU, the view's
  cull compacted into one indirect draw a run, the poses taken at a model's first sight and the
  models ticked for a moving record in view, the alpha-blended models culled and keyed by the CPU
  from their records, the water's cells on the GPU (`world/world_gpu.c`, `render/gpu_world.c`,
  `render/world_layout.c`, `world.hlsl`, `world_transform.hlsl`, `world_cull.hlsl`,
  `world_compact.hlsl`, `water.hlsl`). Deleted with it: the per-leaf mesh queuing
  (`world/model_node_render.c`, `c3d_ground15_render`), the `GROUND_MESH` queue, the per-block and
  per-node meshes (`gfx_mesh_create`, `gfx_draw_mesh`, `gfx_set_world_view`), the water's heights
  and quads built on the CPU every frame, and the mesh and caster shaders. The captures are the
  same as before but for the pixels the plan expected (`docs/custom-review.md`, "The static world
  on the GPU").
- The second half: the shadows as *Shadows, fitted on the GPU* describes them: the depth pass
  (`world_depth.hlsl`), the fit's three compute passes with the range and the squares kept from
  frame to frame (`shadow_range.hlsl`, `shadow_extents.hlsl`, `shadow_cascades.hlsl`, against
  `render/shadow_fit.c`, their constants packed by `render/shadow_fit_gpu.c`), the light's depth
  range from the world's own draw records, the cascades' culls in the cull and compaction passes
  and the shadow pass drawing their casters indirectly (`world_caster.hlsl`). Shadows are optional:
  a fit the device cannot make, or a frame whose shadow passes cannot be recorded, leaves the
  ground's baked shadows. Every shader keeps to four storage buffers a stage, and the world's
  texture arrays make their mipmaps as the sprite atlas's do. Deleted with it:
  `renderer_shadow_frame`'s fit, `ShadowBox`, `view_shadow_boxes`, `view_shadow_bounds`, the
  leaves' `groundMin` and `groundMax`, `view_cast_shadows`, `ShadowReach` and
  `renderer_shadow_reaches`, the `SHADOW_CASTERS` queue, `RP_WORLD_CAST`, `gfx_world_cast`,
  `world_gpu_cast_actor`, `c3d_ground15_cast` and `castFrame`, with the tests that covered them;
  the CPU box fit stays as a fixture of `test_shadow_fit`, which measures the GPU's fit against it.
  The leaves' walk stays for the water's keys, the actors and the models' ticks.
- Checked: the scene set, the hill scenes, the gate scene and the far views from 2,500 to 10,000
  show the same shadows as the reference but where the fit's texels lie otherwise, which moves
  shadow edges by up to a texel and changes the soft edges' grain: 97 % of the shadows' pixels that
  differ from the reference lie within a texel and the soft edge of an edge both draw, the rest in
  wide soft edges on faces steep to the light; away from the shadows they keep within one colour
  step (`docs/custom-review.md`, "Shadows fitted on the GPU"). Where the CPU's choice of casters
  left out a caster that shades the view (the foot of a tree on Prontera's main street at the
  closest camera, shaded by an animated part of another tree, which the CPU's tests of boxes left
  out), the reference and the commit before lack that shadow, and the GPU's cull draws it. In
  every motion measured the squares resize no more often than the CPU fit's did.

### Phase 6: sprites, actors and effects as instances (done)

- Sprites, damage numbers, particles and STR quads as instances whose vertices the vertex shader
  makes as the CPU made them, the particles projected in it; the sprite atlas as texture arrays;
  Granny skinning in the vertex shader.
- Delete the CPU's vertex building for these (`render_object_submit_spr_quad`'s vertices, the
  particles' projection, `snap_flat`), `gfx_batch.c` and the pre-transformed vertex kinds, and with
  Granny's skinning its per-vertex projection and lighting. The sprites' projection
  (`render_object_clip_to_quad`, `renderer_project_point` for anchors) stays on the CPU, which
  needs the quad for picking, name tags and balloons.
- First part (done): every quad and triangle is an instance. Sprites, ground items and effect
  sprites hand the device the quad `render_object_clip_to_quad` makes, as before, and the vertex
  shader makes `render_object_submit_spr_quad`'s vertices; particles hand over their points (in
  view space where each has a matrix of its own) and the shader projects them as
  `renderer_project_vertex` did, the CPU keeping only their rhw for the lists' order; the UI, STR
  effects (built on the screen around one projected anchor, not from world corners), effect
  primitives and Granny's vertices are drawn from the client's vertices, an axis-aligned rectangle
  in a short record. In a multisampled pass the shader moves a flat draw's corners onto pixel
  boundaries as `snap_flat` did on the CPU. The atlas's pages are layers of texture arrays of up
  to 16 MB by size, each going with its last page. Sprites and sprite particles take the
  renderer's quads as they did, so each keeps what the last one in its place left there, as in the
  original. `gfx_batch.c`, `gfx_draw`, the device's vertex kinds (`GFX_VTX_*`), `snap_flat` and the
  two-texture vertices are gone. The scene set, `window_resize`, the wobble and two new scenes, of
  effects (`effects_instances`) and of a map that draws alpha in two passes (`two_pass_alpha`), are
  phase 4's to the pixel, with and without multisampling.
- Second part (done): Granny's actors skinned in the vertex shader (`render/shaders/skin.hlsl`,
  `render/skin_layout.c`, `gfx_create_skin`, `gfx_skin_pose`, `gfx_draw_skin`). A model's meshes go
  up once, when a draw first takes the model, into a vertex and an index buffer of its own (a model
  lives as long as its resource, across maps, where the static world's buffers are made again with
  each map), one mesh behind the other, each vertex with its four bones and weights; each frame an
  actor gives one pose among the frame's quads, and each mesh is queued where its triangles were,
  an actor's meshes that run on with one texture one draw and one mesh of several actors one
  instanced draw. The shader skins, lights and projects each vertex as the CPU did, in plain
  arithmetic: every corner of the game folder's models, in their animations and fuzzed poses, is
  within 1/16 of a pixel, its rhw and depth within 1e-5 relative (nearer than twice the near plane
  the depth within 1e-5 of its range), of the CPU code it replaced (`tests/test_skin_instances.c`,
  which keeps that code as its fixture). Deleted with it: the CPU's
  per-vertex skinning (the shim's deformer and the vertex buffers it filled), lighting and
  projection, `granny_projection.c`, the world-space corners and the record kind they took through
  the camera's view (`QUAD_CAMERA`, `RPQuad.world`, `renderer_world_quads`, the world vertices of
  `gfx_draw_vertices`). With it the last vertices the CPU projected for drawing went to the GPU:
  the effect primitives' triangles of the world (their quad modes as rhw and depth rules, an
  upright quad stood up on the screen in the shader) and the ground's cell tiles (the cursor's cell,
  ground skills' areas, the navigation path) are projected points as the particles' are, bit for
  bit. A quad's two triangles name the four corners they share in a set put up once
  (`GFX_QUAD_CORNERS`), so that they take 136 bytes where their vertices took 208; a primitive
  made one parametric instance later would hand its parameters where the set holds its corners.
  What the CPU still projects is a point an object needs: an anchor (a sprite's, an STR effect's,
  an effect's screen position, the effect streak's two ends, Granny's screen box for picking), or a
  key among the sorted lists. The scene set and a scene of its own
  (`granny_actions`) are within the captures' budget of the commit before, with and without
  multisampling, and the effect gallery's frames are the same (`docs/custom-review.md`).

### Phase 7: measure and settle (done)

- The shaders are plain arithmetic within the tolerance of *Decisions*: `exact.hlsli` (every
  operation `precise`, a correctly rounded division, exact products emulated, constants by their
  bits), the device's check of the GPU's rounding when it is made, the tests' check of the rules in
  the SPIR-V and the vertex fog's table (`render/vertex_fog.c`) are gone, the shaders computing the
  fog's step from the rhw. The quad vertex shader is 34 KB of SPIR-V where it was 288 KB, the
  client's shaders 218 KB where they were 503 KB, and a start with an empty driver cache takes
  2.0 s where it took 2.7. The depths the depth test decides ties by stay the CPU's: the client
  hands over the sprites' rows' z and the particles' and ground tiles' corners' z, which it
  computes from the rhw it has for their keys (plain arithmetic turned whole overlaps of a figure's
  layers about in the load scenes). The tests hold every shader to the tolerance against the same
  CPU fixtures; the scene set's captures, with 1 and 4 samples and with and without shadows, are
  within the budget of the line's, and so are the load scenes' with one sample but for one row of
  46 pixels along an edge of the static world in the castle, which lies within the floats'
  rounding of a row of pixel centres (`docs/custom-review.md`).
- What the phases left: a texture's changed texels go up as the rectangles written, each alone
  (`render/dirty_rects.c`), and a texture not yet sent as what was written on it, its other texels
  cleared on the GPU, so that a cell drawn into a sprite atlas page no longer sends the page; a
  layer with mipmaps makes its levels from its own top level, copied on the GPU. The atlas's
  16-bit texels stay widened to 32 bits on the CPU: SDL's Metal driver takes 16-bit formats only on
  Apple's GPUs, and the floor's Intel Macs would need the widening as a second path.
- Measured on the load scenes against the line and Direct3D 9 (`docs/custom-review.md`, "The
  shaders within a tolerance"; three rounds by turns, 4 samples, a GTX 970): this client's frame is
  Direct3D 9's within what the rounds spread over in the crowds (32.0 ms against 31.9 in the
  Emperium room, 18.1 against 17.6 among the bombers) and shorter in Prontera (11.8 against 15.4)
  and in Lutie's snow (6.1 against 9.6). Its renderer costs the main thread more where the crowd
  is largest: `renderer_draw_scene` and presenting take 8.5 ms in the Emperium room where Direct3D
  9's take 6.6, 5.3 among the bombers against 4.3, most of the difference in presenting (2.1 ms
  against 0.35). A CPU profile of the result (VSDiagnostics, the measured stretch of one run of
  each) splits it: presenting is recording the frame's passes into Vulkan's command buffer (1.3 ms
  in the Emperium room, with SDL's and the driver's work) and copying the frame's records into
  the transfer buffer (0.3 ms), never waiting for the GPU; `renderer_draw_scene` is making the
  quads' records (2.9 ms), sorting the lists (0.7) and batching (0.8). Before them, in
  `view_on_render`, the effect primitives' triangles are built and queued one by one (3.6 ms of
  the Emperium room's 30.3 in the profile, 2.7 of the bombers' 16.5), and the level-99 auras'
  particles take 1.5 ms there and 1.8 of Prontera's 11.2. The rest of the objects' time is the
  CPU's own waste outside the renderer: the sprite atlas's linear search (2.8 ms
  in the Emperium room), robe sprites checked on disk every frame (3.2 ms in Prontera), Lua calls
  per layer (1.4 ms in Prontera), and a linear search of the skills for every effect (2.0 ms in the
  Emperium room). Those are fixed since, outside the renderer (`docs/custom-review.md`, the two
  entries after phase 7's), and the load scenes measured again (*Where the renderer stands*).

### Phase 8: the frame recorded off the main thread (done)

- What presenting was, profiled first (`docs/custom-review.md`, "The frame recorded on a render
  thread"): in the crowds and in Prontera, recording the passes into Vulkan's command buffer (1.3 ms
  in the Emperium room, 0.4 in Prontera), the world's compute and shadow passes (0.2, 0.3) and
  copying the frame's records into the transfer buffer (0.3, 0.1), never waiting; in the light
  scenes (Randgris, Baphomet, the Orc Dungeon), waiting for the GPU, 1.9 to 3.2 ms a frame, which
  the shadows' passes take as far as the frames without shadows tell (*Risks*, "The shadows' GPU
  time").
- A render thread of the device's own (`render/render_thread.c`, the library `render-thread`)
  records each frame into the command buffer and submits it, while the main thread records the next;
  the frames are packets in a ring of four (`render/frame_ring.c`, the library `frame-ring`, pure),
  each written once into transfer buffers of its own, handed over whole with every GPU object its
  passes use, and filled again once the GPU has finished it. GPU objects given up go once the next
  frame handed over is submitted. The main thread presents the last finished screen, waiting for the
  window's picture while the render thread finishes that frame, so a frame is shown at the next
  present; screenshots and captures read the frame just drawn. Command buffers are recorded one at a
  time, the render thread's and the main thread's (the present, the static world's stream, the
  reads, and the one SDL submits for each texture it makes), since SDL moves textures and buffers in
  memory at a submit (*Risks*, "Threads"). A device is made whole or not at all, and a frame the
  render thread could not submit takes the next one with it, which was recorded on its uploads. The
  single-threaded flush, its one transfer buffer and the copy of the records into it are gone.
- Checked: the scene set's captures at 1 and 4 samples, with and without shadows, and the load
  scenes' at one sample, the line's (`f555328e`) to the pixel, with the same counters but for the
  draws of a few frames of three load scenes, one more or one fewer, which the line differs by from
  itself where its memory lies otherwise (`docs/custom-review.md`); the validation layer with
  synchronization validation over the GPU tests, the scene set at both sample counts and three load
  scenes, without a message; the load scenes measured (*Where the renderer stands*): presenting
  takes the main thread 0.01 or 0.02 ms in the crowds and in Prontera where it took 0.75 to 2.0, and
  the frame is 0.1 to 2.0 ms shorter there, `renderer_draw_scene` within 0.15 ms of the line's; a
  device test makes its setup fail at each step and frames fail on either thread, and watches that
  no command buffer is submitted while the render thread records one; in the light scenes the frame
  is the line's, bound by the GPU.

### Phase 9: effect primitives and particles as parametric instances (large)

- What it takes from the CPU: building and queueing the effect primitives' triangles one by one
  (their corners, each triangle's key, an entry and a record each: 3.6 ms of the Emperium room's
  frame, 2.7 of the bombers'), and the level-99 auras' particles (1.5 ms in the Emperium room, 1.8
  in Prontera); measured by phase 7's profile, before the CPU fixes shortened the frames these
  parts lie in (the Emperium room's objects 8.1 ms where they took 14.9, its effects 1.2 where they
  took 2.3), so a profile of the client with them comes first.
- One instance a primitive (a ring of strips, a cylinder, a quad of a mode) or a particle system,
  holding what the CPU steps (a ring's radius, span, turn, tilt and column heights; a system's
  particles as the CPU spawned them, with the original's random numbers), from which the vertex
  shader makes every triangle or particle, as the corner sets of phase 6 already let one primitive
  hand its parameters where the set holds its corners.
- First settled: the original sorted every triangle and particle by its own key among the
  alpha-blended lists, and one instance is one entry. The harness counts, for each primitive and
  particle kind, how it blends and how often its entries interleave with other entries in the lists;
  a kind drawn additively whose entries do not interleave becomes an instance, the others keep
  their entries (the CPU can compute their keys from the parameters alone).
- Checked: the effect gallery's frames and the load scenes within the budget, `test_effect_trace`'s
  digest unchanged, the instances' triangles against the CPU code they replace in a test of their
  own.

### Phase 10: texels widened on the GPU (small)

- What it takes from the CPU: widening the client's 16-bit texels to 32 bits as they go up, 12 to
  16 ms of the first frame of an arrival in a crowd (Direct3D 9 sent them as they are), whose 174 to
  251 ms are mostly the crowd's packets (106 to 134 ms), and 0.1 to 0.5 ms a frame in the crowds.
- The rectangles written go up as they are, 16 bits a texel, into a texture of 16-bit integers the
  device keeps for staging, and a pass of their own widens them into the texture by bit replication
  as `render/texel_convert.c` does (the colour key for a system surface): one path on every GPU of
  the floor, where SDL's Metal driver has no 16-bit colour formats on Intel Macs.
- Checked: every texel of each format against `texel_convert` through a probe, and the captures
  to the pixel.

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
- **The GPU's floats are held to a tolerance of the CPU's, not to its bits** (decided 2026-11-12).
  Every shader is plain arithmetic, and its tests hold it to the CPU code it replaced: a vertex
  within 1/16 of a pixel, its rhw and its depth within 1e-5 relative (nearer than twice the near
  plane, the depth within 1e-5 of the depth range), texture coordinates within 1/4096, colours
  within a step. Every capture is held to a budget against the capture before: the pixels more
  than a colour step apart at most 0.05 % of the screen, each group of such pixels that touch at
  most 16, and every capture that differs looked at. A group one pixel thick along an edge of a
  triangle (a row or column of pixel centres that the floats' rounding puts on the other side of
  the edge) is an edge's difference, held to the share alone. Exactness stays where it is cheap
  and decides something discrete: the C code compiled without contraction (`-ffp-contract=off`),
  the sort keys and the flatness the CPU computes, the depth of the sprites' rows and of the
  particles' and ground tiles' corners, which the CPU computes from the rhw it has for their keys
  and hands over
  (the layers of a figure, and what is drawn where it stands, lie at depths that tie, and the depth
  test decides between them by the bits), and the shadow fit's squares and snapped texels, which
  are its C reference's by construction. The CPU's floats to the bit take a correctly
  rounded division and emulated exact products in every vertex, a check of the GPU's rounding at
  start-up and rules checked in the SPIR-V; they make the quad vertex shader 288 KB of SPIR-V where
  plain arithmetic makes it 34 KB, and a start with an empty driver cache 0.7 s slower; neither
  Vulkan nor Metal's fast-math MSL promises them; and they buy pictures that plain arithmetic draws
  within a few pixels a scene.

## Risks

- **SDL's GPU API is young.** Driver-specific bugs surface on old integrated GPUs first. The scene
  harness only runs where there is a GPU, and CI has none (scene captures need game data and stay
  off CI anyway).
- **Matching the reference.** Pre-transformed vertices must keep Direct3D 9's pixel centres and
  top-left fill rule, as the passes' viewport and `snapped_ndc` keep them (`docs/portability.md`,
  "Pixels"); GPU mips and RGBA8 sampling of quantised texels differ from CPU mips by small amounts.
  Each phase's review entry accounts for the pixels that change.
- **The GPU shadow fit moves when the picture does.** It follows the receivers on screen (the
  ground and the models; characters are not receivers), so the camera and animated model nodes move
  it; the range and the squares kept from frame to frame and the texel snapping hold the edges
  still. The motions measured resize the squares no more often than the CPU fit did; motions not
  measured, such as walking into a wall that comes to fill the view, can resize them more often, as
  the fit follows what the frame shows where the CPU's fit to boxes did not see what hides what
  (`docs/custom-review.md`, "Shadows fitted on the GPU").
- **Texels the hysteresis costs.** The range's far end is kept a hundredth beyond the farthest
  receiver and a square is taken 5 % larger than its receivers need, so in the far views, where
  the CPU fit's boxes held just what the screen shows, a view's median texel is 3 to 5 % larger
  than the CPU fit's. With the camera at its closest and flattest the third cascade's square is 112
  units where the CPU fit's was 96, and the 15 % of Prontera's street that lies in it gets a texel
  17 % larger.
- **The shadows' GPU time.** The depth pass and the fit run every frame at the screen's size: on a
  GTX 970 at 1600 by 900 the frame's wait (`present_ms`) grew by about 0.6 ms against the CPU fit,
  while the CPU's time in `view_on_render` fell by 0.15 to 0.3 ms. In the light load scenes the GPU
  is what the frame waits for: with shadows the main thread waited 1.9 ms a frame for the GPU in
  Randgris's fight, 2.7 in Baphomet's and 3.2 in the Orc Dungeon before phase 8, and 0.1 to 0.5
  without them, which shortens those frames by 2.3 to 3.2 ms (`docs/custom-review.md`, "The frame
  recorded on a render thread"). There the frame is the GPU's: neither a render thread nor more
  frames in flight shortens it, only less GPU work in the shadows' passes. Timed pass by pass
  (`docs/custom-review.md`, "The shadow passes made cheaper"), those passes take 1.35 to 1.81 ms of
  the GPU's frame in the light scenes and Prontera's street: the depth pass 0.13 to 0.17, the fit
  0.26 to 0.29, the casters' cull and compaction 0.05 to 0.16, and the four cascades 0.88 to 1.21,
  most of it the models' alpha-tested casters, whose shader turns the early depth test off. Direct3D
  9's shadows lengthen its frame by 0.7 ms in Randgris's fight, 0.9 in Baphomet's and 1.3 in the Orc
  Dungeon (with and without `RAGEXE_SHADOWS=0`, the same review entry). Each cascade's pass costs
  about 0.05 ms for beginning and clearing even where nothing is drawn into it, which only the CPU
  could skip, and it never sees the cascades.
- **SDL's descriptor caches.** SDL's Vulkan driver gives each command buffer a cache of descriptor
  sets and gets it back only once it finds the command buffer finished, and every texture it makes
  holds one for a while; a command buffer that finds none given back fills an empty one, a
  descriptor pool for each kind of binding the frame binds, which once took NVIDIA's driver 15 to
  19 ms. A frame waits for the frame two before it before it takes its command buffer, so in steady
  play it takes the cache that frame gave back; a frame that makes many textures can still make SDL
  start a new cache that a later frame fills once.
- **The GPU's floats are not the CPU's.** Every shader is plain arithmetic within the tolerance of
  *Decisions*, which its tests hold it to (`test_quad_instances`, `test_static_world_maps`,
  `test_skin_instances`, the fog in `test_gfx_gpu`), and every capture is checked against the
  budget. A decision the GPU takes from its own floats can fall the other way from the CPU's where
  its input lies within the rounding of a boundary: a flat draw's corner may land on the pixel
  boundary next to the CPU's in a multisampled pass, a record whose box touches a plane of the view
  be culled otherwise than `sw_record_in_view` decides (so that a model whose part only touches
  the view's edge is drawn in the pose of its last tick), a wave's corner take the next degree, a
  vertex's fog the next step; the tests count these, and a long straight edge that lies within the
  rounding of a row of pixel centres, or of samples, changes the whole row. A device that rounds or
  divides less exactly than NVIDIA's driver moves more of them; Metal's fast-math MSL has not been
  tried. The static world's vertices likewise land within the tolerance of Direct3D 9's, not on
  its bits: the GPU computes the views and multiplies the points in its own floats.
- **Threads.** The main thread makes GPU objects while the render thread records and submits a
  frame. SDL's API promises no thread safety for that; its Vulkan and Metal drivers lock what the
  two share (command buffer pools, allocators, the lists of objects to release, submission). The
  render thread reads nothing the main thread changes: a frame's packet holds every object its
  passes use, and an object given up goes only once the next frame handed over is submitted. SDL's
  Vulkan driver moves textures and buffers to other memory at a submit (every submit while no
  window is claimed, the submits that present while one is), skipping only allocations with an
  upload or download pending in some command buffer, and never a transfer buffer (each its own
  allocation), so a packet's transfer buffers stay where they were mapped; a render pass or bind
  takes an object's memory as it is recorded, so a submit while another command buffer is recorded
  could split that one's passes between old and new memory. Command buffers are therefore recorded
  one at a time under one mutex: the render thread's frame from acquire to submit, the main
  thread's own and each texture SDL makes (it submits one for it). The test
  `test_gfx_gpu_faults` watches that no submit falls inside the render thread's recording. A
  later SDL that moves memory elsewhere, or other drivers' rules, need the same look.
- **Upstream picks.** Reconstruction fixes to `main`'s Win32 and Direct3D 9 code no longer apply
  here; each is recorded as not needed, and fixes above `gfx.h` and the window layer still pick as
  before.
- **Tests that stub `gfx_*`.** About 14 of them change with each API change in phases 2 to 4.
- **The CPU's floats on every platform.** The sort keys and the flatness the CPU computes decide
  the lists' order and which draws move onto pixel boundaries; they are the same on every platform
  only where the C code rounds each float operation once, in the order written: MSVC's
  `/fp:precise` on x64, GCC and Clang with `-ffp-contract=off`, which the CMake builds pass.
- **Storage buffers a shader stage.** Every Vulkan device takes four
  (`maxPerStageDescriptorStorageBuffers`), and every shader keeps to them, what it reads packed into
  four buffers; the device refuses a shader that reads more (`STORAGE_BUFFERS_MAX`), so a pass added
  later must pack its own too.
- **Start-up with a cold driver cache.** The first start after a driver update compiles every
  pipeline: a run of the scene harness that starts the client and captures the login screen takes
  2.01 s with an empty NVIDIA shader cache on a GTX 970 and 1.59 s with it filled (2.67 and 1.59 s
  with phase 6's quad vertex shader of 288 KB of SPIR-V, which phase 7 brought to 34 KB).

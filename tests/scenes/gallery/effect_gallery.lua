-- Effect gallery: launches RagEffect IDs on the player one after another, as the map server's
-- ZC_NOTIFY_EFFECT2 does (game_mode_on_notify_effect2), and captures a short filmstrip of each.
-- tools/effect_gallery.py runs it and builds the gallery page; see tests/scenes/README.md,
-- "Effect gallery". It is not in scripts/, so run.py and the scenes ctest leave it out.
--
-- Environment (all optional):
--   RAGEXE_GALLERY_IDS     IDs and ranges, "0-2384,10000" (default: 0-2384)
--   RAGEXE_GALLERY_FRAMES  frames after the packet to capture, "1,8,16,32,64"
--   RAGEXE_GALLERY_CROP    "WxH", centred on the frame, or "full" (default 448x384)
--   RAGEXE_GALLERY_JPEG    JPEG quality, or 0 for PNG (default 85)
--   RAGEXE_GALLERY_SETTLE  frames to wait at most for an effect's leftovers to end (default 180)
--   RAGEXE_GALLERY_MAP     "map,x,y" (default prt_fild08,200,200: grass, nothing nearby)
--   RAGEXE_GALLERY_JOB     the player's job (default 0, Novice)
--
-- For every ID it prints, and flushes, "gallery: start <id>" before the packet and then
-- "gallery: done <id> alive=<ids> objects=<n> left=<n>": the effect IDs still attached to the
-- player at the last capture, the world's objects then (minus those before the launch), and
-- the objects still there after clearing and settling. A run that dies leaves a start without
-- a done, which names the ID it died on.

local function env(name, default)
    local v = os.getenv(name)
    if v == nil or v == "" then return default end
    return v
end

local function u16(v) return string.char(v % 256, math.floor(v / 256) % 256) end
local function u32(v) return u16(v % 65536) .. u16(math.floor(v / 65536)) end

local AID = 2000000                     -- SCENE_AID, the player
local function notify_effect2(id) return u16(0x1f3) .. u32(AID) .. u32(id) end
local function clear_effect(id) return u16(0xb0d) .. u32(AID) .. u32(id) end

local function numbers(spec)
    local out = {}
    for part in string.gmatch(spec, "[^,%s]+") do
        local a, b = string.match(part, "^(%d+)%-(%d+)$")
        if a then
            for i = tonumber(a), tonumber(b) do out[#out + 1] = i end
        elseif string.match(part, "^%d+$") then
            out[#out + 1] = tonumber(part)
        else
            error("effect_gallery: not an ID or range: " .. part)
        end
    end
    return out
end

local ids = numbers(env("RAGEXE_GALLERY_IDS", "0-2384"))
local frames = numbers(env("RAGEXE_GALLERY_FRAMES", "1,8,16,32,64"))
local crop_spec = env("RAGEXE_GALLERY_CROP", "448x384")
local jpeg = tonumber(env("RAGEXE_GALLERY_JPEG", "85"))
local settle = tonumber(env("RAGEXE_GALLERY_SETTLE", "180"))
local job = tonumber(env("RAGEXE_GALLERY_JOB", "0"))
local map, map_x, map_y = string.match(env("RAGEXE_GALLERY_MAP", "prt_fild08,200,200"), "^([^,]+),(%d+),(%d+)$")

local shape = {jpeg = jpeg}
if crop_spec ~= "full" then
    local w, h = string.match(crop_spec, "^(%d+)x(%d+)$")
    if not w then error("effect_gallery: RAGEXE_GALLERY_CROP is not WxH or full: " .. crop_spec) end
    shape.w, shape.h = tonumber(w), tonumber(h)
end

local function say(line)
    io.stdout:write("gallery: ", line, "\n")
    io.stdout:flush()
end

-- Clears every effect attached to the player through ZC_CLEAR_EFFECT, as the map server would,
-- then waits until the world holds no more objects than `baseline` or `frames` pass.
local function clear_all(also, baseline, frames_left)
    local alive = scene.effects()
    local seen = {}
    for _, id in ipairs(alive) do
        if not seen[id] then seen[id] = true; scene.send(clear_effect(id)) end
    end
    if also and not seen[also] then scene.send(clear_effect(also)) end
    scene.wait(1)
    local _, objects = scene.effects()
    while objects > baseline and frames_left > 0 do
        scene.wait(1)
        frames_left = frames_left - 1
        _, objects = scene.effects()
    end
    return objects - baseline
end

scene.enter_map(map, tonumber(map_x), tonumber(map_y), {name = "Gallery", job = job, hair = 5, dir = 0})
scene.wait_ready()
scene.wait(30)
-- Effects the map entry put on the player (0xa4) go first, so that every ID starts alone.
local entry = scene.effects()
say("entry effects=" .. table.concat(entry, ","))
clear_all(nil, 0, 0)
scene.wait(settle)
local _, baseline = scene.effects()
say("baseline objects=" .. baseline)

for _, id in ipairs(ids) do
    say("start " .. id)
    scene.send(notify_effect2(id))
    local at = 0
    for _, f in ipairs(frames) do
        if f > at then scene.wait(f - at); at = f end
        scene.capture(string.format("e%05d_f%03d", id, f), shape)
    end
    local alive, objects = scene.effects()
    local left = clear_all(id, baseline, settle)
    say(string.format("done %d alive=%s objects=%d left=%d", id, table.concat(alive, ","), objects - baseline, left))
    if left > 0 then
        -- Something outlived the clear and the settling time; take the baseline from here so the
        -- next ID's numbers are its own.
        _, baseline = scene.effects()
    end
end

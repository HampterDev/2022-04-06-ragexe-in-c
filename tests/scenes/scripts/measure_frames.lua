-- scene.measure: the counters of each of the next frames drawn and shown, a line of JSON each,
-- while the script goes on. Twelve frames of Prontera with a Poring walking past, read back: twelve
-- lines, every counter in each, frames in order and drawn; a capture in between is an error, as it
-- would stall the GPU inside the measured stretch.
local function u16(v) return string.char(v % 256, math.floor(v / 256) % 256) end
local function u32(v) return u16(v % 65536) .. u16(math.floor(v / 65536)) end
local function move(aid, x0, y0, x1, y1)
    local data = string.char(math.floor(x0 / 4), (x0 % 4) * 64 + math.floor(y0 / 16), (y0 % 16) * 16
        + math.floor(x1 / 64), (x1 % 64) * 4 + math.floor(y1 / 256), y1 % 256, 0x88)
    return u16(0x86) .. u32(aid) .. data .. u32(scene.ticks())
end

scene.enter_map("prontera", 156, 186, {name = "Watcher", job = 4008, hair = 5})
scene.wait_ready()
scene.spawn{aid = 110000001, kind = "monster", job = 1002, x = 150, y = 188}
scene.wait(5)
scene.send(move(110000001, 150, 188, 162, 188))
scene.measure(12, "measure_frames")
if scene.measuring() ~= 12 then
    scene.fail("measuring() answers " .. scene.measuring() .. " frames to come, not 12")
end
if pcall(scene.capture, "measure_frames_too_early") then
    scene.fail("a capture inside a measured stretch went through")
end
if pcall(scene.measure, 5, "measure_frames_twice") then
    scene.fail("a second measure inside a measured stretch went through")
end
scene.wait(20)
if scene.measuring() ~= 0 then
    scene.fail("measuring() answers " .. scene.measuring() .. " frames to come after the stretch")
end

local keys = {"frame", "frame_ms", "script_ms", "objects", "draws", "indirect_draws", "draw_calls", "primitives",
              "modes_sent", "modes_skipped", "texture_binds", "vertex_bytes", "instance_bytes", "texture_bytes", "draw_scene_ms", "view_render_ms",
              "present_ms", "packets_ms", "effects_ms", "actors_ms", "objects_render_ms", "effects_render_ms",
              "ui_ms", "atlas_ms", "texels_ms"}
local path = (os.getenv("RAGEXE_SCENE_OUT") or ".") .. "/measure_frames.frames.jsonl"
local lines, last = 0, -1
for line in io.lines(path) do
    lines = lines + 1
    for _, key in ipairs(keys) do
        if not line:find('"' .. key .. '": %-?[%d.]+') then
            scene.fail(path .. " line " .. lines .. " has no number for " .. key)
        end
    end
    local frame = tonumber(line:match('"frame": (%d+)'))
    local draws = tonumber(line:match('"draws": (%d+)'))
    local ms = tonumber(line:match('"frame_ms": ([%d.]+)'))
    if not frame or frame <= last then
        scene.fail(path .. " line " .. lines .. ": frame " .. tostring(frame) .. " after " .. last)
    end
    if not draws or draws == 0 or not ms or ms <= 0 then
        scene.fail(path .. " line " .. lines .. " is not of a frame drawn and shown")
    end
    last = frame or last
end
if lines ~= 12 then
    scene.fail(path .. " has " .. lines .. " lines, not 12")
end
scene.capture("measure_frames")

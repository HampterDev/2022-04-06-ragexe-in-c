-- What the motion scenes share (tests/scenes/README.md, "Motion"): the camera moved frame by frame
-- while each frame's shadow fit is logged, a line a frame:
--   fit <tag> <frame> <cascades> <nearest> <farthest> then per cascade <radius> <x> <y> <far>
-- (scene.shadow_fit: the range the slices were cut from, each square's half size, its centre across
-- the light and its far view depth), for motion_stats.py to count how often the squares change size.
-- With RAGEXE_SCENE_SHADOW_FIT=1 (run.py --shadow-fit) every 25th frame's fit is also checked
-- against render/shadow_fit.c (scene.shadow_fit_check).
local M = {}

local checking = os.getenv("RAGEXE_SCENE_SHADOW_FIT")

-- into the map at x, y with the free camera distance and latitude given, as Prontera's street is
-- seen by default (488.8, -55) or a field from further and flatter (800, -40); then still a while
function M.enter(map, x, y, distance, latitude)
    scene.enter_map(map, x, y, {name = "Watcher", job = 0, hair = 5, dir = 4, distance = distance,
                                latitude = latitude})
    scene.wait_ready()
    scene.wait(map == "prontera" and 60 or 400)
end

function M.log(tag)
    local f = scene.shadow_fit()
    if not f then
        print(string.format("fit %s %d none", tag, scene.frame()))
        return
    end
    local parts = {}
    for k = 1, 4 do
        parts[#parts + 1] = string.format("%.9g %.9g %.9g %.9g", f.radius[k], f.x[k], f.y[k], f.far[k])
    end
    print(string.format("fit %s %d %d %.9g %.9g %s", tag, scene.frame(), f.cascades, f.nearest, f.farthest,
                        table.concat(parts, " ")))
    if checking and scene.frame() % 25 == 0 then
        scene.shadow_fit_check(tag .. "_" .. scene.frame())
    end
end

-- n frames, a line each, step(i) before each frame
function M.run(tag, n, step)
    for i = 1, n do
        if step then
            step(i)
        end
        scene.wait(1)
        M.log(tag)
    end
end

local function u16(v) return string.char(v % 256, math.floor(v / 256) % 256) end
local function u32(v) return u16(v % 65536) .. u16(math.floor(v / 65536)) end

-- ZC_NOTIFY_PLAYERMOVE: the player walks from x0, y0 to x1, y1 from the given tick
local function move(x0, y0, x1, y1, tick)
    local d = string.char(math.floor(x0 / 4) % 256, (x0 % 4) * 64 + math.floor(y0 / 16) % 64,
                          (y0 % 16) * 16 + math.floor(x1 / 64) % 16, (x1 % 64) * 4 + math.floor(y1 / 256) % 4,
                          y1 % 256, 0x88)
    scene.send(u16(0x87) .. u32(tick) .. d)
end

-- the player walks the legs of a path ({x0, y0, x1, y1} each), n frames a leg
function M.walk(tag, path, n)
    local tick, start = 100000000, scene.frame()
    for _, p in ipairs(path) do
        move(p[1], p[2], p[3], p[4], tick + (scene.frame() - start) * 16)
        M.run(tag, n)
    end
end

-- a right drag with a key held (0 for none): dx, dy pixels a frame for n frames, logged with tag
function M.drag(tag, vk, scan, x, y, dx, dy, n)
    if vk ~= 0 then
        scene.key_event(vk, scan, true)
    end
    scene.mouse(x, y)
    scene.button(x, y, true, "right")
    scene.wait(2)
    M.run(tag, n, function()
        x = x + dx
        y = y + dy
        scene.mouse(x, y)
    end)
    scene.button(x, y, false, "right")
    if vk ~= 0 then
        scene.key_event(vk, scan, false)
    end
    scene.wait(2)
end

M.CONTROL, M.CONTROL_SCAN = 0x11, 0x1d
M.SHIFT, M.SHIFT_SCAN = 0x10, 0x2a

return M

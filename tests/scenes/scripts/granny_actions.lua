-- What granny_models does not show of the Granny (3D) actors: several of one model (the device
-- draws a mesh of all of them as one instanced draw), turned to each of the eight directions, one
-- walking (ZC_NOTIFY_MOVE) and two attacking the player (ZC_NOTIFY_ACT), and others at the
-- screen's edges, partly off it (the right one behind a roof). Two captures half a second apart
-- show them in other poses.
local function u16(v) return string.char(v % 256, math.floor(v / 256) % 256) end
local function u32(v) return u16(v % 65536) .. u16(math.floor(v / 65536)) end
local player = 2000000

-- a move from (x0, y0) to (x1, y1), each cell's centre (packets/packet_data.c, packet_decode_move_data)
local function move(aid, x0, y0, x1, y1)
    local data = string.char(math.floor(x0 / 4), (x0 % 4) * 64 + math.floor(y0 / 16),
        (y0 % 16) * 16 + math.floor(x1 / 64), (x1 % 64) * 4 + math.floor(y1 / 256), y1 % 256, 0x88)
    return u16(0x86) .. u32(aid) .. data .. u32(0)
end

local function act(src, target)
    return u16(0x8a) .. u32(src) .. u32(target) .. u32(0) .. u32(400) .. u32(200) .. u16(0) .. u16(1)
        .. string.char(0) .. u16(0)
end

scene.enter_map("prontera", 156, 180, {name = "Watcher", job = 4008, hair = 5})
scene.wait_ready()
-- eight guardians of one model, each turned another way
for dir = 0, 7 do
    scene.spawn{aid = 110005000 + dir, kind = "monster", job = 1286, x = 148 + 2 * dir, y = 186, dir = dir}
end
-- the screen's edges: left, right (behind a roof), top and bottom, partly off it
local edges = {{1287, 136, 181}, {1285, 177, 190}, {1287, 158, 196}, {1285, 154, 165}}
for i, m in ipairs(edges) do
    scene.spawn{aid = 110005100 + i, kind = "monster", job = m[1], x = m[2], y = m[3], dir = i}
end
-- one that walks, two that attack the player
scene.spawn{aid = 110005201, kind = "monster", job = 1285, x = 150, y = 176, dir = 2}
scene.spawn{aid = 110005202, kind = "monster", job = 1287, x = 160, y = 177, dir = 6}
scene.spawn{aid = 110005203, kind = "monster", job = 1286, x = 153, y = 181, dir = 4}
scene.wait(30)
scene.send(move(110005201, 150, 176, 162, 176))
scene.send(act(110005202, player))
scene.send(act(110005203, player))
scene.wait(20)
scene.capture("granny_actions")
scene.wait(30)
scene.capture("granny_actions_later")

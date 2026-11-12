-- The screen wobble: the player's status 0x108 (ZC_MSG_STATE_CHANGE) shakes the frame for eight
-- frames, rows copied over the rows above them and moved right by random multiples of 4 pixels,
-- with the period growing from 2 to 10 rows and shrinking to 4. Every frame of it is captured, and
-- the one after it.
local function u16(v) return string.char(v % 256, math.floor(v / 256) % 256) end
local function u32(v) return u16(v % 65536) .. u16(math.floor(v / 65536)) end

scene.enter_map("prontera", 156, 186, {name = "Watcher", job = 4008, hair = 5})
scene.wait_ready()
scene.send(u16(0x196) .. u16(0x108) .. u32(2000000) .. string.char(1))
for frame = 1, 9 do
    scene.wait(1)
    scene.capture("screen_wobble_" .. frame)
end

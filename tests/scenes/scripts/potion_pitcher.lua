-- Potion Pitcher (skill 231, ZC_USE_SKILL2) thrown by a creator at another player and at himself.
-- A bottle thrown at its own cell has a flight of no frames; the 64-bit client once turned that into
-- a sine table index 8 GB out of the table and crashed on it.
local function u16(v) return string.char(v % 256, math.floor(v / 256) % 256) end
local function u32(v) return u16(v % 65536) .. u16(math.floor(v / 65536)) end
local function use_skill(skill, src, dst, level)
    return u16(0x9cb) .. u16(skill) .. u32(level) .. u32(dst) .. u32(src) .. string.char(1)
end

scene.enter_map("prontera", 156, 180, {name = "Watcher", job = 4008, hair = 5})
scene.wait_ready()
scene.spawn{aid = 2000101, kind = "player", job = 4019, x = 152, y = 186, dir = 6, sex = 1, hair = 3, weapon = 8,
            shield = 4, option = 0x400, name = "Pitcher"}
scene.spawn{aid = 2000102, kind = "player", job = 4008, x = 159, y = 186, dir = 2, sex = 0, hair = 7, weapon = 4,
            name = "Catcher"}
scene.wait(10)
scene.send(use_skill(231, 2000101, 2000102, 1500))
scene.send(use_skill(231, 2000101, 2000101, 1500))
scene.wait(12)
scene.capture("potion_pitcher")

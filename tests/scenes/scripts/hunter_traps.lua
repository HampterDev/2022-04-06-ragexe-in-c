-- Hunter traps as the map server places them (ZC_SKILL_ENTRY5): blast mine, skid trap, ankle snare,
-- land mine, shockwave, sandman, flasher, freezing trap, claymore and talkie box.
local function u16(v) return string.char(v % 256, math.floor(v / 256) % 256) end
local function u32(v) return u16(v % 65536) .. u16(math.floor(v / 65536)) end
local function trap(id, unit, x, y)
    return u16(0x9ca) .. u16(23) .. u32(id) .. u32(2000000) .. u16(x) .. u16(y) .. u32(unit)
        .. string.char(0, 1, 5)
end

scene.enter_map("prontera", 156, 180, {name = "Watcher", job = 4008, hair = 5})
scene.wait_ready()
local units = {0x8f, 0x90, 0x91, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99}
for i, unit in ipairs(units) do
    scene.send(trap(120000000 + i, unit, 147 + (i - 1) % 5 * 4, 186 - math.floor((i - 1) / 5) * 4))
end
scene.wait(60)
scene.capture("hunter_traps")

-- What the rest of the set does not show of the quads the device draws as instances
-- (render/gfx.h, gfx_draw_quad): damage numbers over actors (ZC_NOTIFY_ACT), STR effects of both
-- kinds (ZC_NOTIFY_EFFECT2: Lightning, a rag effect, and Storm Gust, an ez2str one), Evil Land's
-- cells of two textures (the skill unit 0xc7, ZC_SKILL_ENTRY5), and then the same with the screen
-- flipped (the player's status 0x22), which turns every sprite and STR quad about its anchor.
local function u16(v) return string.char(v % 256, math.floor(v / 256) % 256) end
local function u32(v) return u16(v % 65536) .. u16(math.floor(v / 65536)) end
local player = 2000000

local function act(src, target, damage, action)
    return u16(0x8a) .. u32(src) .. u32(target) .. u32(0) .. u32(200) .. u32(200) .. u16(damage) .. u16(1)
        .. string.char(action) .. u16(0)
end

local function effect(aid, id)
    return u16(0x1f3) .. u32(aid) .. u32(id)
end

local function unit(id, kind, x, y, range)
    return u16(0x9ca) .. u16(23) .. u32(id) .. u32(player) .. u16(x) .. u16(y) .. u32(kind)
        .. string.char(range, 1, 5)
end

scene.enter_map("prontera", 156, 186, {name = "Watcher", job = 4008, hair = 5})
scene.wait_ready()
scene.spawn{aid = 110000001, kind = "monster", job = 1002, x = 152, y = 188}
scene.spawn{aid = 110000002, kind = "monster", job = 1113, x = 160, y = 188}
scene.spawn{aid = 110000003, kind = "monster", job = 1031, x = 156, y = 191}
scene.wait(10)
scene.send(act(player, 110000001, 1234, 0))
scene.send(act(player, 110000002, 54321, 10))
scene.send(act(player, 110000003, 0, 0))
scene.send(effect(110000001, 29))
scene.send(effect(110000002, 89))
scene.send(unit(120000001, 0xc7, 156, 183, 2))
scene.wait(20)
scene.capture("effects_instances")
scene.send(u16(0x196) .. u16(0x22) .. u32(player) .. string.char(1))
scene.send(act(player, 110000003, 4321, 0))
scene.send(effect(110000003, 29))
scene.wait(12)
scene.capture("effects_flipped")

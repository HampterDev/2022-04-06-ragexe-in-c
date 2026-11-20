-- Arriving in a crowd at war: the player warps from outside (prt_gld) into Kriemhild's Emperium room
-- (ZC_NPCACK_MAPMOVE) during the War of Emperium, and once the castle has loaded the fight of
-- woe_emperium.lua (113 players of three guilds, the Emperium and its guardians, their ground cells)
-- appears at once and goes on, so every sprite and emblem is new. The measured stretch, 300 frames,
-- starts with the frame they appear in; what it measures is the hitch of the first frames under
-- fire. The camera turns and zooms.
local load = dofile(scene.dir .. "lib/load.lua")
load.enter("prt_gld", 134, 65, {name = "Watcher", job = 4009, hair = 5, hair_color = 3, level = 99, weapon = 8,
                                head_top = 41, dir = 0})
scene.send(load.packet.warp("prtg_cas01", 195, 194))
scene.wait(5)
scene.wait_ready()
local build = dofile(scene.dir .. "lib/woe.lua")
local camera = load.camera()
local step
load.measured("arrive_castle", function(f)
    if f == 1 then
        step = build(load, "emperium")
    end
    step()
    camera(f)
end, {warm = 0, frames = 300})

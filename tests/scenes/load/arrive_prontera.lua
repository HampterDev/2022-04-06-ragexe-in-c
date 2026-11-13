-- Arriving in a crowd: the player, on the south field (prt_fild08), warps into Prontera
-- (ZC_NPCACK_MAPMOVE, as the map server sends it when a player steps into the field's warp), and
-- once the map has loaded the town of town_prontera.lua (180 players, 20 pets, 60 shop signs, 15
-- chat rooms) appears at once, as the map server sends everyone in sight on arrival, so every sprite
-- is new: the sprite atlas decodes and uploads all of them. The measured stretch, 300 frames, starts
-- with the frame they appear in; what it measures is the hitch of the first frames. The camera turns
-- and zooms.
local load = dofile(scene.dir .. "lib/load.lua")
load.enter("prt_fild08", 170, 375, {name = "Watcher", job = 4012, hair = 5, hair_color = 3, level = 99,
                                    weapon = 11, head_top = 41, dir = 0})
scene.send(load.packet.warp("prontera", 156, 160))
scene.wait(5)
scene.wait_ready()
local build = dofile(scene.dir .. "lib/town.lua")
local camera = load.camera()
local step
load.measured("arrive_prontera", function(f)
    if f == 1 then
        step = build(load, "prontera")
    end
    step()
    camera(f)
end, {warm = 0, frames = 300})

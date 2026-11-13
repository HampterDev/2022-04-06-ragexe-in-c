-- A busy Prontera (lib/town.lua): 180 players on and around the main street south of the fountain,
-- 60 of them vending in four rows with their signs, 15 under chat rooms, 70 walking up and down the
-- street (20 with their pets), 35 standing about; carts, mado gears, dragons, Peco Pecos, falcons,
-- guild emblems, long names, chat over their heads and emotions. The player, a sniper, stands in
-- the street; the camera turns and zooms.
local load = dofile(scene.dir .. "lib/load.lua")
load.enter("prontera", 156, 160, {name = "Watcher", job = 4012, hair = 5, hair_color = 3, level = 99, weapon = 11,
                                  head_top = 41, dir = 0})
local step = dofile(scene.dir .. "lib/town.lua")(load, "prontera")
local camera = load.camera()
load.measured("town_prontera", function(f)
    step()
    camera(f)
end)

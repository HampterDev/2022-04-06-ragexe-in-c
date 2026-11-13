-- War of Emperium in Kriemhild's Emperium room (prtg_cas01): 113 players of three guilds with emblems
-- (lib/woe.lua), the Emperium and its three guardians (Granny models), the Emperium hit all the time
-- and breaking every 400 frames. The player is a high priest of the defenders beside the Emperium;
-- the camera turns and zooms.
local load = dofile(scene.dir .. "lib/load.lua")
load.enter("prtg_cas01", 195, 194, {name = "Watcher", job = 4009, hair = 5, hair_color = 3, level = 99, weapon = 8,
                                    head_top = 41, dir = 0})
local step = dofile(scene.dir .. "lib/woe.lua")(load, "emperium")
local camera = load.camera()
load.measured("woe_emperium", function(f)
    step()
    camera(f)
end)

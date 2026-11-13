-- War of Emperium at Kriemhild's last choke (prtg_cas01): the defenders hold the portal to the
-- Emperium room (109,163), the attackers of two guilds push down from the room's north-west, 113
-- players in all (lib/woe.lua) under Storm Gusts, Lords of Vermilion, Meteor Storms, Land
-- Protectors and songs. The player is a high priest among the defenders; the camera turns and zooms.
local load = dofile(scene.dir .. "lib/load.lua")
load.enter("prtg_cas01", 114, 165, {name = "Watcher", job = 4009, hair = 5, hair_color = 3, level = 99, weapon = 8,
                                    head_top = 41, dir = 1})
local step = dofile(scene.dir .. "lib/woe.lua")(load, "choke")
local camera = load.camera()
load.measured("woe_choke", function(f)
    step()
    camera(f)
end)

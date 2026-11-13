-- Christmas in Lutie (xmas): the town's snow (the client lets it fall on xmas and xmas_fild01 while
-- effects are on) over a crowd of lib/town.lua's "lutie": 100 players in Santa hats, 20 vending,
-- 6 chat rooms, 44 walking about (10 with pets), 30 standing. It adds weather to the busy town: the
-- snow's particles are stepped and drawn every frame on top of the crowd. The camera turns and zooms.
local load = dofile(scene.dir .. "lib/load.lua")
load.enter("xmas", 203, 131, {name = "Watcher", job = 4021, hair = 5, hair_color = 3, level = 99, weapon = 14,
                              head_top = 20, dir = 0})
local step = dofile(scene.dir .. "lib/town.lua")(load, "lutie")
local camera = load.camera()
load.measured("snow_lutie", function(f)
    step()
    camera(f)
end)

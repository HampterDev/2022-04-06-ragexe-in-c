-- Valkyrie Randgris in the Odin Shrine (odin_tem03) against a party of 23 (lib/mvp.lua): the boss
-- with her valkyries (1765, 1891), Earthquake (a 1 s casting bar) every 150 frames, Pulse Strike,
-- Thunderstorm, Lord of Vermilion, Holy Cross and Ganbantein in between; the party of third jobs
-- tanks, heals, buffs, plays songs and hits until Randgris dies right after the measured stretch
-- and the MVP is announced. The first capture is taken in the middle of an Earthquake's shaking,
-- before the measured stretch; the camera turns and zooms.
local load = dofile(scene.dir .. "lib/load.lua")
local S = load.SKILL
load.enter("odin_tem03", 331, 99, {name = "Watcher", job = 4063, hair = 5, hair_color = 3, level = 175, weapon = 8,
                                   head_top = 225, dir = 0})
local run = dofile(scene.dir .. "lib/mvp.lua")(load, {
    seed = 4004, job = 1751, x = 335, y = 103, quake_cast = 1000, quake_every = 150,
    slaves = {1765, 1891, 1891, 1891},
    skills = {{S.PULSESTRIKE, "area", 0, 7000}, {S.THUNDERSTORM, "ground", 1000, 3000}, {S.LOV, "ground", 1500, 4000},
              {S.HOLYCROSS, "single", 0, 8000}, {S.GANBANTEIN, "ground", 800, 0}},
    party = {{"royal_guard", 1}, {"arch_bishop", 3}, {"warlock", 3}, {"ranger", 3}, {"sura", 2},
             {"rune_knight", 3}, {"genetic", 2}, {"minstrel", 1}, {"wanderer", 1}, {"sorcerer", 1},
             {"guillotine", 2}, {"mechanic", 1}},
})
run("mvp_randgris")

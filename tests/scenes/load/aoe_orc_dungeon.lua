-- AoE levelling in the Orc Dungeon (orcsdun01): the player, a high wizard, with a pack of 50 orc
-- zombies and orc skeletons (and a few drainliars, chonchons and familiars) around them, casting
-- Storm Gust (a casting bar, then ten waves on the pack, most of it freezing) twice for every Lord
-- of Vermilion. The pack walks in and hits the player; monsters die under the waves and vanish, and
-- new ones walk in from the dark to keep the pack at 50. The camera turns and zooms.
local load = dofile(scene.dir .. "lib/load.lua")
local P, S = load.packet, load.SKILL
local ME = 2000000
local X, Y = 44, 72
load.enter("orcsdun01", X, Y, {name = "Watcher", job = 4010, hair = 5, hair_color = 3, level = 99, weapon = 10,
                               head_top = 36, dir = 0, hp = 12000, sp = 2400})
local F = load.fight(6006)
local rng = F.rng
local me = {aid = ME, x = X, y = Y, alive = true, busy = 0, side = "me", spec = {}}
local jobs = {1153, 1153, 1153, 1153, 1152, 1152, 1152, 1111, 1042, 1005}
local next_aid = 110040000
local frozen = {}

local function spawn_monster(near)
    next_aid = next_aid + 1
    local x, y = load.around(rng, X, Y, near and 6 or 11)
    local m = F.add({aid = next_aid, kind = "monster", job = rng.pick(jobs), x = x, y = y, dir = rng.int(0, 7),
                     max_hp = -1, hp = -1, speed = 200}, "mobs")
    m.hp = rng.int(9000, 15000)
    return m
end
for _ = 1, 50 do
    spawn_monster(true)
end

-- a monster hit by a wave: dead (and a new one on its way), or frozen by a Storm Gust's
local function hit_monster(m, damage, freezes)
    m.hp = m.hp - damage
    if m.hp <= 0 then
        frozen[m.aid] = nil
        F.die(m)
        F.at(rng.int(40, 120), function()
            local n = spawn_monster(false)
            F.walk(n, X + rng.int(-2, 2), Y + rng.int(-2, 2))
        end)
    elseif freezes and rng.chance(70) and not frozen[m.aid] then
        frozen[m.aid] = true
        scene.send(P.state(m.aid, 2, 0, 0))
        F.at(rng.int(90, 200), function()
            if m.alive and frozen[m.aid] then
                frozen[m.aid] = nil
                scene.send(P.state(m.aid, 0, 0, 0))
            end
        end)
    end
end

-- the player's casts: Storm Gust, Storm Gust, Lord of Vermilion, and again
local casts = 0
local function cast()
    casts = casts + 1
    local sg = casts % 3 ~= 0
    local skill, ms = sg and S.STORMGUST or S.LOV, sg and 1800 or 1500
    scene.send(P.cast(ME, 0, X, Y, skill, ms, sg and 1 or 4))
    F.at(ms / 16 + 1, function()
        scene.send(P.ground_skill(skill, ME, X, Y, 10))
        local waves, every, r = sg and 10 or 4, sg and 28 or 60, sg and 3 or 5
        for wave = 0, waves - 1 do
            F.at(wave * every + 1, function()
                for _, m in ipairs(F.within("mobs", X, Y, r)) do
                    local damage = rng.int(1800, 4200)
                    scene.send(P.skill_hit(skill, ME, m.aid, damage, 10, 1, 5))
                    hit_monster(m, damage, sg)
                end
            end)
        end
    end)
end

local camera = load.camera()
local next_cast = 10
local function tick(f)
    F.step()
    camera(f)
    if F.frame >= next_cast then
        cast()
        next_cast = F.frame + 290
    end
    -- the pack: walk in and hit the player, unless frozen
    for _, m in ipairs(F.sides.mobs) do
        if m.alive and not frozen[m.aid] and (F.frame + m.aid) % 50 == 0 then
            if math.abs(m.x - X) > 1 or math.abs(m.y - Y) > 1 then
                F.walk(m, X + rng.int(-1, 1), Y + rng.int(-1, 1))
            else
                F.hit(m, me, 250)
            end
        end
    end
end
load.measured("aoe_orc_dungeon", tick)

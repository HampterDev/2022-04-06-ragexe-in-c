-- The War of Emperium in Kriemhild (prtg_cas01) as a busy server's castle has it: a defending guild
-- of 36 and two attacking ones of 48 and 29, 113 players of the jobs WoE is fought with, on the
-- castle's cells. Where:
--   "emperium": the Emperium room (the Emperium at 197,197 with its three guardians); attackers come
--               in through the portal at 202,183 and hit the Emperium, which breaks now and then;
--   "choke":    the room before it, held at the portal to the Emperium room (109,163); attackers
--               come down from the room's north-west.
-- Every player keeps doing what their job does in WoE: wizards Storm Gust, Lord of Vermilion,
-- Meteor Storm and Ice Wall; priests heal, buff, Sanctuary, Magnus Exorcismus, Pneuma and Safety
-- Wall; professors Land Protector; bards and dancers keep their songs' cells down; knights, crusaders,
-- champions (Asura Strike), assassin crosses, snipers and creators hit; people die and stand up.
-- Returns the fight's step, to be called once a frame.
return function(load, where)
    local P, S, U, E = load.packet, load.SKILL, load.UNIT, load.EFST
    local F = load.fight(where == "emperium" and 1001 or 2002)
    local rng = F.rng
    local emp_room = where == "emperium"

    -- where each side stands, and where the attackers come from
    local hold = emp_room and {x = 197, y = 193, r = 6} or {x = 113, y = 167, r = 5}
    local come = emp_room and {x = 200, y = 185, r = 4} or {x = 100, y = 178, r = 6}

    local defenders = {
        {"high_priest", 4}, {"arch_bishop", 3}, {"high_wizard", 3}, {"warlock", 3}, {"professor", 2},
        {"sorcerer", 1}, {"lord_knight", 3}, {"rune_knight", 2}, {"paladin", 2}, {"royal_guard", 2},
        {"champion", 2}, {"sura", 1}, {"clown", 1}, {"minstrel", 1}, {"gypsy", 1}, {"wanderer", 1},
        {"sniper", 2}, {"creator", 2},
    }
    local attackers = {
        {"high_priest", 4}, {"arch_bishop", 3}, {"high_wizard", 4}, {"warlock", 3}, {"professor", 2},
        {"lord_knight", 5}, {"rune_knight", 3}, {"champion", 4}, {"sura", 3}, {"assassin_cross", 4},
        {"guillotine", 2}, {"clown", 1}, {"gypsy", 1}, {"sniper", 3}, {"ranger", 2}, {"creator", 2},
        {"genetic", 1}, {"paladin", 1},
    }

    local aid = 2100000
    local function spawn_side(list, side, guild, at)
        for _, entry in ipairs(list) do
            for _ = 1, entry[2] do
                aid = aid + 1
                local x, y = load.around(rng, at.x, at.y, at.r)
                local a = F.add(load.player(rng, aid, entry[1], x, y, {guild = guild, emblem = 1}), side)
                a.class = entry[1]
                a.guild = guild
            end
        end
    end
    spawn_side(defenders, "def", 101, hold)
    -- the attackers of two guilds, mixed
    local half = {}
    for i, entry in ipairs(attackers) do
        half[i] = {entry[1], math.ceil(entry[2] / 2)}
    end
    spawn_side(half, "att", 202, come)
    spawn_side(attackers, "att", 303, come)

    local emp, guardians = nil, {}
    if emp_room then
        emp = F.add({aid = 110010001, kind = "monster", job = 1288, x = 197, y = 197, dir = 0, boss = 1,
                     guild = 101, emblem = 1, max_hp = 68430, hp = 41000}, "emp")
        local spots = {{193, 199, 1287}, {201, 199, 1287}, {197, 202, 1286}}
        for i, g in ipairs(spots) do
            guardians[i] = F.add({aid = 110010010 + i, kind = "monster", job = g[3], x = g[1], y = g[2], dir = 4,
                                  guild = 101, emblem = 1}, "def_guardian")
        end
    end

    local function enemy_of(a) return a.side == "def" and "att" or "def" end
    local function allies_of(a) return a.side end

    -- the songs and the Land Protectors, down from the start and laid again now and then
    local function lay_cells(a)
        if a.class == "clown" or a.class == "minstrel" then
            F.cells(a, S.BRAGI, U.BRAGI, a.x, a.y, load.square(3), 900, true)
        elseif a.class == "gypsy" or a.class == "wanderer" then
            F.cells(a, S.SERVICE, U.SERVICE, a.x, a.y, load.square(3), 900, true)
        elseif a.class == "professor" or a.class == "sorcerer" then
            local tx, ty = load.around(rng, a.side == "def" and hold.x or come.x, a.side == "def" and hold.y or come.y, 3)
            F.cells(a, S.LANDPROTECTOR, U.LANDPROTECTOR, tx, ty, load.square(5), 900)
        end
    end
    for _, a in ipairs(F.actors) do
        lay_cells(a)
        if a.class == "champion" or a.class == "sura" then
            scene.send(P.spirits(a.aid, 5))
            scene.send(P.status(a.aid, E.EXPLOSIONSPIRITS, true, 180000))
        elseif a.class == "lord_knight" or a.class == "rune_knight" then
            scene.send(P.status(a.aid, E.TWOHANDQUICKEN, true, 180000))
        end
    end

    local frozen = {}
    local function freeze(t)
        if not frozen[t.aid] and rng.chance(25) then
            frozen[t.aid] = true
            scene.send(P.state(t.aid, 2, 0, t.spec.option))
            F.at(rng.int(60, 150), function()
                frozen[t.aid] = nil
                scene.send(P.state(t.aid, 0, 0, t.spec.option))
            end)
        end
    end
    local function maybe_kill(t, percent)
        if t.side ~= "emp" and t.side ~= "def_guardian" and rng.chance(percent) then
            F.die(t, rng.int(240, 600))
        end
    end

    -- what each player does when its turn comes
    local function act(a)
        local foe_side = enemy_of(a)
        local foe = F.pick(foe_side, a.x, a.y, 9)
        local ally = F.pick(allies_of(a), a.x, a.y, 8)
        local c = a.class
        if c == "high_wizard" or c == "warlock" or c == "sorcerer" then
            local t = foe or F.pick(foe_side, a.x, a.y, 14)
            if not t then return end
            local roll = rng.int(1, 10)
            if roll <= 5 then
                F.ground(a, S.STORMGUST, t.x, t.y, {cast = 1800, hits = 10, every = 28, r = 2, dmg = 4000,
                         enemy = foe_side, element = 1, on_hit = function(v) freeze(v) maybe_kill(v, 1) end})
            elseif roll <= 8 then
                F.ground(a, S.LOV, t.x, t.y, {cast = 1500, hits = 4, every = 60, r = 5, dmg = 3000,
                         enemy = foe_side, element = 4, on_hit = function(v) maybe_kill(v, 1) end})
            elseif roll <= 9 then
                F.ground(a, S.METEOR, t.x, t.y, {cast = 1500, hits = 7, every = 18, r = 3, dmg = 2500,
                         enemy = foe_side, element = 3, on_hit = function(v) maybe_kill(v, 1) end})
            else
                F.cells(a, S.ICEWALL, U.ICEWALL, t.x, t.y + 1, load.CELLS.icewall, 300)
            end
        elseif c == "high_priest" or c == "arch_bishop" then
            local roll = rng.int(1, 20)
            if roll <= 8 and ally then
                F.skill(a, ally, S.HEAL, {level = rng.int(4000, 9000)})
            elseif roll <= 11 and ally then
                local buffs = {{S.BLESSING, E.BLESSING}, {S.INCAGI, E.INCAGI}, {S.KYRIE, E.KYRIE},
                               {S.ASSUMPTIO, E.ASSUMPTIO}}
                local b = rng.pick(buffs)
                F.skill(a, ally, b[1], {status = b[2], cast = b[1] == S.ASSUMPTIO and 700 or 0})
            elseif roll <= 13 then
                F.cells(a, S.SANCTUARY, U.SANCTUARY, a.x, a.y + 1, load.CELLS.sanctuary, 240)
                for wave = 1, 4 do
                    F.at(wave * 60, function()
                        for _, t in ipairs(F.within(a.side, a.x, a.y + 1, 2)) do
                            scene.send(P.use_skill(S.HEAL, a.aid, t.aid, 777))
                        end
                    end)
                end
            elseif roll <= 15 and foe then
                F.cells(a, S.MAGNUS, U.MAGNUS, foe.x, foe.y, load.CELLS.magnus, 300)
            elseif roll <= 17 and ally then
                F.cells(a, S.PNEUMA, U.PNEUMA, ally.x, ally.y, load.square(1), 600)
            elseif ally then
                F.cells(a, S.SAFETYWALL, U.SAFETYWALL, ally.x, ally.y, {{0, 0}}, 300)
            end
        elseif c == "lord_knight" or c == "rune_knight" or c == "royal_guard" or c == "paladin" then
            if not foe then return end
            F.walk(a, foe.x + rng.int(-1, 1), foe.y + rng.int(-1, 1))
            local skill = c == "paladin" and S.PRESSURE or c == "royal_guard" and S.HOLYCROSS
                          or rng.chance(50) and S.BOWLINGBASH or S.SPIRALPIERCE
            F.skill(a, foe, skill, {dmg = 9000, count = skill == S.SPIRALPIERCE and 5 or 2,
                    cast = skill == S.SPIRALPIERCE and 300 or 0, then_ = function(v) maybe_kill(v, 3) end})
        elseif c == "champion" or c == "sura" then
            local t = (emp and a.side == "att" and rng.chance(50) and emp.alive) and emp or foe
            if not t then return end
            F.walk(a, t.x + rng.int(-1, 1), t.y - 1)
            F.skill(a, t, S.ASURA, {dmg = 90000, cast = 1000, element = 0, then_ = function(v)
                scene.send(P.spirits(a.aid, 0))
                maybe_kill(v, 30)
                F.at(90, function() scene.send(P.spirits(a.aid, 5)) end)
            end})
        elseif c == "assassin_cross" or c == "guillotine" then
            local t = (emp and rng.chance(60) and emp.alive) and emp or foe
            if not t then return end
            F.walk(a, t.x + rng.int(-1, 1), t.y - 1)
            if rng.chance(40) and t ~= emp then
                scene.send(P.use_skill(S.METEORASSAULT, a.aid, a.aid, 10))
                for _, v in ipairs(F.within(enemy_of(a), a.x, a.y, 2)) do
                    scene.send(P.skill_hit(S.METEORASSAULT, a.aid, v.aid, rng.int(4000, 9000), 10, 1, 5))
                end
            else
                F.skill(a, t, S.SONICBLOW, {dmg = 12000, count = 8})
            end
        elseif c == "sniper" or c == "ranger" then
            if foe then
                F.skill(a, foe, S.SHARPSHOOTING, {dmg = 15000, cast = 500, then_ = function(v) maybe_kill(v, 4) end})
            end
        elseif c == "creator" or c == "genetic" then
            if foe and rng.chance(60) then
                F.skill(a, foe, S.ACIDDEMO, {dmg = 22000, count = 10, cast = 1000, then_ = function(v) maybe_kill(v, 4) end})
            elseif ally then
                F.skill(a, ally, S.POTIONPITCHER, {level = rng.int(2000, 4000)})
            end
        elseif c == "clown" or c == "minstrel" or c == "gypsy" or c == "wanderer" or c == "professor" then
            if rng.chance(30) then
                lay_cells(a)
            end
        end
    end

    -- the turn of each player: every 50 to 130 frames, each at its own phase
    for _, a in ipairs(F.actors) do
        if a.class then
            a.period = rng.int(50, 130)
            a.phase = rng.int(0, a.period - 1)
            a.walk_period = rng.int(90, 240)
        end
    end
    local lines = {"EMP!!!", "pneuma pls", "heal heal", "LP down", "push!", "SG incoming", "gogogo", "rez me"}

    local function step()
        F.step()
        local f = F.frame
        for _, a in ipairs(F.actors) do
            if a.class and a.alive then
                if (f + a.phase) % a.period == 0 then
                    act(a)
                elseif (f + a.phase) % a.walk_period == 0 then
                    if a.side == "def" then
                        F.wander(a, hold.x, hold.y, hold.r)
                    else
                        F.wander(a, emp_room and (emp.x + rng.int(-3, 3)) or (hold.x - 2), emp_room and (emp.y - 3)
                                 or (hold.y + 3), 4)
                    end
                end
            end
        end
        if emp_room then
            -- the attackers in reach hit the Emperium, the guardians hit the attackers
            if f % 6 == 0 and emp.alive then
                local hitter = F.pick("att", emp.x, emp.y, 2)
                if hitter then
                    F.hit(hitter, emp, 2500)
                end
            end
            if f % 9 == 0 then
                local g = guardians[f % 3 + 1]
                local t = F.pick("att", g.x, g.y, 6)
                if t then
                    F.hit(g, t, 1500)
                end
            end
            -- the Emperium breaks every 400 frames and is there again 90 frames later
            if f % 400 == 399 and emp.alive then
                F.die(emp)
                scene.send(P.broadcast("The [Kriemhild] castle has been conquered by the [Valhalla] guild."))
                F.at(90, function()
                    scene.spawn(emp.spec)
                    emp.alive = true
                end)
            end
        end
        if f % 37 == 0 then
            local a = F.actors[rng.int(1, #F.actors)]
            if a.class and a.alive then
                scene.say(a.aid, a.spec.name .. " : " .. rng.pick(lines))
            end
        end
    end
    return step
end

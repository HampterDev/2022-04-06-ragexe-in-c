-- An MVP fight as a party on a busy server fights it: the boss and its slaves against a party, the
-- boss using the skills rAthena's mob skill database gives it, Earthquake among them, and the party
-- tanking, healing, buffing and hitting it until it dies and the MVP is announced.
--
-- `boss` describes the fight:
--   seed, job, x, y          the boss's monster ID and cell
--   slaves = {job, ...}      monsters around it from the start
--   quake_cast               Earthquake's casting time in ms (Baphomet 2000, Randgris 1000, Ifrit 500)
--   quake_every              an Earthquake every that many frames besides the first; rAthena's delays
--                            between them are 20 to 30 s, the scenes cast it more often so that every
--                            measured stretch has several
--   skills = {{skill, kind, cast, dmg}, ...}  its other skills: kind "single" (on the tank), "area"
--                            (around itself, everyone within 7 cells), "ground" (on the party)
--   party = {{class, n}, ...} the players, around the boss at 3 to 7 cells
--   period = {min, max}      each player acts every min to max frames (default 45 to 110)
-- Earthquake is cast as rAthena casts NPC_EARTHQUAKE (skill 653): the casting bar, the skill on the
-- boss itself (ZC_USE_SKILL2), its ground unit (0xc6), three waves 300 ms apart of ZC_NOTIFY_SKILL2
-- on everyone within 6 cells, and the unit gone after 910 ms. Each of those hits launches effect 666
-- on the boss, which shakes the camera (view_set_quake) at its frames 0, 35 and 70.
-- Players die now and then; a priest's Resurrection stands one up, and the others stand up after a
-- while, as another player's Yggdrasil leaf would.
-- Returns the scene's run(name): 30 frames of fighting, an Earthquake, <name>_quake captured while
-- the camera shakes, then the measured stretch <name> (load.measured), all of it fighting; the boss
-- dies right after it and the MVP is announced before <name> is captured.
return function(load, boss)
    local P, S, E = load.packet, load.SKILL, load.EFST
    local F = load.fight(boss.seed)
    local rng = F.rng
    local B = F.add({aid = 110020001, kind = "monster", job = boss.job, x = boss.x, y = boss.y, dir = 4, boss = 1,
                     max_hp = 9000000, hp = 6300000, speed = 200}, "boss")
    local aid = 110020100
    for _, job in ipairs(boss.slaves) do
        aid = aid + 1
        local x, y = load.around(rng, boss.x, boss.y, 5)
        F.add({aid = aid, kind = "monster", job = job, x = x, y = y, dir = rng.int(0, 7), max_hp = -1, hp = -1}, "boss")
    end
    aid = 2500000
    local tank
    for _, entry in ipairs(boss.party) do
        for _ = 1, entry[2] do
            aid = aid + 1
            local x, y
            repeat
                x, y = load.around(rng, boss.x, boss.y, 7)
            until math.abs(x - boss.x) >= 2 or math.abs(y - boss.y) >= 2
            local a = F.add(load.player(rng, aid, entry[1], x, y, {guild = 404, emblem = 1}), "party")
            a.class = entry[1]
            if not tank and (a.class == "paladin" or a.class == "royal_guard") then
                tank = a
                F.walk(a, boss.x, boss.y - 1)
            end
        end
    end
    tank = tank or F.sides.party[1]

    -- the party's standing buffs and auras
    for _, a in ipairs(F.sides.party) do
        if a.class == "champion" or a.class == "sura" then
            scene.send(P.spirits(a.aid, 5))
            scene.send(P.status(a.aid, E.EXPLOSIONSPIRITS, true, 180000))
        end
        scene.send(P.status(a.aid, E.BLESSING, true, 240000))
        scene.send(P.status(a.aid, E.INCAGI, true, 240000))
    end
    scene.send(P.status(B.aid, E.POWERUP, true, 60000))

    local function maybe_kill(t, percent)
        if t.side == "party" and rng.chance(percent) then
            F.die(t, rng.int(150, 360))
        end
    end

    local function quake()
        local first = F.frame + math.floor(boss.quake_cast / 16) + 2
        scene.cast{src = B.aid, target = B.aid, skill = S.EARTHQUAKE, ms = boss.quake_cast}
        F.at(boss.quake_cast / 16 + 1, function()
            if not B.alive then return end
            scene.send(P.use_skill(S.EARTHQUAKE, B.aid, B.aid, 5))
            local unit = load.place(B.aid, load.UNIT.EARTHQUAKE, B.x, B.y, {{0, 0}}, 5)
            for wave = 0, 2 do
                F.at(wave * 19 + 1, function()
                    if not B.alive then return end
                    for _, t in ipairs(F.within("party", B.x, B.y, 6)) do
                        scene.send(P.skill_hit(S.EARTHQUAKE, B.aid, t.aid, rng.int(3000, 9000), -1, 1, 5))
                        maybe_kill(t, 2)
                    end
                end)
            end
            F.at(57, function() load.remove(unit) end)
        end)
        return first
    end

    local function boss_skill()
        local s = rng.pick(boss.skills)
        if s[2] == "single" then
            F.skill(B, tank, s[1], {cast = s[3], dmg = s[4], then_ = function(v) maybe_kill(v, 5) end})
        elseif s[2] == "area" then
            if s[3] > 0 then
                scene.cast{src = B.aid, target = B.aid, skill = s[1], ms = s[3]}
            end
            F.at(s[3] / 16 + 1, function()
                if not B.alive then return end
                scene.send(P.use_skill(s[1], B.aid, B.aid, 5))
                for _, t in ipairs(F.within("party", B.x, B.y, 7)) do
                    scene.send(P.skill_hit(s[1], B.aid, t.aid, rng.int(math.floor(s[4] * 0.7), s[4]), -1, 1, 5))
                    maybe_kill(t, 3)
                end
            end)
        else
            local t = F.pick("party", B.x, B.y, 9) or tank
            F.ground(B, s[1], t.x, t.y, {cast = s[3], hits = 4, every = 30, r = 3, dmg = s[4], enemy = "party",
                     on_hit = function(v) maybe_kill(v, 2) end})
        end
    end

    local function act(a)
        local c = a.class
        if not B.alive then
            F.wander(a, boss.x, boss.y, 6)
            return
        end
        if c == "high_priest" or c == "arch_bishop" then
            local roll = rng.int(1, 12)
            local ally = F.pick("party", a.x, a.y, 9) or tank
            if roll <= 6 then
                F.skill(a, rng.chance(60) and tank or ally, S.HEAL, {level = rng.int(6000, 12000)})
            elseif roll <= 9 then
                local buffs = {{S.BLESSING, E.BLESSING}, {S.INCAGI, E.INCAGI}, {S.KYRIE, E.KYRIE},
                               {S.ASSUMPTIO, E.ASSUMPTIO}}
                local b = rng.pick(buffs)
                F.skill(a, ally, b[1], {status = b[2], cast = b[1] == S.ASSUMPTIO and 700 or 0})
            elseif roll <= 10 then
                scene.send(P.use_skill(S.MAGNIFICAT, a.aid, a.aid, 5))
                for _, t in ipairs(F.within("party", a.x, a.y, 9)) do
                    scene.send(P.status(t.aid, E.MAGNIFICAT, true, 60000))
                end
            elseif roll <= 11 then
                F.cells(a, S.SANCTUARY, load.UNIT.SANCTUARY, tank.x, tank.y, load.CELLS.sanctuary, 240)
            else
                F.cells(a, S.PNEUMA, load.UNIT.PNEUMA, a.x, a.y, load.square(1), 600)
            end
            -- now and then one of the dead stands up on the priest's Resurrection
            for _, t in ipairs(F.sides.party) do
                if not t.alive and rng.chance(20) then
                    scene.send(P.use_skill(S.RESURRECTION, a.aid, t.aid, 4))
                    F.stand(t)
                    break
                end
            end
        elseif c == "high_wizard" or c == "warlock" then
            if rng.chance(60) then
                F.skill(a, B, S.JUPITEL, {cast = 800, dmg = 6000, count = 10, element = 4})
            else
                F.ground(a, S.LOV, B.x, B.y, {cast = 1500, hits = 4, every = 60, r = 5, dmg = 5000, enemy = "boss"})
            end
        elseif c == "sniper" or c == "ranger" then
            F.skill(a, B, S.SHARPSHOOTING, {cast = 600, dmg = 25000})
        elseif c == "champion" or c == "sura" then
            F.walk(a, B.x + rng.int(-1, 1), B.y + 1)
            F.skill(a, B, S.ASURA, {cast = 1000, dmg = 150000, then_ = function()
                scene.send(P.spirits(a.aid, 0))
                F.at(90, function() scene.send(P.spirits(a.aid, 5)) end)
            end})
        elseif c == "lord_knight" or c == "rune_knight" then
            F.walk(a, B.x + rng.int(-2, 2), B.y + rng.int(-2, 2))
            F.skill(a, B, S.SPIRALPIERCE, {cast = 300, dmg = 30000, count = 5})
        elseif c == "genetic" and rng.chance(40) then
            F.skill(a, B, S.CARTCANNON, {cast = 500, dmg = 25000})
        elseif c == "creator" or c == "genetic" then
            F.skill(a, B, S.ACIDDEMO, {cast = 500, dmg = 40000, count = 10})
        elseif c == "paladin" or c == "royal_guard" then
            F.skill(a, B, a == tank and S.HOLYCROSS or S.PRESSURE, {dmg = 8000, count = 2})
        elseif c == "clown" or c == "minstrel" then
            F.cells(a, S.BRAGI, load.UNIT.BRAGI, a.x, a.y, load.square(3), 600, true)
        elseif c == "gypsy" or c == "wanderer" then
            F.cells(a, S.SERVICE, load.UNIT.SERVICE, a.x, a.y, load.square(3), 600, true)
        else
            F.hit(a, B, 3000)
        end
    end

    local fastest, slowest = (boss.period or {45, 110})[1], (boss.period or {45, 110})[2]
    for _, a in ipairs(F.sides.party) do
        a.period = rng.int(fastest, slowest)
        a.phase = rng.int(0, a.period - 1)
    end
    local next_skill = 90

    -- the boss and its slaves die, and the server announces the MVP
    local function boss_dies()
        local mvp = F.sides.party[rng.int(1, #F.sides.party)]
        F.die(B)
        for _, m in ipairs(F.sides.boss) do
            F.die(m)
        end
        scene.send(P.mvp(mvp.aid))
        scene.send(P.broadcast(mvp.spec.name .. " has defeated the MVP!"))
    end

    local function step()
        F.step()
        local f = F.frame
        for _, a in ipairs(F.sides.party) do
            if a.alive and (f + a.phase) % a.period == 0 then
                act(a)
            end
        end
        if B.alive then
            if f % 40 == 0 then
                F.hit(B, tank, 7000)
            end
            if boss.quake_every > 0 and f % boss.quake_every == 0 then
                quake()
            elseif f >= next_skill then
                boss_skill()
                next_skill = f + rng.int(70, 140)
            end
            -- the slaves walk about and hit whoever is near
            for _, m in ipairs(F.sides.boss) do
                if m ~= B and m.alive and (f + m.aid) % 70 == 0 then
                    local t = F.pick("party", m.x, m.y, 6)
                    if t then
                        F.walk(m, t.x + 1, t.y)
                        F.hit(m, t, 1200)
                    else
                        F.wander(m, boss.x, boss.y, 6)
                    end
                end
            end
        end
    end

    -- <name>_quake is captured 12 frames after the first wave lands, the camera shaking
    return function(name)
        local camera = load.camera()
        local f = 0
        local function tick()
            f = f + 1
            step()
            camera(f)
        end
        for _ = 1, 30 do
            tick()
            scene.wait(1)
        end
        local first = quake()
        while F.frame < first + 12 do
            tick()
            scene.wait(1)
        end
        scene.capture(name .. "_quake")
        load.measured(name, tick, {warm = 60, after = boss_dies})
    end
end

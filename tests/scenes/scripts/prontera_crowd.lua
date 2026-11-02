-- Prontera's centre with monsters, NPCs and players of several jobs, some of them talking.
scene.enter_map("prontera", 156, 186, {name = "Watcher", job = 4008, hair = 5, hair_color = 3, dir = 0})
scene.wait_ready()
local monsters = {1002, 1031, 1063, 1113, 1039}
for i, job in ipairs(monsters) do
    scene.spawn{aid = 110000000 + i, kind = "monster", job = job, x = 148 + i * 3, y = 192, dir = 4}
end
local players = {{7, "Knight"}, {8, "Priest"}, {9, "Wizard"}, {10, "Blacksmith"}, {11, "Hunter"}, {12, "Assassin"}}
for i, p in ipairs(players) do
    scene.spawn{aid = 2000100 + i, kind = "player", job = p[1], name = p[2], x = 150 + i * 2, y = 182,
                hair = i, hair_color = i, sex = i % 2, dir = 0, level = 99}
end
scene.spawn{aid = 110001000, kind = "npc", job = 86, x = 160, y = 189, dir = 2, name = "Guide"}
scene.wait(60)
scene.say(2000101, "Knight : Anyone for Glast Heim?")
scene.say(2000103, "Wizard : Storm Gust ready.")
scene.wait(30)
scene.capture("prontera_crowd")

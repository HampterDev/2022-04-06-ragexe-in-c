-- A map whose RSW asks for two-pass alpha (1@tcamp), with its water and chimney smoke (sprite
-- particles) among sprites. Sprites, sprite particles, the water and the ground's cell quads take
-- their quads from one pool of the renderer, and a quad keeps what the last one in its place left
-- there (render/rp_pool.h, renderer_new_prim): on such a map a sprite leaves alpha_mode 0, which
-- sends the water, smoke or sprite that takes its place in a later frame to the list drawn once
-- after both alpha passes. The second picture has fewer sprites, so the places shift.
scene.enter_map("1@tcamp", 172, 186, {name = "Watcher", job = 4008, hair = 5, dir = 0})
scene.wait_ready()
local monsters = {1002, 1031, 1063, 1113, 1039, 1004, 1007}
for i, job in ipairs(monsters) do
    scene.spawn{aid = 110000000 + i, kind = "monster", job = job, x = 163 + i * 2, y = 190, dir = 4}
end
local players = {{7, "Knight"}, {8, "Priest"}, {9, "Wizard"}, {11, "Hunter"}}
for i, p in ipairs(players) do
    scene.spawn{aid = 2000100 + i, kind = "player", job = p[1], name = p[2], x = 166 + i * 3, y = 183,
                hair = i, hair_color = i, sex = i % 2, dir = 0, level = 99}
end
scene.wait(90)
scene.capture("two_pass_alpha")
for i = 1, 5 do
    scene.vanish(110000000 + i)
end
scene.wait(30)
scene.capture("two_pass_alpha_fewer")

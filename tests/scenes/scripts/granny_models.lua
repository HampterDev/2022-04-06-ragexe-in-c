-- The Granny (3D) models: the emperium, the three guardians, a treasure box and a guild flag.
scene.enter_map("prontera", 156, 180, {name = "Watcher", job = 4008, hair = 5})
scene.wait_ready()
local models = {
    {1288, "monster", 150, 186}, {1285, "monster", 154, 186}, {1286, "monster", 158, 186},
    {1287, "monster", 162, 186}, {1324, "monster", 152, 182}, {722, "npc", 160, 182},
}
for i, m in ipairs(models) do
    scene.spawn{aid = 110002000 + i, kind = m[2], job = m[1], x = m[3], y = m[4], dir = 4}
end
scene.wait(90)
scene.capture("granny_models")

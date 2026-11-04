-- Guild flags (job 722, a Granny model) with the emblems of two guilds, which the fake map server
-- sends when the client asks for them, and one flag without a guild. Two captures half a second
-- apart show the cloth waving.
scene.enter_map("prontera", 156, 180, {name = "Watcher", job = 4008, hair = 5})
scene.wait_ready()
scene.spawn{aid = 110004001, kind = "npc", job = 722, x = 150, y = 186, dir = 4, guild = 101, emblem = 1}
scene.spawn{aid = 110004002, kind = "npc", job = 722, x = 156, y = 186, dir = 4, guild = 202, emblem = 3}
scene.spawn{aid = 110004003, kind = "npc", job = 722, x = 162, y = 186, dir = 4}
scene.wait(90)
scene.capture("guild_flags")
scene.wait(30)
scene.capture("guild_flags_later")

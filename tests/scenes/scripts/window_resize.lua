-- The windowed client resized, as by dragging its frame: the game draws at each new size, the 3D
-- view keeps its height and shows more or less sideways, and the windows stay on screen; the
-- window cannot be made smaller than 1024 by 664.
scene.enter_map("iz_int03", 18, 26, {name = "Novice", hair = 1})
scene.wait_ready()
scene.click(150, 165)  -- skills
scene.wait(5)
scene.click(108, 165)  -- inventory
scene.wait(5)
scene.mouse(700, 600)
scene.wait(5)
scene.capture("window_resize_before")
scene.resize(1280, 720)
scene.wait(5)
scene.capture("window_resize_1280x720")
scene.resize(800, 600)  -- below the smallest size
scene.wait(5)
scene.capture("window_resize_smallest")
scene.resize(1920, 900)
scene.wait(5)
scene.capture("window_resize_wide")

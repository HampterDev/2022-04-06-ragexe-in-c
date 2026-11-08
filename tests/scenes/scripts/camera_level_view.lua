-- Prontera's main street with the unlocked camera at its closest zoom and flattest tilt, where the
-- eye comes down to a few units above the ground. Ground cells that reach the camera's plane used to
-- be drawn as screen-wide triangles sampled at one point, painting the street flat white, grey or
-- olive. The camera is first driven to the limits (Ctrl + right drag zooms, Shift + right drag
-- tilts), so the scene does not depend on the saved camera in SaveData.
scene.enter_map("prontera", 155, 161, {name = "Watcher", job = 0, hair = 5, dir = 0})
scene.wait_ready()
scene.wait(30)

local SHIFT, SHIFT_SCAN = 0x10, 0x2a
local CONTROL, CONTROL_SCAN = 0x11, 0x1d

local function drag(vk, scan, dy, steps, capture)
    local y = 300
    scene.key_event(vk, scan, true)
    scene.mouse(400, y)
    scene.button(400, y, true, "right")
    scene.wait(2)
    for i = 1, steps do
        y = y + dy
        scene.mouse(400, y)
        scene.wait(1)
        if capture and i == capture then
            scene.capture("camera_level_view_moving")
        end
    end
    scene.button(400, y, false, "right")
    scene.key_event(vk, scan, false)
    scene.wait(60)
end

drag(CONTROL, CONTROL_SCAN, 40, 40)
drag(SHIFT, SHIFT_SCAN, -40, 40, 30)
scene.capture("camera_level_view")

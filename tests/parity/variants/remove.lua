-- Removes entries from a table the file built. DV_ARGS (variants.py) is {table, keys}: table the global (or
-- "a.b" for a table inside one), keys the entries' keys separated by "/" (numbers where they read as one).
local t = _G
for part in string.gmatch(DV_ARGS[1], "[^%.]+") do
	if type(t) ~= "table" then break end
	t = t[tonumber(part) or part]
end
if type(t) == "table" then
	for key in string.gmatch(DV_ARGS[2], "[^/]+") do t[tonumber(key) or key] = nil end
end

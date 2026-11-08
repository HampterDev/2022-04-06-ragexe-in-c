-- Changes one key of every entry of a table the file built. DV_ARGS (variants.py) is
-- {table, key, how[, path]}:
--   table  the global the file fills, or "a.b" for a table inside one
--   key    the key changed in each entry (an entry being a table), several as "a/b", or "=" for
--          the entry itself
--   how    nil (the key removed), str ("x"), num (7), neg (-1), zero (0), tab ({}), empty (""),
--          med (40 characters), w100 (100), long (300), huge (4000), caret (unbalanced colour codes), tag (unbalanced
--          <ITEM>/<NAVI>/<URL> tags)
--   path   a dotted path inside each entry to the table holding the key ("" for the entry)
local t = _G
for part in string.gmatch(DV_ARGS[1], "[^%.]+") do
	if type(t) ~= "table" then break end
	t = t[tonumber(part) or part]
end
local how = DV_ARGS[3]
local values = {
	str = "x", num = 7, neg = -1, zero = 0, empty = "",
	med = string.rep("W", 40), w100 = string.rep("W", 100), long = string.rep("W", 300), huge = string.rep("W", 4000),
	caret = "^FF00^^ab^0000FFname^",
	tag = "<ITEM>Apple<INFO>512</INFO> <NAVI>[x]<INFO>prontera,1,1,0,000,0</INFO> <URL>link<INFO>http://x",
}
local function value()
	if how == "nil" then return nil end
	if how == "tab" then return {} end
	return values[how]
end
local function holder(entry)
	local h = entry
	if DV_ARGS[4] and DV_ARGS[4] ~= "" then
		for part in string.gmatch(DV_ARGS[4], "[^%.]+") do
			if type(h) ~= "table" then return nil end
			h = h[tonumber(part) or part]
		end
	end
	return type(h) == "table" and h or nil
end
if type(t) == "table" then
	local keys = {}
	for k in pairs(t) do keys[#keys + 1] = k end
	for _, k in ipairs(keys) do
		if DV_ARGS[2] == "=" then
			t[k] = value()
		else
			local h = holder(t[k])
			if h then
				for key in string.gmatch(DV_ARGS[2], "[^/]+") do h[tonumber(key) or key] = value() end
			end
		end
	end
end

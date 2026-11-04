local greeting = "\190\200\179\231\000suffix"
local function make_reader(value)
    return function(delta)
        return greeting, value + delta, true
    end
end
return make_reader(37)(5)

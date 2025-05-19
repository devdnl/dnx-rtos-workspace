local config_string = "{}"

if arg[1] then
    config_string = io.open(arg[1]):read()
else
    config_string = io.read()
end

if type(config_string) ~= "string" then
    print("Incorrect linker script configuration!")
    os.exit(-1)
end

local config = load("return "..config_string)()
if type(config) ~= "table" then
    print("Linker script configuration syntax error!")
    os.exit(-2)
end

-- sort all table entries that containing __MEM*__ pattern
local memory = {}
for k,v in pairs(config) do
    if k:match("__MEMORY_.*__") then
        local line = tostring(v:gsub('"', ''))
        if line ~= "" then
            table.insert(memory, line)
        end
    end
end
table.sort(memory)

-- compose linker script
print("SEARCH_DIR(.);")
print("SEARCH_DIR(src/cpu/"..config.__ARCH__.."/ld);")
print("MEMORY")
print("{")
for _,mem in ipairs(memory) do print("\t"..mem) end
print("}")
print("INCLUDE common.ld")

#!/usr/bin/env lua

--[[============================================================================
@file    optimize_bsp.lua

@author  Daniel Zorychta

@brief   This script remove unused arcitectures from BSP file.

@note    Copyright (C) 2025 Daniel Zorychta <daniel.zorychta@gmail.com>

         This program is free software; you can redistribute it and/or modify
         it under the terms of the GNU General Public License as published by
         the  Free Software  Foundation;  either version 2 of the License, or
         any later version.

         This  program  is  distributed  in the hope that  it will be useful,
         but  WITHOUT  ANY  WARRANTY;  without  even  the implied warranty of
         MERCHANTABILITY  or  FITNESS  FOR  A  PARTICULAR  PURPOSE.  See  the
         GNU General Public License for more details.

         You  should  have received a copy  of the GNU General Public License
         along  with  this  program;  if not,  write  to  the  Free  Software
         Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


==============================================================================]]

--------------------------------------------------------------------------------
-- @brief  Load table from file
-- @param  file         file to read from
-- @return On success table is returned, otherwise nil
--------------------------------------------------------------------------------
function LoadTable(file)
    if type(file) == "string" then
        local f = io.open(file, "rb")
        if f then
            local input = f:read("*all")
            f:close()

            if pcall(load('return '..input)) then
                local table = load('return '..input)()
                if type(table) == "table" then
                    return table
                end
            end
        end
    end

    return nil
end

--------------------------------------------------------------------------------
-- @brief  Save selected table to file
-- @param  tab          table to convert
-- @param  file         file to dump
-- @return On success true is returned, otherwise false
--------------------------------------------------------------------------------
function SaveTable(tab, file)

    local function spairs(t, order)
        -- collect the keys
        local keys = {}
        for k in pairs(t) do keys[#keys+1] = k end

        -- if order function given, sort by it by passing the table and keys a, b,
        -- otherwise just sort the keys
        if order then
            table.sort(keys, function(a,b) return order(t, a, b) end)
        else
            table.sort(keys)
        end

        -- return the iterator function
        local i = 0
        return function()
            i = i + 1
            if keys[i] then
                return keys[i], t[keys[i]]
            end
        end
    end

    local savedTables = {} -- used to record tables that have been saved, so that we do not go into an infinite recursion
    local outFuncs = {
        ['string']  = function(value) return string.format("%q",value) end;
        ['boolean'] = function(value) if (value) then return 'true' else return 'false' end end;
        ['number']  = function(value) return string.format('%d',value) end;
    }

    local outFuncsMeta = {
        __index = function(t,k) print('Invalid Type For SaveTable(): '..k) os.exit() end
    }

    local indent = ""

    setmetatable(outFuncs,outFuncsMeta)
    local tableOut = function(value)
        indent = indent.."\t"

        if (savedTables[value]) then
            print('There is a cyclical reference (table value referencing another table value) in this set.')
            os.exit()
        end

        local outValue = function(value)
            return outFuncs[type(value)](value)
        end

        local out = '{\n'

        for i,v in spairs(value) do
            out = out..indent..'['..outValue(i)..']='..outValue(v)..';\n'
        end

        savedTables[value] = true; --record that it has already been saved

        indent = indent:sub(1, -2)

        return out..indent..'}'
    end

    outFuncs['table'] = tableOut;

    if type(tab) == "table" and type(file) == "string" then
        local f = io.open(file, "wb")
        if f then
            f:write(tableOut(tab))
            f:close()
            return true
        end
    end

    return false
end

--------------------------------------------------------------------------------
-- @brief  Optimize table to selected architecture.
-- @param  tab      table to optimize
-- @param  arch     architecture name
--------------------------------------------------------------------------------
function Optimize(tab, arch)
    local arch_escaped = arch:gsub("([^%w])", "%%%1")

    for f, v in pairs(tab) do

        local arch_dir = f:match("arch/.*/.*")

        if arch_dir ~= nil then
            if arch_dir:match("noarch") then
                -- leave as is
            elseif arch_dir:match(arch) == nil then
                --print("Removing "..f.."...")
                tab[f] = nil
            end
        end

        --[[if tab[f] and next(v) == nil then
            print("Removing "..f.." (empty)...")
            tab[f] = nil
        end]]
    end
end

--------------------------------------------------------------------------------
-- @brief  Main loop.
-- @param  arg[1]   BSP file path
-- @param  arg[2]   architecture filter
--------------------------------------------------------------------------------
if #arg == 2 then
    local tab = LoadTable(arg[1])
    Optimize(tab, arg[2])
    SaveTable(tab, arg[1])
else
    print("Usage: ./optimize.lua <bsp-file> <architecture>")
end

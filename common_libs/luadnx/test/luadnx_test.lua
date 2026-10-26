--[[
File    luadnx_test.lua

Author  Daniel Zorychta

Brief   Test of luadnx modules (lfs, dnx.sys, dnx.proc).

        Usage (on target):  lua luadnx_test.lua [work_dir]
        work_dir must be writable (default: /tmp).

        MIT License

        Copyright (C) 2026 Daniel Zorychta <daniel.zorychta@gmail.com>

        Permission is hereby granted, free of charge, to any person obtaining
        a copy of this software and associated documentation files (the
        "Software"), to deal in the Software without restriction, including
        without limitation the rights to use, copy, modify, merge, publish,
        distribute, sublicense, and/or sell copies of the Software, and to
        permit persons to whom the Software is furnished to do so, subject to
        the following conditions:

        The above copyright notice and this permission notice shall be included
        in all copies or substantial portions of the Software.

        THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
        OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
        FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
        THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
        OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
        ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
        OTHER DEALINGS IN THE SOFTWARE.
]]

local work = arg and arg[1] or "/tmp"

local passed, failed = 0, 0

local function check(name, cond, ...)
    if cond then
        passed = passed + 1
        print("[ OK ] " .. name)
    else
        failed = failed + 1
        print("[FAIL] " .. name, ...)
    end
end

local function section(name)
    print("\n== " .. name .. " ==")
end

-------------------------------------------------------------------------------
section("modules")
local lfs  = require("lfs")
local dnx  = require("dnx")
local sys  = require("dnx.sys")
local proc = require("dnx.proc")

check("require lfs", type(lfs) == "table")
check("require dnx", type(dnx) == "table" and type(dnx._VERSION) == "string")
check("dnx.sys is the same module", dnx.sys == sys)
check("dnx.proc is the same module", dnx.proc == proc)
check("lfs version", type(lfs._VERSION) == "string", lfs._VERSION)

-------------------------------------------------------------------------------
section("lfs")
local cwd = lfs.currentdir()
check("currentdir", type(cwd) == "string" and #cwd > 0, cwd)

local base = work .. "/luadnx_test"
local file = base .. "/file.txt"

-- cleanup after previous run
os.remove(file)
lfs.rmdir(base)

local ok, err = lfs.mkdir(base)
check("mkdir", ok == true, err)

local ok2, err2, code2 = lfs.mkdir(base)
check("mkdir existing fails with errno", ok2 == nil and type(err2) == "string" and math.type(code2) == "integer",
      err2, code2)

local attr = lfs.attributes(base)
check("attributes(dir).mode", attr and attr.mode == "directory", attr and attr.mode)
check("attributes(dir, 'mode')", lfs.attributes(base, "mode") == "directory")

local f = assert(io.open(file, "w"))
f:write("hello dnx")
f:close()

attr = lfs.attributes(file)
check("attributes(file).mode", attr and attr.mode == "file", attr and attr.mode)
check("attributes(file).size", attr and attr.size == 9, attr and attr.size)
check("attributes(file).permissions", attr and type(attr.permissions) == "string" and #attr.permissions == 9,
      attr and attr.permissions)
check("attributes(file).modification", attr and math.type(attr.modification) == "integer")
check("attributes(file, 'size')", lfs.attributes(file, "size") == 9)

local t = {}
check("attributes(file, table)", lfs.attributes(file, t) == t and t.size == 9)

local bad, bad_msg, bad_code = lfs.attributes(base .. "/none")
check("attributes(missing) -> nil, msg, errno", bad == nil and type(bad_msg) == "string" and
      math.type(bad_code) == "integer", bad_msg)

check("attributes invalid name raises error", not pcall(lfs.attributes, file, "foo"))

check("touch existing", lfs.touch(file) == true)
check("touch new", lfs.touch(base .. "/new.txt") == true and lfs.attributes(base .. "/new.txt", "mode") == "file")
check("touch with times not supported", lfs.touch(file, 1, 1) == nil)

local names = {}
for name in lfs.dir(base) do
    names[name] = true
end
check("dir lists files", names["file.txt"] and names["new.txt"])

local iter, dir = lfs.dir(base)
local first = dir:next()
dir:close()
check("dir object next/close", type(first) == "string" and type(iter) == "function")
check("dir closed raises error", not pcall(dir.next, dir))
check("dir(missing) raises error", not pcall(lfs.dir, base .. "/none"))

check("chdir", lfs.chdir(base) == true)
check("currentdir after chdir", (lfs.currentdir() or ""):find("luadnx_test", 1, true) ~= nil, lfs.currentdir())
check("chdir back", lfs.chdir(cwd) == true)
check("chdir(missing) fails", lfs.chdir(base .. "/none") == nil)

check("rmdir(file) fails", lfs.rmdir(file) == nil)
check("rmdir(non empty) fails", lfs.rmdir(base) == nil)
os.remove(file)
os.remove(base .. "/new.txt")
check("rmdir", lfs.rmdir(base) == true)
check("dir removed", lfs.attributes(base) == nil)

check("setmode", lfs.setmode(io.stdout, "binary") == true)
check("lock not supported", lfs.lock(io.stdout) == nil)

-------------------------------------------------------------------------------
section("dnx.sys")
local up = sys.uptime()
check("uptime", math.type(up) == "integer" and up >= 0, up)

local info = sys.info()
check("info", type(info.os_name) == "string" and type(info.kernel_version) == "string" and
      type(info.platform) == "string")
print(string.format("  %s %s, %s %s, %s, host '%s', cores %s", info.os_name, info.os_version,
      info.kernel_name, info.kernel_version, info.platform, tostring(info.hostname), tostring(info.cpu_cores)))

check("hostname", type(sys.hostname()) == "string")

local mem = sys.memory()
check("memory", mem and mem.size > 0 and mem.free >= 0 and mem.used + mem.free == mem.size,
      mem and mem.size, mem and mem.used, mem and mem.free)

local regions = sys.regions()
check("regions", type(regions) == "table" and #regions > 0 and type(regions[1].name) == "string")
for _, r in ipairs(regions) do
    print(string.format("  %-10s %08Xh %8d %8d %8d %s%s%s", r.name, r.address, r.size, r.heap_size,
          r.heap_usage, r.dma and "D" or "-", r.cache and "C" or "-", r.noheap and "N" or "-"))
end

local load = sys.cpuload()
check("cpuload", load and type(load.avg1sec) == "number" and load.avg1sec >= 0 and load.avg1sec <= 100)

local log, last = sys.syslog()
check("syslog", type(log) == "table" and math.type(last) == "integer")
if #log > 0 then
    check("syslog entry", math.type(log[1].time) == "integer" and type(log[1].msg) == "string")
    local log2 = sys.syslog(last)
    check("syslog incremental", #log2 == 0, #log2)
end

local t0 = sys.uptime()
sys.msleep(100)
local t1 = sys.uptime()
check("msleep(100)", t1 - t0 >= 90, t1 - t0)
sys.sleep(0.05)
check("sleep(0.05)", sys.uptime() - t1 >= 40)

local panic = sys.panicinfo()
check("panicinfo", panic == nil or type(panic.cause) == "string")

-------------------------------------------------------------------------------
section("dnx.proc")
local pid = proc.getpid()
check("getpid", math.type(pid) == "integer" and pid > 0, pid)

local st = proc.stat(pid)
check("stat(self)", st and st.pid == pid and type(st.cmdline) == "string" and st.cmdline:find("lua") ~= nil,
      st and st.cmdline)
check("stat(self).threads", st and st.threads >= 1)

local list = proc.list()
local found = false
for _, p in ipairs(list) do
    if p.pid == pid then found = true end
end
check("list contains self", #list > 0 and found, #list)

local th = proc.threads(pid)
check("threads", #th >= 1 and math.type(th[1].tid) == "integer")
check("priority", math.type(proc.priority()) == "integer")

local out = work .. "/luadnx_test_out.txt"
os.remove(out)
local status = proc.run("echo luadnx", {stdout = out})
check("run with stdout redirection", status == 0, status)
local fo = io.open(out, "r")
local content = fo and fo:read("a") or ""
if fo then fo:close() end
check("redirected output", content:find("luadnx", 1, true) ~= nil, content)
os.remove(out)

local fh = assert(io.open(out, "w"))
status = proc.run("echo handle", {stdout = fh})
fh:close()
fo = io.open(out, "r")
content = fo and fo:read("a") or ""
if fo then fo:close() end
check("run with file handle as stdout", status == 0 and content:find("handle", 1, true) ~= nil, content)
os.remove(out)

local bad_pid, bad_err = proc.spawn("this_program_does_not_exist")
check("spawn(missing) fails", bad_pid == nil and type(bad_err) == "string", bad_err)

-- long running process: wait timeout and kill
local child = proc.spawn("lua -e \"require('dnx.sys').msleep(5000)\"", {stdin = false})
if child then
    local r, e, code = proc.wait(child, 100)
    check("wait timeout", r == nil and type(e) == "string", e, code)
    check("kill", proc.kill(child) == true)
else
    check("spawn lua", false)
end

check("spawn bad options raises error", not pcall(proc.spawn, "echo", {stdout = 12}))

-------------------------------------------------------------------------------
print(string.format("\nPassed: %d, failed: %d", passed, failed))
os.exit(failed == 0 and 0 or 1)

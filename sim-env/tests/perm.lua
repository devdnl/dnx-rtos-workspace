--[[----------------------------------------------------------------------------
  Users and file permissions test executed in dnx RTOS (lua program, system
  built with users and file permissions): accounts of password file (passwd),
  programs started as user (su), file permissions of owner, group and others
  on ramfs, flatfs and FAT (mount options uid, gid, umask), owner and mode of
  new files (umask 022), chmod and chown rights, root-only functions (modprobe,
  mount), kill of processes of other users and sudo (privileged program: real
  and effective user ID, password of service "sudo").
------------------------------------------------------------------------------]]

local T = require("testlib")
local lfs = require("lfs")
local dnx = require("dnx")
local test, expect = T.test, T.expect

local PASSWD = "/mnt/etc/passwd" -- AUTH_PASSWD_RW_FILE
local SHADOW = "/mnt/etc/shadow" -- AUTH_SHADOW_RW_FILE
local FLAT = "/mnt/permflat"
local FLATDEV = "/dev/sdb1"

local function write_file(path, data)
    local f = assert(io.open(path, "wb"))
    f:write(data)
    f:close()
end

-- command as root
local function run(cmd)
    local ok, out = T.run(cmd)
    if not ok then
        error(cmd .. ": " .. out, 2)
    end
    return out
end

-- command with output of stdout and stderr (optional input), returns success and output
local function spawn(cmd, input)
    local opts = { stdout = "/tmp/perm_out", stderr = "/tmp/perm_out" }
    if input then
        write_file("/tmp/perm_in", input)
        opts.stdin = "/tmp/perm_in"
    end
    local pid = assert(dnx.proc.spawn(cmd, opts))
    local status = dnx.proc.wait(pid, 30000)
    local f = io.open("/tmp/perm_out", "rb")
    local out = f and f:read("a") or ""
    if f then
        f:close()
    end
    os.remove("/tmp/perm_in")
    os.remove("/tmp/perm_out")
    return status == 0, out
end

-- command as root with standard input
local function run_input(cmd, input)
    local ok, out = spawn(cmd, input)
    if not ok then
        error(cmd .. ": " .. out, 2)
    end
end

-- command as user, returns success and output (stdout and stderr)
local function as(user, cmd, input)
    return spawn(string.format("su -c '%s' %s", cmd, user), input)
end

local function perms(path)
    local a = lfs.attributes(path)
    return a and a.permissions, a and a.uid, a and a.gid
end

test("Accounts and identity", function()
    os.remove(PASSWD)
    os.remove(SHADOW)
    run_input("passwd -s shell,sudo bob", "bobpw\n")
    run_input("passwd -u 2000 -G 2000 -s shell alice", "alicepw\n")

    local out = run("passwd -l")
    expect(out:find("bob%s+1000%s+1000%s+/%s+%-%s+shell") and out:find("alice%s+2000%s+2000%s+/%s+%-%s+shell"), "list: " .. out)
    expect(select(2, perms(SHADOW)) == 0 and perms(SHADOW) == "rw-------", "shadow file only for root")
    expect(perms(PASSWD) == "rw-r--r--", "public account file: " .. tostring(perms(PASSWD)))
    local f = io.open(PASSWD)
    local users = f and f:read("a") or ""
    if f then
        f:close()
    end
    expect(users == "bob:x:1000:1000:::\nalice:x:2000:2000:::\n", "account file: " .. users)

    local ok
    ok, out = as("bob", "id")
    expect(ok and out:find("uid=1000%(bob%) gid=1000"), "id: " .. out)
    out = run("whoami")
    expect(out:find("^root"), "whoami: " .. out)
    ok, out = as("bob", "whoami")
    expect(ok and out:find("^bob"), "whoami as user: " .. out)
    ok, out = spawn("su bob", "exit\n")
    expect(ok and out:find("bob@[%w_-]+:/"), "prompt of shell: " .. out)
    write_file("/tmp/perm_stat.lua", 'local st = require("dnx").proc.stat(require("dnx").proc.getpid()) print(st.uid, st.gid)')
    ok, out = as("alice", "lua /tmp/perm_stat.lua")
    expect(ok and out:find("^2000%s+2000"), "process statistics: " .. out)
    os.remove("/tmp/perm_stat.lua")
    ok, out = as("bob", "su -c id alice", "alicepw\n")
    expect(ok and out:find("uid=2000%(alice%) gid=2000"), "su as user: " .. out)
    ok, out = as("bob", "su -c id alice", "bobpw\n")
    expect(not ok and out:find("authentication failure"), "su with wrong password: " .. out)
    ok, out = as("bob", "su -c id", "bobpw\n")
    expect(not ok and out:find("authentication failure"), "su to root without password of root: " .. out)
    ok, out = as("nobody", "id")
    expect(not ok and out:find("unknown user"), "unknown user: " .. out)
end)

test("romfs: permissions of image", function()
    local p, uid, gid = perms("/etc/netmgr.conf")
    expect(p == "r--r--r--" and uid == 0 and gid == 0, "default mode: " .. tostring(p))
    p, uid, gid = perms("/etc/README")
    expect(p == "rw-r-----" and uid == 0 and gid == 10, "mode of selected file: " .. tostring(p) .. " " .. tostring(gid))
    local ok, out = as("bob", "cat /etc/README")
    expect(out:find("Permission denied"), "user reads file 0640: " .. out)
    ok, out = as("bob", "cat /etc/netmgr.conf")
    expect(ok, "user reads file 0444: " .. out)
    ok, out = as("bob", "ls /etc")
    expect(ok and out:find("netmgr.conf"), "user lists directory: " .. out)
end)

local function file_rights(dir, name)
    local root_file = dir .. "/root_" .. name
    local user_file = dir .. "/user_" .. name
    os.remove(root_file)
    os.remove(user_file)

    -- file of root (umask 022: rw-r--r--)
    write_file(root_file, "secret")
    local p, uid = perms(root_file)
    expect(p == "rw-r--r--" and uid == 0, "root file mode " .. tostring(p) .. " uid " .. tostring(uid))

    local ok, out = as("bob", "cat " .. root_file)
    expect(ok and out:find("secret"), "others can read: " .. out)
    ok, out = as("bob", "touch " .. root_file)
    expect(not ok or out:find("denied"), "others cannot write: " .. out)

    run("chmod 600 " .. root_file)
    ok, out = as("bob", "cat " .. root_file)
    expect(out:find("Permission denied") and not out:find("secret"), "mode 600 read by others: " .. out)
    ok, out = as("bob", "chmod 666 " .. root_file)
    expect(out:find("not permitted"), "chmod of other user file: " .. out)
    ok, out = as("bob", "chown 1000 " .. root_file)
    expect(out:find("not permitted"), "chown by user: " .. out)

    -- owner bob, group of alice (2000): rw-r----- (group reads, others nothing)
    run("chown bob:2000 " .. root_file)
    run("chmod 640 " .. root_file)
    ok, out = as("bob", "cat " .. root_file)
    expect(ok and out:find("secret"), "owner read: " .. out)
    ok, out = as("alice", "cat " .. root_file)
    expect(ok and out:find("secret"), "group read: " .. out)
    ok, out = as("alice", "touch " .. root_file)
    expect(out:find("Permission denied"), "group write: " .. out)
    run("chown 0:0 " .. root_file)
    ok, out = as("alice", "cat " .. root_file)
    expect(out:find("Permission denied"), "others read: " .. out)
    os.remove(root_file)

    -- new file of user: owner bob, mode rw-r--r--
    ok, out = as("bob", "touch " .. user_file)
    expect(ok, "user creates file: " .. out)
    p, uid = perms(user_file)
    local gid = select(3, perms(user_file))
    expect(uid == 1000 and gid == 1000 and p == "rw-r--r--", "user file " .. tostring(p) .. " " .. tostring(uid))
    ok, out = as("bob", "chmod 600 " .. user_file)
    expect(ok and perms(user_file) == "rw-------", "owner chmod: " .. out)
    ok, out = as("alice", "rm " .. user_file)
    expect(ok, "remove in writable directory: " .. out) -- directory rwxrwxrwx (no sticky bit)
end

test("ramfs: permissions of files", function()
    expect(perms("/tmp") == "rwxrwxrwx", "/tmp mode: " .. tostring(perms("/tmp")))
    file_rights("/tmp", "ramfs")
end)

test("ramfs: directory of root", function()
    lfs.mkdir("/tmp/rootdir")
    expect(perms("/tmp/rootdir") == "rwxr-xr-x", "directory mode")
    write_file("/tmp/rootdir/f", "x")
    local ok, out = as("bob", "touch /tmp/rootdir/new")
    expect(out:find("Permission denied") and not lfs.attributes("/tmp/rootdir/new"), "create: " .. out)
    ok, out = as("bob", "rm /tmp/rootdir/f")
    expect(out:find("Permission denied") and lfs.attributes("/tmp/rootdir/f"), "remove: " .. out)
    write_file("/tmp/perm_rename.lua", "local ok, msg = os.rename(arg[1], arg[2]) print(ok and 'RENAMED' or msg)")
    ok, out = as("bob", "lua /tmp/perm_rename.lua /tmp/rootdir/f /tmp/f")
    expect(out:find("denied") and lfs.attributes("/tmp/rootdir/f"), "rename: " .. out)
    os.remove("/tmp/perm_rename.lua")
    ok, out = as("bob", "ls /tmp/rootdir")
    expect(ok and out:find("f"), "read directory: " .. out)
    run("chmod 700 /tmp/rootdir")
    ok, out = as("bob", "ls /tmp/rootdir")
    expect(out:find("Permission denied"), "read directory 700: " .. out)
    os.remove("/tmp/rootdir/f")
    os.remove("/tmp/rootdir")
end)

test("flatfs: permissions stored in file system", function()
    run("mkflatfs " .. FLATDEV .. " 64")
    lfs.mkdir(FLAT)
    run("mount " .. FLATDEV .. " flatfs " .. FLAT)
    lfs.mkdir(FLAT .. "/pub")
    run("chmod 777 " .. FLAT .. "/pub")
    file_rights(FLAT .. "/pub", "flatfs")

    local ok, out = as("bob", "mkdir " .. FLAT .. "/pub/bobdir")
    expect(ok, "user mkdir: " .. out)
    as("bob", "touch " .. FLAT .. "/pub/bobdir/data")
    run("umount " .. FLAT)
    run("mount " .. FLATDEV .. " flatfs " .. FLAT)
    local p, uid = perms(FLAT .. "/pub/bobdir")
    expect(p == "rwxr-xr-x" and uid == 1000, "after remount: " .. tostring(p) .. " " .. tostring(uid))
    ok, out = as("alice", "touch " .. FLAT .. "/pub/bobdir/x")
    expect(out:find("Permission denied"), "write to directory of other user: " .. out)
    run("umount " .. FLAT)
    os.remove(FLAT)
end)

test("FAT: owner and permissions of mount options", function()
    if not lfs.attributes("/mnt/sda1") then
        io.write("(FAT disk not connected) ")
        return
    end

    run("umount /mnt/sda1")
    run("mount /dev/sda1 fatfs /mnt/sda1 uid=1000,gid=1000,umask=077")
    local p, uid = perms("/mnt/sda1")
    expect(p == "rwx------" and uid == 1000, "root directory: " .. tostring(p) .. " " .. tostring(uid))

    local ok, out = as("bob", "touch /mnt/sda1/perm_bob")
    expect(ok, "owner writes: " .. out)
    expect(perms("/mnt/sda1/perm_bob") == "rw-------", "file mode: " .. tostring(perms("/mnt/sda1/perm_bob")))
    ok, out = as("alice", "cat /mnt/sda1/perm_bob")
    expect(out:find("Permission denied"), "other user reads: " .. out)
    os.remove("/mnt/sda1/perm_bob")

    run("umount /mnt/sda1")
    run("mount /dev/sda1 fatfs /mnt/sda1")
    p, uid = perms("/mnt/sda1")
    expect(p == "rwxr-xr-x" and uid == 0, "default options: " .. tostring(p) .. " " .. tostring(uid))
    ok, out = as("bob", "touch /mnt/sda1/perm_bob")
    expect(out:find("Permission denied"), "user writes with default options: " .. out)

    -- directory listing of other user (directory path checked with slash at the end)
    lfs.mkdir("/mnt/sda1/perm_dir")
    write_file("/mnt/sda1/perm_dir/f", "x")
    ok, out = as("bob", "ls /mnt/sda1/perm_dir")
    expect(ok and out:find("total 1"), "user lists directory: " .. out)
    os.remove("/mnt/sda1/perm_dir/f")
    lfs.rmdir("/mnt/sda1/perm_dir")
end)

test("Root-only functions", function()
    local ok, out = as("bob", "modprobe -i ZERO 7 0 /dev/permzero")
    expect(out:find("not permitted") and not lfs.attributes("/dev/permzero"), "modprobe: " .. out)
    ok, out = as("bob", "mount ramfs none /tmp")
    expect(out:find("not permitted"), "mount: " .. out)
    ok, out = as("bob", "cat /dev/sdb")
    expect(out:find("Permission denied"), "device of root: " .. out)
    ok, out = as("bob", "dd if=/dev/zero of=/dev/null bs=4 count=1")
    expect(ok and not out:find("denied"), "public device: " .. out)
end)

test("Kill of processes", function()
    write_file("/tmp/perm_sleep.lua", "require('dnx').sys.msleep(20000)")
    write_file("/tmp/perm_kill.lua", [[
local dnx = require("dnx")
local ok, msg = dnx.proc.kill(tonumber(arg[1]))
print(ok and "ROOT KILLED" or ("ROOT NOT KILLED " .. tostring(msg)))
local own = assert(dnx.proc.spawn("lua /tmp/perm_sleep.lua"))
ok, msg = dnx.proc.kill(own)
dnx.proc.wait(own, 2000)
print(ok and "OWN KILLED" or ("OWN NOT KILLED " .. tostring(msg)))
]])

    local root_pid = assert(dnx.proc.spawn("lua /tmp/perm_sleep.lua"))
    local ok, out = as("bob", "lua /tmp/perm_kill.lua " .. root_pid)
    expect(out:find("ROOT NOT KILLED") and out:find("OWN KILLED"), "kill: " .. out)

    dnx.proc.kill(root_pid)
    dnx.proc.wait(root_pid, 2000)
    os.remove("/tmp/perm_kill.lua")
    os.remove("/tmp/perm_sleep.lua")
end)

test("sudo", function()
    local ok, out = as("bob", "sudo id", "bobpw\n")
    expect(ok and out:find("uid=0%(root%) gid=0"), "sudo id: " .. out)
    ok, out = as("bob", "sudo -u alice id", "bobpw\n")
    expect(ok and out:find("uid=2000%(alice%) gid=2000"), "sudo -u alice: " .. out)
    ok, out = as("bob", "sudo echo \"a  b\" c", "bobpw\n")
    expect(ok and out:find("a  b c"), "arguments: " .. out)
    ok, out = as("bob", "sudo whoami", "x\ny\nz\n")
    expect(not ok and out:find("Sorry, try again") and out:find("incorrect password"), "wrong password: " .. out)
    ok, out = as("alice", "sudo id", "alicepw\n")
    expect(not ok and out:find("not allowed"), "user without sudo: " .. out)
    out = run("sudo whoami")
    expect(out:find("^root"), "sudo of root: " .. out)

    write_file("/tmp/perm_sudo.lua", 'local st = require("dnx").proc.stat(require("dnx").proc.getpid()) print("UID", st.uid)')
    ok, out = as("bob", "sudo lua /tmp/perm_sudo.lua", "bobpw\n")
    expect(ok and out:find("UID%s+0"), "process of sudo: " .. out)
    os.remove("/tmp/perm_sudo.lua")
    as("bob", "touch /tmp/perm_sudo_file")
    ok, out = as("bob", "sudo chown 0 /tmp/perm_sudo_file", "bobpw\n")
    expect(ok and select(2, perms("/tmp/perm_sudo_file")) == 0, "root-only function by sudo: " .. out)
    os.remove("/tmp/perm_sudo_file")
end)

test("passwd: own password, home and shell", function()
    -- user changes own password (current password is checked)
    local ok, out = as("bob", "passwd", "bobpw\nnewpw\n")
    expect(ok, "own password: " .. out)
    ok, out = as("bob", "sudo id", "newpw\n")
    expect(ok and out:find("uid=0%(root%)"), "new password: " .. out)
    ok, out = as("bob", "passwd", "bad\nx\n")
    expect(not ok and out:find("wrong password"), "wrong current password: " .. out)
    ok, out = as("bob", "passwd alice", "newpw\nx\n")
    expect(not ok and out:find("Only root"), "password of other user: " .. out)
    ok, out = as("bob", "passwd -s sudo bob", "newpw\nx\n")
    expect(not ok and out:find("no password of these services"), "new service by user: " .. out)
    ok, out = as("bob", "passwd -u 0 bob", "")
    expect(not ok and out:find("Only root"), "account change by user: " .. out)
    ok, out = as("bob", "passwd", "newpw\nbobpw\n")
    expect(ok, "password restored: " .. out)

    -- account without password change: home and shell
    lfs.mkdir("/tmp/bobhome")
    run("passwd -H /tmp/bobhome -S id bob")
    local f = io.open(PASSWD)
    local file = f and f:read("a") or ""
    if f then
        f:close()
    end
    expect(file:find("bob:x:1000:1000::/tmp/bobhome:id\n", 1, true), "account file: " .. file)
    ok, out = spawn("su bob")
    expect(ok and out:find("uid=1000%(bob%)"), "shell of account: " .. out)
    ok, out = spawn("login", "bob\nbobpw\n")
    expect(ok and out:find("uid=1000%(bob%)"), "login starts shell of account: " .. out)
    ok, out = spawn("login -l", "bob\nbobpw\nbob\nbobpw\n")
    local _, sessions = out:gsub("uid=1000%(bob%)", "")
    expect(sessions == 2, "login -l: login prompt after end of session: " .. out)
    ok, out = spawn("login dsh", "bob\nwrong\n")
    expect(not ok and out:find("Login incorrect"), "login with wrong password: " .. out)
    ok, out = as("bob", "login", "bob\nbobpw\n")
    expect(not ok and out:find("use su"), "login started by user: " .. out)
    ok, out = spawn("su - -c dsh bob", "exit\n")
    expect(ok and out:find("bob@[%w_-]+:/tmp/bobhome"), "su - (home directory): " .. out)
    write_file("/tmp/bobhome/home_marker", "x")
    ok, out = spawn("su -c dsh bob", "cd /\ncd\nls\nexit\n")
    expect(ok and out:find("home_marker"), "cd to home directory: " .. out)
    os.remove("/tmp/bobhome/home_marker")
    ok, out = as("bob", "sudo id", "bobpw\n")
    expect(ok, "password kept: " .. out)
    run("passwd -H '' -S '' bob")
    lfs.rmdir("/tmp/bobhome")
end)

test("Close", function()
    os.remove(PASSWD)
    os.remove(SHADOW)
end)

T.summary()

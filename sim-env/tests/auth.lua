--[[----------------------------------------------------------------------------
  Authentication test executed in dnx RTOS (lua program) with board
  boards/test.lua (network is needed): random generator (/dev/urandom,
  getentropy() used by passwd), password file (passwd program, auth library),
  logins of telnetd (service "telnet", account with "shell" only is refused), ftpd (service "ftp") and mqtt_broker
  (service "mqtt") by nc connected to 127.0.0.1: no accounts, correct and wrong
  password, account without service, data sent after telnet password (passed
  to shell), user ID of telnet shell and ftp session (owner of created files),
  separate passwords of services, limit of failed logins, anonymous MQTT
  broker.
------------------------------------------------------------------------------]]

local T = require("testlib")
local dnx = require("dnx")
local lfs = require("lfs")
local test, expect = T.test, T.expect

local PASSWD = "/mnt/etc/passwd" -- AUTH_PASSWD_RW_FILE
local SHADOW = "/mnt/etc/shadow" -- AUTH_SHADOW_RW_FILE
local PASSWORD = "Tajne haslo 123"

local servers = {}

local function read_file(path)
    local f = io.open(path, "rb")
    if not f then
        return nil
    end
    local s = f:read("a")
    f:close()
    return s
end

local function write_file(path, data)
    local f = assert(io.open(path, "wb"))
    f:write(data)
    f:close()
end

-- owner of file (nil: no file)
local function owner(path)
    local attr = lfs.attributes(path)
    return attr and attr.uid
end

local function run(cmd, stdin)
    os.remove("/tmp/auth_out")
    local opts = { stdout = "/tmp/auth_out", stderr = "/tmp/auth_out" }
    if stdin then
        write_file("/tmp/auth_in", stdin)
        opts.stdin = "/tmp/auth_in"
    end
    local pid = assert(dnx.proc.spawn(cmd, opts))
    local status = dnx.proc.wait(pid, 30000)
    if not status then
        dnx.proc.kill(pid)
        dnx.proc.wait(pid, 1000)
    end
    return status == 0, read_file("/tmp/auth_out") or ""
end

local function server(cmd)
    local log = "/tmp/auth_srv" .. (#servers + 1)
    local pid = assert(dnx.proc.spawn(cmd, { stdout = log, stderr = log }))
    servers[#servers + 1] = pid
    dnx.sys.msleep(500)
end

-- send data to TCP port of the system, returns received data
local function session(port, data, opts)
    write_file("/tmp/auth_tx", data)
    os.remove("/tmp/auth_rx")
    run(string.format("nc -w %d %s -f /tmp/auth_tx -o /tmp/auth_rx 127.0.0.1 %d", opts and opts.wait or 3,
        opts and opts.count and ("-c " .. opts.count) or "", port))
    return read_file("/tmp/auth_rx") or ""
end

local function telnet(user, password, commands)
    return session(23, user .. "\r\n" .. password .. "\r\n" .. (commands or "exit\r\n"), { wait = 4 })
end

local function ftp(user, password, commands)
    return session(21, "USER " .. user .. "\r\nPASS " .. password .. "\r\nPWD\r\n" .. (commands or "") .. "QUIT\r\n")
end

-- MQTT 3.1.1 CONNECT, returns CONNACK return code (nil: no answer)
local function mqtt(port, user, password)
    local function str(s)
        return string.pack(">s2", s)
    end
    local flags = 0x02 | (user and 0x80 or 0) | (password and 0x40 or 0)
    local body = str("MQTT") .. string.char(4, flags) .. string.pack(">I2", 60) .. str("auth-test")
        .. (user and str(user) or "") .. (password and str(password) or "")
    local rx = session(port, string.char(0x10, #body) .. body, { count = 4 })
    if #rx == 4 and rx:byte(1) == 0x20 then
        return rx:byte(4)
    end
    return nil
end

test("Random generator", function()
    local f = assert(io.open("/dev/urandom", "rb"))
    local a, b = f:read(32), f:read(32)
    f:close()
    expect(a and b and #a == 32 and #b == 32, "read")
    expect(a ~= b, "the same data")
    local zeros = 0
    for i = 1, 32 do
        zeros = zeros + ((a:byte(i) == 0) and 1 or 0)
    end
    expect(zeros < 8, "too many zeros")
end)

test("No accounts", function()
    os.remove(PASSWD)
    os.remove(SHADOW)
    local ok, out = run("passwd -l")
    expect(not ok and out:find("No account file"), "passwd -l: " .. out)

    write_file("/tmp/auth_ftpd.conf", 'root-dir = "/tmp"\n')
    write_file("/tmp/auth_mqtt.conf", "port = 1883\n")
    write_file("/tmp/auth_mqtt_anon.conf", "port = 1884\nanonymous = true\n")
    server("telnetd")
    server("ftpd --config=/tmp/auth_ftpd.conf")
    server("mqtt_broker --config=/tmp/auth_mqtt.conf")
    server("mqtt_broker --config=/tmp/auth_mqtt_anon.conf")

    local out = telnet("admin", PASSWORD)
    expect(out:find("No accounts"), "telnet: " .. out)
    out = ftp("admin", PASSWORD)
    expect(out:find("530 Login incorrect"), "ftp: " .. out)
    expect(mqtt(1883, "admin", PASSWORD) == 4, "mqtt accepted")
end)

test("Create accounts", function()
    local ok, out = run("passwd -s telnet,ftp admin", PASSWORD .. "\n")
    expect(ok, "passwd admin: " .. out)
    ok, out = run("passwd -g -s mqtt sensor")
    expect(ok, "passwd sensor: " .. out)
    local generated = out:match("Password: (%w+)")
    expect(generated and #generated == 20, "generated password: " .. out)
    ok, out = run("passwd -l")
    expect(ok and out:find("admin%s+1000%s+1000%s+/%s+%-%s+telnet,ftp%s+10000") and out:find("sensor%s+1001%s+1001%s+/%s+%-%s+mqtt%s+1"),
        "list: " .. out)
    local file = read_file(SHADOW) or ""
    expect(file:find("^admin:%$pbkdf2%-sha256%$10000%$[%w+/]+%$[%w+/]+:telnet,ftp\n"), "shadow file: " .. file)
    expect(lfs.attributes(SHADOW, "permissions") == "rw-------", "shadow file mode")
    expect(not file:find(PASSWORD, 1, true), "password in file")
    file = read_file(PASSWD) or ""
    expect(file == "admin:x:1000:1000:::\nsensor:x:1001:1001:::\n", "account file: " .. file)
    expect(lfs.attributes(PASSWD, "permissions") == "rw-r--r--", "account file mode")
    servers.sensor_password = generated
end)

-- shell output "TELNET OK" differs from command (arguments separated by 3 spaces)
test("telnetd: login", function()
    local out = telnet("admin", PASSWORD, "echo TELNET   OK\r\nexit\r\n")
    expect(out:find("Login:") and out:find("Password:"), "prompts: " .. out)
    expect(out:find("TELNET OK"), "shell output: " .. out)
    expect(not out:find(PASSWORD, 1, true), "password echoed")
end)

test("telnetd, ftpd: user of session", function()
    os.remove("/tmp/auth_tel")
    local out = telnet("admin", PASSWORD, "id\r\n") -- one command per session
    expect(out:find("uid=1000%(admin%) gid=1000"), "telnet id: " .. out)
    out = telnet("admin", PASSWORD, "touch /tmp/auth_tel\r\n")
    expect(owner("/tmp/auth_tel") == 1000, "owner of telnet file: " .. tostring(owner("/tmp/auth_tel")) .. out)
    os.remove("/tmp/auth_tel")
    lfs.rmdir("/tmp/auth_ftp")
    out = ftp("admin", PASSWORD, "MKD /auth_ftp\r\n")
    expect(out:find("257 \"/auth_ftp\""), "ftp mkdir: " .. out)
    expect(owner("/tmp/auth_ftp") == 1000, "owner of ftp directory: " .. tostring(owner("/tmp/auth_ftp")))
    lfs.rmdir("/tmp/auth_ftp")
end)

test("telnetd: wrong password, account without service", function()
    local out = telnet("admin", "wrong", "")
    expect(out:find("Login incorrect") and not out:find("TELNET OK"), "wrong password: " .. out)
    out = telnet("sensor", servers.sensor_password, "echo TELNET   OK\r\nexit\r\n")
    expect(out:find("Login incorrect") and not out:find("TELNET OK"), "account without service: " .. out)
end)

test("ftpd: login", function()
    local out = ftp("admin", PASSWORD)
    expect(out:find("331") and out:find("230") and out:find("257"), "ftp: " .. out)
    out = ftp("admin", "wrong")
    expect(out:find("530 Login incorrect") and not out:find("230"), "wrong password: " .. out)
    out = ftp("nobody", PASSWORD)
    expect(out:find("331") and out:find("530 Login incorrect"), "unknown user: " .. out)
end)

test("mqtt_broker: login", function()
    expect(mqtt(1883, "sensor", servers.sensor_password) == 0, "sensor refused")
    expect(mqtt(1883, "sensor", servers.sensor_password) == 0, "sensor refused (cache)")
    expect(mqtt(1883, "admin", PASSWORD) == 4, "account without service accepted")
    expect(mqtt(1883, "sensor", "wrong") == 4, "wrong password accepted")
    expect(mqtt(1883, nil, nil) == 4, "no credentials accepted")
    expect(mqtt(1884, nil, nil) == 0, "anonymous broker refused")
end)

test("Separate passwords of services", function()
    local ok, out = run("passwd -s ftp admin", "ftp password\n")
    expect(ok, "passwd -s ftp: " .. out)
    expect(ftp("admin", "ftp password"):find("230"), "ftp password refused")
    expect(ftp("admin", PASSWORD):find("530"), "telnet password accepted by ftp")
    expect(telnet("admin", PASSWORD, "echo TELNET   OK\r\nexit\r\n"):find("TELNET OK"), "telnet password refused")
    expect(not telnet("admin", "ftp password", "echo TELNET   OK\r\nexit\r\n"):find("TELNET OK"),
        "ftp password accepted by telnet")
    ok, out = run("passwd -l")
    expect(out:find("admin%s+1000%s+1000%s+/%s+%-%s+telnet%s") and out:find("admin%s+1000%s+1000%s+/%s+%-%s+ftp%s"), "list: " .. out)
    ok, out = run("passwd admin", "x\n")
    expect(not ok and out:find("several services"), "password of several lines: " .. out)
    ok, out = run("passwd -d -s ftp admin")
    expect(ok, "passwd -d -s ftp: " .. out)
    expect(ftp("admin", "ftp password"):find("530"), "removed ftp password accepted")
    ok, out = run("passwd -s telnet,ftp admin", PASSWORD .. "\n")
    expect(ok, "passwd -s telnet,ftp: " .. out)
end)

test("Console only account (service shell)", function()
    local ok, out = run("passwd -s shell console", PASSWORD .. "\n")
    expect(ok, "passwd -s shell: " .. out)
    out = telnet("console", PASSWORD, "echo TELNET   OK\r\nexit\r\n")
    expect(not out:find("TELNET OK"), "console account accepted by telnet: " .. out)
    ok, out = run("passwd -d console")
    expect(ok, "passwd -d console: " .. out)
end)

test("Password change and account removal", function()
    local ok, out = run("passwd admin", "new password\n")
    expect(ok, "passwd: " .. out)
    expect(ftp("admin", PASSWORD):find("530"), "old password accepted")
    expect(ftp("admin", "new password"):find("230"), "new password refused")
    ok, out = run("passwd -l")
    expect(out:find("admin%s+1000%s+1000%s+/%s+%-%s+telnet,ftp"), "services changed: " .. out)
    ok, out = run("passwd -d admin")
    expect(ok, "passwd -d: " .. out)
    ok, out = run("passwd -d admin")
    expect(not ok, "removed twice")
    expect(ftp("admin", "new password"):find("530"), "removed account accepted")
end)

test("Limit of failed logins", function()
    -- mqtt_broker: 3 failures above, address is blocked after 5 failures
    expect(mqtt(1883, "sensor", "wrong") == 4, "failure 4")
    expect(mqtt(1883, "sensor", "wrong") == 4, "failure 5")
    expect(mqtt(1883, "sensor", servers.sensor_password) == 5, "address not blocked")
end)

test("Close", function()
    for i, pid in ipairs(servers) do
        dnx.proc.kill(pid)
        dnx.proc.wait(pid, 3000)
        os.remove("/tmp/auth_srv" .. i)
    end
    os.remove(PASSWD)
    os.remove(SHADOW)
    for _, f in ipairs({ "auth_tel", "auth_out", "auth_in", "auth_tx", "auth_rx", "auth_ftpd.conf", "auth_mqtt.conf",
        "auth_mqtt_anon.conf" }) do
        os.remove("/tmp/" .. f)
    end
end)

T.summary()

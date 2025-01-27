-- Brand spanking new Lua-based cross-platform piece of dogshit build system
-- Compiling for Windows was designed to be done with mingw on Linux.
-- Compiling on Windows may exist at a specific moment in time somewhere in the future.

local function parseShell()
    local args = {}
    local opts = {}

    for i=1,#arg do
        ---@diagnostic disable-next-line: unused-local dont care
        local arg = arg[i]

        if arg:sub(1, 2) == "--" then
            local rest = arg:sub(3)
            local v = string.find(rest, "=")
            if v then
                -- opt with value
                local key = rest:sub(1, v-1)
                local val = rest:sub(v+1)
                opts[key] = val
            else
                -- bool opt
                opts[rest] = true
            end
        elseif arg:sub(1, 1) == "-" then
            local o = arg:sub(2)
            for j=1,#o do
                opts[o:sub(j, j)] = true
            end
        else
            table.insert(args, arg)
        end
    end
    args[0] = arg[0]

    return args, opts
end

---@generic T
---@param unix T
---@param windows T
---@return T
local function unixOrWindows(unix, windows)
    return package.config:sub(1, 1) == "/" and unix or windows
end

---@param fmt string
---@return string
local function fixPath(fmt, ...)
    -- we shove it into a local because Lua multivalues CAN and WILL fuck us over
    local s = fmt:format(...):gsub("%/", package.config:sub(1, 1))
    return s
end

---@param path string
---@return boolean
local function fexists(path)
    local f = io.open(path, "r")
    if f then
        f:close()
    end
    return f ~= nil
end

local args, opts = parseShell()

local lfs = require(opts.lfs or "lfs")

local buildDir = opts.buildDir or "build"

lfs.mkdir(buildDir)

local verbose = opts.verbose or opts.v or false
local compiler = opts.cc or "clang" -- Default to clang cuz its cooler
local linker = opts.ld or compiler
local mode = opts.mode or "debug" -- Debug cool
mode = mode:lower() -- muscle memory is a bitch sometimes

local cflags = opts.cflags or "-c -fPIC"
if opts.strict then
    cflags = cflags .. " -Wall -Werror -Wno-unused-but-set-variable" -- we disable that warning because fucking raygui
end

local lflags = opts.lflags or ""
local coolLinkFolderWithMagicLibs = opts.linkFolder or unixOrWindows("/usr/lib", ".")
local raylibDLL = opts.raylibDLL or unixOrWindows("libraylib.so", "./raylib.dll")

local target = opts.target
if not target then
    target = unixOrWindows("linux", "windows")
end
assert(target == "windows" or target == "linux", "bad target")

if mode == "debug" then
    local ourFuckingDebugger = "lldb"
    if compiler == "gcc" or compiler == "cc" then
        ourFuckingDebugger = "gdb"
    elseif compiler ~= "clang" then
        ourFuckingDebugger = "" -- just pass -g and hope
    end
    cflags = cflags .. " -g" .. ourFuckingDebugger
    cflags = cflags .. " -Og" -- debug-focused optimizations
elseif mode == "release" then
    cflags = cflags .. " -O3"
    if compiler == "gcc" then
        cflags = cflags .. " -flto=auto"
    end
    if compiler == "clang" then
        cflags = cflags .. " -flto=full"
    end
    if linker == "clang" then
        lflags = lflags .. " -flto=full"
    end
    lflags = lflags .. " -O3"
elseif mode == "turbo" then
    cflags = cflags .. " -DTSC_TURBO -O3 -ffast-math"
    if compiler == "gcc" then
        cflags = cflags .. " -flto=auto"
    end
    if compiler == "clang" then
        cflags = cflags .. " -flto=full"
    end
    if linker == "clang" then
        lflags = lflags .. " -flto=full"
    end
    lflags = lflags .. " -Ofast"
else
    error("bad mode")
end

if opts.singleThreaded then
    cflags = cflags .. " -DTSC_SINGLE_THREAD"
end

if opts.march then
    if type(opts.march) == "string" then
        cflags = cflags .. " -march=" .. opts.march
    else
        cflags = cflags .. " -march=native"
    end
end

local raylibExtraBullshit = {
    linux = "-lGL -lpthread -ldl -lrt -lX11 -lm", -- we use cool libs not pesky winmm
    windows = "-lm -lwinmm -lgdi32 -lopengl32", -- dont ask
}

local platformSpecificBullshit = {
    linux = "-lm -lpthread -ldl",
    windows = "-static-libgcc -Wl,-Bstatic -lpthread",
}

lflags = lflags .. " -L" .. coolLinkFolderWithMagicLibs .. " -l:" .. raylibDLL
lflags = lflags .. " " .. raylibExtraBullshit[target] .. " " .. platformSpecificBullshit[target]

local platformSpecificCBullshit = {
    linux = "", -- none, we cool
    windows = unixOrWindows("-I /usr/local/include", ""), -- fuck you
}

cflags = cflags .. " " .. platformSpecificCBullshit[target]

local libs = {
    linux = "libtsc.so",
    windows = "libtsc.dll",
}

local exes = {
    linux = "thesandboxcell",
    windows = "thesandboxcell.exe", -- me when no +x permission flag
}

local lib = libs[target]
local exe = exes[target]

---@param file string
---@return string
local function cacheName(file)
    local s = file:gsub("%/", ".")
    return s
end

---@param cmd string
local function exec(cmd, ...)
    cmd = cmd:format(...)
    if verbose then print(cmd) end
    if not os.execute(cmd) then
        error("Command failed. Fix it.")
    end
end

---@param file string
---@return number
local function mtimeOf(file)
    local attrs = lfs.attributes(file)
    -- yeah if it errors then the last build was on January 1st, 1970, 12:00 AM GMT
    if not attrs then return 0 end
    return attrs.modification or 0
end

---@param file string
---@return string
local function compile(file)
    local buildFile = fixPath("%s/%s.o", buildDir, cacheName(file))
    local src = mtimeOf(file)
    local out = mtimeOf(buildFile)
    if (src > out) or opts.nocache then
        exec("%s %s %s -o %s", compiler, cflags, file, buildFile)
    else
        if verbose then
            print("Skipping recompilation of " .. file .. " into " .. buildFile)
        end
    end
    return buildFile
end

---@param out string
---@param ftype "exe"|"shared"
---@param specialFlags string
---@param objs string[]
local function link(out, ftype, specialFlags, objs)
    local ltypeFlag = ftype == "shared" and "-shared" or ""
    exec("%s %s -o %s %s %s %s", linker, lflags, out, specialFlags, ltypeFlag, table.concat(objs, " "))
end

-- Who needs automatic source graphs when we have manual C file lists
local libtsc = {
    "src/threads/workers.c",
    "src/threads/tinycthread.c",
    "src/utils.c",
    "src/cells/cell.c",
    "src/cells/grid.c",
    "src/cells/ticking.c",
    "src/cells/subticks.c",
    "src/graphics/resources.c",
    "src/graphics/rendering.c",
    "src/graphics/ui.c",
    "src/saving/saving.c",
    "src/saving/saving_buffer.c",
    "src/api/api.c",
    "src/api/tscjson.c",
    "src/api/value.c",
    "src/api/modloader.c",
}

local game = {
    "src/main.c", -- wink wink
}

local function buildTheDamnLib()
    local objs = {}

    for i=1,#libtsc do
        table.insert(objs, compile(libtsc[i]))
    end

    link(lib, "shared", "", objs)
end

local function buildTheDamnExe()
    local objs = {}

    for i=1,#game do
        table.insert(objs, compile(game[i]))
    end

    link(exe, "exe", "-L. -l:./" .. lib, objs)
end

-- Depends on zip existing because yes
local function packageTheDamnGame()
    local toZip = {
        exe,
        lib,
        "-r data",
        "-r mods",
        "-r platforms",
    }

    if fexists(raylibDLL) then
        table.insert(toZip, raylibDLL)
    end

    local zip = string.format('"TheSandboxCell %s.zip"', target:sub(1, 1):upper() .. target:sub(2))

    exec("zip %s %s", zip, table.concat(toZip, " "))
end

args[1] = args[1] or "game"

if args[1] == "lib" then
    buildTheDamnLib()
end

if args[1] == "game" then
    if opts.background or opts.b then
        repeat
            buildTheDamnLib()
            buildTheDamnExe()
            print("Compilation finished")
            local line = io.read("l")
        until not line
    else
        buildTheDamnLib()
        buildTheDamnExe()

        if opts.bundle or opts.package then
            packageTheDamnGame()
        end
    end
end

if args[1] == "bundle" or args[1] == "package" then
    packageTheDamnGame()
end

if args[1] == "clean" then
    os.remove(lib)
    os.remove(exe)
    for file in lfs.dir(buildDir) do
        os.remove(fixPath("%s/%s", buildDir, file))
    end
    lfs.rmdir(buildDir)
end

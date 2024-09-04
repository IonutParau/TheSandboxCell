#!/usr/bin/env lua

local task = arg[1]

if task == "help" then
    print("libtsc.lua -- Helper script for libtsc")
    print("Syntax: " .. arg[0] .. " <mode> <options>")
    print("\thelp - Print out this page")
    print("\tgenerate - Generate the header libtsc.h")
    print("\t\t--no-<grid / subticks / saving / resources / ui / workers / utils> - Omit specific parts you do not need")
    print("\tcompile - Compile the libtsc.dll library (can cross-compile!)")
    print("\t\t--target <windows / linux> - The target to compile for. If omitted, it is the native target.")
    print("\t\t--compiler <gcc> - The C compiler to use. Currently only gcc is supported. When cross-compiling, it uses MinGW.")
    print("\t\t--mode <release / debug> - The build mode")
    print("\t\t--game - Compiling the whole game")
    print("\tflags - Output recommended linking flags")
end

local function fixPath(path)
    local out = ""
    for i=1,#path do
        local c = string.sub(path, i, i)
        if c == "/" then c = package.config:sub(1, 1) end
        out = out .. c
    end
    return out
end

local function strbegin(s, prefix)
    return string.sub(s, 1, #prefix) == prefix
end

if task == "generate" then
    local headers = {
        grid = "src/cells/grid.h",
        subticks = "src/cells/subticks.h",
        saving = "src/saving/saving.h",
        resources = "src/graphics/resources.h",
        ui = "src/graphics/ui.h",
        -- libtsc does provide a thread pool, yes.
        workers = "src/threads/workers.h",
        -- This is because some stuff wants string interning (like IDs)
        utils = "src/utils.h",
        -- api.h is confusingly named by yours truly, but I don't care
        api = "src/api/api.h",
        value = "src/api/value.h",
        json = "src/api/tscjson.h",
    }

    local usedHeaders = {}

    local function useHeader(header)
        if header == nil then return end
        table.insert(usedHeaders, header)
    end

    for i=2,#arg do
        if strbegin(arg[i], "--no-") then
            local thing = arg[i]:sub(6)
            headers[thing] = nil
        end
    end

    useHeader(headers.grid)
    useHeader(headers.subticks)
    useHeader(headers.saving)
    useHeader(headers.resources)
    useHeader(headers.ui)
    useHeader(headers.workers)
    useHeader(headers.utils)
    useHeader(headers.value)
    useHeader(headers.api)
    useHeader(headers.json)

    local out = io.stdout
    for i=2,#arg do
        if arg[i] == "--output" or arg[i] == "-o" then
---@diagnostic disable-next-line: cast-local-type
            out = io.open(arg[i+1], "w")
            assert(out ~= nil, "Unable to open " .. arg[i+1])
        end
    end

    for line in io.lines("LICENSE", "l") do
        out:write("// " .. line .. "\n")
    end

    out:write("#ifdef __cplusplus\n")
    out:write("extern \"C\" {\n")
    out:write("#endif\n")

    for _, header in ipairs(usedHeaders) do
        local file = io.open(fixPath(header), "r")
        assert(file ~= nil, "Unable to find header " .. header)
        local hidden = false
        for line in file:lines() do
            local ignore = hidden
            if strbegin(line, "#include \"") then
                ignore = true
            end
            if strbegin(line, "// hideapi") then
                ignore = true
                hidden = not hidden
            end
            if not ignore then
                out:write(line, "\n")
            end
        end
        file:close()
    end

    out:write("#ifdef __cplusplus\n")
    out:write("}\n")
    out:write("#endif\n")

    if out ~= io.stdout then
        out:close()
    end
end

local function testOutCompilers()
    local toCheck = {
        "gcc",
        "clang",
        "zig",
    }

    for _, compiler in ipairs(toCheck) do
        local proc = io.popen(compiler .. " --version", "r")
        if proc then
            proc:flush()
            proc:close()
            return compiler
        end
    end

    error("No suitable compiler found. Please install gcc, clang or zig")
end

if task == "compile" then
    -- Well shit
    local host = package.config:sub(1, 1) == "/" and "linux" or "windows"
    local target = host
    local compiler = testOutCompilers()
    local raylibDLL
    local mode = "release"
    local step = "library"
    local bundle = false

    for i=2,#arg do
        if arg[i] == "--target" then
            target = arg[i+1]
        end
        if arg[i] == "--compiler" then
            compiler = arg[i+1]
        end
        if arg[i] == "--raylib-dll" then
            raylibDLL = arg[i+1]
        end
        if arg[i] == "--mode" then
            mode = string.upper(arg[i+1])
        end
        if arg[i] == "--game" then
            step = "all"
        end
        if arg[i] == "--clean" then
            step = "clean"
        end
        if arg[i] == "--bundle" then
            bundle = true
        end
    end
    
    local library = target == "linux" and "libtsc.so" or "libtsc.dll"
    local exe = target == "linux" and "thesandboxcell" or "thesandboxcell.exe"

    local linkRaylib = ""

    if host == "linux" and target == "linux" then
        if raylibDLL then
            linkRaylib = string.format("-l:./%s", raylibDLL)
        else
            linkRaylib = "-lraylib -lGL -lm -lpthread -ldl -lrt -lX11"
        end

        -- Super easy.
        if compiler == "zig" then compiler = "zig cc" end
        os.execute("make " .. step .. " MODE=" .. mode .. " CC=\"" .. compiler .. "\" LINKER=\"" .. compiler .. "\" LINKRAYLIB=\"" .. linkRaylib .. "\"")
    end
    if host == "linux" and target == "windows" then
        if raylibDLL ~= nil and raylibDLL ~= "raylib.dll" then
            error("Raylib DLL must be raylib.dll (for Windows reasons)")
        end
        -- Oh fuck
        if compiler == "gcc" then
            -- We can't use gcc, we have to use x86_64-w64-mingw32-gcc
            compiler = "x86_64-w64-mingw32-gcc"
            local windowsSystem = mode == "release" and "-Wl,--subsystem,windows" or ""
            os.execute("make " .. step .. " MODE=" .. mode .. " CC=\"" .. compiler .. "\" LINKER=\"" .. compiler .. "\" OUTPUT=\"thesandboxcell.exe\" ECFLAGS=\"-I /usr/local/include\" LIBRARY=libtsc.dll LFLAGS=\"-L. -lraylib -lm -lwinmm -lgdi32 -lopengl32 -static-libgcc -Wl,-Bstatic -lpthread " .. windowsSystem .. "\" LINKRAYLIB=\"-L. -lraylib -lm -lwinmm -lgdi32 -lopengl32\"")
        end
    end

    if bundle then
        if host == "linux" then
            -- Use zip to make a zip
            local files = {
                exe,
                library,
                "-r resources",
                "-r shaders",
                "mods",
            }
            if raylibDLL ~= nil then table.insert(files, raylibDLL) end
            os.execute("zip TheSandboxCell.zip " .. table.concat(files, " ") ..  " -9")
        end
    end
end

Config.tscLibs = {
    system = {"m", "winmm", "gdi32", "opengl32"},
    custom = {
        ["."] = {"raylib.dll"},
    },
}

Config.gameLibs = Config.tscLibs

if Config.host == "linux" then
    local subsystem = Config.opts.console and "" or " -Wl,--subsystem,windows"
    Config.lflags = Config.lflags .. subsystem .. " -static-libgcc -Wl,-Bstatic -lpthread"
end

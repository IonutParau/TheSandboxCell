Config.tscLibs = {
    system = {"m", "winmm", "gdi32", "opengl32"},
    custom = {
        ["."] = {"raylib.dll"},
    },
}

Config.gameLibs = Config.tscLibs

if Config.host == "linux" then
    Config.lflags = Config.lflags .. " -static-libgcc -Wl,--subsystem,windows -Wl,-Bstatic -lpthread"
end

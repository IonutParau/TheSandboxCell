Config.tscLibs = {
    system = {"m", "pthread", "dl", "raylib", "GL", "rt", "X11"},
    custom = {},
}

Config.gameLibs.system = Config.tscLibs.system -- this works

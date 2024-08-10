# Actual TODO

For v0.0.2:
- Chunk System
- JSON parser library (tscjson)
- Tooltips
- Notification system (useful for error reporting, debugging, etc.)
- Backgrounds
- Add more comments for the headers in libtsc
- Finish implementing `ui.c`
- Add a main menu
- Add a settings menu
- `config.json` and custom `platforms` support.
- `pack.json` file in Resource Packs

For v0.1:
- Switch from Raylib to SDL2 (will be HARD)
- TSC Saving Format Encoder / Decoder
- Lua platform
- Python platform
- Simplify build step in make (by making it make sub-projects too)

# Planned features

- V1, V2, V3 decoding.
- Spacing in subticks
- Basic cell bar
- SBF1 encoding / decoding
- Basic main menu
- Basic editor menu
- Basic settings menu
- VX / SSF1 / TSC format support
- Resource Pack support
- Modding (DLL, Lua, Python)
- Mod Signals (Mod-to-Mod communication)

# Overviews

Clarifying the ambiguous stuff

## Resource Pack Support

A `pack.json` file would contain the following (though some information can be omitted)
```json
{
    "name": "My Resource Pack".
    "description": "A resource pack",
    "author": "The Creator",
}
```

NOTE: Not everything in here might actually be implemented.
UI Themes and shaders may not be implemented for a while, or ever, due to their difficulty.

### Texture Pack and Audio structure

All file names in the `textures` and `audio` directories and its subdirectories (except `mods`)
will be treated as `id.format` and used as the texture / audio for `id`.
In the case of `mods`, it must have directories which contain the mod ID, and then the same rules apply to it,
except the texture / audio is applied to `modID:id`.

### Shaders

There are a few shaders.
`fullscreen.glsl` does a fullscreen shader. It goes through the entire rendered screen and applies post-processing effects.
`grid.glsl` does a shader effect on the grid of empties.
`bg.glsl` does a shader effect on placeables.
`cell.glsl` does a shader effect on cells.

### Fonts

The font would be stored in `font.ttf`.
It would just be the font.

### UI Themes

In `ui.json`, UI theme information would be specified.

```yaml
{
    "text": "FFFFFF",
}
```

## Mod Signals

```c
// Overall representation of a signal
// Payload would store stuff like Lua runtime for Lua mods or WebAssembly information for WebAssembly mods.
tsc_value signal(void *payload, tsc_value *args) {
    // Helper functions for the internal implementation.
    return tsc_null();
}

struct tsc_frame {
    struct tsc_value *argv;
    size_t argc;
};

// A value.
// All heap memory is reference counted.
struct tsc_value {
    size_t tag;
    union {
        int64_t integer;
        double number;
        bool boolean;
        struct tsc_string *string;
        struct tsc_array *array;
        // string key, tsc_value value
        struct tsc_object *object;
    };
};
```

## Modding (Config)

```json
{
    "name": "MyMod",
    "description": "The mod",
    "platform": "lua",
}
```

## Modding (Lua)

```lua
local actualID = TSC.AddCell {
    "id",
    "Name",
    "Description here",
    -- Universal texture
    texture = "id.png",
    -- Explicit texture pack textures
    texture = {
        -- Fallback
        default = "id.png",
        -- Presumably with textures_dev's style
        photon = "id_photon.png",
    },
    -- VTable stuff
    update = function(cell, x, y)

    end,
}

-- Subticks
local actualSubtickID = TSC.newSubtick {
    "my_stuff",
    priority = 2.5,
    cells = {actualID},
    -- These would have defaults
    -- Yes, parallel is a default
    mode = "ticked",
    parallel = true,
    spacing = 0,
}

-- Add to other subtick (a cell should only have one subtick though)
TSC.getSubtick("movers"):add(actualID)

-- Grid stuff
local cell = TSC.Grid.set(x, y)
TSC.Grid.set(x, y, cell)
-- One of the rare cases the actual grid ID is the passed in gridID (not prefixed)
TSC.Grid.create("myGrid", 100, 100)
local old = TSC.Grid.switch("myGrid")
-- replaceCell is optional
-- Push and Pull return how many cells were pushed (0 if it failed)
TSC.Grid.push(x, y, dir, force, replaceCell)
TSC.Grid.pull(x, y, dir, force, replaceCell)
-- Nudge returns if it moved
TSC.Grid.nudge(x, y, dir, force, replaceCell)

-- Cell stuff
cell.id
cell.rot
cell.texture
cell:get("key")
cell:set("key", "value")
cell:set("key", nil) -- removes the key
cell:clone()
cell:owned() -- Returns if the cell is a pointer or a copy (from :clone())
-- VTable stuff
cell:canMove(x, y, dir, forceType)
TSC.Cell.create()

function TSC.Exports.doStuff(a, b, c)
    print(a, b, c)
end

local val = TSC.Library.bmod.doSomething(1, 2, "stuff")
```

## Modding (Python)

```py
from tsc import cell, Cell, export, Grid, library

@cell
class MyCell:
    id: str = "id"
    name = "Name"
    description = "Description here"
    # Universal texture
    texture = "textures/id.png"
    # Texture pack specific
    texture = {
        "default": "textures/id.png",
        "photon": "textures/id_photon.png",
    }

    # VTable stuff
    @staticmethod
    def update(cell: Cell, x: int, y: int, ux: int, uy: int):
        # Do stuff
        pass

@export(name="doStuff")
def doStuff(a, b, c):
    print(a, b, c)

val = library.bmod.doSomething(1, 2, "stuff")

# Everything else similar to Lua but more pythonic
```

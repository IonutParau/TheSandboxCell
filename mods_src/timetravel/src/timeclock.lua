print("Loading TimeClock")

IDs.TimeClock = TSC.Cell.Add {
    "timeclock",
    "Time Clock",
    "A cell which can store and restore the world",
    update = function(cell, x, y)
        -- This means "backup"
        if cell:hasFlag(1) then
            cell:set("backup", TSC.Grid.save())
        end
        -- This means restore
        if cell:hasFlag(2) then
            -- If nil then we just dont restore lol
            RestorePoint = cell:get("backup") or RestorePoint
        end
    end,
    isTrash = function(grid, cell, x, y, dir, forceType, force, eating)
        return dir == cell.rot or dir == (cell.rot + 2) % 4
    end,
    onTrash = function(grid, cell, x, y, dir, forceType, force, eating)
        if dir == cell.rot then
            cell:enableFlag(1)
        else
            cell:enableFlag(2)
        end
    end,
    reset = function(cell, x, y)
        cell:resetFlag() -- Resets all flags
    end
}

TSC.Categories.Root:addCell(IDs.TimeClock)

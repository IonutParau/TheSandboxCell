require("src.timeclock")

IDs = {}

RestorePoint = nil

TSC.Subticks.AddCallback("time-restore", math.huge, function()
    if RestorePoint ~= nil then
        TSC.Grid.load(RestorePoint)
        RestorePoint = nil
    end
end)
local PATH = "C:/Users/Silve/Develop/Projects/CSOL24H/"
dofile(PATH .. "source/Keyboard.lua")
dofile(PATH .. "source/Mouse.lua")
dofile(PATH .. "source/Console.lua")
dofile(PATH .. "source/Setting.lua")

if (not Craft)
then
    Craft = {}
    function Craft:combineParts()
        local counter = 0
        Console:infomation("开始合成配件")
        while (Keyboard:getKeyLockState() == 1 and not Runtime.exit)
        do
            Keyboard:press(Keyboard.ENTER, Delay.MINI)
            Mouse:clickOn(Setting.CRAFT_PARTS_AUTO_FILL_X, Setting.CRAFT_PARTS_AUTO_FILL_Y)
            Mouse:clickOn(Setting.CRAFT_PARTS_COMBINE_X, Setting.CRAFT_PARTS_COMBINE_Y)
            counter = counter + 1
            if (counter > 30)
            then
                Mouse:clickOn(Setting.CRAFT_PARTS_CLEAR_X, Setting.CRAFT_PARTS_CLEAR_Y)
                counter = 0
            end
            Keyboard:release(Keyboard.ENTER, Delay.MINI)
        end
        Console:infomation("合成配件结束")
    end
end

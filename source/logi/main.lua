local PATH = "C:/Users/Silve/Develop/CSOL-24H/"

dofile(PATH .. "source/Runtime.lua")
dofile(PATH .. "source/Keyboard.lua")
dofile(PATH .. "source/Round.lua")
dofile(PATH .. "source/Mouse.lua")
dofile(PATH .. "source/Console.lua")
dofile(PATH .. "source/Store.lua")
dofile(PATH .. "source/Craft.lua")
dofile(PATH .. "source/Delay.lua")
dofile(PATH .. "source/Dispacher.lua")

function OnEvent(event, arg, family)

    Runtime.exit = false
    if (Keyboard:getKeyLockState() > 0 and Keyboard:is_modifier_pressed(Keyboard.LALT) and Keyboard:is_modifier_pressed(Keyboard.RALT))
    then
        if (event == "MOUSE_BUTTON_PRESSED" and arg == 7 and family == "mouse")
        then
            Round:start()
            Mouse:move_cursor_to(32768, 32768)
        elseif (event == "MOUSE_BUTTON_PRESSED" and arg == 6 and family == "mouse")
        then
            local x, y = Mouse:locate_cursor()
            Console:infomation(string.format("Coordinate: (%d, %d)", x, y))
        elseif (event == "MOUSE_BUTTON_PRESSED" and arg == 5 and family == "mouse")
        then
            Store:purchaseItems()
        elseif (event == "MOUSE_BUTTON_PRESSED" and arg == 4 and family == "mouse")
        then
            Craft:combineParts()
        end
    end
end

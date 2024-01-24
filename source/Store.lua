local PATH = "C:/Users/Silve/Develop/Projects/CSOL24H/source/"
dofile(PATH .. "Keyboard.lua")
dofile(PATH .. "Mouse.lua")
dofile(PATH .. "Console.lua")
dofile(PATH .. "Setting.lua")

if (not Store)
then
    Store = {}
    function Store:purchaseItems()
        local buyX, buyY = Mouse:locate() -- “兑换”按钮坐标
        Console:infomation(string.format("开始批量购买物品，兑换按钮位置：(%d, %d)", buyX, buyY))
        while (Keyboard:getKeyLockState() == 1 and not Runtime.exit)
        do
            Mouse:clickOn(buyX, buyY)
            Mouse:clickOn(Setting.STORE_BUY_OPTION_X, Setting.STORE_BUY_OPTION_Y) -- 选项
            Mouse:clickOn(Setting.STORE_BUY_BUY_X, Setting.STORE_BUY_BUY_Y) -- 二级“兑换”按钮
            Mouse:clickOn(Setting.STORE_BUY_BUY_CONFIRM_X, Setting.STORE_BUY_BUY_CONFIRM_Y) -- “确认”按钮
            Keyboard:click(Keyboard.ESCAPE, Delay.SHORT)
        end
        Console:infomation("物品购买结束")
    end
end
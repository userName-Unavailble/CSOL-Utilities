local PATH = "C:/Users/Silve/Develop/Projects/CSOL24H/"
dofile(PATH .. "source/Delay.lua")
dofile(PATH .. "source/Runtime.lua")
----- 键盘按键设定 -----
if (not Keyboard)
then
    Keyboard = {
        TILDE = "tilde",
        ESCAPE = "escape",  ONE = "1", TWO = "2", THREE = "3", FOUR = "4", FIVE = "5", SIX = "6", SEVEN = "7",
        EIGHT = "8", NINE = "9", ZERO = "0", MINUS = "-", EQUAL = "=", BACKSPACE = "backspace", TAB = "tab",
        Q = "q", W = "w", E = "e", R = "r", T = "t", Y = "y", U = "u", I = "i", O = "o",
        P = "p", LBRACKET = "lbracket", RBRACKET = "rbracket", BACKSLASH = "backslash", 
        A = "a", S = "s", D = "d", F = "f", G = "g", H = "h", J = "j", K = "k", L = "l", SEMICOLON = "semicolon",
        QUOTE = "quote", ENTER = "enter", NON_US_LAYOUT_SLASH = "non_us_slash", 
        Z = "z", X = "x", C = "c", V = "v", B = "b", N = "n", M = "m", COMMA = "comma", PERIOD = "period", SLASH = "slash",
        LSHIFT = "lshift", RSHIFT = "rshift",
        LWIN = "lgui", RWIN = "rgui",
        LALT = "lalt", RALT = "ralt",
        LCTRL = "lctrl", RCTRL = "rctrl",
        SPACE = "spacebar",
        APPS = "appkey",
        INSERT = "insert", HOME = "home", PAGEUP = "pageup",
        DELETE = "delete", END = "end", PAGEDOWN = "pagedown",
        UP = "up", LEFT = "left", DOWN = "down", RIGHT = "right",
        NUM_SLASH = "numslash", NUM_MINUS = "numminus",
        NUM_SEVEN = "num7", NUM_EIGHT = "num8", NUM_NINE = "num9", NUM_PLUS = "numplus",
        NUM_FOUR = "num4", NUM_FIVE = "num5", NUM_SIX = "num6",
        NUM_ONE = "num1", NUM_TWO = "num2", NUM_THREE = "num3", NUM_ZERO = "num0", NUM_PERIOD = "numperiod",
        NUM_ENTER = "numenter",
        PRINTSCREEN = "printscreen", PAUSE = "pause",
        CAPS_LOCK = "capslock", NUM_LOCK = "numlock", SCROLL_LOCK = "scrolllock",
        F1 = "f1", F2 = "f2", F3 = "f3", F4 = "f4", F5 = "f5", F6 = "f6",
        F7 = "f7", F8 = "f8", F9 = "f9", F10 = "f10", F11 = "f11", F12 = "f12",
        F13 = "f13", F14 = "f14", F15 = "f15", F16 = "f16", F17 = "f17", F18 = "f18",
        F19 = "f19", F20 = "f20", F21 = "f21", F22 = "f22", F23 = "f23", F24 = "f24"
    }
    -- 键盘按下但未释放的按键，true表示按键按下，nil表示弹起
    Keyboard.unreleased = {}

    -- 按下某个按键，不回弹
    -- @param key 按下的按键，如Keyboard.ESCAPE
    -- @param [delay] 按下按键之后的延迟时间，单位为毫秒，可以直接使用预定义于Delay表中的字段，如Delay.NORMAL
    -- @return nil
    -- @remark 当Runtime.exit为true时，该函数将直接返回，不进行任何操作
    function Keyboard:press(key, delay)
        if (Runtime.exit) -- 当下达退出指令时，不进行任何操作
        then
            return
        end
        PressKey(key)
        self.unreleased[key] = true
        if (type(delay) == "number")
        then
            Runtime:sleep(delay)
        end
    end

    -- 释放按键
    -- @param key 按键名称，如Keyboard.ESCAPE
    -- @param [delay] 按下按键之后的延迟时间，单位为毫秒，可以直接使用预定义于Delay表中的字段，如Delay.NORMAL
    -- @return nil
    -- @remark 当Runtime.exit为true时，该函数将直接返回，不进行任何操作
    function Keyboard:release(key, delay)
        if (Runtime.exit)
        then
            return
        end
        ReleaseKey(key)
        self.unreleased[key] = nil
        if (type(delay) == "number")
        then
            Runtime:sleep(delay)
        end
    end

    -- 按下，而后释放按键（视为一次点击）
    -- @param key 按键名称，如Keyboard.ESCAPE
    -- @param [delay] 点击按键之后的延迟时间，单位为毫秒，可以直接使用预定义于Delay表中的字段，如Delay.NORMAL
    -- @return nil
    -- @remark 当Runtime.exit为true时，该函数将直接返回，不进行任何操作
    function Keyboard:click(key, delay)
        if (Runtime.exit)
        then
            return
        end
        PressAndReleaseKey(key)
        if (type(delay) == "number")
        then
            Runtime:sleep(delay)
        end
    end

    -- 释放所有按键
    -- @param [delay] 每释放一个按键之后的延迟时间，单位为毫秒，可以直接使用预定义于Delay表中的字段，如Delay.NORMAL
    -- @return nil
    -- @remark 当Runtime.exit为true时，该函数将直接返回，不进行任何操作
    function Keyboard:releaseAll(delay)
        if (Runtime.exit) -- 当下达退出指令时，不进行任何操作
        then
            return
        end
        for key, _ in pairs(self.keyState)
        do
            self:release(key, delay)
            self.unreleased[key] = false
        end
    end

    -- 点击某个按键若干次
    -- @param key 按键名称，如Keyboard.ESCAPE
    -- @param times 重复次数
    -- @param [delay] 每释放一个按键之后的延迟时间，单位为毫秒，可以直接使用预定义于Delay表中的字段，如Delay.NORMAL
    -- @return nil
    -- @remark 当Runtime.exit为true时，该函数将直接返回，不进行任何操作
    function Keyboard:clickSeveralTimes(key, times, delay)
        if (Runtime.exit)
        then
            return
        end
        while (times > 0)
        do
        self:click(key, delay)
        times = times - 1
        end
    end

    -- keyLockState为3 bits无符号整数，大小范围：0~7，从高位到低位依次表示NUMLOCK，CAPSLOCK，SCROLLLOCK
    ---   2               1             0
    --- NUM_LOCK       CAPS_LOCK     SCROLL_LOCK
    -- 以上排布对应于键盘上指示灯排布，可表示8种状态
    -- 例如， keylock = 5 = 0b101，代表NUMLOCK和SCROLLLOCK打开
    Keyboard.NUMLOCK_ON = 0x4
    Keyboard.CAPSLOCK_ON = 0x2
    Keyboard.SCROLLLOCK_ON = 0x1
    -- 获取keyLockState值
    -- @return number (1~7)
    function Keyboard:getKeyLockState()
        local keyLockState = 0
        if (IsKeyLockOn(self.NUM_LOCK))
        then
            keyLockState = keyLockState | self.NUMLOCK_ON
        else
            keyLockState = keyLockState & ~self.NUMLOCK_ON
        end
        if (IsKeyLockOn(self.CAPS_LOCK))
        then
            keyLockState = keyLockState | self.CAPSLOCK_ON
        else
            keyLockState = keyLockState & ~self.CAPSLOCK_ON
        end
        if (IsKeyLockOn(self.SCROLL_LOCK))
        then
            keyLockState = keyLockState | self.SCROLLLOCK_ON
        else
            keyLockState = keyLockState & ~self.SCROLLLOCK_ON
        end
        return keyLockState
    end

    -- 判断修饰键（如CTRL，ALT等）是否按下
    -- @return boolean
    function Keyboard:modifierPressed(key)
        return IsModifierPressed(key)
    end

end

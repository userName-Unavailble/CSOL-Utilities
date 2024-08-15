#include "CSOL24H.hpp"
#include "Command.hpp"
#include <Windows.h>
#include <synchapi.h>
#include <Console.hpp>

DWORD CSOL24H::LocateCursor(LPVOID lpParam) noexcept
{
    while (WAIT_OBJECT_0 == WaitForSingleObject(hEnableLocateCursorEvent, INFINITE))
    {
        if (bDestroy)
        {
            break;
        }
        GiveCommand(LUA_CMD_LOCATE_CURSOR);
        Sleep(1000);
    }
    ConsoleLog("线程 hLocateCursorThread 退出。", ENUM_CONSOLE_LOG_LEVEL::CLL_MESSAGE);
    return 0;
}
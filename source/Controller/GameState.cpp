#include <Windows.h>
#include <cstdio>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <fileapi.h>
#include <iterator>
#include <memory>
#include <minwinbase.h>
#include <minwindef.h>
#include <processenv.h>
#include <processthreadsapi.h>
#include <regex>
#include <synchapi.h>
#include <timezoneapi.h>
#include <wincon.h>
#include <winerror.h>
#include <winnt.h>
#include "CSOL24H.hpp"
#include "GameState.hpp"
#include "Util.hpp"
#include "CSOL24H_EXCEPT.hpp"
#include "Command.hpp"
#include "Console.hpp"

static void AdjustWindowRectToMiddleOfScreen(RECT& rect) noexcept
{
    RECT rcScreen = {
        .left = 0,
        .top = 0
    };
    rcScreen.right = GetSystemMetrics(SM_CXSCREEN);
    rcScreen.bottom = GetSystemMetrics(SM_CYSCREEN);
    LONG lDeltaX = (rcScreen.right - (rect.left + rect.right)) / 2;
    LONG lDeltaY = (rcScreen.bottom - (rect.top + rect.bottom)) / 2;
    rect.left += lDeltaX;
    rect.right += lDeltaX;
    rect.top += lDeltaY;
    rect.bottom += lDeltaY;
}
static void MakeWindowBorderless(HWND hWnd) noexcept
{
    WINDOWINFO windowInfo;
    if (!IsWindow(hWnd) || !GetWindowInfo(hWnd, &windowInfo))
    {
        return;
    }
    DWORD dwStyle = windowInfo.dwStyle & ~WS_CAPTION;
    RECT& rcClient = windowInfo.rcClient;
    AdjustWindowRectToMiddleOfScreen(rcClient);
    ShowWindow(hWnd, SW_SHOW);
    SetWindowLongPtrW(hWnd, GWL_STYLE, dwStyle);
    UpdateWindow(hWnd);
    MoveWindow(hWnd, rcClient.left, rcClient.top, rcClient.right - rcClient.left, rcClient.bottom - rcClient.top, TRUE);
}
DWORD CALLBACK CSOL24H::WatchInGameState(LPVOID lpParam) noexcept
{
    while (
        WAIT_OBJECT_0 == WaitForSingleObject(
            hEnableWatchGameStateEvent,
            INFINITE
        )) /* 获取到所有可等待对象才开始运行 */
    {
        if (bDestroy)
        {
            break;
        }
        UpdateErrorLogBuffer();
        TransferGameState();
        DispatchCommand();
        Sleep(100);
    }
    ConsoleLog("【消息】线程 hWatchInGameStateThread 退出。\r\n");
    return 0;
}

/*
@brief 状态迁移函数。
*/
void CSOL24H::TransferGameState() noexcept
{
    if (cbGameErrorLogSize <= 0) return;
    int64_t begin = cbGameErrorLogSize;
    int64_t end = cbGameErrorLogSize;
    int64_t current_time;
    int64_t log_timestamp;
    std::string line;
    const char* msg = nullptr;
    GameState gs;
    /* 例行检查 */
    if (bGameErrorLogBufferResolved)
    {
        current_time = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count(); /* 获取时间戳 */
        if (game_state.get_state() == ENUM_GAME_STATE::GS_LOADING && std::abs(current_time - game_state.get_timestamp()) > 30) /* 加载时间达到 30 秒 */
        {
            gs.update(ENUM_GAME_STATE::GS_MAP, current_time); /* 状态由 LOADING 转为 MAP，时间戳更新为当前时刻 */
            msg =  "预设加载时间已过，认为已经进入游戏场景";
        }
        if (game_state.get_state() == ENUM_GAME_STATE::GS_LOGIN && std::abs(current_time - game_state.get_timestamp()) > 15) /* 登陆后等待 15 秒 */
        {
            HWND hWnd = FindWindowW(NULL, L"Counter-Strike Online");
            if (hWnd)
            {
                SetForegroundWindow(hWnd);
                SetFocus(hWnd);
                SetCapture(hWnd);
                MakeWindowBorderless(hWnd);
                ConsoleLog("【消息】去除窗口标题栏。\r\n");
            }
            gs.update(ENUM_GAME_STATE::GS_HALL, current_time); /* 状态由 LOGIN 转为 HALL，时间戳更新为当前时刻 */
            msg = "登陆后等待时间已过，认为游戏客户端已经完全加载";
        }
        if (game_state.update(gs))
        {
            ConsoleLog("【消息】%s。游戏日志时间戳更新为当前时间戳：%lld。\r\n", msg, current_time);
        }
        return;
    }
    /* 自后向前，按照 CRLF 方式划分子串，以文件行为单位解析 buffer 内容 */
    while (begin >= 0)
    {
        if (begin == 0 && begin != end) /* [begin, end) 可划分为子串 */
        {
            line = std::string(lpGameErrorLogBuffer + begin, end - begin);
            begin--; /* begin 减小为负数后退出 */
        }
        else if (lpGameErrorLogBuffer[begin - 1] == '\n' && begin != end) /* [begin, end) 可划分为子串 */
        {
            line = std::string(lpGameErrorLogBuffer + begin, end - begin);
            begin -= 2; /* 越过 \r\n */
            end = begin;
        }
        else if (lpGameErrorLogBuffer[begin - 1] == '\n' && begin == end) /* [begin, end) 长度为 0 */
        {
            begin -= 2;
            end = begin;
            continue;
        }
        else
        {
            begin--;
            continue;
        }
        current_time = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count(); /* 获取时间戳 */
        log_timestamp = ResolveMessageTimestamp(line, nullptr);
        /* 按行解析日志消息并下达命令 */
        if (line.find("Join HostServer") != std::string::npos)
        {
            gs.update(ENUM_GAME_STATE::GS_LOADING, log_timestamp);
            msg = "连接到服务器主机，加载游戏场景";
        }
        else if (line.find("Result Confirm") != std::string::npos)
        {
            gs.update(ENUM_GAME_STATE::GS_ROOM, log_timestamp);
            msg = "游戏结算，回到房间";
        }
        else if (line.find("S_ROOM_ENTER") != std::string::npos)
        {
            gs.update(ENUM_GAME_STATE::GS_ROOM, log_timestamp);
            msg = "进入游戏房间";
        }
        // else if (line.find("Start Game Room") != std::string::npos)
        // {
        //     gs.update(ENUM_GAME_STATE::GS_LOADING, log_timestamp);
        //     msg = "开始游戏，即将加载游戏场景";
        // }
        else if (line.find("Leave Game Room") != std::string::npos)
        {
            gs.update(ENUM_GAME_STATE::GS_HALL, log_timestamp);
            msg = "返回到游戏大厅";
        }
        else if (line.find("Return to Room") != std::string::npos)
        {
            gs.update(ENUM_GAME_STATE::GS_ROOM, log_timestamp);
            msg = "回到游戏房间";
        }
        else if (line.find("QuitLog: game shutdown") != std::string::npos)
        {
            gs.update(ENUM_GAME_STATE::GS_SHUTDOWN, log_timestamp);
            msg = "游戏退出";
        }
        else if (line.find("Game Server Login") != std::string::npos)
        {
            gs.update(ENUM_GAME_STATE::GS_LOGIN, log_timestamp);
            msg = "正在登录客户端";
        }
        else if (line.find("Log File Opened") != std::string::npos)
        {
            gs.update(ENUM_GAME_STATE::GS_UNKNOWN, log_timestamp);
            msg = "未知状态";
        }
        else
        {
            continue;
        }
        if (game_state.update(gs))
        {
            ConsoleLog("【消息】%s。游戏日志时间戳：%lld，当前时间戳：%lld。\r\n", msg, log_timestamp, current_time);
        }
        bGameErrorLogBufferResolved = true;
        return;
    }
}


/*
@brief 更新日志文件缓冲区。
@return 日志文件缓冲区是否更新。
*/
bool CSOL24H::UpdateErrorLogBuffer() noexcept
{
    DWORD dwBytesWritten;
    FILETIME filetime;
    GetFileTime(hGameErrorLogFile, nullptr, nullptr, (LPFILETIME)&filetime);
    int64_t file_last_modified_time = (int64_t)filetime.dwLowDateTime | (int64_t)filetime.dwHighDateTime << 32; /* 文件时间本质实际上是 64 位数 */
    if (log_buffer_last_modified_time != file_last_modified_time) /* 文件已经被修改，需要加载 */
    {
        log_buffer_last_modified_time = file_last_modified_time;
        bGameErrorLogBufferResolved = false;
        INT64 cbUpdatedLog = 0;
        DWORD dwNewSizeHigh = 0;
        DWORD dwNewSizeLow = GetFileSize(hGameErrorLogFile, &dwNewSizeHigh);
        cbUpdatedLog = dwNewSizeLow | ((INT64)dwNewSizeHigh << 32);
        if (cbUpdatedLog > cbGameErrorLogSize) /* 文件变大且时间戳非零（初始状态），则说明只是向 Error.log 追加新内容，故只需要读取新增部分 */
        {
            ReadFile(
                hGameErrorLogFile,
                lpGameErrorLogBuffer + cbGameErrorLogSize, /* append to buffer */
                dwNewSizeLow - cbGameErrorLogSize, /* new content */
                &dwBytesWritten,
                nullptr
            );
            cbGameErrorLogSize += dwBytesWritten;
        }
        else
        {
            SetFilePointer(
                hGameErrorLogFile,
                0,
                0,
                FILE_BEGIN
            );
            ReadFile(hGameErrorLogFile, lpGameErrorLogBuffer, dwNewSizeLow, &dwBytesWritten, nullptr);
            cbGameErrorLogSize = dwBytesWritten;
        }
        // qwGameErrorLogFileDateTime = ResolveLogDate(lpGameErrorLogBuffer, cbGameErrorLogSize); /* 更新日志时间（废弃此方法，因为日期变更后若日志文件不刷新则从日志文件解析出的时间不准确） */
        CSOL24H::game_error_log_file_date = ResolveLogDate(filetime); /* 将文件修改的日期作为日志的日期，获取日期的 00:00:00（需要考虑时区） 时刻的时间戳 */
        return true;
    }
    return false;
}

void CSOL24H::DispatchCommand() noexcept
{
    int64_t timestamp = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count(); /* 获取时间戳 */
    static int64_t last_confirm_time = 0;
    bool need_confirm = false;
    const char* cmd = LUA_CMD_NOP;
    auto state = game_state.get_state();
    if (state == ENUM_GAME_STATE::GS_LOADING)
    {
        cmd = LUA_CMD_NOP;
    }
    else if (state == ENUM_GAME_STATE::GS_MAP)
    {
        if (std::abs(timestamp - game_state.get_timestamp()) < 10)
        {
            cmd = LUA_CMD_CHOOSE_CLASS;
        }
        else if (std::abs(timestamp - game_state.get_timestamp()) < 120)
        {
            cmd = LUA_CMD_PLAY_GAME_NORMAL;
        }
        else /* 不需要确认，且游戏时间超过 2 分钟，资金充足 */
        {
            cmd = bAllowExtendedMode ? LUA_CMD_PLAY_GAME_EXTEND : LUA_CMD_PLAY_GAME_NORMAL;
        }
    }
    else if (state == ENUM_GAME_STATE::GS_ROOM)
    {
        cmd = LUA_CMD_START_GAME_ROOM;
    }
    else if (state == ENUM_GAME_STATE::GS_HALL)
    {
        cmd = LUA_CMD_CREATE_ROOM;
    }
    GiveCommand(cmd);
}
#include "CSOL24H.hpp"
#include <cstddef>
#include <handleapi.h>
#include <Windows.h>
#include <errhandlingapi.h>
#include <minwinbase.h>
#include <minwindef.h>
#include <processthreadsapi.h>
#include <synchapi.h>
#include <winerror.h>
#include <winnt.h>
#include <tlhelp32.h>
#include "Console.hpp"

/*
@brief 尝试以安全方式结束进程。
@param `hProcess` 进程句柄，需要具有 `QUERY_LIMITED_INFORMATION | SYNCHRONIZE` 访问权限。
@param `hThread = NULL` 进程的主线程句柄，默认为 `NULL`。
@return 通过向进程中的所有线程发出 `WM_QUIT` 消息安全地结束进程。如果进程退出成功，则关闭传入的进程和线程句柄并返回 `true`，否则返回 `false`。
*/
bool CSOL24H::TryStopProcessSafely(HANDLE hProcess, HANDLE hThread) noexcept
{
    if (hProcess == NULL || hProcess == INVALID_HANDLE_VALUE) return true;
    HANDLE hThreadSnap;
    THREADENTRY32 thread_entry;
    thread_entry.dwSize = sizeof(THREADENTRY32);
    DWORD dwProcessId = GetProcessId(hProcess);
    DWORD dwThreadId = hThread == NULL ? 0 : GetThreadId(hThread);
    /* 首先尝试向主线程发送退出消息 */
    if (hThread != NULL)
    {
        PostThreadMessageW(dwThreadId, WM_QUIT, 0, 0);
        if (WAIT_OBJECT_0 == WaitForSingleObject(hProcess, 2000)) /* 等待 2 s */
        {
            goto close_handles; /* 成功退出 */
        }
    }
    /* 如不成功，再尝试获取进程的所有线程 */
    hThreadSnap = CreateToolhelp32Snapshot(
        TH32CS_SNAPTHREAD, dwProcessId
    ); /* 创建线程 snapshot */
    if (hThreadSnap == INVALID_HANDLE_VALUE) /* 创建 thread snapshot 失败 */
    {
        return false;
    }
    if (!Thread32First(hThreadSnap, &thread_entry))
    {
        CloseHandle(hThreadSnap);
        return false;
    }
    /* 尝试通过向各个线程发送退出消息结束进程 */
    do
    {
        if (thread_entry.th32OwnerProcessID == dwProcessId)
        {
            PostThreadMessageW(thread_entry.th32ThreadID, WM_QUIT, 0, 0); /* 向所有的线程发送 WM_QUIT */
        }
    } while (Thread32Next(hThreadSnap, &thread_entry));
    CloseHandle(hThreadSnap); /* 关闭 snapshot */
    if (WAIT_OBJECT_0 != WaitForSingleObject(hProcess, 2000)) /* 等待 2 s */
    {
        return false;
    }
close_handles:
    CloseHandle(hProcess);
    if (hThread != NULL)
        CloseHandle(hThread);
    return true;
}

DWORD CALLBACK CSOL24H::WatchGameProcess(LPVOID lpParam) noexcept
{
    PROCESS_INFORMATION tcg_process_info{};
    while (WAIT_OBJECT_0 == WaitForSingleObject(hEnableWatchGameProcessEvent, INFINITE))
    {
        /* 是否退出 */
        if (bDestroy)
        {
            break;
        }
        /* 如果本线程曾被挂起（模式切换），挂起后游戏启动，用户手动结束游戏，那么将导致状态机无法更新，故需要按下述方式处理 */
        if (bWasWatchGameProcessStateThreadPaused)
        {
            if (game_process_state == ENUM_GAME_PROCESS_STATE::GPS_BEING_CREATED)
                game_process_state = ENUM_GAME_PROCESS_STATE::GPS_UNKNOWN; /* 重置状态机 */
            bWasWatchGameProcessStateThreadPaused = false;
        }
        Sleep(1000); /* 每隔 1 秒运行一次本线程 */
        if (game_process_state == ENUM_GAME_PROCESS_STATE::GPS_BEING_CREATED) /* 等待游戏进程窗口打开 */
        {
            hGameWindow = FindWindowW(NULL, L"Counter-Strike Online");
            if (!hGameWindow)
            {
                continue; /* 游戏窗口未创建 */
            }
            GetWindowThreadProcessId(hGameWindow, &dwGameProcessId);
            if (dwGameProcessId == 0)
            {
                ConsoleLog(ENUM_CONSOLE_LOG_LEVEL::CLL_ERROR, "获取反恐精英 Online 进程标识符时发生错误。错误代码：%lu。", GetLastError());
                return -1;
            }
            hGameProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_TERMINATE |SYNCHRONIZE, FALSE, dwGameProcessId);
            if (!hGameProcess)
            {
                ConsoleLog(ENUM_CONSOLE_LOG_LEVEL::CLL_ERROR, "尝试获取反恐精英 Online 进程信息时发生错误。错误代码：%lu。", GetLastError());
                return -1;
            }
            game_process_state = ENUM_GAME_PROCESS_STATE::GPS_RUNNING; /* 跳转执行监测游戏进程运行状态的代码块 */
            ConsoleLog(ENUM_CONSOLE_LOG_LEVEL::CLL_MESSAGE, "成功获取游戏进程信息。进程标识符：%lu。", dwGameProcessId);
            SetEvent(hGameProcessRunningEvent);
        }
        else if (game_process_state == ENUM_GAME_PROCESS_STATE::GPS_RUNNING) /* 游戏进程正在运行 */
        {
            DWORD dwResult = WaitForSingleObject(hGameProcess, 0); /* 监测游戏进程状态 */
            if (dwResult == WAIT_OBJECT_0) /* 进程结束 */
            {
                ResetEvent(hGameProcessRunningEvent); /* 暂停监视游戏内状态 */
                game_process_state = ENUM_GAME_PROCESS_STATE::GPS_EXITED;
                ConsoleLog(ENUM_CONSOLE_LOG_LEVEL::CLL_WARNING, "游戏进程退出，执行掉线重连。");
                CloseHandle(hGameProcess); /* 关闭已经退出的的游戏进程句柄 */
                hGameProcess = NULL;
                dwGameProcessId = 0;
                hGameWindow = NULL;
            }
            else if (dwResult == WAIT_TIMEOUT) /* 本轮等待超时 */
            {
                continue;
            }
            else /* 等待失败 */
            {
                ConsoleLog(ENUM_CONSOLE_LOG_LEVEL::CLL_ERROR, "hWatchGameProcess 线程运行遇到错误。错误代码：%lu。\r\n", GetLastError());
                return -1;
            }
        }
        else if (game_process_state == ENUM_GAME_PROCESS_STATE::GPS_EXITED) /* 游戏进程退出，重启游戏 */
        {
            ResetEvent(hGameProcessRunningEvent); /* 游戏进程结束 */
            STARTUPINFOW startup_info_w;
            ZeroMemory(&startup_info_w, sizeof(startup_info_w));
            ZeroMemory(&tcg_process_info, sizeof(tcg_process_info));
            startup_info_w.cb = sizeof(startup_info_w);
            PROCESS_INFORMATION process_information;
            BOOL bRet = CreateProcessW(
                NULL,
                pwsTCGRunCSOCmd.get(),
                NULL,
                NULL,
                FALSE,
                NORMAL_PRIORITY_CLASS,
                NULL,
                L"C:\\WINDOWS\\SYSTEM32",
                &startup_info_w,
                &tcg_process_info
            );
            if (bRet)
            {
                ConsoleLog(ENUM_CONSOLE_LOG_LEVEL::CLL_MESSAGE, "等待游戏进程启动。");
            }
            else 
            {
                ConsoleLog(ENUM_CONSOLE_LOG_LEVEL::CLL_WARNING, "通过 TCGame 自动创建游戏进程失败。错误代码：%lu。请尝试手动运行游戏。", GetLastError());
            }
            game_process_state = ENUM_GAME_PROCESS_STATE::GPS_BEING_CREATED; /* 跳转执行等待游戏进程启动的代码块 */
            if (tcg_process_info.dwProcessId != 0)
            {
                CloseHandle(tcg_process_info.hProcess);
                CloseHandle(tcg_process_info.hThread);
                ZeroMemory(&tcg_process_info, sizeof(tcg_process_info));
            }
        }
        else if (game_process_state == ENUM_GAME_PROCESS_STATE::GPS_UNKNOWN) /* 游戏进程状态未确定，确定游戏进程状态 */
        {            
            if (hGameProcess)
            {
                CloseHandle(hGameProcess);
                hGameProcess = NULL;
            }
            ConsoleLog(ENUM_CONSOLE_LOG_LEVEL::CLL_MESSAGE, "游戏进程状态未知，检测游戏进程状态。");
            hGameWindow = FindWindowW(NULL, L"Counter-Strike Online"); /* 尝试获取游戏进程窗口 */
            if (!hGameWindow)
            {
                game_process_state = ENUM_GAME_PROCESS_STATE::GPS_EXITED; /* 游戏进程未启动 */
                continue; /* 跳转至执行启动游戏进程的代码块 */
            }
            GetWindowThreadProcessId(hGameWindow, &dwGameProcessId);
            if (!dwGameProcessId)
            {
                ConsoleLog(ENUM_CONSOLE_LOG_LEVEL::CLL_ERROR, "获取反恐精英 Online 进程标识符时发生错误。错误代码：%lu。", GetLastError());
                break;
            }
            hGameProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_TERMINATE | SYNCHRONIZE, FALSE, dwGameProcessId);
            if (!hGameProcess)
            {
                ConsoleLog(ENUM_CONSOLE_LOG_LEVEL::CLL_ERROR, "尝试获取反恐精英 Online 进程信息时发生错误。错误代码：%lu。", GetLastError());
                break;
            }
            ConsoleLog(ENUM_CONSOLE_LOG_LEVEL::CLL_MESSAGE, "成功获取游戏进程信息。游戏进程标识符：%lu。", dwGameProcessId);
            game_process_state = ENUM_GAME_PROCESS_STATE::GPS_RUNNING;
            SetEvent(hGameProcessRunningEvent);
        }
    }

    if (hGameProcess)
    {
        CloseHandle(hGameProcess);
        hGameProcess = NULL;
    }
    hGameWindow = 0;
    dwGameProcessId = 0;
    ConsoleLog(ENUM_CONSOLE_LOG_LEVEL::CLL_MESSAGE, "线程 hWatchGameProcessThread 退出。");
    return 0;
}
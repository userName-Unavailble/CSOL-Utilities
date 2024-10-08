#include "CSOL24H.hpp"
#include <regex>
#include <iostream>
/*
@brief 判断 `dwYear` 是否为闰年。
@return 判断结果。
*/
static bool is_leap(uint32_t dwYear)
{
    if (dwYear % 4 == 0 && dwYear % 100 != 0)
    {
        return true;
    }
    else if (dwYear % 400 == 0)
    {
        return true;
    }
    return false;
}

/*
@brief 将给定事件转换为 UNIX 时间戳。
@param `dwYear = 0`
@param `dwMonth = 0`
@param `dwDay = 0`
@param `dwHour = 0`
@param `dwSecond = 0`
@param `fTimeZone = 0` 时区，东半球为正，西半球为服。如东八区（UTC + 8）对应的 `fTimeZone` 为 `8.0`。
@return UNIX 时间戳。
*/
uint64_t CSOL24H::GetUNIXTimestamp(uint32_t dwYear, uint32_t dwMonth, uint32_t dwDay, uint32_t dwHour, uint32_t dwMinute, uint32_t dwSecond, float fTimeZone)
{
    uint64_t ret = 0ull;
    uint64_t total_days = 0ull;
    uint64_t i;
    for (i = 1970; i < dwYear; i++)
    {
        if (is_leap(i))
            total_days += 366;
        else
            total_days += 365;
    }
    i = 1;
    for (i = 1; i < dwMonth; i++)
    {
        if (i == 1 || i == 3 || i == 5 || i == 7 || i == 8 || i == 10 || i == 12)
        {
            total_days += 31;
        }
        else if (i == 2)
        {
            total_days += 28;
        }
        else
        {
            total_days += 30;
        }
    }
    if (dwMonth > 2 && is_leap(dwYear)) /* 已过 2 月，且为闰年 */
    {
        total_days++;
    }
    total_days += dwDay - 1;
    ret = total_days * 86400 + dwHour * 3600 + dwMinute * 60 + dwSecond;
    ret -= fTimeZone * 3600;
    return ret;
}

/*
@brief 将消息中的时间解析为 UNIX 时间戳（调整为 UTC 标准时）。
@param `message` 消息。
@return 成功时返回时间戳，失败时返回 `0`。
*/
int64_t CSOL24H::ResolveMessageTimestamp(const std::string message, int32_t* lpMilliseconds) noexcept
{
    static std::regex time_pattern("(\\d{1,2}):(\\d{2}):(\\d{2})\\.(\\d{3})"); /* 忽略毫秒 */
    std::smatch match;
    int32_t hour = 0, minute = 0, second = 0, millisecond = 0;
    int64_t ret;
    if (
        std::regex_search(message, match, time_pattern) &&
        match.size() - 1 == 4 /* 时、分、秒、毫秒 */
    )
    {
        hour = std::atoi(match[1].str().c_str());
        minute = std::atoi(match[2].str().c_str());
        second = std::atoi(match[3].str().c_str());
        millisecond = std::atoi(match[4].str().c_str());
        int32_t time_elapsed_since_midnight = hour * 3600 + minute * 60 + second;
        int32_t utc_time_elapsed_since_midnight = time_elapsed_since_midnight + time_bias;
        if (utc_time_elapsed_since_midnight < 0) /* UTC 比本地少一天 */
        {
            utc_time_elapsed_since_midnight += 24 * 60 * 60;
        }
        else if (utc_time_elapsed_since_midnight == 0) /* UTC 时间为 00:00:00 */
        {

        }
        else if (0 < utc_time_elapsed_since_midnight && utc_time_elapsed_since_midnight < 24 * 60 * 60)
        {

        }
        else if (utc_time_elapsed_since_midnight == 24 * 60 * 60) /* UTC 时间为 24:00，取模后为 00:00 */
        {
            utc_time_elapsed_since_midnight = 0;
        }
        else /* > 24 * 60 * 60，UTC 比本地多一天 */
        {
            utc_time_elapsed_since_midnight -= 24 * 60 * 60;
        }
        ret = game_error_log_file_date /* UTC 当日 00:00:00 时间戳 */ + utc_time_elapsed_since_midnight /* UTC 自从00:00:00 以来经过的时间 */;
    }
    if (lpMilliseconds) *lpMilliseconds = millisecond;
    return ret;
}

/*
@brief 解析日志日期，以时间戳形式返回。
@param `lpBuffer` 日志存储的缓冲区。
@param `dwLength` 日志文件长度。
@return 解析所得日期的 00:00:00 时刻（非 UTC 标准时）的 UNIX 时间戳。
*/
[[deprecated("从日志文件中获取到的日期在日期变更（23:59 之后）后可能不准确。请使用该函数重载版本。")]] int64_t CSOL24H::ResolveLogDate(LPCSTR lpBuffer, INT64 cbLength) noexcept
{
    static std::regex date_pattern("\\d{1,2}:\\d{2}:\\d{2}\\.\\d{3}.+?(\\d{2})/(\\d{2})/(\\d{4})");
    std::time_t ret = 0llu;
    INT32 begin = 0, end = 1;
    while (end < cbLength)
    {
        if (lpBuffer[end] != '\n')
        {
            end++;
            continue;
        }
        std::string line(lpBuffer + begin, end - begin);
        begin = end + 1;
        end = begin;
        std::smatch smatch_result;
        if (
            std::regex_search(line, smatch_result, date_pattern) &&
            smatch_result.size() - 1 == 3
        )
        {
            auto month = std::atoi(smatch_result[1].str().c_str());
            auto day = std::atoi(smatch_result[2].str().c_str());
            auto year = std::atoi(smatch_result[3].str().c_str());
            ret = GetUNIXTimestamp(year, month, day);
            break;
        }
    }
    return ret;
}
/*
@brief 解析日志文件日期，返回结果为文件最近一次修改日期当日的 00:00:00 时刻的 UNIX 时间戳（UTC）
@param `filetime` 使用 `GetFileTime` 获取到的文件时间。
*/
int64_t CSOL24H::ResolveLogDate(FILETIME filetime) noexcept
{
    SYSTEMTIME system_time;
    FileTimeToSystemTime((LPFILETIME)&filetime, &system_time); /* 获取日志文件被修改的系统时间 */
    int64_t ret = GetUNIXTimestamp(system_time.wYear, system_time.wMonth, system_time.wDay); /* 将文件修改的日期作为日志的日期，获取日期的 00:00:00（需要考虑时区） 时刻的时间戳 */
    return ret;
}
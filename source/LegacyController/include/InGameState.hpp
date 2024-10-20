#pragma once

#include <cstdint>

/* 游戏状态 */
enum ENUM_IN_GAME_STATE
{
    IGS_LOGIN, /* 正在登陆 */
    IGS_IN_HALL, /* 在大厅中 */
    IGS_IN_ROOM, /* 在房间内，尚未开始游戏 */
    IGS_IN_ROOM_ABNORMAL, /* 在房间内（因网络波动等异常原因导致的回到房间） */
    IGS_LOADING, /* 游戏场景正在加载 */
    IGS_IN_MAP, /* 在游戏地图中 */
    IGS_SHUTDOWN, /* 游戏关闭 */
    IGS_UNKNOWN /* 未知状态 */
};

class InGameState
{
private:
    ENUM_IN_GAME_STATE m_InGameState;
    int64_t m_Timestamp;
public:
    InGameState() : m_InGameState(ENUM_IN_GAME_STATE::IGS_UNKNOWN), m_Timestamp(0) {};
    InGameState(ENUM_IN_GAME_STATE state, int64_t timestamp) : m_InGameState(state), m_Timestamp(timestamp) {};
    ~InGameState() = default;
    bool update(ENUM_IN_GAME_STATE in_game_state, int64_t timestamp) noexcept
    {
        if (this->m_Timestamp > timestamp) /* 不允许用旧状态覆盖新状态 */
        {
            return false;
        }
        if (in_game_state == this->m_InGameState && timestamp == this->m_Timestamp) /* 对于完全相同的状态不执行更新 */
        {
            return false;
        }
        this->m_InGameState = in_game_state;
        this->m_Timestamp = timestamp;
        return true;
    }
    /*
    @brief 更新 `GameState` 对象。
    @param `gs` 当前日志对象更新为 `gs`。
    @note 如更新成功，返回 `true`，否则返回 `false`。`gs` 时间戳为 `0`（无效时间戳）或 `gs` 与 `*this` 完全相同会导致更新失败。
    */
    bool update(const InGameState& gs) noexcept
    {
        if (gs.m_Timestamp == 0) return false; /* 无效 GameState */
        if (gs.m_Timestamp > this->m_Timestamp) return false; /* 尝试用旧状态覆盖新状态 */
        if (gs == *this) return false; /* 两个 GameState 相同，无需更新 */
        this->m_InGameState = gs.m_InGameState;
        this->m_Timestamp = gs.m_Timestamp;
        return true;
    }
    inline bool operator==(const InGameState& gs) const
    {
        return this->m_InGameState == gs.m_InGameState && this->m_Timestamp == gs.m_Timestamp;
    }
    inline bool operator!=(const InGameState& gs) const
    {
        return this->m_Timestamp != gs.m_Timestamp || this->m_InGameState != gs.m_InGameState;
    }
    InGameState& operator=(const InGameState& gs)
    {
        if (gs.m_Timestamp == 0) return *this;
        if (gs == *this) return *this; /* 两个 GameState 相同，无需更新 */
        this->m_InGameState = gs.m_InGameState;
        this->m_Timestamp = gs.m_Timestamp;
        return *this;
    }
    inline ENUM_IN_GAME_STATE get_state() { return m_InGameState; }
    inline int64_t get_timestamp() { return m_Timestamp; }
};
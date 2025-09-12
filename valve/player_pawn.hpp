#pragma once
#include "../memory/memory.hpp"
#include "offsets.hpp"

struct vec3_t {
    float x, y, z;
};

class c_cs_player_pawn {
public:
    int m_iHealth(c_game* game) {
        return game->read<int>(reinterpret_cast<uintptr_t>(this) + offsets::m_iHealth);
    }
    int m_iTeamNum(c_game* game) {
        return game->read<int>(reinterpret_cast<uintptr_t>(this) + offsets::m_iTeamNum);
    }
    vec3_t m_vOldOrigin(c_game* game) {
        return game->read<vec3_t>(reinterpret_cast<uintptr_t>(this) + offsets::m_vOldOrigin);
    }
};
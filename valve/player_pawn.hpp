#pragma once
#include "../memory/memory.hpp"
#include "offsets.hpp"

struct vec3_t {
    float x{}, y{}, z{};

	const auto& is_zero() noexcept {
		return x == 0.f && y == 0.f && z == 0.f;
	}
};

struct vec2_t {
    float x{}, y{};
};

class c_cs_player_pawn {
public:
    int m_iHealth(c_game* game) {
        return game->read<int>(reinterpret_cast<uintptr_t>(this) + offsets::m_iHealth).value_or(0);
    }
    int m_iTeamNum(c_game* game) {
        return game->read<int>(reinterpret_cast<uintptr_t>(this) + offsets::m_iTeamNum).value_or(0);
    }
    vec3_t m_vOldOrigin(c_game* game) {
        return game->read<vec3_t>(reinterpret_cast<uintptr_t>(this) + offsets::m_vOldOrigin).value_or(vec3_t());
    }
};
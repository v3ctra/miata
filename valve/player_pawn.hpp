#pragma once
#include "../memory/memory.hpp"
#include "offsets.hpp"

struct vec3_t {
    constexpr vec3_t(const float x = 0.f, const float y = 0.f, const float z = 0.f) : x(x), y(y), z(z) { }

	const auto& is_zero() noexcept {
		return (this->x == 0.f && this->y == 0.f && this->z == 0.f);
	}

    float x{}, y{}, z{};
};

struct vec2_t {
    constexpr vec2_t(const float x = 0.f, const float y = 0.f) : x(x), y(y) { }

    const auto& is_zero() noexcept {
        return (this->x == 0.f && this->y == 0.f);
    }

    vec2_t rotate_point(vec2_t* mid_point, float angle) {
        // Convert our angle from degrees to radians.
        angle = static_cast<float>(angle * (std::numbers::pi / 180.f));

        // Get our current angle as a cosine and sine.
        float cos_angle = std::cos(angle);
        float sin_angle = std::sin(angle);
		
        vec2_t out{};

        // Calculate the rotation.
        out.x = cos_angle * (this->x - mid_point->x) - sin_angle * (this->y - mid_point->y);
        out.y = sin_angle * (this->x - mid_point->x) + cos_angle * (this->y - mid_point->y);

        // Add the mid-point to the calculated point.
        out.x += mid_point->x;
        out.y += mid_point->y;

        return out;
    }

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
        return game->read<vec3_t>(reinterpret_cast<uintptr_t>(this) + offsets::m_vOldOrigin).value_or(vec3_t(0, 0, 0));
    }
};
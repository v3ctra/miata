#pragma once
#include "../memory/memory.hpp"
#include "offsets.hpp"

struct vec3_t {
    constexpr vec3_t(const float x = 0.f, const float y = 0.f, const float z = 0.f) : x(x), y(y), z(z) { }

    [[nodiscard]] bool is_zero() const noexcept {
        return (this->x == 0.f && this->y == 0.f && this->z == 0.f);
    }

    float x{}, y{}, z{};
};

struct vec2_t {
    constexpr vec2_t(const float x = 0.f, const float y = 0.f) : x(x), y(y) { }

    [[nodiscard]] bool is_zero() const noexcept {
        return (this->x == 0.f && this->y == 0.f);
    }

    [[nodiscard]]
    vec2_t rotate_point(vec2_t* mid_point, float angle) const noexcept {
        // Convert our angle from degrees to radians.
        angle = static_cast<float>(angle * (std::numbers::pi / 180.f));

        // Get our current angle as a cosine and sine.
        const auto cos_angle = std::cos(angle);
        const auto sin_angle = std::sin(angle);
		
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
    [[nodiscard]]
    int m_iHealth(c_game* game) const noexcept {
        return *game->read<int>(reinterpret_cast<uintptr_t>(this) + offsets::m_iHealth);
    }
    [[nodiscard]]
    int m_iTeamNum(c_game* game) const noexcept {
        return *game->read<int>(reinterpret_cast<uintptr_t>(this) + offsets::m_iTeamNum);
    }
    [[nodiscard]]
    vec3_t m_vOldOrigin(c_game* game) const noexcept {
        return *game->read<vec3_t>(reinterpret_cast<uintptr_t>(this) + offsets::m_vOldOrigin);
    }
};
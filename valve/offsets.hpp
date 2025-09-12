#pragma once
#include <cstddef>

namespace offsets {
    constexpr std::ptrdiff_t dwLocalPlayerPawn = 0x1BF2490;
    constexpr std::ptrdiff_t dwEntityList = 0x1D15F88;
    constexpr std::ptrdiff_t dwViewAngles = 0x1E3DC20;

    constexpr std::ptrdiff_t m_hPlayerPawn = 0x8FC; // CHandle<C_CSPlayerPawn>
    constexpr std::ptrdiff_t m_iHealth = 0x34C; // int32
    constexpr std::ptrdiff_t m_iTeamNum = 0x3EB; // uint8
    constexpr std::ptrdiff_t m_vOldOrigin = 0x15B0; // Vector
}
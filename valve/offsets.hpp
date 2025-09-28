#pragma once
#include <cstddef>

namespace offsets {
    constexpr std::ptrdiff_t dwLocalPlayerPawn = 0x1BDFD10;
    constexpr std::ptrdiff_t dwEntityList = 0x1D042D8;
    constexpr std::ptrdiff_t dwViewAngles = 0x1E2BBA0;

    constexpr std::ptrdiff_t m_hPlayerPawn = 0x8FC; // CHandle<C_CSPlayerPawn>
    constexpr std::ptrdiff_t m_iHealth = 0x34C; // int32
    constexpr std::ptrdiff_t m_iTeamNum = 0x3EB; // uint8
    constexpr std::ptrdiff_t m_vOldOrigin = 0x15B8; // Vector
}
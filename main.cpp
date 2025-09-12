#include <Windows.h>
#include <thread>
#include <mutex>

#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
#include <map>

#include "d3d/device.hpp"
#include "utils/utils.hpp"
#include "thirdparty/daisy.hpp"
#include "memory/memory.hpp"

#pragma comment(lib, "d3d9.lib")

#include "valve/player_pawn.hpp"

#include "main.hpp"

std::vector<c_cs_player_pawn*> m_players{};
std::mutex m_players_mutex{};

void entity_cache_thread(c_game* game) {
    std::vector<c_cs_player_pawn*> temp_players{};
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        uintptr_t entity_list = game->read<uintptr_t>(game->get_client_base() + offsets::dwEntityList);//dwEntityList
        temp_players.clear();
        for (int i = 1; i < 64; i++) {
            uintptr_t list_entity_controller = game->read<uintptr_t>(entity_list + ((8 * (i & 0x7FFF) >> 9) + 16));
            uintptr_t entity_controller = game->read<uintptr_t>(list_entity_controller + (120) * (i & 0x1FF));

            uintptr_t entity_controller_pawn = game->read<uintptr_t>(entity_controller + offsets::m_hPlayerPawn); 
            uintptr_t list_entity = game->read<uintptr_t>(entity_list + (0x8 * ((entity_controller_pawn & 0x7FFF) >> 9) + 16));

            c_cs_player_pawn* entity = game->read<c_cs_player_pawn*>(list_entity + (120) * (entity_controller_pawn & 0x1FF));
            temp_players.emplace_back(entity);
        }
        {
            std::unique_lock<std::mutex> lock(m_players_mutex);
            m_players = temp_players;
        }
    }
}

void rotate_point(vec2_t point_to_rotate, vec2_t* mid_point, float angle, vec2_t* out) {
    // Convert our angle from degrees to radians.
    angle = static_cast<float>(angle * (3.14 / 180.f));//std::numbers::

    // Get our current angle as a cosine and sine.
    float cosAngle = (float)cos(angle);
    float sinAngle = (float)sin(angle);

    // Calculate the rotation.
    out->x = cosAngle * (point_to_rotate.x - mid_point->x) - sinAngle * (point_to_rotate.y - mid_point->y);
    out->y = sinAngle * (point_to_rotate.x - mid_point->x) + cosAngle * (point_to_rotate.y - mid_point->y);

    // Add the mid-point to the calculated point.
    out->x += mid_point->x;
    out->y += mid_point->y;
}

std::vector<vec2_t> radar_points{};
std::mutex radar_points_mutex{};

void entity_data_thread(c_game* game) {
    std::vector<vec2_t> temp_radar_points{};
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        c_cs_player_pawn* local_pawn = game->read<c_cs_player_pawn*>(game->get_client_base() + offsets::dwLocalPlayerPawn);
        if (!local_pawn)
            continue;

        vec3_t local_pos = local_pawn->m_vOldOrigin(game);
        int local_team = local_pawn->m_iTeamNum(game);

        vec2_t radar_center{};

        // Get the center of our radar position.
        radar_center.x = (static_cast<float>(g_config.size_w) / 2);
        radar_center.y = (static_cast<float>(g_config.size_h) / 2);

        vec3_t viewangles = game->read<vec3_t>(game->get_client_base() + offsets::dwViewAngles);

        temp_radar_points.clear();
        std::unique_lock<std::mutex> lock(m_players_mutex);
        for (const auto& players : m_players) {
            if (!players || local_pawn == players || local_team == players->m_iTeamNum(game))
                continue;

            if (players->m_iHealth(game) <= 0)
                continue;

            vec2_t screen_pos{};

            // Get player radar position.
            vec3_t origin = players->m_vOldOrigin(game);
            screen_pos.x = (local_pos.x - origin.x) / 15;
            screen_pos.y = (local_pos.y - origin.y) / 15;

            // Prevent inverted x axis position.
            screen_pos.x *= -1;

            // Ensure position starts at center.
            screen_pos.x += radar_center.x;
            screen_pos.y += radar_center.y;

            // Rotate the point based on our yaw.
            vec2_t rotated_point{};
            rotate_point(screen_pos, &radar_center, viewangles.y - 90.f, &rotated_point);

            // Prevent player from going out of bounds.
            rotated_point.x = std::clamp(rotated_point.x, 5.f, static_cast<float>(g_config.size_w) - 9);
            rotated_point.y = std::clamp(rotated_point.y, 5.f, static_cast<float>(g_config.size_h) - 9);

            // Store the point in the list
            temp_radar_points.emplace_back(vec2_t{ rotated_point.x, rotated_point.y });
        }
        {
            std::unique_lock<std::mutex> lock(radar_points_mutex);
            radar_points = temp_radar_points;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

using config_map = std::map<std::string, std::map<std::string, std::map<std::string, int>>>;
bool parse_config(const std::string& filename, config_map& config) {
    config.clear();

    std::ifstream file(filename);
    if (!file) {
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line.find('=') == std::string::npos) continue;

        auto pos = line.find('=');
        std::string key = line.substr(0, pos);
        std::string value_str = line.substr(pos + 1);

        std::istringstream key_stream(key);
        std::string part;
        std::string parts[3];
        int idx = 0;
        while (std::getline(key_stream, part, '.') && idx < 3) {
            parts[idx++] = part;
        }
        if (idx != 3) {
            return false;
        }

        int value = std::stoi(value_str);
        config[parts[0]][parts[1]][parts[2]] = value;
    }

    return true;
}
/*
void send_points_to_serial(const std::vector<vec2_t>& points, HANDLE& hSerial) {
    for (const auto& p : points) {
        std::ostringstream ss;
        ss << static_cast<int>(p.x) << "," << static_cast<int>(p.y) << "\n";
        std::string out = ss.str();
        DWORD written;
        WriteFile(hSerial, out.c_str(), out.size(), &written, nullptr);
    }
    DWORD written2;
    WriteFile(hSerial, "END\n", 4, &written2,nullptr); // Signal end of frame
}*/

int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous_instance, LPSTR cmd_line, int cmd_show) {
    config_map config{};
    bool success = parse_config("settings.txt", config);// Example: Access radar.pos.x
    if (success) {
        g_config.pos_x = config["radar"]["pos"]["x"];
        g_config.pos_y = config["radar"]["pos"]["y"];
        g_config.size_w = config["radar"]["size"]["w"];
        g_config.size_h = config["radar"]["size"]["h"];
        g_config.hide_from_recording = config["cheat"]["window"]["hide"];
    }
    else {
        MessageBox(0, L"Failed to parse settings.txt, using default values", 0, MB_OK | MB_ICONWARNING);
        g_config.pos_x = 10;
        g_config.pos_y = 200;
        g_config.size_w = 150;
        g_config.size_h = 150;
        g_config.hide_from_recording = 0;
    }

    c_game game{};
    if (!game.initialize()) {
        MessageBox(0, L"Failed to query game info.", 0, MB_OK | MB_ICONERROR);
        return EXIT_FAILURE;
    }
    
    const wchar_t WINDOW_NAME[] = L"Miata(c) staromodny";
    const wchar_t CLASS_NAME[] = L"SimpleWinClass";

    WNDCLASS wc = {};
    wc.lpfnWndProc = wnd_proc;
    wc.hInstance = instance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

    if (!RegisterClass(&wc)) {
        utils::show_last_error(L"RegisterClass");
        return EXIT_FAILURE;
    }

    // Step 2: Create the window
    HWND hwnd = CreateWindowExW(
        WS_EX_TOPMOST,
        CLASS_NAME,
        WINDOW_NAME,
        WS_POPUP | WS_VISIBLE,
        g_config.pos_x, g_config.pos_y, g_config.size_w, g_config.size_h,
        nullptr, nullptr, instance, nullptr
    );

    if (!hwnd) {
        utils::show_last_error(L"CreateWindowEx");
        return EXIT_FAILURE;
    }

    c_d3d_device device(hwnd, true);
    if (!device.initialize(g_config.size_w, g_config.size_h)) {
        return EXIT_FAILURE;
    }

    if (g_config.hide_from_recording == 1) {
        BOOL result = SetWindowDisplayAffinity(hwnd, WDA_EXCLUDEFROMCAPTURE);
        if (!result)
            MessageBox(0, L"Failed to hide window from capture.", 0, MB_OK | MB_ICONWARNING);
    }

    ShowWindow(hwnd, cmd_show);
    UpdateWindow(hwnd);

    // prepare d3d9 state for rendering
    // if daisy is the only thing you plan on rendering with, you can only do this once *safely*
    // if you're implementing daisy alongside in a codebase that changes device state
    // you probably want to call this each frame before flushing render queues
    daisy::daisy_prepare();

    bool done = false;

    daisy::c_renderqueue queue{};
    if (!queue.create(64, 128)) {
        return MessageBox(0, L"Failed to create buffer queue.", 0, MB_OK | MB_ICONERROR);
    }

    std::thread cache_thread(entity_cache_thread, &game); cache_thread.detach();
    std::thread data_thread(entity_data_thread, &game); data_thread.detach();
    /*
    HANDLE hSerial = CreateFile(L"\\\\.\\COM5", GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (hSerial == INVALID_HANDLE_VALUE) {
       // std::cout << "Error opening serial port.\n";
        return 1;
    }

    DCB dcbSerialParams = { 0 };
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    GetCommState(hSerial, &dcbSerialParams);
    dcbSerialParams.BaudRate = CBR_115200;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;
    SetCommState(hSerial, &dcbSerialParams);
    */
    while (!done || FindWindow(L"SDL_app", L"Counter-Strike 2") || !GetAsyncKeyState(VK_DELETE) & 1) {
        MSG msg{};
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;
        
        device.get()->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_RGBA(0, 0, 0, 255), 1.0f, 0);
        device.get()->BeginScene();
        
        queue.clear();
        
        {
            std::unique_lock<std::mutex> lock(radar_points_mutex);
          //  send_points_to_serial(radar_points,hSerial);
            for (const auto& pt : radar_points) {
                queue.push_filled_rectangle({ pt.x, pt.y }, { 5, 5 }, daisy::color_t(0, 255, 0, 255));
            }
        }
        
        queue.flush();
        device.get()->EndScene();
        device.get()->Present(nullptr, nullptr, nullptr, nullptr);

        // 60 fps
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    ::DestroyWindow(hwnd);
    ::UnregisterClass(wc.lpszClassName, wc.hInstance);
    return EXIT_SUCCESS;
}
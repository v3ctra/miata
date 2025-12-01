#include <Windows.h>
#include <windowsx.h>

#include <thread>
#include <mutex>

#include <xmmintrin.h>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <numbers>
#include <string>
#include <map>

#include "d3d/device.hpp"
#include "utils/utils.hpp"
#include "thirdparty/daisy.hpp"
#include "memory/memory.hpp"

#pragma comment(lib, "d3d9.lib")

#include "valve/player_pawn.hpp"

#include "main.hpp"

std::atomic_bool done = false;

std::vector<c_cs_player_pawn*> m_players{};
std::mutex m_players_mutex{};

static void entity_cache_thread(c_game* game) {
    std::vector<c_cs_player_pawn*> temp_players{};

    const auto client_base = game->get_client_base();
    if (!client_base) {
        return;
    }

    while (!done) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        const auto entity_list = game->read<uintptr_t>(client_base + offsets::dwEntityList).value_or(0);
        if (!entity_list)
            continue;

        temp_players.clear();
        for (int i = 1; i < 64; i++) {
            const auto list_entity_controller = game->read<uintptr_t>(entity_list + ((8 * (i & 0x7FFF) >> 9) + 16)).value_or(0);
            if (!list_entity_controller)
                continue;

            const auto entity_controller = game->read<uintptr_t>(list_entity_controller + (120) * (i & 0x1FF)).value_or(0);
            if (!entity_controller)
                continue;

            const auto entity_controller_pawn = game->read<uintptr_t>(entity_controller + offsets::m_hPlayerPawn).value_or(0);
            if (!entity_controller_pawn)
                continue;

            const auto list_entity = game->read<uintptr_t>(entity_list + (0x8 * ((entity_controller_pawn & 0x7FFF) >> 9) + 16)).value_or(0);
            if (!list_entity)
                continue;

            const auto entity = game->read<c_cs_player_pawn*>(list_entity + (120) * (entity_controller_pawn & 0x1FF)).value_or(nullptr);
            if (!entity)
                continue;

            temp_players.emplace_back(entity);
        }
        {
            std::unique_lock<std::mutex> lock(m_players_mutex);
            m_players = std::move(temp_players);
        }
    }
}

std::vector<vec2_t> radar_points{};
std::mutex radar_points_mutex{};

static void entity_data_thread(c_game* game) {
    std::vector<vec2_t> temp_radar_points{};

    const auto client_base = game->get_client_base();
    if (!client_base)
        return;

    while (!done) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        vec3_t viewangles = game->read<vec3_t>(client_base + offsets::dwViewAngles).value();
		if (viewangles.is_zero()) // probably not even in the game
			continue;

        const auto local_pawn = game->read<c_cs_player_pawn*>(client_base + offsets::dwLocalPlayerPawn).value_or(nullptr);
        if (!local_pawn)
            continue;

        const auto local_pos = local_pawn->m_vOldOrigin(game);
        const auto local_team = local_pawn->m_iTeamNum(game);

        vec2_t radar_center{};
        radar_center.x = (static_cast<float>(g_config.size_w) / 2);
        radar_center.y = (static_cast<float>(g_config.size_h) / 2);

        temp_radar_points.clear();
        std::unique_lock<std::mutex> lock(m_players_mutex);
        for (const auto& players : m_players) {
            if (!players || local_pawn == players || local_team == players->m_iTeamNum(game))
                continue;

            if (players->m_iHealth(game) <= 0)
                continue;

            // Get player radar position.
            const auto origin = players->m_vOldOrigin(game);
            if (origin.is_zero())
                continue;

            vec2_t screen_pos{};
            screen_pos.x = (local_pos.x - origin.x) / 15; //scale. 
            screen_pos.y = (local_pos.y - origin.y) / 15;

            // Prevent inverted x axis position.
            screen_pos.x *= -1;

            // Ensure position starts at center.
            screen_pos.x += radar_center.x;
            screen_pos.y += radar_center.y;

            // Rotate the point based on our yaw.
            auto rotated_point = screen_pos.rotate_point(&radar_center, viewangles.y - 90.f);

            // Prevent player from going out of bounds.
            rotated_point.x = std::clamp(rotated_point.x, 5.f, static_cast<float>(g_config.size_w) - 10.f);
            rotated_point.y = std::clamp(rotated_point.y, 5.f, static_cast<float>(g_config.size_h) - 10.f);

            // Store the point in the list
            temp_radar_points.emplace_back(rotated_point);
        }
        {
            std::unique_lock<std::mutex> lock(radar_points_mutex);
            radar_points = std::move(temp_radar_points);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

POINT click_pos{};
bool dragging{ false };

LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_LBUTTONDOWN: {
        dragging = true;
        SetCapture(hwnd); // capture mouse even when leaving window

        click_pos.x = GET_X_LPARAM(lParam);
        click_pos.y = GET_Y_LPARAM(lParam);
        return 0;
    }
    case WM_MOUSEMOVE: {
        if (dragging) {
            POINT p;
            GetCursorPos(&p);

            int newX = p.x - click_pos.x;
            int newY = p.y - click_pos.y;

            SetWindowPos(hwnd, nullptr, newX, newY, 0, 0,
                SWP_NOZORDER | SWP_NOSIZE);

            return 0;
        }
        break;
    }
    case WM_LBUTTONUP: {
        dragging = false;
        ReleaseCapture();
        return 0;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

using config_map = std::map<std::string, std::map<std::string, std::map<std::string, int>>>;
static bool parse_config(const std::string& filename, config_map& config) {
    config.clear();

    std::ifstream file(filename);
    if (!file) {
        return false;
    }

    std::string line{};
    while (std::getline(file, line)) {
        if (line.empty() || line.find('=') == std::string::npos) continue;

        auto pos = line.find('=');
        std::string key = line.substr(0, pos);
        std::string value_str = line.substr(pos + 1);

        std::istringstream key_stream(key);
        std::string part{};
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

int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous_instance, LPSTR cmd_line, int cmd_show) {
    config_map config{};
    const auto success = parse_config("settings.txt", config);
    if (success) {
        g_config.pos_x = config["radar"]["pos"]["x"];
        g_config.pos_y = config["radar"]["pos"]["y"];
        g_config.size_w = config["radar"]["size"]["w"];
        g_config.size_h = config["radar"]["size"]["h"];
        g_config.hide_from_recording = config["cheat"]["window"]["hide"];
        g_config.vertical_sync = config["cheat"]["window"]["vsync"];
    }
    else {
        show_message_box<wchar_t>(L"Failed to parse settings.txt, using default values", L"", buttons::OK | icons::Warning);
        g_config.pos_x = 10;
        g_config.pos_y = 200;
        g_config.size_w = 150;
        g_config.size_h = 150;
        g_config.hide_from_recording = 0;
        g_config.vertical_sync = 0;
    }

    bool offsets_parser = parse_config("offsets.txt", config);
    if (offsets_parser) {
        offsets::dwLocalPlayerPawn = config["offsets"]["client"]["dwLocalPlayerPawn"];
        offsets::dwEntityList = config["offsets"]["client"]["dwEntityList"];
        offsets::dwViewAngles = config["offsets"]["client"]["dwViewAngles"];

        offsets::m_hPlayerPawn = config["offsets"]["client"]["m_hPlayerPawn"];
        offsets::m_iHealth = config["offsets"]["client"]["m_iHealth"];
        offsets::m_iTeamNum = config["offsets"]["client"]["m_iTeamNum"];
        offsets::m_vOldOrigin = config["offsets"]["client"]["m_vOldOrigin"];
    }
    else {
        return *show_message_box<wchar_t>(L"Failed to parse offsets.", L"", buttons::OK | icons::Error);
    }

    const auto game = std::make_unique<c_game>();
    if (!game->initialize()) {
        return *show_message_box<wchar_t>(L"Failed to query game info.", L"", buttons::OK | icons::Error);
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
        return utils::show_last_error(L"RegisterClass");
    }

    // Step 2: Create the window
    const auto hwnd = CreateWindowEx(
        WS_EX_TOPMOST,
        CLASS_NAME,
        WINDOW_NAME,
        WS_POPUP | WS_VISIBLE,
        g_config.pos_x, g_config.pos_y, g_config.size_w, g_config.size_h,
        nullptr, nullptr, instance, nullptr
    );

    if (!hwnd) {
        return utils::show_last_error(L"CreateWindowEx");
    }

    const auto device = std::make_unique<c_d3d_device>(hwnd, static_cast<bool>(g_config.vertical_sync));
    if (!device->initialize(g_config.size_w, g_config.size_h)) {
        return EXIT_FAILURE;
    }

    if (g_config.hide_from_recording == 1) {
        if (const auto result = SetWindowDisplayAffinity(hwnd, WDA_EXCLUDEFROMCAPTURE); !result) { // introduced in windows 10 2004
            utils::show_last_error(L"SetWindowDisplayAffinity"); // not a critical failure.
        }
    }

    ShowWindow(hwnd, cmd_show);
    UpdateWindow(hwnd);

    // prepare d3d9 state for rendering
    // if daisy is the only thing you plan on rendering with, you can only do this once *safely*
    // if you're implementing daisy alongside in a codebase that changes device state
    // you probably want to call this each frame before flushing render queues
    daisy::daisy_prepare();

    const auto queue = std::make_unique<daisy::c_renderqueue>();
    if (!queue->create(64, 128)) {
        return *show_message_box<wchar_t>(L"Failed to create buffer queue", L"", buttons::OK | icons::Error);
    }

    std::thread cache_thread(entity_cache_thread, game.get()); cache_thread.detach();
    std::thread data_thread(entity_data_thread, game.get()); data_thread.detach();
   
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
        
        if (device->begin_rendering()) {
            queue->clear();
            {
                std::unique_lock<std::mutex> lock(radar_points_mutex);
                //  send_points_to_serial(radar_points,hSerial);
                for (const auto& pt : radar_points) {
                    queue->push_filled_rectangle({ pt.x, pt.y }, { 5, 5 }, daisy::color_t(0, 255, 0, 255));
                }

                // Vertical center line
                queue->push_line({ static_cast<float>(g_config.size_w / 2.f), 0.f }, { static_cast<float>(g_config.size_w / 2.f), static_cast<float>(g_config.size_h) }, daisy::color_t(0, 255, 0, 128));
                // Horizontal center line
                queue->push_line({ 0, static_cast<float>(g_config.size_h / 2.f) }, { static_cast<float>(g_config.size_w), static_cast<float>(g_config.size_h / 2.f) }, daisy::color_t(0, 255, 0, 128));
            }
            queue->flush();
            device->end_rendering();
        }

        // 60 fps (vsync btw)
        //std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    daisy::daisy_shutdown();

    ::DestroyWindow(hwnd);
    ::UnregisterClass(wc.lpszClassName, wc.hInstance);
    return EXIT_SUCCESS;
}
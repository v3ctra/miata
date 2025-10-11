#include "utils.hpp"

#include <format>
#include <string>

int utils::show_last_error(LPCWSTR context) {
    DWORD error_code = GetLastError();
    LPWSTR message_buffer = nullptr;

    FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        error_code,
        0, // Default language
        std::bit_cast<LPWSTR>(&message_buffer),
        0,
        nullptr
    );

    std::wstring full_message = std::format(L"{} failed with error {}:\n{}", context, error_code, message_buffer);
    LocalFree(message_buffer);

    return MessageBoxW(nullptr, full_message.c_str(), L"", MB_ICONERROR | MB_OK);
}
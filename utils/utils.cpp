#include "utils.hpp"

#include <format>
#include <string>

int utils::show_last_error(LPCWSTR context) {
    const auto error_code = GetLastError();
    LPWSTR message_buffer = nullptr;

    FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        error_code,
        0, // Default language
        reinterpret_cast<LPWSTR>(&message_buffer),
        0,
        nullptr
    );

    const auto full_message = std::format(L"{} failed with error {}:\n{}", context, error_code, message_buffer);
    LocalFree(message_buffer);

    return *show_message_box<wchar_t>(full_message, L"", buttons::OK | icons::Error);
}
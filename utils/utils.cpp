#include "utils.hpp"

void utils::show_last_error(LPCWSTR context) {
    DWORD errorCode = GetLastError();
    LPWSTR messageBuffer = nullptr;

    FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        errorCode,
        0, // Default language
        (LPWSTR)&messageBuffer,
        0,
        nullptr
    );

    WCHAR fullMessage[512];
    wsprintfW(fullMessage, L"%s failed with error %lu:\n%s", context, errorCode, messageBuffer);

    MessageBoxW(nullptr, fullMessage, L"", MB_ICONERROR | MB_OK);

    LocalFree(messageBuffer);
}
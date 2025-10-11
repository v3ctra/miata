#include "memory.hpp"

#include <TlHelp32.h>
#include <thread>

bool c_game::initialize() {
    const std::wstring_view& target_name = L"cs2.exe";
    const unique_handle snapshot(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));
    if (!snapshot)
        return false;

    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);

    //Enumerate running processes
    for (Process32First(snapshot.get(), &entry); Process32Next(snapshot.get(), &entry); ) {
        if (target_name.compare(entry.szExeFile) == 0) {
            m_process_id = entry.th32ProcessID;
            m_process_handle = open_process(m_process_id.value(), PROCESS_VM_READ).value();
            break;
        }
    }

    if (m_process_id == 0)
        return false;

    while (m_client_base == 0) {
        m_client_base = get_module_by_name(m_process_id.value(), L"client.dll");
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    return m_client_base != 0 && m_process_id != 0 && m_process_handle != nullptr;
}

std::optional<DWORD64> c_memory::get_module_by_name(DWORD pid, const std::wstring_view& target_module) {
    const unique_handle snapshot{ CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid) };
    if (!snapshot) {
        return std::nullopt;
    }

    MODULEENTRY32W entry{};
    entry.dwSize = sizeof(entry);

    // First module
    if (!Module32FirstW(snapshot.get(), &entry)) {
        return std::nullopt;
    }

    do {
        if (target_module.compare(entry.szModule) == 0) {
            return std::bit_cast<uintptr_t>(entry.modBaseAddr);
        }
    } while (Module32NextW(snapshot.get(), &entry));

    return std::nullopt;
}

std::optional<unique_handle> c_memory::open_process(DWORD pid, DWORD access_flags) {
    HANDLE handle = OpenProcess(access_flags, FALSE, pid); //If the function fails, the return value is NULL.
    if (handle == NULL) {
        return std::nullopt;
    }

    return unique_handle{ handle };
}
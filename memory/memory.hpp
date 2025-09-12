#pragma once
#include <Windows.h>

#include <string_view>
#include <optional>
#include <memory>

struct handle_disposer_t {
    using pointer = HANDLE;

    void operator()(HANDLE handle) const {
        if (handle != NULL || handle != INVALID_HANDLE_VALUE) {
            CloseHandle(handle);
        }
    }
};
using unique_handle = std::unique_ptr<HANDLE, handle_disposer_t>;

class c_memory {
public:
	std::optional<DWORD> get_pid_by_name(const std::wstring_view& target_name);
    std::optional<DWORD64> get_module_by_name(DWORD pid, const std::wstring_view& target_module);
    std::optional<unique_handle> open_process(DWORD pid, DWORD access_flags);
};

class c_game : public c_memory {
public:
    c_game() : m_process_handle(nullptr), m_process_id(0), m_client_base(0) {}
    ~c_game() { }
    bool initialize();

    const auto& get_game_id() noexcept { return m_process_id.value_or(0); }
    const auto& get_client_base() noexcept { return m_client_base.value_or(0); }

    template <typename T>
    constexpr const T read(const std::uintptr_t& address) const noexcept {
        T value = { };
        ::ReadProcessMemory(m_process_handle.get(), reinterpret_cast<const void*>(address), &value, sizeof(T), nullptr);
        return value;
    }
private:
    unique_handle m_process_handle{};
    std::optional<DWORD64> m_client_base{};
    std::optional<DWORD> m_process_id{};
};
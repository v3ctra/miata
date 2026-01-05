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
    [[nodiscard]]
    std::optional<uintptr_t> get_module_by_name(DWORD pid, const std::wstring_view& target_module);
    
    [[nodiscard]]
    std::optional<unique_handle> open_process(DWORD pid, DWORD access_flags);
};

class c_game : public c_memory {
public:
    c_game() : m_process_handle(nullptr), m_process_id(0), m_client_base(0) {}
    ~c_game() { }

    [[nodiscard]]
    bool initialize();

    [[nodiscard]]
    const auto& get_client_base() noexcept { return *this->m_client_base; }

    template <typename T>
    [[nodiscard]]
    constexpr std::optional<T> read(const std::uintptr_t& address) const noexcept {
        T value = { };
        if (::ReadProcessMemory(this->m_process_handle.get(), reinterpret_cast<const void*>(address), &value, sizeof(T), nullptr))
            return value;
        return std::nullopt;
    }
private:
    unique_handle m_process_handle{};
    std::optional<uintptr_t> m_client_base{};
    std::optional<DWORD> m_process_id{};
};
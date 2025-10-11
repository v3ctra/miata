#pragma once
#include <Windows.h>

#include <wrl/client.h>

#include <d3d9.h>
#include <memory>

class c_d3d_device {
public:
    c_d3d_device(const HWND& target_window, const bool& vsync) : m_hwnd(target_window) {};
    ~c_d3d_device() = default;

    bool initialize(const int& width, const int& height);

    bool begin_rendering();
    void end_rendering();

    const auto& get() noexcept {
        return this->m_device;
    }
private:
    D3DPRESENT_PARAMETERS m_params{};
    Microsoft::WRL::ComPtr<IDirect3D9> m_d3d{};
    Microsoft::WRL::ComPtr<IDirect3DDevice9> m_device{};

	bool m_vsync{ false };
	HWND m_hwnd{ 0 };
};
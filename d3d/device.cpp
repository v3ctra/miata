#include "device.hpp"

#include "../thirdparty/daisy.hpp"

bool c_d3d_device::initialize(const int& width, const int& height) {
	// create d3d9 interface
	m_d3d = Microsoft::WRL::ComPtr<IDirect3D9>(Direct3DCreate9(D3D_SDK_VERSION));
	if (!m_d3d) {
		MessageBoxA(m_hwnd, "Failed to create D3D interface", "", MB_OK | MB_ICONERROR);
		return false;
	}

	ZeroMemory(&m_params, sizeof(m_params));
	m_params.Windowed = TRUE;
	m_params.SwapEffect = D3DSWAPEFFECT_DISCARD;
	m_params.BackBufferFormat = D3DFMT_A8R8G8B8;
	m_params.BackBufferWidth = width;
	m_params.BackBufferHeight = height;
	m_params.EnableAutoDepthStencil = TRUE;
	m_params.AutoDepthStencilFormat = D3DFMT_D16;
	m_params.PresentationInterval = m_vsync ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_IMMEDIATE; // V-sync (one), TODO; customizable.

	HRESULT res = m_d3d->CreateDevice(
		D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL,
		m_hwnd,
		D3DCREATE_HARDWARE_VERTEXPROCESSING,
		&m_params,
		m_device.GetAddressOf());

	if (FAILED(res)) {
		res = m_d3d->CreateDevice(
			D3DADAPTER_DEFAULT,
			D3DDEVTYPE_HAL,
			m_hwnd,
			D3DCREATE_SOFTWARE_VERTEXPROCESSING,
			&m_params,
			m_device.GetAddressOf());
	}

	if (FAILED(res)) {
		MessageBoxA(m_hwnd, "Failed to create D3D device", "", MB_OK | MB_ICONERROR);
		return false;
	}

	// initialize the library with the d3d9 device pointer
	daisy::daisy_initialize(m_device.Get());

	return true;
}

bool c_d3d_device::begin_rendering() {
	m_device->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_RGBA(0, 0, 0, 255), 1.0f, 0);
	return m_device->BeginScene() == D3D_OK;
}

void c_d3d_device::end_rendering() {
	m_device->EndScene();
	m_device->Present(nullptr, nullptr, nullptr, nullptr);
}
#include <windows.h>

#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "d3dcompiler.lib")

#include <d3d11.h>
#include <d3dcompiler.h>
#include <cstdio>
#include <iostream>

#include "External/dxerr.h"

#include "External/imgui.h"
#include "External/imgui_impl_dx11.h"

#include "Globals.h"
#include "Editor.h"

extern LRESULT ImGui_ImplDX11_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

IDXGISwapChain *gSwapChain;

ID3D11Device *gDevice;
ID3D11DeviceContext *gDeviceContext;

ID3D11DepthStencilView *gDepthbufferDSV;
ID3D11ShaderResourceView *gDepthbufferSRV;
ID3D11RenderTargetView *gDepthbufferRTV;
ID3D11RenderTargetView *gBackbufferRTV;

ID3D11DepthStencilState *gDepthReadWrite;
ID3D11DepthStencilState *gDepthRead;

void createDepthBuffer()
{
	ID3D11Texture2D* pDepthStencil = NULL;
	D3D11_TEXTURE2D_DESC descDepth;
	descDepth.Width = EWIDTH;
	descDepth.Height = EHEIGHT;
	descDepth.MipLevels = 1;
	descDepth.ArraySize = 1;
	descDepth.Format = DXGI_FORMAT_R24G8_TYPELESS;
	descDepth.SampleDesc.Count = 1;
	descDepth.SampleDesc.Quality = 0;
	descDepth.Usage = D3D11_USAGE_DEFAULT;
	descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	descDepth.CPUAccessFlags = 0;
	descDepth.MiscFlags = 0;
	DXCALL(gDevice->CreateTexture2D(&descDepth, NULL, &pDepthStencil));

	D3D11_DEPTH_STENCIL_DESC dsDesc;
	dsDesc.DepthEnable = true;
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
	dsDesc.StencilEnable = true;
	dsDesc.StencilReadMask = 0xFF;
	dsDesc.StencilWriteMask = 0xFF;
	dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	dsDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	dsDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	DXCALL(gDevice->CreateDepthStencilState(&dsDesc, &gDepthReadWrite));
	gDeviceContext->OMSetDepthStencilState(gDepthReadWrite, 1);

	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	DXCALL(gDevice->CreateDepthStencilState(&dsDesc, &gDepthRead));

	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
	ZeroMemory(&descDSV, sizeof(descDSV));
	descDSV.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice = 0;

	D3D11_SHADER_RESOURCE_VIEW_DESC sdesc;
	ZeroMemory(&sdesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));

	sdesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	sdesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	sdesc.Texture2D.MipLevels = 1;
	sdesc.Texture2D.MostDetailedMip = 0;

	DXCALL(gDevice->CreateShaderResourceView(pDepthStencil, &sdesc, &gDepthbufferSRV));
	DXCALL(gDevice->CreateDepthStencilView(pDepthStencil, &descDSV, &gDepthbufferDSV));
}

HRESULT createDirect3DContext(HWND wndHandle)
{
	DXGI_SWAP_CHAIN_DESC scd;
	ZeroMemory(&scd, sizeof(DXGI_SWAP_CHAIN_DESC));

	scd.BufferCount = 1;
	scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	scd.BufferDesc.RefreshRate.Numerator = 60;
	scd.BufferDesc.RefreshRate.Denominator = 1;
	scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scd.OutputWindow = wndHandle;
	scd.SampleDesc.Count = 1;
	scd.Windowed = true;

	HRESULT hr = D3D11CreateDeviceAndSwapChain(NULL,
		D3D_DRIVER_TYPE_HARDWARE,
		NULL,
		D3D11_CREATE_DEVICE_DEBUG,
		NULL,
		NULL,
		D3D11_SDK_VERSION,
		&scd,
		&gSwapChain,
		&gDevice,
		NULL,
		&gDeviceContext);

	if (SUCCEEDED(hr))
	{
		ID3D11Texture2D* pBackBuffer = nullptr;
		DXCALL(gSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer));

		DXCALL(gDevice->CreateRenderTargetView(pBackBuffer, NULL, &gBackbufferRTV));

		createDepthBuffer();
	}

	return hr;
}

LRESULT WINAPI WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplDX11_WndProcHandler(hWnd, message, wParam, lParam))
		return true;

	switch (message)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

HWND InitWindow(HINSTANCE hInstance)
{
	WNDCLASSEX wcex = { 0 };
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.cbSize = sizeof(WNDCLASSEX);

	//wcex.hIcon = ;
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.hInstance = hInstance;
	wcex.lpszClassName = L"ParticleEditor";
	if (!RegisterClassEx(&wcex))
		return false;

	RECT rc = { 0, 0, EWIDTH, EHEIGHT };
	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

	HWND handle = CreateWindow(
		L"ParticleEditor",
		L"Particle Editor",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		rc.right - rc.left,
		rc.bottom - rc.top,
		nullptr,
		nullptr,
		wcex.hInstance,
		nullptr
	);

	return handle;
}

HWND wndHandle;

long long time_ms() {
	static LARGE_INTEGER s_frequency;
	static BOOL s_use_qpc = QueryPerformanceFrequency(&s_frequency);
	if (s_use_qpc) {
		LARGE_INTEGER now;
		QueryPerformanceCounter(&now);
		return (1000LL * now.QuadPart) / s_frequency.QuadPart;
	}
	else {
		return GetTickCount();
	}
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	MSG msg = { 0 };
	wndHandle = InitWindow(hInstance);

	HICON icon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(101));
	SendMessage(wndHandle, WM_SETICON, ICON_BIG, (LPARAM)icon);

	if (wndHandle) {
		createDirect3DContext(wndHandle);

		ImGui_ImplDX11_Init(wndHandle, gDevice, gDeviceContext);

		Editor::Init();

		ShowWindow(wndHandle, nCmdShow);

		long long start = time_ms();
		long long prev = start;

		bool quit = false;
		while (!quit) {
			long long newtime = time_ms();
			long long elapsed = newtime - prev;

			while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
				if (msg.message == WM_QUIT)
					quit = true;

				if (msg.message == WM_KEYUP) {
					int wk = (int)msg.wParam;

					if (wk == VK_ESCAPE) quit = true;
				}

				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}

			ImGui_ImplDX11_NewFrame();

			float dt = (elapsed) / 1000.f;

			Editor::Update(dt);
			Editor::Render(dt);

			//ImGui::Render();

			gSwapChain->Present(1, 0);
			prev = newtime;
		}

		ImGui_ImplDX11_Shutdown();
		DestroyWindow(wndHandle);
	}

	return (int)msg.wParam;
}

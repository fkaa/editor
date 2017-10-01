#include <windows.h>

#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "d3dcompiler.lib")

#include <d3d11.h>
#include <d3dcompiler.h>
#include <cstdio>
#include <iostream>
#include <vector>
#include <fstream>

#include "External/dxerr.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_dx11.h>
#include <ImwWindowManagerDX11.h>
#include <ImwPlatformWindowDX11.h>

#include "External\DirectXTK.h"

#include "External\DebugDraw.h"
#include "External\Helpers.h"
#include "External\IconsMaterialDesign.h"

#include "Camera.h"

#include "Editor.h"


using namespace ImWindow;
using namespace DX;

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
	Editor::Run();

	ImGui::Shutdown();

	return (int)1;
}
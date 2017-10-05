#include "ImwPlatformWindowDX11.h"

#include "ImwWindowManager.h"

//#include <DxErr.h>
#include "../dxerr.h"
#include <Windows.h>
#include <stdio.h>
int (WINAPIV * __vsnprintf)(char *, size_t, const char*, va_list) = _vsnprintf;

#include "Win32MessageHelper.h">

#pragma comment (lib, "d3d11.lib")
//#pragma comment (lib, "d3dx11.lib")
//#pragma comment (lib, "d3dx10.lib")
#pragma comment (lib, "d3dcompiler.lib")
#pragma comment (lib, "dxgi.lib")
//#pragma comment (lib, "dxerr.lib")

using namespace ImWindow;

ImwPlatformWindowDX11::InstanceMap		ImwPlatformWindowDX11::s_mInstances;
bool									ImwPlatformWindowDX11::s_bClassInitialized = false;
WNDCLASSEX								ImwPlatformWindowDX11::s_oWndClassEx;

IDXGIFactory*							ImwPlatformWindowDX11::s_pFactory = NULL;
ID3D11Device*							ImwPlatformWindowDX11::s_pDevice = NULL;
ID3D11DeviceContext*					ImwPlatformWindowDX11::s_pDeviceContext = NULL;
ID3D11RenderTargetView*					ImwPlatformWindowDX11::s_pRTV = NULL;

ImwPlatformWindow*						ImwPlatformWindowDX11::s_pLastHoveredWindow = NULL;

INT64									ImwPlatformWindowDX11::g_Time = 0;
INT64									ImwPlatformWindowDX11::g_TicksPerSecond = 0;


IMGUI_API void							ImGui_ImplDX11_RenderDrawLists(ImDrawData* draw_data);


ImwPlatformWindowDX11::ImwPlatformWindowDX11(EPlatformWindowType eType, bool bCreateState)
	: ImwPlatformWindow( eType, bCreateState )
{
	m_pSwapChain = NULL;
	m_pRenderTargetView = NULL;
	m_bDrag = false; 
	m_oSize = ImVec2(0,0);
	m_oPosition = ImVec2(-1,-1);
}

ImwPlatformWindowDX11::~ImwPlatformWindowDX11()
{
	s_mInstances.erase(m_hWnd);

	//ImwSafeRelease(m_pDevice);
	//ImwSafeRelease(m_pDeviceContext);

	ImwSafeRelease(m_pSwapChain);
	ImwSafeRelease(m_pRenderTargetView);

	DestroyWindow(m_hWnd);
}

bool ImwPlatformWindowDX11::Init(ImwPlatformWindow* pMain)
{
	InitWndClassEx();

	HRESULT hr;

	DWORD iWindowStyle;
	if (m_eType == E_PLATFORM_WINDOW_TYPE_DRAG_PREVIEW)
	{
		iWindowStyle = WS_POPUP;
	}
	else if (m_eType == E_PLATFORM_WINDOW_TYPE_SECONDARY)
	{
		iWindowStyle = WS_POPUP | WS_VISIBLE | WS_THICKFRAME;
		iWindowStyle = WS_OVERLAPPEDWINDOW;
	}
	else
	{
		iWindowStyle = WS_OVERLAPPEDWINDOW;
	}

	RECT wr = { 0, 0, 1800, 960 };
	AdjustWindowRect(&wr, iWindowStyle, FALSE);

	m_hWnd = CreateWindowExA(NULL,
		"ImwPlatformWindowDX11",
		"ImwWindow",
		iWindowStyle,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		wr.right - wr.left,
		wr.bottom - wr.top,
		(pMain != NULL) ? ((ImwPlatformWindowDX11*)pMain)->GetHWnd() : NULL,
		NULL,
		GetModuleHandleA(NULL),
		NULL);

	if (m_eType == E_PLATFORM_WINDOW_TYPE_DRAG_PREVIEW)
	{
		SetWindowLongA(m_hWnd, GWL_EXSTYLE, GetWindowLongA(m_hWnd, GWL_EXSTYLE) | WS_EX_LAYERED);
		SetLayeredWindowAttributes(m_hWnd, RGB(0, 0, 0), 128, LWA_ALPHA);
	}

	s_mInstances.insert(std::pair<HWND, ImwPlatformWindowDX11*>(m_hWnd, this));

	DXGI_SWAP_CHAIN_DESC scd;
	ZeroMemory(&scd, sizeof(DXGI_SWAP_CHAIN_DESC));
	scd.BufferCount = 1;
	scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scd.OutputWindow = m_hWnd;
	scd.SampleDesc.Count = 1;
	scd.Windowed = true;

	//hr = s_pFactory->CreateSwapChainForHwnd( )
	DXCALL(s_pFactory->CreateSwapChain( s_pDevice, &scd, &m_pSwapChain ));
	DXCALL(s_pFactory->MakeWindowAssociation(m_hWnd, DXGI_MWA_NO_ALT_ENTER ));


	if (s_pRTV) {
		s_pRTV->Release();
		s_pRTV = NULL;
	}
	//Create our BackBuffer
	ID3D11Texture2D* pBackBuffer;
	DXCALL(m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer));


	//Create our Render Target
	DXCALL(s_pDevice->CreateRenderTargetView(pBackBuffer, NULL, &s_pRTV));
	pBackBuffer->Release();

	//Set our Render Target
	s_pDeviceContext->OMSetRenderTargets(1, &s_pRTV, NULL);

	SetState();
	ImGui_ImplDX11_Init(m_hWnd, s_pDevice, s_pDeviceContext);

	if (!QueryPerformanceFrequency((LARGE_INTEGER *)&g_TicksPerSecond)) 
		return false;
	if (!QueryPerformanceCounter((LARGE_INTEGER *)&g_Time))
		return false;

	ImGuiIO& io = ImGui::GetIO();
	io.KeyMap[ImGuiKey_Tab] = VK_TAB;                       // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array that we will update during the application lifetime.
	io.KeyMap[ImGuiKey_LeftArrow] = VK_LEFT;
	io.KeyMap[ImGuiKey_RightArrow] = VK_RIGHT;
	io.KeyMap[ImGuiKey_UpArrow] = VK_UP;
	io.KeyMap[ImGuiKey_DownArrow] = VK_DOWN;
	io.KeyMap[ImGuiKey_PageUp] = VK_PRIOR;
	io.KeyMap[ImGuiKey_PageDown] = VK_NEXT;
	io.KeyMap[ImGuiKey_Home] = VK_HOME;
	io.KeyMap[ImGuiKey_End] = VK_END;
	io.KeyMap[ImGuiKey_Delete] = VK_DELETE;
	io.KeyMap[ImGuiKey_Backspace] = VK_BACK;
	io.KeyMap[ImGuiKey_Enter] = VK_RETURN;
	io.KeyMap[ImGuiKey_Escape] = VK_ESCAPE;
	io.KeyMap[ImGuiKey_A] = 'A';
	io.KeyMap[ImGuiKey_C] = 'C';
	io.KeyMap[ImGuiKey_V] = 'V';
	io.KeyMap[ImGuiKey_X] = 'X';
	io.KeyMap[ImGuiKey_Y] = 'Y';
	io.KeyMap[ImGuiKey_Z] = 'Z';

	io.RenderDrawListsFn = ImGui_ImplDX11_RenderDrawLists;  // Alternatively you can set this to NULL and call ImGui::GetDrawData() after ImGui::Render() to get the same ImDrawData pointer.
	io.ImeWindowHandle = m_hWnd;

	RestoreState();

	m_hCursorArrow = LoadCursor( NULL, IDC_ARROW );
	m_hCursorResizeNS = LoadCursor( NULL, IDC_SIZENS );
	m_hCursorResizeWE = LoadCursor( NULL, IDC_SIZEWE );

	if( pMain == NULL )
	{
		ImGui_ImplDX11_NewFrame();
	}

	return true;
}

ImVec2 ImwPlatformWindowDX11::GetPosition() const
{
	return m_oPosition;
}

ImVec2 ImwPlatformWindowDX11::GetSize() const
{
	return m_oSize;
}

bool ImwPlatformWindowDX11::IsWindowMaximized() const
{
	return IsMaximized(m_hWnd);
}

void ImwPlatformWindowDX11::Show(bool bShow)
{
	ShowWindow(m_hWnd, bShow ? SW_SHOW : SW_HIDE);
}

void ImwPlatformWindowDX11::SetSize(int iWidth, int iHeight)
{
	RECT oRect;
	oRect.left = 0;
	oRect.top = 0;
	oRect.right = iWidth;
	oRect.bottom = iHeight;
	AdjustWindowRect(&oRect, GetWindowLongA(m_hWnd, GWL_STYLE), false);
	SetWindowPos(m_hWnd, 0, 0, 0, oRect.right - oRect.left, oRect.bottom - oRect.top, SWP_NOMOVE);
}

void ImwPlatformWindowDX11::SetPosition(int iX, int iY)
{
	RECT oRect;
	oRect.left = iX;
	oRect.top = iY;
	oRect.right = iX + m_oSize.x;
	oRect.bottom = iY + m_oSize.y;
	AdjustWindowRect(&oRect, GetWindowLongA(m_hWnd, GWL_STYLE), false);
	SetWindowPos(m_hWnd, 0, oRect.left, oRect.top, 0, 0, SWP_NOSIZE);
}

void ImwPlatformWindowDX11::SetWindowMaximized(bool bMaximized)
{
	ShowWindow(m_hWnd, bMaximized ? SW_MAXIMIZE : SW_NORMAL);
}

void ImwPlatformWindowDX11::SetTitle(const ImwChar* pTtile)
{
	SetWindowTextA(m_hWnd, pTtile);
}

void ImwPlatformWindowDX11::PreUpdate()
{
	MSG msg;
	int iCount = 0;
	while (iCount < 10 && PeekMessageA(&msg, m_hWnd, 0, 0, PM_REMOVE)) // Max 10 messages
	{
		TranslateMessage(&msg);
		DispatchMessageA(&msg);
		//OnMessage(msg.)
		//OnMessage(msg.message, msg.wParam, msg.lParam);
		++iCount;
	}

	if (NULL != m_pSwapChain) {
		float bgColor[] = { 0.3f, 0.3f, 0.3f, 1.0f };
		ImwIsSafe(s_pDeviceContext)->ClearRenderTargetView(s_pRTV, bgColor);
	}
}

void ImwPlatformWindowDX11::Render()
{
	if (!m_bNeedRender)
		return;

	if (m_bDrag)
	{
		//GetCursorPos()

		RECT oRect;
		GetWindowRect(m_hWnd, &oRect);

		POINT oCursorPoint;
		GetCursorPos(&oCursorPoint);

		int iX = m_iWindowPosStartDrag.x + oCursorPoint.x - m_iCursorPosStartDrag.x;
		int iY = m_iWindowPosStartDrag.y + oCursorPoint.y - m_iCursorPosStartDrag.y;
		SetWindowPos(m_hWnd, 0, iX, iY, 0, 0, SWP_NOSIZE);
	}

	if ( NULL != m_pSwapChain )
	{


		s_pDeviceContext->OMSetRenderTargets(1, &s_pRTV, NULL);


		SetState();
		ImGui::GetIO().DisplaySize = m_oSize;

		ImGui::Render();

		ImGui_ImplDX11_NewFrame();

		if (this == s_pLastHoveredWindow)
		{
			/*switch (ImGui::GetMouseCursor())
			{
			case ImGuiMouseCursor_Arrow:
				SetCursor(m_hCursorArrow);
				break;
			case ImGuiMouseCursor_TextInput:         // When hovering over InputText, etc.
				SetCursor(m_hCursorArrow);
				break;
			case ImGuiMouseCursor_Move:              // Unused
				SetCursor(m_hCursorArrow);
				break;
			case ImGuiMouseCursor_ResizeNS:          // Unused
				SetCursor(m_hCursorResizeNS);
				break;
			case ImGuiMouseCursor_ResizeEW:          // When hovering over a column
				SetCursor(m_hCursorResizeWE);
				break;
			case ImGuiMouseCursor_ResizeNESW:        // Unused
				SetCursor(m_hCursorArrow);
				break;
			case ImGuiMouseCursor_ResizeNWSE:        // When hovering over the bottom-right corner of a window
				SetCursor(m_hCursorArrow);
				break;
			}*/
		}

		RestoreState();

		//Present the backbuffer to the screen
		ImwIsSafe(m_pSwapChain)->Present(0, 0);
	}
}

void ImwPlatformWindowDX11::Destroy()
{

}

void ImwPlatformWindowDX11::StartDrag()
{
	m_bDrag = true;
	RECT oRect;
	GetWindowRect(m_hWnd, &oRect);
	m_iWindowPosStartDrag.x = oRect.left;
	m_iWindowPosStartDrag.y = oRect.top;

	POINT oCursorPoint;
	GetCursorPos(&oCursorPoint);
	m_iCursorPosStartDrag.x = oCursorPoint.x;
	m_iCursorPosStartDrag.y = oCursorPoint.y;
}

void ImwPlatformWindowDX11::StopDrag()
{
	m_bDrag = false;
}

bool ImwPlatformWindowDX11::IsDraging()
{
	return m_bDrag;
}


HWND ImwPlatformWindowDX11::GetHWnd()
{
	return m_hWnd;
}

#include "../../Source/Editor.h"

LRESULT ImwPlatformWindowDX11::OnMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
	if (IsStateSet())
	{

		return DefWindowProcA(m_hWnd, message, wParam, lParam);
	}

	SetState();
	ImGuiIO& io = ImGui::GetIO();
	RestoreState();

	switch (message)
	{
	case WM_CLOSE:
		OnClose();
		return 1;
		break;
	//case WM_ENTERSIZEMOVE:

		break;
	case WM_SIZE:
		{
			//RECT wr = { 0, 0, LOWORD(lParam), HIWORD(lParam) };
			//AdjustWindowRect(&wr, GetWindowLong(m_hWnd, GWL_STYLE), FALSE);
			if (NULL != m_pSwapChain)
			{
				s_pDeviceContext->OMSetRenderTargets(0, 0, 0);

				// Release all outstanding references to the swap chain's buffers.
				s_pRTV->Release();

				HRESULT hr;
				// Preserve the existing buffer count and format.
				// Automatically choose the width and height to match the client rect for HWNDs.
				hr = m_pSwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);

				hr = s_pFactory->MakeWindowAssociation(m_hWnd, DXGI_MWA_NO_ALT_ENTER );

				// Perform error handling here!

				// Get buffer and create a render-target-view.
				ID3D11Texture2D* pBuffer;
				hr = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBuffer);
				// Perform error handling here!

				hr = s_pDevice->CreateRenderTargetView(pBuffer, NULL, &s_pRTV);
				// Perform error handling here!
				pBuffer->Release();

				s_pDeviceContext->OMSetRenderTargets(1, &s_pRTV, NULL);

				RECT oRect;
				GetClientRect(m_hWnd, &oRect);
				m_oSize = ImVec2(oRect.right - oRect.left, oRect.bottom - oRect.top);
				ClientToScreen(m_hWnd, reinterpret_cast<POINT*>(&oRect.left)); // convert top-left
				ClientToScreen(m_hWnd, reinterpret_cast<POINT*>(&oRect.right)); // convert bottom-right
				m_oPosition = ImVec2(oRect.left, oRect.top);

				IM_ASSERT(m_oSize.x == LOWORD(lParam));
				IM_ASSERT(m_oSize.y == HIWORD(lParam));

				//IM_ASSERT(m_oSize.x > 0);
				//IM_ASSERT(m_oSize.y > 0);
				//m_iWidth = LOWORD(lParam);
				//m_iHeight = HIWORD(lParam);

				// Set up the viewport.
				D3D11_VIEWPORT vp;
				vp.Width = m_oSize.x;
				vp.Height = m_oSize.y;
				vp.MinDepth = 0.0f;
				vp.MaxDepth = 1.0f;
				vp.TopLeftX = 0;
				vp.TopLeftY = 0;
				s_pDeviceContext->RSSetViewports(1, &vp);
			}
			return 0;
		}
		//break; // Not a forget
	case WM_GETMINMAXINFO:
		{
			MINMAXINFO* mmi = (MINMAXINFO*)lParam;
			mmi->ptMinTrackSize.x = 100;
			mmi->ptMinTrackSize.y = 100;
			return 0;
		}
	case WM_MOVE:
		{
			RECT oRect;
			GetClientRect(m_hWnd, &oRect);
			m_oSize = ImVec2(oRect.right - oRect.left, oRect.bottom - oRect.top);
			ClientToScreen(m_hWnd, reinterpret_cast<POINT*>(&oRect.left)); // convert top-left
			ClientToScreen(m_hWnd, reinterpret_cast<POINT*>(&oRect.right)); // convert bottom-right
			m_oPosition = ImVec2(oRect.left, oRect.top);
			//AdjustWindowRect(&oRect, GetWindowLong(m_hWnd, GWL_STYLE), FALSE);
			//m_oPosition = ImVec2(oRect.left, oRect.top);
		}
		break;
	case WM_KILLFOCUS:
		{
			OnLoseFocus();
		}
		break;

	case WM_DESTROY:
		//OutputDebugString("WM_DESTROY\n");
		//PostQuitMessage(0);
		break;
	case WM_ERASEBKGND:
		return 1;
		break;

	case WM_LBUTTONDOWN:
		io.MouseDown[0] = true;
		return 1;
	case WM_LBUTTONUP:
		io.MouseDown[0] = false;
		return 1;
	case WM_RBUTTONDOWN:
		io.MouseDown[1] = true; 
		return 1;
	case WM_RBUTTONUP:
		io.MouseDown[1] = false; 
		return 1;
	case WM_MBUTTONDOWN:
		io.MouseDown[2] = true; 
		return 1;
	case WM_MBUTTONUP:
		io.MouseDown[2] = false; 
		return 1;
	case WM_MOUSEWHEEL:
		io.MouseWheel += GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
		return 1;
	case WM_MOUSEMOVE:
		io.MousePos.x = (signed short)(lParam);
		io.MousePos.y = (signed short)(lParam >> 16);
		s_pLastHoveredWindow = this;
		return 1;
	case WM_KEYDOWN:
		if (wParam < 256)
			io.KeysDown[wParam] = 1;
		break;
	case WM_KEYUP:
		if (wParam < 256)
			io.KeysDown[wParam] = 0;
		break;
	case WM_CHAR:
		// You can also use ToAscii()+GetKeyboardState() to retrieve characters.
		if (wParam > 0 && wParam < 0x10000)
			io.AddInputCharacter((unsigned short)wParam);
		return 1;
	}

	return DefWindowProcA(m_hWnd, message, wParam, lParam);
}

// Static

int ImwPlatformWindowDX11::GetInstanceCount()
{
	return s_mInstances.size();
}

void ImwPlatformWindowDX11::InitWndClassEx()
{
	if (!s_bClassInitialized)
	{
		WNDCLASSEXA wc;

		ZeroMemory(&wc, sizeof(WNDCLASSEXA));

		wc.cbSize = sizeof(WNDCLASSEXA);
		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc = ImwPlatformWindowDX11Proc;
		//wc.hInstance = hInstance;
		wc.hInstance = GetModuleHandle(NULL);
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
		wc.lpszClassName = "ImwPlatformWindowDX11";

		RegisterClassExA(&wc);

		s_bClassInitialized = true;
	}
}

LRESULT ImwPlatformWindowDX11::ImwPlatformWindowDX11Proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	std::map<HWND, ImwPlatformWindowDX11*>::iterator it = s_mInstances.find(hWnd);
	if (it != s_mInstances.end())
	{
		return it->second->OnMessage(message, wParam, lParam);
	}
	/*else
	{
		ImAssert(false, "HWND not found in ImwPlatformWindowDX11 instances");
	}
	*/

	return DefWindowProcA(hWnd, message, wParam, lParam);
}

bool ImwPlatformWindowDX11::InitDX11()
{
	DXCALL(CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)(&s_pFactory)));

	DXCALL(D3D11CreateDevice(NULL,
		D3D_DRIVER_TYPE_HARDWARE,
		NULL,
		D3D11_CREATE_DEVICE_DEBUG,
		NULL,
		NULL,
		D3D11_SDK_VERSION,
		&s_pDevice,
		NULL,
		&s_pDeviceContext));

	return true;
}

void ImwPlatformWindowDX11::ShutdownDX11()
{
	ImwSafeRelease(s_pDevice);
	ImwSafeRelease(s_pDeviceContext);
	ImwSafeRelease(s_pFactory);
}

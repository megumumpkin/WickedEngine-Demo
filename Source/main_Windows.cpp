// TemplateWindows.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "main_Windows.h"

#include <sstream>
#include <iterator>

// Game Software Entry
#include "Config.h"
#include "Core.h"

#ifdef IS_DEV
#include "Dev.h"
#endif

// Enable macro and follow instructions from here: https://devblogs.microsoft.com/directx/gettingstarted-dx12agility/
//#define USING_D3D12_AGILITY_SDK
#ifdef USING_D3D12_AGILITY_SDK
extern "C"
{
	// Used to enable the "Agility SDK" components
	__declspec(dllexport) extern const UINT D3D12SDKVersion = 608 /* D3D12_SDK_VERSION*/;
	__declspec(dllexport) extern const char* D3D12SDKPath = u8".\\D3D12\\";
}
#endif

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
Game::App app;                          // Wicked Engine Application

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

	wi::arguments::Parse(lpCmdLine); // if you wish to use command line arguments, here is a good place to parse them...
	
#ifdef IS_DEV
    if (Dev::ReadCMD()){
#endif

	Game::Filesystem::Register_FS("content/", "Data/Content/", false);
    Game::Filesystem::Register_FS("shader/", "Data/Shader/", false);

    wi::renderer::SetShaderSourcePath(Game::Filesystem::GetActualPath("shader/"));
    wi::renderer::SetShaderPath(Game::Filesystem::GetActualPath("shader/"));

    // AppSettings settings = AppSettings_Load();

    BOOL dpi_success = SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    assert(dpi_success);

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_TEMPLATEWINDOWS, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_TEMPLATEWINDOWS));

	// just show some basic info:
	// app.infoDisplay.active = true;
	// app.infoDisplay.watermark = true;
	// app.infoDisplay.resolution = true;
	// app.infoDisplay.fpsinfo = true;

	MSG msg = { 0 };
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else {

			app.Run(); // run the update - render loop (mandatory)

		}
	}

    return (int) msg.wParam;
#ifdef IS_DEV
    }
    return 0;
#endif
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TEMPLATEWINDOWS));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    // wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_TEMPLATEWINDOWS);
    wcex.lpszMenuName = L"";
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

BOOL CreateAppWindow(int nCmdShow)
{
	// int width = CW_USEDEFAULT;
	// int height = 0;
    int width = 1280;
    int height = 720;
	bool fullscreen = false;
	bool borderless = false;

	HWND hWnd = NULL;

	if (borderless || fullscreen)
	{
		width = std::max(100, width);
		height = std::max(100, height);

		hWnd = CreateWindowEx(
			WS_EX_APPWINDOW,
			szWindowClass,
			szTitle,
			WS_POPUP,
			CW_USEDEFAULT, 0, width, height,
			NULL,
			NULL,
			hInst,
			NULL
		);
	}
	else
	{
		hWnd = CreateWindow(
			szWindowClass,
			szTitle,
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, 0, width, height,
			NULL,
			NULL,
			hInst,
			NULL
		);
	}

	if (!hWnd)
	{
		return FALSE;
	}

	if (fullscreen)
	{
		HMONITOR monitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
		MONITORINFO info;
		info.cbSize = sizeof(MONITORINFO);
		GetMonitorInfo(monitor, &info);
		width = info.rcMonitor.right - info.rcMonitor.left;
		height = info.rcMonitor.bottom - info.rcMonitor.top;
		MoveWindow(hWnd, 0, 0, width, height, FALSE);
	}

	app.SetWindow(hWnd);

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	// RegisterHotKey(hWnd, PRINTSCREEN, 0, VK_SNAPSHOT);

	return TRUE;
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   return CreateAppWindow(nCmdShow);
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_SIZE:
    case WM_DPICHANGED:
		if (app.is_window_active)
			app.SetWindow(hWnd);
        break;
	case WM_CHAR:
		switch (wParam)
		{
		case VK_BACK:
			wi::gui::TextInputField::DeleteFromInput();
			break;
		case VK_RETURN:
			break;
		default:
		{
			const wchar_t c = (const wchar_t)wParam;
			wi::gui::TextInputField::AddInput(c);
		}
		break;
		}
		break;
	case WM_INPUT:
		wi::input::rawinput::ParseMessage((void*)lParam);
		break;
	case WM_KILLFOCUS:
		// app.is_window_active = false;
		break;
	case WM_SETFOCUS:
		// app.is_window_active = true;
		break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

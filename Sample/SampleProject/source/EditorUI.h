#pragma once
#include <Windows.h>

HWND cpphwin_hwnd; /// Host Window Handle
HWND wpf_hwnd; /// WPF Wrapper Handle

class EditorUI
{
	HMODULE dotNetGUILibrary;
	WNDCLASSEX HostWindowClass;

	typedef LRESULT(*HostProc)(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
public:
	typedef HWND(*CreateUserInterfaceFunc)(HWND);
	CreateUserInterfaceFunc CreateUserInterface;

	typedef void(*GetRendererViewportFunc)(HWND);
	GetRendererViewportFunc ParentRenderer;

	typedef void(*SetChildPositionFunc)(void);
	SetChildPositionFunc SetViewportPosition;

	typedef void(*DisplayUserInterfaceFunc)(void);
	DisplayUserInterfaceFunc DisplayUserInterface;

	typedef void(*DestroyUserInterfaceFunc)(void);
	DestroyUserInterfaceFunc DestroyUserInterface;

	typedef void(*UpdateLoggerFunc)(char*);
	UpdateLoggerFunc UpdateLogger;

	EditorUI()
	{
		dotNetGUILibrary = LoadLibraryA("EditorUI.dll");
		CreateUserInterface = (CreateUserInterfaceFunc)GetProcAddress(dotNetGUILibrary, "CreateUserInterface");
		DisplayUserInterface = (DisplayUserInterfaceFunc)GetProcAddress(dotNetGUILibrary, "DisplayUserInterface");
		DestroyUserInterface = (DestroyUserInterfaceFunc)GetProcAddress(dotNetGUILibrary, "DestroyUserInterface");
		ParentRenderer = (GetRendererViewportFunc)GetProcAddress(dotNetGUILibrary, "ParentRenderer");
		UpdateLogger = (UpdateLoggerFunc)GetProcAddress(dotNetGUILibrary, "UpdateLogger");
		SetViewportPosition = (SetChildPositionFunc)GetProcAddress(dotNetGUILibrary, "UpdateChildPosition");
	};

	~EditorUI()
	{

	};

	bool InitUI(HostProc HostWndProc)
	{
		HostWindowClass.cbSize = sizeof(WNDCLASSEX); HostWindowClass.lpfnWndProc = HostWndProc;
		HostWindowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
		HostWindowClass.cbClsExtra = 0; HostWindowClass.style = 0;
		HostWindowClass.cbWndExtra = 0;    HostWindowClass.hInstance = GetModuleHandle(NULL);
		HostWindowClass.lpszClassName = "CppMAppHostWinClass"; HostWindowClass.lpszMenuName = NULL;

		if (!RegisterClassEx(&HostWindowClass))
		{
			return 0;
		}

		/// Creating Unmanaged Host Window
		cpphwin_hwnd = CreateWindowEx(
			WS_EX_CONTROLPARENT,
			"CppMAppHostWinClass",
			"Gray Engine: World Editor",
			WS_THICKFRAME | WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, CW_USEDEFAULT, 1280, 720,
			NULL, NULL, GetModuleHandle(NULL), NULL);

		/// Check if How Window is valid
		if (cpphwin_hwnd == NULL)
		{
			return 0;
		}

		/// Centering Host Window
		RECT window_r; RECT desktop_r;
		GetWindowRect(cpphwin_hwnd, &window_r); GetWindowRect(GetDesktopWindow(), &desktop_r);
		int xPos = (desktop_r.right - (window_r.right - window_r.left)) / 2;
		int yPos = (desktop_r.bottom - (window_r.bottom - window_r.top)) / 2;

		/// Set Window Position
		SetWindowPos(cpphwin_hwnd, 0, xPos, yPos, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

		/// Creating .Net GUI
		wpf_hwnd = CreateUserInterface(cpphwin_hwnd);


		/// Set Thread to STA
		CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

		/// Check if WPF Window is valid
		if (wpf_hwnd != nullptr)
		{

			/// Disable Host Window Updates & Draws
			SendMessage(cpphwin_hwnd, WM_SETREDRAW, FALSE, 0);

			/// Disable Host Window Double Buffering
			long dwExStyle = GetWindowLong(cpphwin_hwnd, GWL_EXSTYLE);
			dwExStyle &= ~WS_EX_COMPOSITED;
			SetWindowLong(cpphwin_hwnd, GWL_EXSTYLE, dwExStyle);

			/// Set WPF Window to a Child Control
			SetWindowLong(wpf_hwnd, GWL_STYLE, WS_CHILD | WS_CLIPCHILDREN);

			/// Get your host client area rect
			RECT hwin_rect;
			GetClientRect(cpphwin_hwnd, &hwin_rect);

			/// Set WPF Control Order , Size and Position
			MoveWindow(wpf_hwnd, 0, 0, hwin_rect.right - hwin_rect.left, hwin_rect.bottom - hwin_rect.top, TRUE);
			SetWindowPos(wpf_hwnd, HWND_TOP, 0, 0, hwin_rect.right - hwin_rect.left, hwin_rect.bottom - hwin_rect.top, SWP_NOMOVE);

			/// Set WPF as A Child to Host Window...
			SetParent(wpf_hwnd, cpphwin_hwnd);

			/// Skadoosh!
			ShowWindow(wpf_hwnd, SW_RESTORE);

			/// Display WPF Control by Reseting its Opacity
			DisplayUserInterface();
		}

		/// Display Window
		ShowWindow(cpphwin_hwnd, SW_SHOW);
		UpdateWindow(cpphwin_hwnd);
		BringWindowToTop(cpphwin_hwnd);
	}

	void SetViewportHWND(HWND viewport)
	{
		long style = GetWindowLong(viewport, GWL_STYLE);
		style &= ~WS_POPUP; // remove popup style
		style &= ~WS_BORDER; // remove border style
		style |= WS_CHILDWINDOW; // add childwindow style
		SetWindowLong(viewport, GWL_STYLE, style);

		ParentRenderer(viewport);
		ShowWindow(viewport, SW_SHOW);
	}
};
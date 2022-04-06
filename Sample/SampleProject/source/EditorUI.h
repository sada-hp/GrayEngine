#pragma once
#include <GrayEngine.h>
#include <Windows.h>

#define VIEWPORT_EDITOR 0
#define VIEWPORT_MODEL_BROWSER 1

class EditorUI
{


	HMODULE dotNetGUILibrary;
	WNDCLASSEX HostWindowClass;

	typedef LRESULT(*HostProc)(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
public:
	HWND cpphwin_hwnd; /// Host Window Handle
	HWND wpf_hwnd; /// WPF Wrapper Handle

	typedef HWND(*CreateUserInterfaceFunc)(HWND, UINT);
	CreateUserInterfaceFunc CreateUserInterface;

	typedef void(*GetRendererViewportFunc)(HWND, UINT);
	GetRendererViewportFunc ParentRenderer;

	typedef void(*SetChildPositionFunc)(UINT);
	SetChildPositionFunc SetViewportPosition;

	typedef void(*DisplayUserInterfaceFunc)(UINT);
	DisplayUserInterfaceFunc DisplayUserInterface;

	typedef void(*DestroyUserInterfaceFunc)(UINT);
	DestroyUserInterfaceFunc DestroyUserInterface;

	typedef void(*UpdateLoggerFunc)(char*);
	UpdateLoggerFunc UpdateLogger;

	typedef void(*UpdateFramecounterFunc)(double);
	UpdateFramecounterFunc UpdateFramecounter;

	EditorUI()
	{
		dotNetGUILibrary = LoadLibraryA("EditorUI.dll");
		CreateUserInterface = (CreateUserInterfaceFunc)GetProcAddress(dotNetGUILibrary, "CreateUserInterface");
		DisplayUserInterface = (DisplayUserInterfaceFunc)GetProcAddress(dotNetGUILibrary, "DisplayUserInterface");
		DestroyUserInterface = (DestroyUserInterfaceFunc)GetProcAddress(dotNetGUILibrary, "DestroyUserInterface");
		ParentRenderer = (GetRendererViewportFunc)GetProcAddress(dotNetGUILibrary, "ParentRenderer");
		UpdateLogger = (UpdateLoggerFunc)GetProcAddress(dotNetGUILibrary, "UpdateLogger");
		SetViewportPosition = (SetChildPositionFunc)GetProcAddress(dotNetGUILibrary, "UpdateChildPosition");
		UpdateFramecounter = (UpdateFramecounterFunc)GetProcAddress(dotNetGUILibrary, "UpdateFrameCounter");
	};

	~EditorUI()
	{
		DestroyWindow(wpf_hwnd);
		DestroyWindow(cpphwin_hwnd);
	};

	bool InitUI(HostProc HostWndProc, LPCSTR HostClassName, UINT viewport_index)
	{
		HostWindowClass.cbSize = sizeof(WNDCLASSEX); HostWindowClass.lpfnWndProc = HostWndProc;
		HostWindowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
		HostWindowClass.cbClsExtra = 0; HostWindowClass.style = 0;
		HostWindowClass.cbWndExtra = 0;    HostWindowClass.hInstance = NULL;
		HostWindowClass.lpszClassName = HostClassName; HostWindowClass.lpszMenuName = NULL;

		if (!RegisterClassEx(&HostWindowClass))
		{
			return false;
		}

		/// Creating Unmanaged Host Window
		cpphwin_hwnd = CreateWindowEx(
			WS_EX_CONTROLPARENT,
			HostClassName,
			"Gray Engine: World Editor",
			WS_THICKFRAME | WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, CW_USEDEFAULT, 1280, 720,
			NULL, NULL, NULL, NULL);

		if (cpphwin_hwnd == NULL)
		{
			return false;
		}

		/// Centering Host Window
		RECT window_r; RECT desktop_r;
		GetWindowRect(cpphwin_hwnd, &window_r); GetWindowRect(GetDesktopWindow(), &desktop_r);
		int xPos = (desktop_r.right - (window_r.right - window_r.left)) / 2;
		int yPos = (desktop_r.bottom - (window_r.bottom - window_r.top)) / 2;
		SetWindowPos(cpphwin_hwnd, 0, xPos, yPos, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

		/// Creating .Net GUI
		wpf_hwnd = CreateUserInterface(cpphwin_hwnd, viewport_index);

		/// Set Thread to STA
		CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

		if (wpf_hwnd != nullptr)
		{
			SendMessage(cpphwin_hwnd, WM_SETREDRAW, FALSE, 0);
			long dwExStyle = GetWindowLong(cpphwin_hwnd, GWL_EXSTYLE);
			dwExStyle &= ~WS_EX_COMPOSITED;
			SetWindowLong(cpphwin_hwnd, GWL_EXSTYLE, dwExStyle);
			SetWindowLong(wpf_hwnd, GWL_STYLE, WS_CHILD | WS_CLIPCHILDREN);

			RECT hwin_rect;
			GetClientRect(cpphwin_hwnd, &hwin_rect);

			MoveWindow(wpf_hwnd, 0, 0, hwin_rect.right, hwin_rect.bottom, TRUE);
			SetWindowPos(wpf_hwnd, HWND_TOP, 0, 0, hwin_rect.right, hwin_rect.bottom, SWP_NOMOVE);

			SetParent(wpf_hwnd, cpphwin_hwnd);

			ShowWindow(wpf_hwnd, SW_RESTORE);
			DisplayUserInterface(viewport_index);
		}

		ShowWindow(cpphwin_hwnd, SW_SHOW);
		UpdateWindow(cpphwin_hwnd);
		BringWindowToTop(cpphwin_hwnd);

		return true;
	}

	bool destroyUI(UINT viewport_index)
	{
		DestroyUserInterface(viewport_index);
		DestroyWindow(wpf_hwnd);

		TCHAR className[MAX_PATH];
		GetClassName(cpphwin_hwnd, className, MAX_PATH);
		DestroyWindow(cpphwin_hwnd);

		if (!UnregisterClass(className, NULL))
		{
			auto err = GetLastError();
			Logger::Out("UnregisterClass error: %d", OutputColor::Red, OutputType::Error, err);
			return false;
		}

		return true;
	}

	void SetViewportHWND(HWND child, UINT viewport_index)
	{
		long style = GetWindowLong(child, GWL_STYLE);
		style &= ~WS_POPUP; // remove popup style
		style &= ~WS_BORDER; // remove border style
		style |= WS_CHILDWINDOW; // add childwindow style
		SetWindowLong(child, GWL_STYLE, style);

		ParentRenderer(child, viewport_index);
		ShowWindow(child, SW_SHOW);
	}
};
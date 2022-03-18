#pragma once
#include <GrayEngine.h>
#include <Windows.h>
namespace GrEngine {
    class Application : public Engine
    {
    public:
        Application()
        {
            MouseClickEvent([](std::vector<double> para)
                {
                    Logger::Out("MouseClickEvent", OutputColor::Blue);
                });
            WindowResizeEvent([](std::vector<double> para)
                {
                    Logger::Out("ResizeEvent", OutputColor::Blue);
                });
            KeyPressEvent([](std::vector<double> para)
                {
                    Logger::Out("KeyEvent", OutputColor::Blue);
                });
            MouseScrollEvent([](std::vector<double> para)
                {
                    Logger::Out("ScrollEvent", OutputColor::Blue);
                });
            MouseMoveEvent([](std::vector<double> para)
                {
                    //WLogger::Out("CursorMoveEvent %f", OutputColor::Blue, para.back());
                });
            WindowClosedEvent([](std::vector<double> para)
                {
                    Logger::Out("Window is now being closed", OutputColor::Gray);
                });

            CustomEvent("MyEvent", [](std::vector<double> para)
                {
                    Logger::Out("Custom Event with a parameter %d", OutputColor::Blue, (int)para.front());
                });

            std::vector<double> para = { 1 };

            CallEvent("MyEvent", para);
        }

        ~Application()
        {
            pWindow.get()->~Window();
        }
    };
}

WNDCLASSEX HostWindowClass; /// Our Host Window Class Object
MSG loop_message; /// Loop Message for Host Window
HINSTANCE hInstance = GetModuleHandle(NULL); /// Application Image Base Address
HWND cpphwin_hwnd; /// Host Window Handle
HWND wpf_hwnd; /// WPF Wrapper Handle
typedef void (*LZ4_Compress_File_Ptr)(void);
typedef void (*LZ4_Decompress_File_Ptr)(void);
typedef HWND(*CreateUserInterfaceFunc)(LZ4_Compress_File_Ptr, LZ4_Decompress_File_Ptr);
CreateUserInterfaceFunc CreateUserInterface;
typedef void(*DisplayUserInterfaceFunc)(void);
DisplayUserInterfaceFunc DisplayUserInterface;
typedef void(*DestroyUserInterfaceFunc)(void);
DestroyUserInterfaceFunc DestroyUserInterface;
HMODULE dotNetGUILibrary;
RECT hwin_rect;

//// Global Configs
const wchar_t cpphwinCN[] = L"CppMAppHostWinClass"; /// Host Window Class Name
bool isHWindowRunning = false; /// Host Window Running State
#define FIXED_WINDOW false
#define HWIN_TITLE L"C++ Application With WPF User Interface by H.Memar"

//// Host Window Callback
LRESULT CALLBACK HostWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_CLOSE:
		DestroyUserInterface(); //// Destroy WPF Control before Destorying Host Window
		DestroyWindow(hwnd);
		break;
	case WM_DESTROY:
		isHWindowRunning = false;
		break;
	case WM_SIZE: //// Resize WPF Control on Host Window Resizing
		if (wpf_hwnd != nullptr) {
			GetClientRect(cpphwin_hwnd, &hwin_rect);
			MoveWindow(wpf_hwnd, 0, 0, hwin_rect.right - hwin_rect.left, hwin_rect.bottom - hwin_rect.top, TRUE);
		}
		break;
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}

BOOL LZ4_Compress_File(char* filename) {

	return TRUE;
}

BOOL LZ4_Decompress_File(char* filename, long originalSize) {

	return TRUE;
}

int main(int argc, char** argv)
{
    Logger::Out("Starting the engine", OutputColor::Gray);
    auto app = (GrEngine::Engine*) new GrEngine::Application();



	/// Defining Our Host Window Class
	HostWindowClass.cbSize = sizeof(WNDCLASSEX); HostWindowClass.lpfnWndProc = HostWindowProc;
	HostWindowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	HostWindowClass.cbClsExtra = 0; HostWindowClass.style = 0;
	HostWindowClass.cbWndExtra = 0;    HostWindowClass.hInstance = hInstance;
	HostWindowClass.lpszClassName = cpphwinCN; HostWindowClass.lpszMenuName = NULL;

	//// Register Window
	if (!RegisterClassEx(&HostWindowClass))
	{
		getchar(); return 0;
	}

	/// Creating Unmanaged Host Window
	cpphwin_hwnd = CreateWindowEx(
		WS_EX_CLIENTEDGE,
		cpphwinCN,
		HWIN_TITLE,
		WS_THICKFRAME | WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, 800, 715,
		NULL, NULL, hInstance, NULL);

	/// Check if How Window is valid
	if (cpphwin_hwnd == NULL)
	{
		getchar(); return 0;
	}

	/// Making Window Fixed Size
	if (FIXED_WINDOW) { ::SetWindowLong(cpphwin_hwnd, GWL_STYLE, GetWindowLong(cpphwin_hwnd, GWL_STYLE) & ~WS_SIZEBOX); }

	/// Centering Host Window
	RECT window_r; RECT desktop_r;
	GetWindowRect(cpphwin_hwnd, &window_r); GetWindowRect(GetDesktopWindow(), &desktop_r);
	int xPos = (desktop_r.right - (window_r.right - window_r.left)) / 2;
	int yPos = (desktop_r.bottom - (window_r.bottom - window_r.top)) / 2;

	/// Set Window Position
	::SetWindowPos(cpphwin_hwnd, 0, xPos, yPos, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

	/// Loading dotNet UI Library
	dotNetGUILibrary = LoadLibrary(L"ManagedUIKitWPF.dll");
	CreateUserInterface = (CreateUserInterfaceFunc)GetProcAddress(dotNetGUILibrary, "CreateUserInterface");
	DisplayUserInterface = (DisplayUserInterfaceFunc)GetProcAddress(dotNetGUILibrary, "DisplayUserInterface");
	DestroyUserInterface = (DestroyUserInterfaceFunc)GetProcAddress(dotNetGUILibrary, "DestroyUserInterface");

	/// Creating .Net GUI
	wpf_hwnd = CreateUserInterface(
		(LZ4_Compress_File_Ptr)&LZ4_Compress_File, (LZ4_Decompress_File_Ptr)&LZ4_Decompress_File);

	/// Set Thread to STA
	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);


	/// Check if WPF Window is valid
	if (wpf_hwnd != nullptr) {

		/// Disable Host Window Updates & Draws
		SendMessage(cpphwin_hwnd, WM_SETREDRAW, FALSE, 0);

		/// Disable Host Window Double Buffering
		long dwExStyle = GetWindowLong(cpphwin_hwnd, GWL_EXSTYLE);
		dwExStyle &= ~WS_EX_COMPOSITED;
		SetWindowLong(cpphwin_hwnd, GWL_EXSTYLE, dwExStyle);

		/// Set WPF Window to a Child Control
		SetWindowLong(wpf_hwnd, GWL_STYLE, WS_CHILD);

		/// Get your host client area rect
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
	isHWindowRunning = true;


    app->Run();
    app->~Engine();
    return 0;
}

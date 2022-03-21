#pragma once
#include <GrayEngine.h>
#include <Windows.h>
#include <glfw/glfw3.h>
#include <functional>

const char* logfile = "d:/aw.txt";

typedef HWND(*CreateUserInterfaceFunc)(HWND);
CreateUserInterfaceFunc CreateUserInterface;

typedef HWND(*GetRendererViewportFunc)(void);
GetRendererViewportFunc GetRendererViewport;

typedef void(*DisplayUserInterfaceFunc)(void);
DisplayUserInterfaceFunc DisplayUserInterface;

typedef void(*DestroyUserInterfaceFunc)(void);
DestroyUserInterfaceFunc DestroyUserInterface;

typedef void(*UpdateLoggerFunc)(char*);
UpdateLoggerFunc UpdateLogger;


namespace GrEngine 
{
    class Application : public Engine
    {
    public:
        Application()
        {
            MouseClickEvent([](std::vector<double> para)
                {
                    Logger::Out("MouseClickEvent", OutputColor::Blue, OutputType::Log);
                });
            WindowResizeEvent([](std::vector<double> para)
                {
                    Logger::Out("ResizeEvent", OutputColor::Blue, OutputType::Log);
                });
            KeyPressEvent([](std::vector<double> para)
                {
                    Logger::Out("KeyEvent", OutputColor::Blue, OutputType::Log);
                });
            MouseScrollEvent([](std::vector<double> para)
                {
                    Logger::Out("ScrollEvent", OutputColor::Blue, OutputType::Log);
                });
            MouseMoveEvent([](std::vector<double> para)
                {
                    //WLogger::Out("CursorMoveEvent %f", OutputColor::Blue, para.back());
                });
            WindowClosedEvent([](std::vector<double> para)
                {
                    Logger::Out("Window is now being closed", OutputColor::Gray, OutputType::Log);
                });

            CustomEvent("MyEvent", [](std::vector<double> para)
                {
                    Logger::Out("Custom Event with a parameter %d", OutputColor::Blue, OutputType::Log, (int)para.front());
                });

            std::vector<double> para = { 1 };

            CallEvent("MyEvent", para);
        }

        ~Application()
        {
        }

		void InitializeLogger()
		{
			LogEvent
			([](std::vector<double> para)
				{
					Application::UpdateAppLogger(para);
				}	
			);
		}

		static void UpdateAppLogger(std::vector<double> para)
		{
			std::fstream log_file;
			std::string message = "";
			char* msg = new char[para.size()+2];
			int i = 0;
			for (char letter : para)
			{
				msg[i++] = (int)letter;
			}
			msg[i++] = '\n';
			msg[i] = '\0';


			log_file.open(logfile, std::fstream::out | std::ios::app);

			log_file << msg;

			log_file.close();

			UpdateLogger(msg);

			delete[] msg;
		}
    };
}


//// Global Objects
WNDCLASSEX HostWindowClass; /// Our Host Window Class Object
MSG loop_message; /// Loop Message for Host Window
HINSTANCE hInstance = GetModuleHandle(NULL); /// Application Image Base Address
HWND cpphwin_hwnd; /// Host Window Handle
HWND wpf_hwnd; /// WPF Wrapper Handle

HMODULE dotNetGUILibrary;
RECT hwin_rect;

//// Global Configs
const LPCSTR cpphwinCN = "CppMAppHostWinClass"; /// Host Window Class Name
bool isHWindowRunning = false; /// Host Window Running State

GrEngine::Application* app;

//// Host Window Callback
LRESULT CALLBACK HostWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CLOSE:
		app->Stop();
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
			app->getAppWindow()->MaximizeGLFW(true);
        }
        break;
	case 0x1200:
		app->loadModelFromPath((const char*)lParam);
		break;
	case 0x1201:
		app->clearScene();
		break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int main(int argc, char** argv)
{
    Logger::Out("Starting the engine", OutputColor::Gray, OutputType::Log);
	app = new GrEngine::Application();


	/// Defining Our Host Window Class
	HostWindowClass.cbSize = sizeof(WNDCLASSEX); HostWindowClass.lpfnWndProc = HostWindowProc;
	HostWindowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	HostWindowClass.cbClsExtra = 0; HostWindowClass.style = 0;
	HostWindowClass.cbWndExtra = 0;    HostWindowClass.hInstance = hInstance;
	HostWindowClass.lpszClassName = cpphwinCN; HostWindowClass.lpszMenuName = NULL;

	//// Register Window
	if (!RegisterClassEx(&HostWindowClass))
	{
		return 0;
	}

	/// Creating Unmanaged Host Window
	cpphwin_hwnd = CreateWindowEx(
		WS_EX_CONTROLPARENT,
		cpphwinCN,
		"Gray Engine: World Editor",
		WS_THICKFRAME | WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, 1280, 720,
		NULL, NULL, hInstance, NULL);

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
	::SetWindowPos(cpphwin_hwnd, 0, xPos, yPos, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

	/// Loading dotNet UI Library
	dotNetGUILibrary = LoadLibrary("EditorUI.dll");
	CreateUserInterface = (CreateUserInterfaceFunc)GetProcAddress(dotNetGUILibrary, "CreateUserInterface");
	DisplayUserInterface = (DisplayUserInterfaceFunc)GetProcAddress(dotNetGUILibrary, "DisplayUserInterface");
	DestroyUserInterface = (DestroyUserInterfaceFunc)GetProcAddress(dotNetGUILibrary, "DestroyUserInterface");
	GetRendererViewport = (GetRendererViewportFunc)GetProcAddress(dotNetGUILibrary, "GetRendererViewport");
	UpdateLogger = (UpdateLoggerFunc)GetProcAddress(dotNetGUILibrary, "UpdateLogger");

	/// Creating .Net GUI
	wpf_hwnd = CreateUserInterface(cpphwin_hwnd);

	HWND viewport = GetRendererViewport();

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
		SetWindowLong(wpf_hwnd, GWL_STYLE, WS_CHILD | WS_CLIPCHILDREN);

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

	HWND _parent = reinterpret_cast<HWND>(app->getWndNative());

	SetParent(_parent, viewport);

	long style = GetWindowLong(_parent, GWL_STYLE);
	style &= ~WS_POPUP; // remove popup style
	style &= ~WS_BORDER; // remove border style
	style |= WS_CHILDWINDOW; // add childwindow style
	SetWindowLong(_parent, GWL_STYLE, style);


	app->getAppWindow()->MaximizeGLFW(true);
	ShowWindow(_parent, SW_SHOW);

	/// Display Window
	ShowWindow(cpphwin_hwnd, SW_SHOW);
	UpdateWindow(cpphwin_hwnd);
	BringWindowToTop(cpphwin_hwnd);
	isHWindowRunning = true;

	
	app->InitializeLogger();
    app->Run();
	app->~Application();

    return 0;
}

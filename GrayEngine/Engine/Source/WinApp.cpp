#include <pch.h>
#include "WinApp.h"
#define GLFW_EXPOSE_NATIVE_WIN32
#include <glfw/glfw3native.h>


namespace GrEngine
{
	AppWindow* AppWindow::Init(const AppParameters& Properties)
	{
		return new WinApp(Properties);
	}

	WinApp::WinApp(const AppParameters& Properties)
	{
		StartUp(Properties);
	}

	WinApp::~WinApp()
	{
		ShutDown();
	}

	void WinApp::StartUp(const AppParameters& Properties)
	{
		p_AppRenderer = new GrEngine_Vulkan::VulkanAPI();

		props = Properties;
		props.p_Renderer = p_AppRenderer;

		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_DOUBLEBUFFER, 1);

		glfwWindowHint(GLFW_RESIZABLE, false); //TBD
		glfwWindowHint(GLFW_VISIBLE, false); //TBD

		window = glfwCreateWindow(props.Width, props.Height, props.Title, nullptr, nullptr);
		glfwSetWindowUserPointer(window, &props);

		nativeWindow = glfwGetWin32Window(window);

		if (!window)
		{
			Logger::Out("Could not set up a window. Terminating the program", OutputColor::Red, OutputType::Error);
			ShutDown();
		}
		else
		{
			SetVSync(true);
			SetUpEvents(window);
			RECT desktop;
			GetWindowRect(GetDesktopWindow(), &desktop);
			glfwSetWindowPos(window, desktop.right/2 - Properties.Width/2, desktop.bottom/2 - Properties.Height/2);

			if (!p_AppRenderer->init(window, p_AppRenderer))
			{
				Logger::Out("Failed to initialize Vulkan!", OutputColor::Red, OutputType::Error);
				ShutDown();
			}
		}

		time = glfwGetTime();
	}

	void WinApp::ShutDown()
	{
		Logger::Out("Shutting down the engine", OutputColor::Gray, OutputType::Log);
		p_AppRenderer->destroy();
		glfwDestroyWindow(window);
		glfwTerminate();

		delete p_AppRenderer;
	}

	void WinApp::MaximizeGLFW(bool state)
	{
		if (state)
			glfwMaximizeWindow(window);
		else
			glfwRestoreWindow(window);
	}

	void WinApp::MinimizeGLFW(bool state)
	{
		if (state)
			glfwIconifyWindow(window);
		else
			glfwRestoreWindow(window);
	}

	void WinApp::OnStep()
	{
		double currentTime = glfwGetTime();
		
		glfwPollEvents();
		p_AppRenderer->drawFrame();
		EventListener::pollEngineEvents();
		glfwSwapBuffers(window);
		frames++;

		if (currentTime - time >= 1.0) {
			std::string new_title = props.Title;
			new_title += " [" + std::to_string(frames) + " fps, " + std::to_string(1000.0 / double(frames)) + " ms/frame]";

			glfwSetWindowTitle(window, new_title.c_str());
			frames = 0;
			time += 1.0;
		}
	}

	void WinApp::SetVSync(bool state)
	{
		glfwSwapInterval((int)state);
		Logger::Out("VSync is now set to %d", OutputColor::Green, OutputType::Log, state);
	}

	void WinApp::SetUpEvents(GLFWwindow* target)
	{
		glfwSetWindowSizeCallback(target, [](GLFWwindow* win, int width, int height)
		{
			AppParameters& data = *(AppParameters*)glfwGetWindowUserPointer(win);

			std::vector<double> para = {
				(double)width, (double)height
			};

			EventListener::registerEvent(EventType::WindowResize, para);

			if (data.p_Renderer != nullptr)
			{
				data.p_Renderer->Update();
			}
			else
			{
				Logger::Out("Renderer is not specified!", OutputColor::Red, OutputType::Error);
			}
		});
		glfwSetMouseButtonCallback(target, [](GLFWwindow* win, int button, int action, int mods)
		{
			AppParameters& data = *(AppParameters*)glfwGetWindowUserPointer(win);
			double xpos, ypos;
			glfwGetCursorPos(win, &xpos, &ypos);

			std::vector<double> para = {
				xpos, ypos, (double)button, (double)action, (double)mods
			};

			glfwFocusWindow(win);

			EventListener::registerEvent(EventType::MouseClick, para);
		});
		glfwSetKeyCallback(target, [](GLFWwindow* win, int key, int scancode, int action, int mods)
		{
			AppParameters& data = *(AppParameters*)glfwGetWindowUserPointer(win);

			std::vector<double> para = {
				(double)key, (double)scancode, (double)action, (double)mods
			};

			EventListener::registerEvent(EventType::KeyPress, para);
		});
		glfwSetScrollCallback(target, [](GLFWwindow* win, double xoffset, double yoffset)
		{
			AppParameters& data = *(AppParameters*)glfwGetWindowUserPointer(win);

			std::vector<double> para = {
				xoffset, yoffset
			};

			EventListener::registerEvent(EventType::Scroll, para);
		});
		glfwSetCursorPosCallback(target, [](GLFWwindow* win, double xpos, double ypos)
		{
			AppParameters& data = *(AppParameters*)glfwGetWindowUserPointer(win);

			std::vector<double> para = {
				xpos, ypos
			};

			EventListener::registerEvent(EventType::MouseMove, para);
		});
		glfwSetWindowCloseCallback(target, [](GLFWwindow* win)
		{
			AppParameters& data = *(AppParameters*)glfwGetWindowUserPointer(win);

			std::vector<double> para = {};

			EventListener::registerEvent(EventType::WindowClosed, para);
		});
	}
}
#include <pch.h>
#include "WinApp.h"

namespace GrEngine
{
	Window* Window::Init(const AppParameters& Properties)
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
		props = Properties;
		props.pEventListener = EventListener::GetListener();

		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_DOUBLEBUFFER, 1);
		window = glfwCreateWindow(props.Width, props.Height, props.Title, nullptr, nullptr);
		glfwSetWindowUserPointer(window, &props);

		if (!window)
		{
			Logger::Out("Could not set up a window. Terminating the program", OutputColor::Red);
			ShutDown();
		}
		else
		{
			SetVSync(true);
			SetUpEvents(window);
			RECT desktop;
			GetWindowRect(GetDesktopWindow(), &desktop);
			glfwSetWindowPos(window, desktop.right/2 - Properties.Width/2, desktop.bottom/2 - Properties.Height/2);

			if (!vkAPI.initVulkan(window, &vkAPI))
			{
				Logger::Out("Failed to initialize Vulkan!", OutputColor::Red);
				ShutDown();
			}
		}

		time = glfwGetTime();
		//SetParent(glfwGetWin32Window(window), );
	}

	void WinApp::ShutDown()
	{
		Logger::Out("Shutting down the engine", OutputColor::Gray);
		vkAPI.destroy();
		glfwDestroyWindow(window);
		glfwTerminate();
	}

	void WinApp::OnStep()
	{
		double currentTime = glfwGetTime();
		
		glfwPollEvents();
		vkAPI.drawFrame();
		EventListener::GetListener()->pollEngineEvents();
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
		Logger::Out("VSync is now set to %d", OutputColor::Green, state);
	}

	void WinApp::SetUpEvents(GLFWwindow* target)
	{
		glfwSetWindowSizeCallback(target, [](GLFWwindow* win, int width, int height)
		{
			AppParameters& data = *(AppParameters*)glfwGetWindowUserPointer(win);

			std::vector<double> para = {
				(double)width, (double)height
			};

			data.pEventListener->registerEvent(EventType::WindowResize, para);
		});
		glfwSetMouseButtonCallback(target, [](GLFWwindow* win, int button, int action, int mods)
		{
			AppParameters& data = *(AppParameters*)glfwGetWindowUserPointer(win);
			double xpos, ypos;
			glfwGetCursorPos(win, &xpos, &ypos);

			std::vector<double> para = {
				xpos, ypos, (double)button, (double)action, (double)mods
			};

			data.pEventListener->registerEvent(EventType::MouseClick, para);
		});
		glfwSetKeyCallback(target, [](GLFWwindow* win, int key, int scancode, int action, int mods)
		{
			AppParameters& data = *(AppParameters*)glfwGetWindowUserPointer(win);

			std::vector<double> para = {
				(double)key, (double)scancode, (double)action, (double)mods
			};

			data.pEventListener->registerEvent(EventType::KeyPress, para);
		});
		glfwSetScrollCallback(target, [](GLFWwindow* win, double xoffset, double yoffset)
		{
			AppParameters& data = *(AppParameters*)glfwGetWindowUserPointer(win);

			std::vector<double> para = {
				xoffset, yoffset
			};

			data.pEventListener->registerEvent(EventType::Scroll, para);
		});
		glfwSetCursorPosCallback(target, [](GLFWwindow* win, double xpos, double ypos)
		{
			AppParameters& data = *(AppParameters*)glfwGetWindowUserPointer(win);

			std::vector<double> para = {
				xpos, ypos
			};

			data.pEventListener->registerEvent(EventType::MouseMove, para);
		});
		glfwSetWindowCloseCallback(target, [](GLFWwindow* win)
		{
			AppParameters& data = *(AppParameters*)glfwGetWindowUserPointer(win);

			std::vector<double> para = {};

			data.pEventListener->registerEvent(EventType::WindowClosed, para);
		});
	}
}
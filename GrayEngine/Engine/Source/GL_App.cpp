#include <pch.h>
#include "GL_App.h"
#define GLFW_EXPOSE_NATIVE_WIN32
#include <glfw/glfw3native.h>
#include "Engine.h"

namespace GrEngine
{
	AppWindow* AppWindow::Init(const AppParameters& Properties)
	{
		return new GL_APP(Properties);
	}

	GL_APP::GL_APP(const AppParameters& Properties)
	{
		StartUp(Properties);
	}

	GL_APP::~GL_APP()
	{
		ShutDown();
	}

	void GL_APP::StartUp(const AppParameters& Properties)
	{
		pAppRenderer = new GrEngine_Vulkan::VulkanRenderer();

		props = Properties;
		props.p_Renderer = pAppRenderer;
		props.p_Renderer->listener = props.eventListener;

		auto res = glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_DOUBLEBUFFER, 1);

		//glfwWindowHint(GLFW_RESIZABLE, false); //TBD
		//glfwWindowHint(GLFW_VISIBLE, false); //TBD

		window = glfwCreateWindow(props.Width, props.Height, props.Title, nullptr, nullptr);

		if (!window)
		{
			Logger::Out("Could not set up a window. Terminating the program", OutputType::Error);
			ShutDown();
		}
		else
		{
			nativeWindow = glfwGetWin32Window(window);
			glfwSetWindowUserPointer(window, &props);

			//SetVSync(true);
			SetUpEvents(window);
			RECT desktop;
			GetWindowRect(GetDesktopWindow(), &desktop);
			glfwSetWindowPos(window, desktop.right/2 - Properties.Width/2, desktop.bottom/2 - Properties.Height/2);

			if (!pAppRenderer->init(window))
			{
				Logger::Out("Failed to initialize Vulkan!", OutputType::Error);
				ShutDown();
			}
		}

		time = glfwGetTime();
	}

	void GL_APP::ShutDown()
	{
		props.eventListener->pollEngineEvents();
		Logger::Out("Shutting down the engine", OutputType::Log);
		if (pAppRenderer)
		{
			pAppRenderer->destroy();
			delete pAppRenderer;
		}
		if (window)
		{
			glfwDestroyWindow(window);
		}
	}

	void GL_APP::MaximizeWindow(bool state)
	{
		if (state)
			glfwMaximizeWindow(window);
		else
			glfwRestoreWindow(window);
	}

	void GL_APP::MinimizeWindow(bool state)
	{
		if (state)
			glfwIconifyWindow(window);
		else
			glfwRestoreWindow(window);
	}

	void GL_APP::AllowResize(bool allow)
	{
		glfwSetWindowAttrib(window, GLFW_RESIZABLE, allow);
	}

	void GL_APP::ShowBorder(bool show)
	{
		glfwSetWindowAttrib(window, GLFW_DECORATED, show);
	}

	void GL_APP::OnStep()
	{
		//60 fps hard lock
		//static auto frame_time = std::chrono::steady_clock::now();
		//std::chrono::steady_clock::time_point now;
		//long long duration;
		//do
		//{
		//	now = std::chrono::steady_clock::now();
		//	duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - frame_time).count();
		//} while (duration < 1000/60);
		//frame_time = now;
		//60 fps hard lock

		glfwPollEvents();
		props.eventListener->pollEngineEvents();
		ProccessInputs();
		Engine::GetContext()->GetPhysics()->SimulateStep();
		glfwSwapBuffers(window);
		pAppRenderer->RenderFrame();
		double currentTime = glfwGetTime();
		Globals::delta_time = (currentTime - time);
		std::vector<double> para{ 1 / Globals::delta_time, time };
		props.eventListener->registerEvent(EventType::Step, para);
		time = currentTime;
	}


	void GL_APP::SetVSync(bool state)
	{
		glfwSwapInterval((int)state);
		pAppRenderer->VSyncState(state);
		Logger::Out("VSync is now set to %d", OutputType::Log, state);
	}

	void GL_APP::ProccessInputs()
	{
		for (auto input : inputs_vector)
		{
			input.second();
		}
	}

	bool GL_APP::IsKeyDown(int KEY)
	{
		return glfwGetKey(window, KEY) == GLFW_PRESS;
	}

	void GL_APP::AppShowCursor(bool show)
	{
		ShowCursor(show);
		if (show)
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		else
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
	}

	void GL_APP::Focus()
	{
		glfwFocusWindow(window);
	}

	void GL_APP::SetUpEvents(GLFWwindow* target)
	{
		glfwSetWindowFocusCallback(target, [](GLFWwindow* win, int focused)
			{
				AppParameters& data = *(AppParameters*)glfwGetWindowUserPointer(win);

				std::vector<double> para = {
					(double)focused
				};

				data.eventListener->registerEvent(EventType::FocusChanged, para);
			});
		glfwSetWindowSizeCallback(target, [](GLFWwindow* win, int width, int height)
		{
			AppParameters& data = *(AppParameters*)glfwGetWindowUserPointer(win);

			std::vector<double> para = {
				(double)width, (double)height
			};

			data.eventListener->registerEvent(EventType::WindowResize, para);

			if (data.p_Renderer != nullptr)
			{
				data.p_Renderer->Update();
			}
			else
			{
				Logger::Out("Renderer is not specified!", OutputType::Error);
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
			
			data.eventListener->registerEvent(EventType::MouseClick, para);
		});
		glfwSetKeyCallback(target, [](GLFWwindow* win, int key, int scancode, int action, int mods)
		{
			AppParameters& data = *(AppParameters*)glfwGetWindowUserPointer(win);

			std::vector<double> para = {
				(double)key, (double)scancode, (double)action, (double)mods
			};

			data.eventListener->registerEvent(EventType::KeyPress, para);
		});
		glfwSetScrollCallback(target, [](GLFWwindow* win, double xoffset, double yoffset)
		{
			AppParameters& data = *(AppParameters*)glfwGetWindowUserPointer(win);

			std::vector<double> para = {
				xoffset, yoffset
			};

			data.eventListener->registerEvent(EventType::Scroll, para);
		});
		glfwSetCursorPosCallback(target, [](GLFWwindow* win, double xpos, double ypos)
		{
			AppParameters& data = *(AppParameters*)glfwGetWindowUserPointer(win);

			std::vector<double> para = {
				xpos, ypos
			};

			data.eventListener->registerEvent(EventType::MouseMove, para);
		});
		glfwSetWindowCloseCallback(target, [](GLFWwindow* win)
		{
			AppParameters& data = *(AppParameters*)glfwGetWindowUserPointer(win);

			std::vector<double> para = {};

			data.eventListener->registerEvent(EventType::WindowClosed, para);
		});
	}
}
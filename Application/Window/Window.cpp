#include "Core/Base.h"
#include "Window/Window.h"

#include "Events/Input.h"

#include "Events/EventTypes/ApplicationEvent.h"
#include "Events/EventTypes/MouseEvent.h"
#include "Events/EventTypes/KeyEvent.h"

#include "Renderer/Renderer.h"

#include "Platform/OpenGL/OpenGLContext.h"

#include "Logger.h"

namespace Application {

	static uint8_t s_GLFWWindowCount = 0;

	static void GLFWErrorCallback(int error, const char* description)
	{
		HZ_CORE_ERROR("GLFW Error ({0}): {1}", error, description);
	}

	WindowsWindow::WindowsWindow(const WindowProps& props)
	{
		HZ_PROFILE_FUNCTION();

		Init(props);
		InitMonitors();
	}

	WindowsWindow::~WindowsWindow()
	{
		HZ_PROFILE_FUNCTION();

		Shutdown();
	}

	void WindowsWindow::Init(const WindowProps& props)
	{
		HZ_PROFILE_FUNCTION();

		m_Data.Title = props.Title;
		m_Data.Width = props.Width;
		m_Data.Height = props.Height;

		HZ_CORE_INFO("Creating window {0} ({1}, {2})", props.Title, props.Width, props.Height);

		if (s_GLFWWindowCount == 0)
		{
			HZ_PROFILE_SCOPE("glfwInit");
			int success = glfwInit();
			HZ_CORE_ASSERT(success, "Could not initialize GLFW!");
			glfwSetErrorCallback(GLFWErrorCallback);
		}

		{
			HZ_PROFILE_SCOPE("glfwCreateWindow");
#if defined(HZ_DEBUG)
			if (Renderer::GetAPI() == RendererAPI::API::OpenGL)
				glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif
			m_Window = glfwCreateWindow((int)props.Width, (int)props.Height, m_Data.Title.c_str(), nullptr, nullptr);
			++s_GLFWWindowCount;
		}

		glfwWindowHint(GLFW_SAMPLES, 8);
		m_Context = Graphics::GraphicsContext::Create(m_Window);
		m_Context->Init();

		glfwSetWindowUserPointer(m_Window, &m_Data);
		SetVSync(true);

		// Set GLFW callbacks
		glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, int width, int height)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
				data.Width = width;
				data.Height = height;

				WindowResizeEvent event(width, height);
				data.EventCallback(event);
			});

		glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
				WindowCloseEvent event;
				data.EventCallback(event);
			});

		glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, int scancode, int action, int mods)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

				switch (action)
				{
				case GLFW_PRESS:
				{
					KeyPressedEvent event(key, 0);
					data.EventCallback(event);
					break;
				}
				case GLFW_RELEASE:
				{
					KeyReleasedEvent event(key);
					data.EventCallback(event);
					break;
				}
				case GLFW_REPEAT:
				{
					KeyPressedEvent event(key, true);
					data.EventCallback(event);
					break;
				}
				}
			});

		glfwSetCharCallback(m_Window, [](GLFWwindow* window, unsigned int keycode)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

				KeyTypedEvent event(keycode);
				data.EventCallback(event);
			});

		glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, int button, int action, int mods)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

				switch (action)
				{
				case GLFW_PRESS:
				{
					MouseButtonPressedEvent event(button);
					data.EventCallback(event);
					break;
				}
				case GLFW_RELEASE:
				{
					MouseButtonReleasedEvent event(button);
					data.EventCallback(event);
					break;
				}
				}
			});

		glfwSetScrollCallback(m_Window, [](GLFWwindow* window, double xOffset, double yOffset)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

				MouseScrolledEvent event((float)xOffset, (float)yOffset);
				data.EventCallback(event);
			});

		glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, double xPos, double yPos)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

				MouseMovedEvent event((float)xPos, (float)yPos);
				data.EventCallback(event);
			});

		glfwSetMonitorCallback([](GLFWmonitor* monitor, int event)
			{
				LOG_DEBUG_STREAM << "Monitor Event";
				LOG_DEBUG_STREAM << (event == GLFW_CONNECTED ? "Monitor Connected..." : "Monitor Disconnected....");

				//auto window = glfwGetCurrentContext();
				//WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
				//data.m_Settings.primaryMonitor = glfwGetPrimaryMonitor();
				//data.m_Settings.monitors = glfwGetMonitors(&data.m_Settings.monitorCount);

				//int xpos, ypos, width, height;
				//glfwGetMonitorWorkarea(monitor, &xpos, &ypos, &width, &height);
				//const GLFWvidmode* mode = glfwGetVideoMode(monitor);
				//glfwSetWindowMonitor(window, data.m_Settings.primaryMonitor, xpos, ypos, width, height,mode->refreshRate);
			});

	}

	void WindowsWindow::InitMonitors() {
		m_Data.m_Settings.primaryMonitor = glfwGetPrimaryMonitor();
		m_Data.m_Settings.monitors = glfwGetMonitors(&m_Data.m_Settings.monitorCount);
	}

	void WindowsWindow::Shutdown()
	{
		HZ_PROFILE_FUNCTION();

		glfwDestroyWindow(m_Window);
		--s_GLFWWindowCount;

		if (s_GLFWWindowCount == 0)
		{
			glfwTerminate();
		}
	}

	void WindowsWindow::OnUpdate()
	{
		HZ_PROFILE_FUNCTION();

		glfwPollEvents();
		m_Context->SwapBuffers();
	}

	void WindowsWindow::SetVSync(bool enabled)
	{
		HZ_PROFILE_FUNCTION();

		if (enabled)
			glfwSwapInterval(1);
		else
			glfwSwapInterval(0);

		m_Data.m_Settings.VSync = enabled;
	}

	void WindowsWindow::SetPolygonSmooth(bool enabled)
	{
		HZ_PROFILE_FUNCTION();

		if (enabled)
			Graphics::Renderer::PolygonSmooth(true);
		else
			Graphics::Renderer::PolygonSmooth(false);

		m_Data.m_Settings.PolygonSmooth = enabled;
	}

	bool WindowsWindow::IsVSync() const
	{
		return m_Data.m_Settings.VSync;
	}

	bool WindowsWindow::IsPolygonSmooth() const
	{
		return m_Data.m_Settings.PolygonSmooth;
	}

	Graphics::Scope<Window> Window::Create(const WindowProps& props)
	{
		return Graphics::CreateScope<WindowsWindow>(props);
	}

}
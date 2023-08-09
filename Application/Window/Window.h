#pragma once

#include "Events/Event.h"

#include <GLFW/glfw3.h>
#include <Renderer/GraphicsContext.h>

namespace Application {

	struct WindowProps
	{
		std::string Title;
		uint32_t Width;
		uint32_t Height;

		WindowProps(const std::string& title = "Test GUI",
			uint32_t width = 1600,
			uint32_t height = 900)
			: Title(title), Width(width), Height(height)
		{
		}
	};

	// Interface representing a desktop system based Window
	class Window
	{
	public:
		using EventCallbackFn = std::function<void(Event&)>;

		virtual ~Window() = default;

		virtual void OnUpdate() = 0;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;

		// Window attributes
		virtual void SetEventCallback(const EventCallbackFn& callback) = 0;
		virtual void SetVSync(bool enabled) = 0;
		virtual bool IsVSync() const = 0;

		virtual void* GetNativeWindow() const = 0;

		virtual int GetMonitorCount() const = 0;
		virtual const char* GetPrimaryMonitorName() const = 0;

		static Graphics::Scope<Window> Create(const WindowProps& props = WindowProps());
	};

	class WindowsWindow : public Window
	{
	public:
		WindowsWindow(const WindowProps& props);
		virtual ~WindowsWindow();

		void OnUpdate() override;

		unsigned int GetWidth() const override { return m_Data.Width; }
		unsigned int GetHeight() const override { return m_Data.Height; }

		// Window attributes
		void SetEventCallback(const EventCallbackFn& callback) override { m_Data.EventCallback = callback; }
		void SetVSync(bool enabled) override;
		bool IsVSync() const override;

		virtual void* GetNativeWindow() const { return m_Window; }

		virtual int GetMonitorCount() const { return m_Data.m_Settings.monitorCount; }

		virtual const char* GetPrimaryMonitorName() const { return glfwGetMonitorName(m_Data.m_Settings.primaryMonitor); }
	private:
		virtual void Init(const WindowProps& props);
		virtual void InitMonitors();
		virtual void Shutdown();
	private:
		GLFWwindow* m_Window;
		Graphics::Scope<Graphics::GraphicsContext> m_Context;

		struct WindowSettings {
			bool VSync = true;
			bool fullScreen = false;
			GLFWmonitor* primaryMonitor;
			GLFWmonitor** monitors;
			int monitorCount;
		};

		struct WindowData
		{
			std::string Title;
			unsigned int Width, Height;
			WindowSettings m_Settings;

			EventCallbackFn EventCallback;
		};


		WindowData m_Data;
	};

}
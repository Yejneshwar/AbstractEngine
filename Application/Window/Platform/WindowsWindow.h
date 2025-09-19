#include "Window/Window.h"

namespace Application {

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
		void SetPolygonSmooth(bool enabled) override;
		bool IsVSync() const override;
		bool IsPolygonSmooth() const override;

		virtual void* GetNativeWindow() const override { return m_Window; }

		virtual int GetMonitorCount() const override { return m_Data.m_Settings.monitorCount; }

		virtual const char* GetPrimaryMonitorName() const override { return glfwGetMonitorName(m_Data.m_Settings.primaryMonitor); }
	private:
		virtual void Init(const WindowProps& props);
		virtual void InitMonitors();
		virtual void Shutdown();
	private:
		GLFWwindow* m_Window;
		Graphics::Scope<Graphics::GraphicsContext> m_Context;

		struct WindowSettings {
			bool VSync = true;
			bool PolygonSmooth = false;
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
			std::chrono::high_resolution_clock::time_point m_mousePressStartLeft;
			std::chrono::high_resolution_clock::time_point m_mousePressEndLeft;

			std::chrono::high_resolution_clock::time_point m_mousePressStartRight;
			std::chrono::high_resolution_clock::time_point m_mousePressEndRight;

			EventCallbackFn EventCallback;
		};


		WindowData m_Data;
	};
}
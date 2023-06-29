
#include <string>
#include "Core/Base.h"
#include "Core/Layer.h"
#include "Core/LayerStack.h"
#include "Events/Event.h"
#include "Events/EventTypes/ApplicationEvent.h"
#include "Window/Window.h"
#include "ImGuiHandler/ImGuiHandler.h"
#include <Renderer/3DCamera.h>
#include <Renderer/UniformBuffer.h>

namespace GUI {

	struct UBOCamera {
		glm::mat4 view;
		glm::mat4 projection;
		glm::vec4 cameraPos;
	};

	struct ApplicationCommandLineArgs
	{
		int Count = 0;
		char** Args = nullptr;

		const char* operator[](int index) const
		{
			HZ_CORE_ASSERT(index < Count);
			return Args[index];
		}
	};

	struct ApplicationSpecification
	{
		std::string Name = "Abstract Application";
		std::string WorkingDirectory;
		ApplicationCommandLineArgs CommandLineArgs;
	};

	class AbstractApplication {
	public:

		AbstractApplication(const ApplicationSpecification& specification);
		virtual ~AbstractApplication();

		void OnEvent(Application::Event& e);

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* layer);

		Application::Window& GetWindow() { return *m_Window; }

		void Close();

		ImGuiHandler* GetImGuiHandler() { return m_ImGuiHandler; }

		static AbstractApplication& Get() { return *s_Instance; }

		const ApplicationSpecification& GetSpecification() const { return m_Specification; }

		void SubmitToMainThread(const std::function<void()>& function);
		void Run();
	private:
		bool OnWindowClose(Application::WindowCloseEvent& e);
		bool OnWindowResize(Application::WindowResizeEvent& e);

		void ExecuteMainThreadQueue();
	private:
		ApplicationSpecification m_Specification;
		Graphics::Scope<Application::Window> m_Window;
		ImGuiHandler* m_ImGuiHandler;
		bool m_Running = true;
		bool m_Minimized = false;
		Application::LayerStack m_LayerStack;
		float m_LastFrameTime = 0.0f;

		Graphics::ThreeDCamera m_ApplicationCamera;
		UBOCamera uboDataCamera;
		Graphics::Ref<Graphics::UniformBuffer> m_CameraBuffer;

		std::vector<std::function<void()>> m_MainThreadQueue;
		std::mutex m_MainThreadQueueMutex;
	private:
		static AbstractApplication* s_Instance;
	};
}
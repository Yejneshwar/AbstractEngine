
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
#include <Renderer/FrameBuffer.h>
#include <Renderer/Shader.h>

namespace GUI {

	struct SceneDataUBO {
			// Vectors are multiplied on the right.
			glm::mat4 projViewMatrix;
			glm::mat4 viewMatrix;
			glm::mat4 projectionMatrix;
			glm::mat4 viewMatrixInverseTranspose;
			glm::vec4 cameraPos;

			glm::ivec3 viewport;  // (width, height, width*height)
			// For SIMPLE, INTERLOCK, SPINLOCK, LOOP, and LOOP64, the number of OIT layers;
			// for LINKEDLIST, the total number of elements in the A-buffer.
			glm::uint linkedListAllocatedPerElement;

			float alphaMin;
			float alphaWidth;
			glm::vec2  _pad1;
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

	struct ApplicationSettings {




	};

	struct ApplicationSpecification
	{
		std::string Name = "Abstract Application";
		std::string WorkingDirectory;
		ApplicationCommandLineArgs CommandLineArgs;
		ApplicationSettings ApplicationSettings;
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

		static void BindFrameBufferTextures() { 
			s_Instance->m_Framebuffer->BindTexture(1, 1);
			s_Instance->m_Framebuffer->BindTexture(2, 2);
		}

		static void UnBindFrameBufferTextures() {
			//s_Instance->m_Framebuffer->UnBindTextures();
		}

		const ApplicationSpecification& GetSpecification() const { return m_Specification; }

		void SubmitToMainThread(const std::function<void()>& function);
		void Run();
	private:
		bool OnWindowClose(Application::WindowCloseEvent& e);
		bool OnWindowResize(Application::WindowResizeEvent& e);

		void CoreUI();

		void ExecuteMainThreadQueue();

		static void UseShader(const ImDrawList* parent_list, const ImDrawCmd* cmd);

		static void ClearFrameBuffer(const ImDrawList* parent_list, const ImDrawCmd* cmd);

		void CoreUI();
	private:
		ApplicationSpecification m_Specification;
		Graphics::Scope<Application::Window> m_Window;
		ImGuiHandler* m_ImGuiHandler;
		bool m_Running = true;
		bool m_Minimized = false;
		Application::LayerStack m_LayerStack;
		float m_LastFrameTime = 0.0f;

		Graphics::ThreeDCamera m_ApplicationCamera;
		SceneDataUBO m_uboDataScene;
		Graphics::Ref<Graphics::UniformBuffer> m_CameraBuffer;

		Graphics::Ref<Graphics::Framebuffer> m_Framebuffer;

		const std::string m_passDefine = "#define PASS PASS_COMPOSITE\n";
		const std::string m_compositeDefine = "#define PASS PASS_COMPOSITE\n";

		Graphics::Ref<Graphics::Shader> m_PostProcessingShader;

		bool m_ViewportFocused = false, m_ViewportHovered = false;
		glm::vec2 m_ViewportSize = { 0.0f, 0.0f };
		glm::vec2 m_ViewportBounds[2];

		std::vector<std::function<void()>> m_MainThreadQueue;
		std::mutex m_MainThreadQueueMutex;
	private:
		static AbstractApplication* s_Instance;
	};
}
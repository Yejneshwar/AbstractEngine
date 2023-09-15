
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
#include "glm/gtc/matrix_inverse.hpp"

namespace GUI {

	struct SceneDataUBO {
			// Vectors are multiplied on the right.
			glm::mat4 projViewMatrix;
			glm::mat4 viewMatrix;
			glm::mat4 projectionMatrix;
			glm::mat4 viewMatrixInverseTranspose;
			glm::vec4 cameraPos;
			glm::vec3 viewDirection;

			glm::ivec3 viewport;  // (width, height, width*height)
			// For SIMPLE, INTERLOCK, SPINLOCK, LOOP, and LOOP64, the number of OIT layers;
			// for LINKEDLIST, the total number of elements in the A-buffer.
			glm::uint linkedListAllocatedPerElement;

			float alphaMin;
			float alphaWidth;
			glm::vec2  _pad1;
	};

	struct ViewPort {
		const uint32_t id;
		Graphics::Ref<Graphics::Framebuffer> Framebuffer;
		Graphics::ThreeDCamera ViewPortCamera;
		SceneDataUBO uboDataScene;
		bool ViewportFocused = true, ViewportHovered = false;
		glm::vec2 ViewportSize = { 0.0f, 0.0f };
		glm::vec2 ViewportBounds[2];

		explicit ViewPort(Graphics::FramebufferSpecification fbSpec, uint32_t _id) : id(_id) {
			Framebuffer = Graphics::Framebuffer::Create(fbSpec);

			ViewPortCamera = Graphics::ThreeDCamera(45.0f, 800.0f / 600.0f, 0.1f, 1000.0f);

			uboDataScene.alphaMin = 0.0f;
			uboDataScene.alphaWidth = 1.0f;
			uboDataScene.viewMatrixInverseTranspose = glm::inverseTranspose(uboDataScene.viewMatrix);
			uboDataScene.viewport = { 1280,720,1280 * 720 };
			uboDataScene.linkedListAllocatedPerElement = 8;

			this->update();

		}
		void update() {
			uboDataScene.viewMatrix = ViewPortCamera.GetViewMatrix();  // Set your view matrix here
			uboDataScene.projectionMatrix = ViewPortCamera.GetProjection();  // Set your projection matrix here
			uboDataScene.projViewMatrix = ViewPortCamera.GetViewProjection();
			uboDataScene.cameraPos = glm::vec4(ViewPortCamera.GetPosition(), 1.0f);
			uboDataScene.viewDirection = ViewPortCamera.GetViewDirection();
		}
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

		void CoreUI();

		void ExecuteMainThreadQueue();
	private:
		ApplicationSpecification m_Specification;
		Graphics::Scope<Application::Window> m_Window;
		ImGuiHandler* m_ImGuiHandler;
		bool m_Running = true;
		bool m_Minimized = false;
		Application::LayerStack m_LayerStack;
		float m_LastFrameTime = 0.0f;

		Graphics::FramebufferSpecification m_fbSpec;

		std::vector<ViewPort> m_ViewPorts;
		uint32_t m_viewPortCount = 0;
		bool m_updateAllViewPorts = false;

		Graphics::Ref<Graphics::UniformBuffer> m_CameraBuffer;

		std::vector<std::function<void()>> m_MainThreadQueue;
		std::mutex m_MainThreadQueueMutex;
	private:
		static AbstractApplication* s_Instance;
	};
}
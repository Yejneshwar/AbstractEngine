#pragma once

#include <string>
#include "Core/Base.h"
#include "Core/Layer.h"
#include "Core/LayerStack.h"
#include "Events/Event.h"
#include "Events/EventTypes/ApplicationEvent.h"
#include "Window/Window.h"
#include "ImGuiHandler/ImGuiHandler.h"
#include <Renderer/2DCamera.h>
#include <Renderer/3DCamera.h>
#include <Renderer/UniformBuffer.h>
#include <Renderer/FrameBuffer.h>
#include <Renderer/ComputeShader.h>
#include "glm/gtc/matrix_inverse.hpp"
#include <Logger.h>
#include <Renderer/Shader.h>
#include <Renderer/Texture.h>
namespace GUI {

	struct SceneDataUBO {
			// Vectors are multiplied on the right.
			glm::mat4 projViewMatrix;
			glm::mat4 viewMatrix;
			glm::mat4 viewMatrixInverse;
			glm::mat4 viewMatrixInverseTranspose;
			glm::mat4 projectionMatrix;
			glm::mat4 projectionMatrixInverse;
			glm::vec4 cameraPos;
			glm::vec4 viewDirection;
			glm::vec4 gridMinMax; //xmin, xmax, ymin, ymax
			glm::vec4 viewport; //width, height, width*height, 0
			glm::f32 aspectRatio;
			glm::f32 gridMajor;
			glm::f32 gridMinor;
			glm::f32 gridZoom;
			int selectedObject;
#if BUILDING_METAL
			// Padding for Metal API
            int _padding[3];
#endif


			//glm::ivec3 viewport;  // (width, height, width*height)
			// For SIMPLE, INTERLOCK, SPINLOCK, LOOP, and LOOP64, the number of OIT layers;
			// for LINKEDLIST, the total number of elements in the A-buffer.
			//glm::uint linkedListAllocatedPerElement;

			//float alphaMin;
			//float alphaWidth;
			//glm::vec2  _pad1;
	};

	struct ObjectSelection {
		int objectID;
		bool state; //true if selected - false if deselected
	};

	enum CameraType {
		ThreeD,
		TwoD
	};

	struct ViewPort {
		uint32_t id;
		CameraType cameraType = CameraType::ThreeD;
		Graphics::Ref<Graphics::Framebuffer> Framebuffer;
        Graphics::Ref<Graphics::Texture> JFATextureA, JFATextureB, JFAResultTexture, JFACompositeTexture;
		Graphics::Ref<Graphics::Camera> ViewPortCamera;
		SceneDataUBO uboDataScene;
		bool ViewportFocused = true, ViewportHovered = false;
		glm::u32vec2 ViewportSize = { 1.0f, 1.0f };
		glm::vec2 ViewportBounds[2];
		bool isOpen = true;
		bool updateViewport = false;

		static int s_selectedObject;

		explicit ViewPort(Graphics::FramebufferSpecification fbSpec, CameraType camera, uint32_t _id) : cameraType(camera), id(_id) {
			Framebuffer = Graphics::Framebuffer::Create(fbSpec);
        
            JFATextureA = Graphics::Texture2D::Create(fbSpec.Width, fbSpec.Height, Graphics::TextureFormat::RGBA32FLOAT);
            JFATextureB = Graphics::Texture2D::Create(fbSpec.Width, fbSpec.Height, Graphics::TextureFormat::RGBA32FLOAT);
            JFAResultTexture = Graphics::Texture2D::Create(fbSpec.Width, fbSpec.Height, Graphics::TextureFormat::RGBA8);
            JFACompositeTexture = Graphics::Texture2D::Create(fbSpec.Width, fbSpec.Height, Graphics::TextureFormat::RGBA8);

			//Note: It gets weird when near plane is set to 0.0f
			if(camera == CameraType::ThreeD)
				ViewPortCamera = Graphics::CreateRef<Graphics::ThreeDCamera>(45.0f, 800.0f / 600.0f, 0.1f, 5000.0f);
			else
				ViewPortCamera = Graphics::CreateRef<Graphics::TwoDCamera>( -100.0f, 100.0f);
			 
			//uboDataScene.alphaMin = 0.0f;
			//uboDataScene.alphaWidth = 1.0f;
			//uboDataScene.viewport = { 1280,720,1280 * 720 };
			//uboDataScene.linkedListAllocatedPerElement = 8;

			this->update();
		}

		inline void SetGridValues() {
			LOG_TRACE_STREAM << "World x min and x max" << static_cast<float>(ViewPortCamera->getWorldXmin()) << " " << static_cast<float>(ViewPortCamera->getWorldXmax());
			uboDataScene.gridMinMax = { static_cast<float>(ViewPortCamera->getWorldXmin()), static_cast<float>(ViewPortCamera->getWorldXmax()), static_cast<float>(ViewPortCamera->getWorldYmin()), static_cast<float>(ViewPortCamera->getWorldYmax()) };
			uboDataScene.gridMajor = ViewPortCamera->getGridMajorSpacing();
			uboDataScene.gridMinor = ViewPortCamera->getGridMinorSpacing();
			uboDataScene.gridZoom = ViewPortCamera->getZoom();
			LOG_TRACE_STREAM << "Grid Major Spacing : " << static_cast<float>(ViewPortCamera->getGridMajorSpacing()) << "Grid Minor Spacing : " << static_cast<float>(ViewPortCamera->getGridMajorSpacing());
		}

		void update() {
			uboDataScene.viewMatrix = ViewPortCamera->GetViewMatrix();  // Set your view matrix here
			uboDataScene.viewMatrixInverse = glm::inverse(uboDataScene.viewMatrix);
			uboDataScene.viewMatrixInverseTranspose = glm::inverseTranspose(uboDataScene.viewMatrix);
			uboDataScene.projectionMatrix = ViewPortCamera->GetProjection();  // Set your projection matrix here
			uboDataScene.projectionMatrixInverse = glm::inverse(uboDataScene.projectionMatrix);
			uboDataScene.projViewMatrix = ViewPortCamera->GetViewProjection();
			uboDataScene.cameraPos = glm::vec4(ViewPortCamera->GetPosition(), 1.0f);
			uboDataScene.viewDirection = glm::vec4(ViewPortCamera->GetViewDirection(),1.0);
			uboDataScene.viewport = ViewPortCamera->getViewport();
			uboDataScene.aspectRatio = static_cast<float>(ViewPortCamera->getAspectRatio());
			uboDataScene.selectedObject = s_selectedObject;
			SetGridValues();
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

		AbstractApplication(const ApplicationSpecification& specification, void* nativeWindow);
		virtual ~AbstractApplication();

		void CreateShaders();

		void OnEvent(Application::Event& e);

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* layer);

		Application::Window& GetWindow() { return *m_Window; }

		void Close();

		ImGuiHandler* GetImGuiHandler() { return m_ImGuiHandler; }

		static AbstractApplication& Get() { return *s_Instance; }

		const ApplicationSpecification& GetSpecification() const { return m_Specification; }

		void SubmitToMainThread(const std::function<void()>& function);

		//Programmatic selection: behaves exactly like clicking the object in
		//a viewport - the selection outline appears and every layer receives
		//OnSelection. Pass -1 to clear. Main thread only.
		void SelectObject(int objectId);

		//Center every 2D viewport on a world position (zoom preserved),
		//creating a 2D viewport if none is open and raising its window.
		//Main thread only.
		void FocusViewportsOn(const glm::vec2& worldPosition);

		//Guarantees a viewport of the given camera type exists (closed
		//viewports are destroyed, so this may create a fresh one) and
		//requests ImGui focus for it. Returns its id. Main thread only.
		uint32_t EnsureViewportOpen(CameraType cameraType);

		void Run();
        
        // Used for IOS
        void RunLoop();
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
		//Declared BEFORE the layer stack so it is destroyed AFTER it:
		//layers joining worker threads in OnDetach may still receive
		//SubmitToMainThread calls from those workers until the join returns
		std::vector<std::function<void()>> m_MainThreadQueue;
		std::mutex m_MainThreadQueueMutex;
		Application::LayerStack m_LayerStack;
		float m_LastFrameTime = 0.0f;
		bool showBuffers = false;

		Graphics::FramebufferSpecification m_fbSpec;

		std::vector<ViewPort> m_ViewPorts;

		Graphics::Ref<Graphics::Shader> m_gridShader;
		Graphics::Ref<Graphics::Shader> m_gridShader2D;

        Graphics::Ref<Graphics::ComputeShader> m_JFAComputeSeed, m_JFAComputeShader, m_JFAComputeVisualize, m_JFAComposite;

		Graphics::Ref<Graphics::Texture> m_font;

		uint32_t m_viewPortCount = 0;
		bool m_updateAllViewPorts = false;
		//Viewport id whose ImGui window should be focused/raised on the
		//next CoreUI pass (-1 = none); set by EnsureViewportOpen
		int m_focusViewportId = -1;

		Graphics::Ref<Graphics::UniformBuffer> m_CameraBuffer;

		//TODO : Move to application event bus
		ObjectSelection m_ObjectSelection = {-1, false};
		bool m_emitSelectionEvent = false;


	private:
		static AbstractApplication* s_Instance;
	};
}
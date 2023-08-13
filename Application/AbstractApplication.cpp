#include "AbstractApplication.h"
#include "Renderer/Renderer.h"
#include "glm/gtc/matrix_inverse.hpp"

namespace GUI {
	AbstractApplication* AbstractApplication::s_Instance = nullptr;

	AbstractApplication::AbstractApplication(const ApplicationSpecification& specification)
		: m_Specification(specification)
	{
		HZ_PROFILE_FUNCTION();

		HZ_CORE_ASSERT(!s_Instance, "Application already exists!");
		s_Instance = this;

		// Set working directory here
		if (!m_Specification.WorkingDirectory.empty())
			std::filesystem::current_path(m_Specification.WorkingDirectory);

		m_Window = Application::Window::Create(Application::WindowProps(m_Specification.Name));
		m_Window->SetEventCallback(APP_BIND_EVENT_FN(AbstractApplication::OnEvent));

		Graphics::FramebufferSpecification fbSpec;
		fbSpec.Attachments = {
			Graphics::FramebufferTextureFormat::RGBA8,
			Graphics::FramebufferTextureFormat::RED_INTEGER,
			Graphics::FramebufferTextureFormat::Depth,
		};
		fbSpec.Width = 1280;
		fbSpec.Height = 720;

		Graphics::Renderer::Init();

		m_Framebuffer = Graphics::Framebuffer::Create(fbSpec);

		m_ApplicationCamera = Graphics::ThreeDCamera(45.0f, 800.0f / 600.0f, 0.1f, 1000.0f);
		m_CameraBuffer = Graphics::UniformBuffer::Create(sizeof(SceneDataUBO), 0);

        m_uboDataScene.viewMatrix = m_ApplicationCamera.GetViewMatrix();  // Set your view matrix here
		m_uboDataScene.projectionMatrix = m_ApplicationCamera.GetProjection();  // Set your projection matrix here
		m_uboDataScene.projViewMatrix = m_ApplicationCamera.GetViewProjection();
		m_uboDataScene.cameraPos = glm::vec4(m_ApplicationCamera.GetPosition(), 1.0f);
		m_uboDataScene.viewDirection = m_ApplicationCamera.GetViewDirection();

		m_uboDataScene.alphaMin = 0.0f;
		m_uboDataScene.alphaWidth = 1.0f;
		m_uboDataScene.viewMatrixInverseTranspose = glm::inverseTranspose(m_uboDataScene.viewMatrix);
		m_uboDataScene.viewport = { 1280,720,1280 * 720 };
		m_uboDataScene.linkedListAllocatedPerElement = 8;
		m_CameraBuffer->SetData(&m_uboDataScene, sizeof(SceneDataUBO));


		m_ImGuiHandler = new ImGuiHandler((GLFWwindow*)m_Window->GetNativeWindow(), "#version 330");
	}

	AbstractApplication::~AbstractApplication()
	{
		HZ_PROFILE_FUNCTION();

		Graphics::Renderer::Shutdown();
	}

	void AbstractApplication::PushLayer(Layer* layer)
	{
		HZ_PROFILE_FUNCTION();

		m_LayerStack.PushLayer(layer);
		layer->OnAttach();
	}

	void AbstractApplication::PushOverlay(Layer* layer)
	{
		HZ_PROFILE_FUNCTION();

		m_LayerStack.PushOverlay(layer);
		layer->OnAttach();
	}

	void AbstractApplication::Close()
	{
		m_Running = false;
	}

	void AbstractApplication::SubmitToMainThread(const std::function<void()>& function)
	{
		std::scoped_lock<std::mutex> lock(m_MainThreadQueueMutex);

		m_MainThreadQueue.emplace_back(function);
	}

	void AbstractApplication::OnEvent(Application::Event& e)
	{
		HZ_PROFILE_FUNCTION();

		Application::EventDispatcher dispatcher(e);
		dispatcher.Dispatch<Application::WindowCloseEvent>(APP_BIND_EVENT_FN(AbstractApplication::OnWindowClose));
		dispatcher.Dispatch<Application::WindowResizeEvent>(APP_BIND_EVENT_FN(AbstractApplication::OnWindowResize));


		if (ImGui::GetIO().WantCaptureMouse && !m_ViewportHovered) return;

		//Update camera and camera uniform here
		m_ApplicationCamera.OnUpdate();
		m_uboDataScene.viewMatrix = m_ApplicationCamera.GetViewMatrix();  // Set your view matrix here
		m_uboDataScene.projectionMatrix = m_ApplicationCamera.GetProjection();  // Set your projection matrix here
		m_uboDataScene.projViewMatrix = m_ApplicationCamera.GetViewProjection();
		m_uboDataScene.viewMatrixInverseTranspose = glm::inverseTranspose(m_uboDataScene.viewMatrix);
		m_uboDataScene.cameraPos = glm::vec4(m_ApplicationCamera.GetPosition(), 1.0f);
		m_uboDataScene.viewDirection = m_ApplicationCamera.GetViewDirection();

		m_CameraBuffer->SetData(&m_uboDataScene, sizeof(SceneDataUBO));
		//

		m_ApplicationCamera.OnEvent(e);

		for (auto it = m_LayerStack.rbegin(); it != m_LayerStack.rend(); ++it)
		{
			if (e.Handled)
				break;
			(*it)->OnEvent(e);
		}
	}

	void AbstractApplication:: Run()
	{
		HZ_PROFILE_FUNCTION();

		while (m_Running)
		{
			HZ_PROFILE_SCOPE("RunLoop");

			ExecuteMainThreadQueue();

			if (!m_Minimized)
			{
				{
					HZ_PROFILE_SCOPE("LayerStack OnUpdate");

					Graphics::Renderer::ClearBuffers();
					m_Framebuffer->Bind();
					Graphics::Renderer::Clear();

					for (Layer* layer : m_LayerStack)
						layer->OnUpdate();

                    m_Framebuffer->Unbind();

					m_ImGuiHandler->Update([&]() {
						CoreUI();
						for (Layer* layer : m_LayerStack)
							layer->OnImGuiRender();
					});

				}
			}

			m_Window->OnUpdate();
		}
	}

	bool AbstractApplication::OnWindowClose(Application::WindowCloseEvent& e)
	{
		m_Running = false;
		return true;
	}

	bool AbstractApplication::OnWindowResize(Application::WindowResizeEvent& e)
	{
		HZ_PROFILE_FUNCTION();

		if (e.GetWidth() == 0 || e.GetHeight() == 0)
		{
			m_Minimized = true;
			return false;
		}

		m_Minimized = false;
		Graphics::Renderer::OnWindowResize(e.GetWidth(), e.GetHeight());

		return false;
	}

	void AbstractApplication::ExecuteMainThreadQueue()
	{
		std::scoped_lock<std::mutex> lock(m_MainThreadQueueMutex);

		for (auto& func : m_MainThreadQueue)
			func();

		m_MainThreadQueue.clear();
	}

	void AbstractApplication::CoreUI() {
    		{
    			ImGui::Begin("Hello, world!");

    			ImGui::Text("Viewport Hovered : %s" , (m_ViewportHovered ? "Yes" : "No"));

    			auto io = ImGui::GetIO();
    			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);


    			ImGui::End();
    		}

    		{
    			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
    			ImGui::Begin("Viewport");
    			ImDrawList* drawList = ImGui::GetWindowDrawList();

    			auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
    			auto viewportMaxRegion = ImGui::GetWindowContentRegionMax();
    			auto viewportOffset = ImGui::GetWindowPos();
    			m_ViewportBounds[0] = { viewportMinRegion.x + viewportOffset.x, viewportMinRegion.y + viewportOffset.y };
    			m_ViewportBounds[1] = { viewportMaxRegion.x + viewportOffset.x, viewportMaxRegion.y + viewportOffset.y };

    			m_ViewportFocused = ImGui::IsWindowFocused();
    			m_ViewportHovered = ImGui::IsWindowHovered();

    			//Application::Get().GetImGuiLayer()->BlockEvents(!m_ViewportHovered);

    			ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
    			m_ViewportSize = { viewportPanelSize.x, viewportPanelSize.y };

    			uint64_t textureID = m_Framebuffer->GetColorAttachmentRendererID(0);

    			ImGui::Image(reinterpret_cast<void*>(textureID), ImVec2{ m_ViewportSize.x, m_ViewportSize.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });

    			ImGui::End();

    			ImGui::PopStyleVar();
    		}

    		{
    			ImGui::Begin("Settings");

    			ImGui::Text("Monitor count : %d", m_Window->GetMonitorCount());

    			ImGui::Text("Primary Monitor : %s", m_Window->GetPrimaryMonitorName());

    			if(ImGui::Button("VSync")) m_Window->SetVSync(!m_Window->IsVSync());

    			auto cameraFocalPoint = m_ApplicationCamera.GetFocalPoint();
    			ImGui::Text("Camera Focus point : %.3f %.3f %.3f", cameraFocalPoint.x, cameraFocalPoint.y, cameraFocalPoint.z);

				auto viewDirection = m_ApplicationCamera.GetViewDirection();
				ImGui::Text("Camera View Direction : %.3f %.3f %.3f", viewDirection.x, viewDirection.y, viewDirection.z);
				//auto fragNormal = glm::inverseTranspose(m_ApplicationCamera.GetViewMatrix()) * glm::vec3(0.0,0.0,1.0);

    			if (ImGui::Button("Reset Camera")) m_ApplicationCamera.ResetFocalPoint();

    			ImGui::End();
    		}
    	}
}
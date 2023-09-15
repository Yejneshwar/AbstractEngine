#include "AbstractApplication.h"
#include "Renderer/Renderer.h"
#include <Logger.h>


namespace GUI {
	AbstractApplication* AbstractApplication::s_Instance = nullptr;

	AbstractApplication::AbstractApplication(const ApplicationSpecification& specification)
		: m_Specification(specification)
	{
		HZ_PROFILE_FUNCTION();

		assert(!s_Instance, "Application already exists!");
		s_Instance = this;

		// Set working directory here
		if (!m_Specification.WorkingDirectory.empty())
			std::filesystem::current_path(m_Specification.WorkingDirectory);

		m_Window = Application::Window::Create(Application::WindowProps(m_Specification.Name));
		m_Window->SetEventCallback(APP_BIND_EVENT_FN(AbstractApplication::OnEvent));

		Graphics::Renderer::Init();

		m_fbSpec.Attachments = {
			Graphics::FramebufferTextureFormat::RGBA8,
			Graphics::FramebufferTextureFormat::RED_INTEGER,
			Graphics::FramebufferTextureFormat::Depth,
		};
		m_fbSpec.Width = 1280;
		m_fbSpec.Height = 720;

		m_ViewPorts.push_back(ViewPort(m_fbSpec,m_viewPortCount)); // Default viewport
		m_viewPortCount++;

		m_CameraBuffer = Graphics::UniformBuffer::Create(sizeof(SceneDataUBO), 0);

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


		if (ImGui::GetIO().WantCaptureMouse && std::all_of(m_ViewPorts.begin(), m_ViewPorts.end(), [](ViewPort v) { return v.ViewportHovered == false; })) return;

		for (ViewPort& viewPort : m_ViewPorts) {
			if (!viewPort.ViewportHovered && !viewPort.ViewportFocused) continue;
			viewPort.ViewPortCamera.OnUpdate();
			viewPort.update();
			viewPort.ViewPortCamera.OnEvent(e);
		}

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
					for (ViewPort& v : m_ViewPorts) {
						if (!v.ViewportHovered && !v.ViewportFocused) continue;
						m_CameraBuffer->SetData(&v.uboDataScene, sizeof(SceneDataUBO));
						v.Framebuffer->Bind();
						Graphics::Renderer::Clear();
						for (Layer* layer : m_LayerStack)
							layer->OnUpdate();
						v.Framebuffer->Unbind();
					}

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

				if (ImGui::Button("Add ViewPort")) {
					m_ViewPorts.push_back(ViewPort(m_fbSpec, m_viewPortCount));
					m_viewPortCount++;
				}

				for (ViewPort v : m_ViewPorts) {
    				ImGui::Text("Viewport Hovered : %s" , (v.ViewportHovered ? "Yes" : "No"));
					ImGui::SameLine();
					ImGui::Text("| Viewport Focused : %s", (v.ViewportFocused ? "Yes" : "No"));
				}


    			auto io = ImGui::GetIO();
    			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);


    			ImGui::End();
    		}

			for (ViewPort& v : m_ViewPorts)
			{
				ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
				ImGui::Begin(std::format("Viewport {}", v.id).c_str());
				ImDrawList* drawList = ImGui::GetWindowDrawList();

				auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
				auto viewportMaxRegion = ImGui::GetWindowContentRegionMax();
				auto viewportOffset = ImGui::GetWindowPos();
				v.ViewportBounds[0] = { viewportMinRegion.x + viewportOffset.x, viewportMinRegion.y + viewportOffset.y };
				v.ViewportBounds[1] = { viewportMaxRegion.x + viewportOffset.x, viewportMaxRegion.y + viewportOffset.y };

				v.ViewportFocused = ImGui::IsWindowFocused();
				v.ViewportHovered = ImGui::IsWindowHovered();

				//Application::Get().GetImGuiLayer()->BlockEvents(!m_ViewportHovered);

				ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
				v.ViewportSize = { viewportPanelSize.x, viewportPanelSize.y };

				uint64_t textureID = v.Framebuffer->GetColorAttachmentRendererID(0);

				ImGui::Image(reinterpret_cast<void*>(textureID), ImVec2{ v.ViewportSize.x, v.ViewportSize.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });

				ImGui::End();

				ImGui::PopStyleVar();
			}

    		{
    			ImGui::Begin("Settings");

    			ImGui::Text("Monitor count : %d", m_Window->GetMonitorCount());

    			ImGui::Text("Primary Monitor : %s", m_Window->GetPrimaryMonitorName());

    			if(ImGui::Button("VSync")) m_Window->SetVSync(!m_Window->IsVSync());


				for (ViewPort& v : m_ViewPorts) {
    				auto cameraFocalPoint = v.ViewPortCamera.GetFocalPoint();
    				ImGui::Text("Camera Focus point : %.3f %.3f %.3f", cameraFocalPoint.x, cameraFocalPoint.y, cameraFocalPoint.z);

					auto viewDirection = v.ViewPortCamera.GetViewDirection();
					ImGui::Text("Camera View Direction : %.3f %.3f %.3f", viewDirection.x, viewDirection.y, viewDirection.z);
					//auto fragNormal = glm::inverseTranspose(m_ApplicationCamera.GetViewMatrix()) * glm::vec3(0.0,0.0,1.0);
					if (ImGui::Button(std::format("Reset Camera {}", v.id).c_str())) { v.ViewPortCamera.ResetFocalPoint(); v.update(); };
				}


    			ImGui::End();
    		}
    	}
}
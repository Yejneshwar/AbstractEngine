#include "AbstractApplication.h"
#include "Renderer/Renderer.h"
#include <Logger.h>
#include <imgui_internal.h>
#include "Renderer/BatchRenderer.h"


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

		m_ViewPorts.push_back(ViewPort(m_fbSpec, CameraType::ThreeD ,m_viewPortCount)); // Default viewport
		m_viewPortCount++;

		m_CameraBuffer = Graphics::UniformBuffer::Create(sizeof(SceneDataUBO), 0);

		m_font = Graphics::Texture2D::Create("./Resources/Textures/FontAtlas.png");
		m_gridShader = Graphics::Shader::Create("./Resources/Shaders/Grid.glsl", false);
		m_gridShader2D = Graphics::Shader::Create("./Resources/Shaders/Grid2D.glsl", false);
		Graphics::BatchRenderer::Init();

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
			if (!viewPort.ViewportHovered || !viewPort.ViewportFocused) continue;
			viewPort.ViewPortCamera->OnUpdate();
			viewPort.ViewPortCamera->OnEvent(e);
			viewPort.update();
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

					for (Layer* layer : m_LayerStack) {
						if (layer->IsUpdateLayer())
						{
							layer->OnUpdateLayer();
							layer->UpdateLayer(false);
							m_updateAllViewPorts = true;
						}
					}

					Graphics::Renderer::ClearBuffers();
					m_font->Bind();
					LOG_TRACE_STREAM << "Begin Viewports";
					for (ViewPort& v : m_ViewPorts) {
						//On viewport resize
						if ((v.ViewportSize.x != v.Framebuffer->GetSpecification().Width) || (v.ViewportSize.y != v.Framebuffer->GetSpecification().Height)) {
							LOG_TRACE_STREAM << "Viewport resized to: " << v.ViewportSize.x << " x " << v.ViewportSize.y;
							v.Framebuffer->Resize((uint32_t)v.ViewportSize.x, (uint32_t)v.ViewportSize.y);
							v.ViewPortCamera->SetViewportSize(v.ViewportSize.x, v.ViewportSize.y);
							v.update();
						}
						if (!v.ViewportHovered && !v.ViewportFocused && !m_updateAllViewPorts) continue;
						m_CameraBuffer->SetData(&v.uboDataScene, sizeof(v.uboDataScene));

						v.Framebuffer->Bind();

						Graphics::BatchRenderer::BeginScene();
						Graphics::Renderer::Clear();

						for (Layer* layer : m_LayerStack)
							layer->OnDrawUpdate();

						Graphics::BatchRenderer::EndScene();

						if (v.cameraType == CameraType::ThreeD) {
							//Grid Shader
							m_gridShader->Bind();
							Graphics::Renderer::DrawGridTriangles();
							m_gridShader->Unbind();
						}
						else {
							//Grid Shader
							m_gridShader2D->Bind();
							Graphics::Renderer::DrawGridTriangles();
							m_gridShader2D->Unbind();
						}
						v.Framebuffer->Unbind();
					}
					LOG_TRACE_STREAM << "End Viewports";

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

	void DrawVec3Control(const std::string& label, glm::vec3& values, float resetValue = 0.0f, float columnWidth = 100.0f)
	{
		ImGuiIO& io = ImGui::GetIO();
		auto boldFont = io.Fonts->Fonts[0];

		ImGui::PushID(label.c_str());

		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, columnWidth);
		ImGui::Text("%s", label.c_str());
		ImGui::NextColumn();

		ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

		float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
		ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("X", buttonSize))
			values.x = resetValue;
		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("Y", buttonSize))
			values.y = resetValue;
		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("Z", buttonSize))
			values.z = resetValue;
		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();

		ImGui::PopStyleVar();

		ImGui::Columns(1);

		ImGui::PopID();
	}

	void AbstractApplication::CoreUI() {
    		{
    			ImGui::Begin("Hello, world!");

				if (ImGui::Button("Add 3D ViewPort")) {
					m_ViewPorts.push_back(ViewPort(m_fbSpec, CameraType::ThreeD, m_viewPortCount));
					m_viewPortCount++;
				}
				ImGui::SameLine();
				if (ImGui::Button("Add 2D ViewPort")) {
					m_ViewPorts.push_back(ViewPort(m_fbSpec, CameraType::TwoD, m_viewPortCount));
					m_viewPortCount++;
				}

				for (ViewPort v : m_ViewPorts) {
    				ImGui::Text("Viewport Hovered : %s" , (v.ViewportHovered ? "Yes" : "No"));
					ImGui::SameLine();
					ImGui::Text("| Viewport Focused : %s", (v.ViewportFocused ? "Yes" : "No"));
				}


    			auto io = ImGui::GetIO();
    			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);

				ImGui::Text("Quad Count %d", Graphics::BatchRenderer::GetStats().QuadCount);
				if (ImGui::Button("Recreate SHaders")) {
					Graphics::BatchRenderer::ReCreateShaders();
				}

    			ImGui::End();
    		}

			auto ViewPortIt = m_ViewPorts.begin();
			while (ViewPortIt != m_ViewPorts.end()) {
				if (!ViewPortIt->isOpen) { ViewPortIt = m_ViewPorts.erase(ViewPortIt); continue; }
				ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
				ImGui::Begin(std::format("Viewport {}", ViewPortIt->id).c_str(), &ViewPortIt->isOpen);
				ImDrawList* drawList = ImGui::GetWindowDrawList();

				auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
				auto viewportMaxRegion = ImGui::GetWindowContentRegionMax();
				auto viewportOffset = ImGui::GetWindowPos();
				ViewPortIt->ViewportBounds[0] = { viewportMinRegion.x + viewportOffset.x, viewportMinRegion.y + viewportOffset.y };
				ViewPortIt->ViewportBounds[1] = { viewportMaxRegion.x + viewportOffset.x, viewportMaxRegion.y + viewportOffset.y };

				ViewPortIt->ViewportFocused = ImGui::IsWindowFocused();
				ViewPortIt->ViewportHovered = ImGui::IsWindowHovered();

				//Application::Get().GetImGuiLayer()->BlockEvents(!m_ViewportHovered);

				ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();

				ViewPortIt->ViewportSize = { viewportPanelSize.x, viewportPanelSize.y };

				uint64_t textureID = ViewPortIt->Framebuffer->GetColorAttachmentRendererID(0);

				ImGui::Image(reinterpret_cast<void*>(textureID), ImVec2{ ViewPortIt->ViewportSize.x, ViewPortIt->ViewportSize.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });

				ImGui::End();

				ImGui::PopStyleVar();
				ViewPortIt++;
			}

    		{
    			ImGui::Begin("Settings");

    			ImGui::Text("Monitor count : %d", m_Window->GetMonitorCount());

    			ImGui::Text("Primary Monitor : %s", m_Window->GetPrimaryMonitorName());

    			if(ImGui::Button("VSync")) m_Window->SetVSync(!m_Window->IsVSync());


				for (ViewPort& v : m_ViewPorts) {
					auto camerPosition = v.ViewPortCamera->GetPosition();
					ImGui::Text("Camera Position : %.3f %.3f %.3f", camerPosition.x, camerPosition.y, camerPosition.z);

    				auto cameraFocalPoint = v.ViewPortCamera->GetFocalPoint();
    				ImGui::Text("Camera Focus point : %.3f %.3f %.3f", cameraFocalPoint.x, cameraFocalPoint.y, cameraFocalPoint.z);

					auto tmp = cameraFocalPoint;
					DrawVec3Control("Transform", cameraFocalPoint);
					if (tmp != cameraFocalPoint) {
						v.ViewPortCamera->SetFocalPoint(cameraFocalPoint);
						v.update();
					}


					auto viewDirection = v.ViewPortCamera->GetViewDirection();
					ImGui::Text("Camera View Direction : %.3f %.3f %.3f", viewDirection.x, viewDirection.y, viewDirection.z);
					//auto fragNormal = glm::inverseTranspose(m_ApplicationCamera.GetViewMatrix()) * glm::vec3(0.0,0.0,1.0);
					if (ImGui::Button(std::format("Reset Camera {}", v.id).c_str())) { v.ViewPortCamera->ResetFocalPoint(); v.update(); };
					auto zoom = v.ViewPortCamera->getZoom();
					ImGui::Text("Camera Zoom : %.20f", zoom);
				}


    			ImGui::End();
    		}
    	}
}
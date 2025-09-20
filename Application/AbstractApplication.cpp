#include "AbstractApplication.h"
#include "Renderer/Renderer.h"
#include <Logger.h>
#include <imgui_internal.h>
#include "Renderer/BatchRenderer.h"
#include <Events/Input.h>

#define MAX_SELECTED_OBJECT_ID 10000
namespace GUI {
	bool Layer::m_updateLayers = true;

	int ViewPort::s_selectedObject = -1;

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
			Graphics::FramebufferTextureFormat::RGBA8,
			Graphics::FramebufferTextureFormat::Depth,
		};
		m_fbSpec.Width = 1280;
		m_fbSpec.Height = 720;

		m_ViewPorts.push_back(ViewPort(m_fbSpec, CameraType::ThreeD ,m_viewPortCount)); // Default viewport
		m_viewPortCount++;

		m_CameraBuffer = Graphics::UniformBuffer::Create(sizeof(SceneDataUBO), 0);

		this->CreateShaders();
		Graphics::BatchRenderer::Init();

		m_ImGuiHandler = new ImGuiHandler((GLFWwindow*)m_Window->GetNativeWindow(), "#version 330");
	}

	AbstractApplication::~AbstractApplication()
	{
		HZ_PROFILE_FUNCTION();

		Graphics::Renderer::Shutdown();
	}

	void AbstractApplication::CreateShaders() {
		m_font = Graphics::Texture2D::Create("./Resources/Textures/FontAtlas.png");
		m_gridShader = Graphics::Shader::Create("./Resources/Shaders/Grid.glsl", true);
		m_gridShader2D = Graphics::Shader::Create("./Resources/Shaders/Grid2D.glsl", true);
		m_JumpFlood_init = Graphics::Shader::Create("./Resources/Shaders/JumpFloodInit.glsl", true);
		m_JumpFlood_init2 = Graphics::Shader::Create("./Resources/Shaders/JumpFloodInit2.glsl", true);
		m_JumpFlood_pass = Graphics::Shader::Create("./Resources/Shaders/JumpFloodPass.glsl", true);
		m_JumpFlood_composite = Graphics::Shader::Create("./Resources/Shaders/JumpFloodComposite.glsl", true);
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

		LOG_TRACE_STREAM << e.ToString();

		Application::EventDispatcher dispatcher(e);
		dispatcher.Dispatch<Application::WindowCloseEvent>(APP_BIND_EVENT_FN(AbstractApplication::OnWindowClose));
		dispatcher.Dispatch<Application::WindowResizeEvent>(APP_BIND_EVENT_FN(AbstractApplication::OnWindowResize));


		if (ImGui::GetIO().WantCaptureMouse && std::all_of(m_ViewPorts.begin(), m_ViewPorts.end(), [](ViewPort v) { return v.ViewportHovered == false; })) return;

		for (ViewPort& viewPort : m_ViewPorts) {
			if (!viewPort.ViewportHovered || !viewPort.ViewportFocused) continue;
			viewPort.ViewPortCamera->OnEvent(e);

			if (e.GetEventType() == Application::EventType::MouseButtonReleased) {

				auto mouseEvent = dynamic_cast<Application::MouseButtonReleasedEvent*>(&e);
				LOG_TRACE_STREAM << "Mouse button hold duration: " << mouseEvent->GetPressDuration();

				//This means mouse button was held down
				if (mouseEvent->GetPressDuration() > std::chrono::milliseconds(350)) break;

				auto [mx, my] = ImGui::GetMousePos();
				mx -= viewPort.ViewportBounds[0].x;
				my -= viewPort.ViewportBounds[0].y;
				glm::vec2 viewportSize = viewPort.ViewportBounds[1] - viewPort.ViewportBounds[0];
				my = viewportSize.y - my;
				int mouseX = (int)mx;
				int mouseY = (int)my;
				LOG_TRACE_STREAM << "MouseX: " << mouseX << " MouseY: " << mouseY;
				viewPort.Framebuffer->Bind();
				int selectedObject = viewPort.Framebuffer->ReadPixel(1, mouseX, mouseY);
				LOG_TRACE_STREAM << "Selected Object :" << selectedObject;
				viewPort.Framebuffer->Unbind();

				if (selectedObject != -1 && selectedObject < MAX_SELECTED_OBJECT_ID) {
					m_ObjectSelection.objectID = selectedObject;
					m_ObjectSelection.state = true;
					m_emitSelectionEvent = true;
				}

				else if (m_ObjectSelection.objectID != -1) {
					m_ObjectSelection.state = false;
					m_emitSelectionEvent = true;
				}

			}
		}

		if (m_emitSelectionEvent) {

			//set the seectedObject static var here.
			if (m_ObjectSelection.state) {
				ViewPort::s_selectedObject = m_ObjectSelection.objectID;
			}
			else 
				ViewPort::s_selectedObject = -1;

			for (auto it = m_LayerStack.rbegin(); it != m_LayerStack.rend(); ++it)
			{
				(*it)->OnSelection(m_ObjectSelection.objectID, m_ObjectSelection.state);
			}
			m_emitSelectionEvent = false;

			if (m_ObjectSelection.state == false) 
				m_ObjectSelection.objectID = -1;
		}

		//Finish all event processing and then update the vieports
		for (ViewPort& viewPort : m_ViewPorts) {
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
							const auto& xSize = v.ViewportSize.x;
							const auto& ySize = v.ViewportSize.y;
							LOG_TRACE_STREAM << "Viewport resized to: " << xSize << " x " << ySize;
							//Update here coz this runs only when viewport size changes
							v.JumpFloodFramebuffer->Resize((uint32_t)xSize, (uint32_t)ySize);
							v.Framebuffer->Resize((uint32_t)xSize, (uint32_t)ySize);
							v.ViewPortCamera->SetViewportSize(xSize, ySize);
							v.update();
						}
						if (!v.ViewportHovered && !v.ViewportFocused && !m_updateAllViewPorts) continue;
						LOG_TRACE_STREAM << "Viewport: " << v.id << " Hovered: " << v.ViewportHovered << " Focused: " << v.ViewportFocused << " UpdateAll : " << m_updateAllViewPorts;
						m_CameraBuffer->SetData(&v.uboDataScene, sizeof(v.uboDataScene));

						v.Framebuffer->Bind();
						v.Framebuffer->ClearAttachment(1, -1); // Clear ID buffer
						Graphics::Renderer::DepthTest(true);

						Graphics::BatchRenderer::BeginScene();
						Graphics::Renderer::Clear();
						v.Framebuffer->SetDrawBuffer(2); // Clear just the selection buffer to full transparent
						Graphics::Renderer::Clear(0.0);
						v.Framebuffer->DrawToAllColorBuffers();

							for (Layer* layer : m_LayerStack)
								layer->OnDrawUpdate();

						Graphics::BatchRenderer::EndScene();
						Graphics::Renderer::DisableStencil();

						v.Framebuffer->SetDrawBuffer(0); // prevent drawing to id buffer from here nothing should be drawn to the id buffer anyway...


						/////////////////////////////////////////////////////////////JUMP FLOOD - FOR SELECTED OBJECT/////////////////////////////////////////////////////////////////////////
						if (m_ObjectSelection.objectID > -1 && m_ObjectSelection.objectID < MAX_SELECTED_OBJECT_ID) {

							v.Framebuffer->Unbind();

							v.JumpFloodICFramebuffer->Bind();
							Graphics::Renderer::Clear(0.0); // Clear the jumpflood init frameBuffer

							v.Framebuffer->BindColorAttachmentAsTexture(2, 2);

							m_JumpFlood_init2->Bind();
							Graphics::Renderer::DrawGridTriangles();
							m_JumpFlood_init2->Unbind();

							v.JumpFloodICFramebuffer->Unbind();

							v.JumpFloodFramebuffer->Bind();
							Graphics::Renderer::Clear(0.0); // Clear the jumpflood frameBuffer

							v.JumpFloodFramebuffer->BlitBuffers(v.JumpFloodICFramebuffer->getID(), 0, 0, v.JumpFloodICFramebuffer->GetSpecification().Width, v.JumpFloodICFramebuffer->GetSpecification().Height, 0, 0, v.JumpFloodFramebuffer->GetSpecification().Width, v.JumpFloodFramebuffer->GetSpecification().Height, GL_COLOR_BUFFER_BIT, GL_NEAREST);


							v.JumpFloodFramebuffer->BindColorAttachmentAsTexture(0, 1);
							int steps = 2;
							int step = (int)glm::round(glm::pow<int>(steps - 1, 2));
							int index = 0;
							glm::vec2 texelSize = { 1.0f / v.Framebuffer->GetSpecification().Width, 1.0f / v.Framebuffer->GetSpecification().Height };
							glm::float32 invTexelRatio = texelSize.y / texelSize.x;
							while (step != 0) {

								m_JumpFlood_pass->Bind();
								m_JumpFlood_pass->SetFloat2("a_texelSize", texelSize);
								m_JumpFlood_pass->SetFloat("a_invTexelRatio", invTexelRatio);
								m_JumpFlood_pass->SetInt("a_step", step);
								Graphics::Renderer::DrawGridTriangles();
								m_JumpFlood_pass->Unbind();
								index = (index + 1) % 2;
								step /= 2;
							}

							v.JumpFloodFramebuffer->Unbind();
							v.Framebuffer->Bind();

							m_JumpFlood_composite->Bind();
							Graphics::Renderer::DrawGridTriangles();
							m_JumpFlood_composite->Unbind();

						}
						/////////////////////////////////////////////////////////////JUMP FLOOD - FOR SELECTED OBJECT/////////////////////////////////////////////////////////////////////////


						if (v.cameraType == CameraType::ThreeD) {
							//Grid Shader
							m_gridShader->Bind();
							Graphics::Renderer::DrawGridTriangles();
							m_gridShader->Unbind();
						}
						else {
							Graphics::Renderer::DepthTest(false);
							//Grid Shader
							m_gridShader2D->Bind();
							Graphics::Renderer::DrawGridTriangles();
							m_gridShader2D->Unbind();

							Graphics::BatchRenderer::BeginScene();

							for (Layer* layer : m_LayerStack)
								layer->OnDrawUpdate();

							Graphics::BatchRenderer::EndScene();
							Graphics::Renderer::DepthTest(true);
						}

						v.Framebuffer->DrawToAllColorBuffers(); // prevent drawing to id buffer


						v.Framebuffer->Unbind();
					}
					LOG_TRACE_STREAM << "End Viewports";
					if (m_updateAllViewPorts) m_updateAllViewPorts = false;

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

		float lineHeight = ImGui::GetFontSize() + GImGui->Style.FramePadding.y * 2.0f;
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

			glm::vec2 pos = Application::Input::GetMousePosition();

			std::string mousePos = std::format("Mouse pos screen : {} {}", pos.x, pos.y);
			ImGui::Text(mousePos.c_str());

			for (ViewPort v : m_ViewPorts) {
				float worldXmax = v.ViewPortCamera->getWorldXmax();
				float worldXmin = v.ViewPortCamera->getWorldXmin();
				float worldYmax = v.ViewPortCamera->getWorldYmax();
				float worldYmin = v.ViewPortCamera->getWorldYmin();
				float screenWidth = v.ViewportSize.x;
				float screenHeight = v.ViewportSize.y;

				//ToDo: account for the vieport position.
				glm::vec2 world = { ((pos.x / screenWidth) * (worldXmax - worldXmin)) + worldXmin , worldYmax - ((pos.y / screenHeight) * (worldYmax - worldYmin)) };

				//Note: The mouse coordinates lose precision because of the below two lines 
				world.x += v.ViewPortCamera->GetFocalPoint().x;
				world.y += v.ViewPortCamera->GetFocalPoint().y;


				ImGui::Text("WorldX : %f -> %f", worldXmin, worldXmax);
				ImGui::Text("WorldY : %f -> %f", worldYmin, worldYmax);
				//Note: Warning! Mouse coordinates in world space will lose 0,0 precision if screen size is set to odd number
				ImGui::Text("Screen : %f %f", screenWidth, screenHeight);

				std::string viewPortMousePos = std::format("Mouse pos world : {} {}", world.x, world.y);

				ImGui::Text(viewPortMousePos.c_str());
				ImGui::Text("Viewport Hovered : %s", (v.ViewportHovered ? "Yes" : "No"));
				ImGui::SameLine();
				ImGui::Text("| Viewport Focused : %s", (v.ViewportFocused ? "Yes" : "No"));
			}


			auto io = ImGui::GetIO();
			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);

			ImGui::Text("Quad Count %d", Graphics::BatchRenderer::GetStats().QuadCount);
			if (ImGui::Button("Recreate application SHaders")) {
				this->CreateShaders();
			}
			if (ImGui::Button("Recreate SHaders")) {
				Graphics::BatchRenderer::ReCreateShaders();
			}

			if (ImGui::Button("Show Buffers")) {
				this->showBuffers = !this->showBuffers;
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


			glm::vec2 tmp = ViewPortIt->ViewportSize;
			ViewPortIt->ViewportSize = { viewportPanelSize.x, viewportPanelSize.y };
			
			//one of the viewports has resized
			// TODO: Make this more efficient by marking individual viewports as dirty
			if (tmp != ViewPortIt->ViewportSize) { m_updateAllViewPorts = true; }

			uint64_t textureID = ViewPortIt->Framebuffer->GetColorAttachmentRendererID();

			ImGui::Image(reinterpret_cast<void*>(textureID), ImVec2{ ViewPortIt->ViewportSize.x, ViewPortIt->ViewportSize.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
			auto rectMin = ImVec2{ ViewPortIt->ViewportBounds[0].x, ViewPortIt->ViewportBounds[0].y };
			auto rectMax = ImVec2{ ViewPortIt->ViewportBounds[1].x, ViewPortIt->ViewportBounds[1].y };
			//ImGui::GetForegroundDrawList()->AddRect(rectMin, rectMax, IM_COL32(255, 255, 0, 255));

			ImGui::End();

			ImGui::PopStyleVar();
			ViewPortIt++;
		}

		if (this->showBuffers){
			ImGui::Begin("Buffers", &this->showBuffers);
			
			ImDrawList* drawList = ImGui::GetWindowDrawList();

			for (auto viewPort : m_ViewPorts) {

				uint64_t textureID;
				size_t colorAttachmentCount = viewPort.Framebuffer->GetColorAttachmentCount(); // No -1 due to depth buffer
				auto string = std::format("Viewport {} - Main frameBuffer", viewPort.id);
				ImGui::Text(string.c_str());
				ImGui::BeginChild(std::format("v{}colsMain",viewPort.id).c_str(), ImVec2(0, 200));
				ImGui::BeginColumns(string.c_str(), colorAttachmentCount);
				auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
				auto viewportMaxRegion = ImGui::GetWindowContentRegionMax();
				auto viewportOffset = ImGui::GetWindowPos();

				for (int i = 1; i < colorAttachmentCount; i++) {
					std::string string = std::format("ColorBuffer {}", i);
					ImGui::Text(string.c_str());
					textureID = viewPort.Framebuffer->GetColorAttachmentRendererID(i);
					ImGui::Image(reinterpret_cast<void*>(textureID), ImGui::GetContentRegionAvail(), ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
					ImGui::NextColumn();
				}

				string = std::format("DepthBuffer Main {}", viewPort.id);
				ImGui::Text(string.c_str());
				textureID = viewPort.Framebuffer->GetDepthAttachmentRendererID();
				ImGui::Image(reinterpret_cast<void*>(textureID), ImGui::GetContentRegionAvail(), ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
				ImGui::NextColumn();

				ImGui::EndColumns();
				ImGui::EndChild();



				colorAttachmentCount = viewPort.JumpFloodICFramebuffer->GetColorAttachmentCount() + 1; //for depth Buffer
				string = std::format("Viewport {} - JumpFlood Init frameBuffer", viewPort.id);
				ImGui::Text(string.c_str());
				ImGui::BeginChild(std::format("v{}colsJumpInit", viewPort.id).c_str(), ImVec2(0, 200));
				ImGui::BeginColumns(string.c_str(), colorAttachmentCount);

				for (int i = 0; i < colorAttachmentCount - 1; i++) {
					std::string string = std::format("ColorBuffer {}", i);
					ImGui::Text(string.c_str());
					textureID = viewPort.JumpFloodICFramebuffer->GetColorAttachmentRendererID(i);
					ImGui::Image(reinterpret_cast<void*>(textureID), ImGui::GetContentRegionAvail(), ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
					ImGui::NextColumn();
				}

				string = std::format("DepthBuffer Jump {}", viewPort.id);
				ImGui::Text(string.c_str());
				textureID = viewPort.JumpFloodFramebuffer->GetDepthAttachmentRendererID();
				ImGui::Image(reinterpret_cast<void*>(textureID), ImGui::GetContentRegionAvail(), ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
				ImGui::NextColumn();

				ImGui::EndColumns();
				ImGui::EndChild();



				colorAttachmentCount = viewPort.JumpFloodFramebuffer->GetColorAttachmentCount(); //for depth Buffer
				string = std::format("Viewport {} - JumpFlood frameBuffer", viewPort.id);
				ImGui::Text(string.c_str());
				ImGui::BeginChild(std::format("v{}colsJump", viewPort.id).c_str(), ImVec2(0, 200));
				ImGui::BeginColumns(string.c_str(), colorAttachmentCount);

				for (int i = 0; i < colorAttachmentCount; i++) {
					std::string string = std::format("ColorBuffer {}", i);
					ImGui::Text(string.c_str());
					textureID = viewPort.JumpFloodFramebuffer->GetColorAttachmentRendererID(i);
					ImGui::Image(reinterpret_cast<void*>(textureID), ImGui::GetContentRegionAvail(), ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
					ImGui::NextColumn();
				}

				ImGui::EndColumns();
				ImGui::EndChild();

				ImGui::Separator();

			}

			ImGui::End();
		}
    	{
    		ImGui::Begin("Settings");

    		ImGui::Text("Monitor count : %d", m_Window->GetMonitorCount());

    		ImGui::Text("Primary Monitor : %s", m_Window->GetPrimaryMonitorName());

    			if(ImGui::Button("VSync")) m_Window->SetVSync(!m_Window->IsVSync());
				ImGui::SameLine();
				if (ImGui::Button("Polygon Smooth")) { m_Window->SetPolygonSmooth(!m_Window->IsPolygonSmooth()); m_updateAllViewPorts = true; };


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
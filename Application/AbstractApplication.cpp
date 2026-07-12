#ifdef __APPLE__
#define NS_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#include <Metal/Metal.hpp>
#include <Foundation/Foundation.hpp>
#include <QuartzCore/QuartzCore.hpp>
#endif

#include "AbstractApplication.h"
#include "Renderer/Renderer.h"
#include <Logger.h>
#include <imgui_internal.h>
#include "Renderer/BatchRenderer.h"
#include "Renderer/Environment.h"
#include <Events/Input.h>

// Upper bound for valid pick ids read back from the ID attachment (filters
// garbage reads). Apps assign one id per selectable object — a dense PCB
// easily exceeds tens of thousands, so keep this generous.
#define MAX_SELECTED_OBJECT_ID (1 << 30)
namespace GUI {
	bool Layer::m_updateLayers = true;

	int ViewPort::s_selectedObject = -1;

	AbstractApplication* AbstractApplication::s_Instance = nullptr;

	AbstractApplication::AbstractApplication(const ApplicationSpecification& specification, void* nativeWindow)
		: m_Specification(specification)
	{
		HZ_PROFILE_FUNCTION();

		assert(!s_Instance && "Application already exists!");
		s_Instance = this;

		// Set working directory here
		if (!m_Specification.WorkingDirectory.empty())
			std::filesystem::current_path(m_Specification.WorkingDirectory);

		m_Window = Application::Window::Create(Application::WindowProps(m_Specification.Name), nativeWindow);
		m_Window->SetEventCallback(APP_BIND_EVENT_FN(AbstractApplication::OnEvent));

		Graphics::Renderer::Init();

		m_fbSpec.Attachments = {
			Graphics::FramebufferTextureFormat::RGBA8,
			Graphics::FramebufferTextureFormat::RED_INTEGER,
			Graphics::FramebufferTextureFormat::RGBA8,
#if __APPLE__
            Graphics::FramebufferTextureFormat::DEPTH32STENCIL8,
#else
			Graphics::FramebufferTextureFormat::Depth,
#endif
		};
		m_fbSpec.Width = 1280;
		m_fbSpec.Height = 720;

		m_ViewPorts.push_back(ViewPort(m_fbSpec, CameraType::ThreeD ,m_viewPortCount)); // Default viewport
		m_viewPortCount++;

		m_CameraBuffer = Graphics::UniformBuffer::Create(sizeof(SceneDataUBO), 0, "Camera Buffer");
		m_LightingBuffer = Graphics::UniformBuffer::Create(sizeof(Graphics::LightingUBOData), Graphics::kLightingUBOBinding, "Lighting Buffer");

		this->CreateShaders();
		Graphics::BatchRenderer::Init();

#if BUILDING_METAL
        m_ImGuiHandler = new ImGuiHandler(m_Window->GetNativeWindow(), "");
#else
		m_ImGuiHandler = new ImGuiHandler(m_Window->GetNativeWindow(), "#version 330");
#endif
	}

	AbstractApplication::~AbstractApplication()
	{
		HZ_PROFILE_FUNCTION();

		Graphics::Renderer::Shutdown();
	}

	void AbstractApplication::CreateShaders() {
		m_font = Graphics::Texture2D::Create("Resource/Textures/FontAtlas.png");
		m_gridShader = Graphics::Shader::Create("./Resource/Shaders/Grid.glsl", true);
		m_gridShader2D = Graphics::Shader::Create("./Resource/Shaders/Grid2D.glsl", true);
		m_EnvBackgroundShader = Graphics::Shader::Create("./Resource/Shaders/EnvBackground.glsl", true);
        m_JFAComputeSeed = Graphics::ComputeShader::Create(std::filesystem::path("./Resource/ComputeShaders/JFASeed.glsl"));
        m_JFAComputeShader = Graphics::ComputeShader::Create(std::filesystem::path("./Resource/ComputeShaders/JFAPass.glsl"));
        m_JFAComputeVisualize = Graphics::ComputeShader::Create(std::filesystem::path("./Resource/ComputeShaders/JFAVisualize.glsl"));
        m_JFAComposite = Graphics::ComputeShader::Create(std::filesystem::path("./Resource/ComputeShaders/JFAComposite.glsl"));
        m_GTAOCompute = Graphics::ComputeShader::Create(std::filesystem::path("./Resource/ComputeShaders/GTAO.glsl"));
        m_TonemapCompute = Graphics::ComputeShader::Create(std::filesystem::path("./Resource/ComputeShaders/Tonemap.glsl"));
	}

	// Fill the per-viewport lighting UBO: key light (headlight follows the
	// camera), environment SH + intensity, and the shading/post flags the
	// shared shaders read.
	void AbstractApplication::UpdateLighting(ViewPort& viewPort)
	{
		Graphics::LightingUBOData& lighting = viewPort.uboLighting;
		const Graphics::RenderSettings& settings = viewPort.renderSettings;
		const bool threeD = viewPort.cameraType == CameraType::ThreeD;

		int dirCount = 0;
		if (threeD && settings.keyIntensity > 0.0f) {
			glm::vec3 direction;
			if (settings.headlight) {
				// Camera axes from the inverse view matrix; tilt the light a
				// little off-axis so facets facing the camera still shade.
				const glm::mat4& viewInverse = viewPort.uboDataScene.viewMatrixInverse;
				const glm::vec3 right = glm::normalize(glm::vec3(viewInverse[0]));
				const glm::vec3 up = glm::normalize(glm::vec3(viewInverse[1]));
				const glm::vec3 forward = -glm::normalize(glm::vec3(viewInverse[2]));
				direction = glm::normalize(forward - 0.35f * up + 0.25f * right);
			} else {
				direction = glm::normalize(settings.keyDirection);
			}
			lighting.dirLights[0].direction = glm::vec4(direction, 0.0f);
			lighting.dirLights[0].color = glm::vec4(settings.keyColor * settings.keyIntensity, 0.0f);
			dirCount = 1;
		}

		const glm::vec4* sh = Graphics::EnvironmentIBL::GetIrradianceSH();
		for (int i = 0; i < 9; ++i)
			lighting.shIrradiance[i] = sh[i];

		lighting.params0 = {
			(float)dirCount,
			0.0f, // point lights: none yet (UBO slots reserved)
			threeD ? settings.envIntensity : 0.0f,
			(float)Graphics::EnvironmentIBL::GetSpecularMipCount(),
		};
		lighting.params1 = {
			threeD ? (float)(int)settings.shading : (float)(int)Graphics::SceneShading::Unlit,
			threeD ? 1.0f : 0.0f, // outputLinear: the HDR->tonemap post chain runs
			settings.backgroundBlur,
			0.0f,
		};
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

#if __APPLE__
        auto eventType = e.GetEventType();
        // Manually dispatch events to ImGUI
        if(eventType == Application::EventType::MouseButtonPressed || eventType == Application::EventType::MouseButtonReleased || eventType == Application::EventType::MouseMoved)
            m_ImGuiHandler->OnEvent(e);
#endif
		if (ImGui::GetIO().WantCaptureMouse && std::all_of(m_ViewPorts.begin(), m_ViewPorts.end(), [](ViewPort v) { return v.ViewportHovered == false; })) return;

		for (ViewPort& viewPort : m_ViewPorts) {
			// Picking only needs the cursor over the viewport. Requiring focus
			// too meant the first click on an unfocused viewport did nothing
			// (it only transferred focus).
			if (!viewPort.ViewportHovered) continue;
			if (viewPort.ViewportFocused)
				viewPort.ViewPortCamera->OnEvent(e);

			if (e.GetEventType() == Application::EventType::MouseButtonReleased) {

				auto mouseEvent = dynamic_cast<Application::MouseButtonReleasedEvent*>(&e);
				LOG_TRACE_STREAM << "Mouse button hold duration: " << mouseEvent->GetPressDuration();

				//This means mouse button was held down (a camera drag, not a
				//click). 100ms dropped unhurried-but-genuine clicks.
				if (mouseEvent->GetPressDuration() > std::chrono::milliseconds(200)) break;

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

					m_updateAllViewPorts = true;
				}

				else if (m_ObjectSelection.objectID != -1) {
					m_ObjectSelection.state = false;
					m_emitSelectionEvent = true;

					m_updateAllViewPorts = true;
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

    void AbstractApplication::Run()
	{
        HZ_PROFILE_FUNCTION();

        while (m_Running)
        {
            this->RunLoop();
        }
    }

	void AbstractApplication::RunLoop()
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
                    Graphics::Renderer::BeginLoop();
					Graphics::Renderer::ClearBuffers();

					Graphics::BatchRenderer::SetSelectionActive(
						m_ObjectSelection.objectID > -1 && m_ObjectSelection.objectID < MAX_SELECTED_OBJECT_ID);

					LOG_TRACE_STREAM << "Begin Viewports";
					for (ViewPort& v : m_ViewPorts) {
						//On viewport resize
						if ((v.ViewportSize.x != v.Framebuffer->GetSpecification().Width) || (v.ViewportSize.y != v.Framebuffer->GetSpecification().Height)) {
							const auto& xSize = v.ViewportSize.x;
							const auto& ySize = v.ViewportSize.y;
							LOG_TRACE_STREAM << "Viewport resized to: " << xSize << " x " << ySize;
							//Update here coz this runs only when viewport size changes
							v.Framebuffer->Resize(xSize, ySize);
							v.ViewPortCamera->SetViewportSize(xSize, ySize);
                            v.JFATextureA->Resize(xSize, ySize);
                            v.JFATextureB->Resize(xSize, ySize);
                            v.JFAResultTexture->Resize(xSize, ySize);
                            v.JFACompositeTexture->Resize(xSize, ySize);
                            v.AOTexture->Resize(std::max(1u, xSize / 2), std::max(1u, ySize / 2));
                            v.DisplayTexture->Resize(xSize, ySize);
							v.update();
						}
						if (!v.ViewportHovered && !v.ViewportFocused && !v.updateViewport && !m_updateAllViewPorts) continue;
						LOG_TRACE_STREAM << "Viewport: " << v.id << " Hovered: " << v.ViewportHovered << " Focused: " << v.ViewportFocused << " UpdateAll : " << m_updateAllViewPorts;


						const bool threeD = v.cameraType == CameraType::ThreeD;

						// The 3D background clear is in linear light (the post
						// chain tonemaps + gamma-encodes it back).
						const glm::vec4 backgroundColor = threeD
							? glm::vec4(glm::pow(glm::vec3(v.renderSettings.backgroundColor), glm::vec3(2.2f)), 1.0f)
							: glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
						v.Framebuffer->SetAttachmentClearColor(0, backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a); // Metal load-action clear
						Graphics::Renderer::SetClearColor(backgroundColor); // GL glClear path

						v.Framebuffer->Bind();
                        m_font->Bind();
                        m_CameraBuffer->SetData(&v.uboDataScene, sizeof(v.uboDataScene));
                        UpdateLighting(v);
                        m_LightingBuffer->SetData(&v.uboLighting, sizeof(v.uboLighting));
                        Graphics::BatchRenderer::BindSceneResources(); // material table + IBL textures
						v.Framebuffer->ClearAttachment(1, -1); // Clear ID buffer
						Graphics::Renderer::DepthTest(true);

						Graphics::BatchRenderer::BeginScene();
						Graphics::Renderer::Clear();
						v.Framebuffer->SetDrawBuffer(2); // Clear just the selection buffer to full transparent
						Graphics::Renderer::Clear(0.0);
						v.Framebuffer->DrawToAllColorBuffers();

						// Environment background (3D): fullscreen pass that
						// also resets the pick-ID/selection attachments.
						if (threeD && v.renderSettings.environmentBackground) {
							Graphics::Renderer::SetDepthState(Graphics::DepthState::Disabled);
							m_EnvBackgroundShader->Bind();
							Graphics::Renderer::DrawGridTriangles();
							m_EnvBackgroundShader->Unbind();
							Graphics::Renderer::SetDepthState(Graphics::DepthState::Default);
						}

							for (Layer* layer : m_LayerStack)
								layer->OnDrawUpdate();

						Graphics::BatchRenderer::EndScene();
						Graphics::Renderer::DisableStencil();

						v.Framebuffer->SetDrawBuffer(0); // prevent drawing to id buffer from here nothing should be drawn to the id buffer anyway...


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

						// ---- HDR post chain (3D viewports): GTAO + tonemap
						// into the LDR DisplayTexture the UI shows. ----
						if (threeD) {
							const Graphics::RenderSettings& settings = v.renderSettings;
							const uint32_t halfW = std::max(1u, v.ViewportSize.x / 2);
							const uint32_t halfH = std::max(1u, v.ViewportSize.y / 2);
#if BUILDING_METAL
							// Metal dispatch takes total threads; GL takes 8x8 workgroup counts.
							const int dispatchFullX = (int)v.ViewportSize.x, dispatchFullY = (int)v.ViewportSize.y;
							const int dispatchHalfX = (int)halfW, dispatchHalfY = (int)halfH;
							const float depthRange01 = 1.0f;
#else
							const int dispatchFullX = ((int)v.ViewportSize.x + 7) / 8, dispatchFullY = ((int)v.ViewportSize.y + 7) / 8;
							const int dispatchHalfX = ((int)halfW + 7) / 8, dispatchHalfY = ((int)halfH + 7) / 8;
							const float depthRange01 = 0.0f;
#endif

							if (settings.aoEnabled) {
								const glm::mat4& projection = v.uboDataScene.projectionMatrix;
								struct { glm::vec4 proj; glm::vec4 params; glm::vec4 dims; } gtaoParams = {
									{ projection[0][0], std::abs(projection[1][1]), projection[2][2], projection[3][2] },
									{ settings.aoRadius, settings.aoIntensity, depthRange01, 0.0f },
									{ (float)halfW, (float)halfH, (float)v.ViewportSize.x, (float)v.ViewportSize.y },
								};
								m_GTAOCompute->Bind();
								m_GTAOCompute->BindTexture(v.AOTexture->GetRendererID(), 0);          // [[texture(0)]] AO out
								m_GTAOCompute->BindSampledTexture(v.Framebuffer->GetDepthAttachmentRendererID(), 1); // [[texture(1)]] depth
								m_GTAOCompute->SetData(&gtaoParams, sizeof(gtaoParams), 0);           // [[buffer(0)]]
								m_GTAOCompute->Dispatch(dispatchHalfX, dispatchHalfY, 1);
								m_GTAOCompute->Unbind();
							}

							struct { glm::vec4 p0; glm::vec4 dims; } tonemapParams = {
								{ settings.exposure, (float)(int)settings.tonemap, settings.aoEnabled ? 1.0f : 0.0f, settings.aoIntensity },
								{ (float)v.ViewportSize.x, (float)v.ViewportSize.y, (float)halfW, (float)halfH },
							};
							m_TonemapCompute->Bind();
							m_TonemapCompute->BindTexture(v.DisplayTexture->GetRendererID(), 0);                  // [[texture(0)]] LDR out
							m_TonemapCompute->BindTexture(v.Framebuffer->GetColorAttachmentRendererID(0), 1);     // [[texture(1)]] HDR in
							m_TonemapCompute->BindTexture(v.AOTexture->GetRendererID(), 2);                       // [[texture(2)]] AO
							m_TonemapCompute->SetData(&tonemapParams, sizeof(tonemapParams), 0);                  // [[buffer(0)]]
							m_TonemapCompute->Dispatch(dispatchFullX, dispatchFullY, 1);
							m_TonemapCompute->Unbind();
						}

                        if (m_ObjectSelection.objectID > -1 && m_ObjectSelection.objectID < MAX_SELECTED_OBJECT_ID) {
#if BUILDING_METAL
                            int numGroupsX = v.ViewportSize.x;
                            int numGroupsY = v.ViewportSize.y;
#else
							// Texture dimensions
							int textureWidth = v.ViewportSize.x;
							int textureHeight = v.ViewportSize.y;

							// Local size from the shader
							int localSizeX = 8;
							int localSizeY = 8;

							// Calculate the number of workgroups needed
							// This is a common way to do integer ceiling division
							int numGroupsX = (textureWidth + localSizeX - 1) / localSizeX;
							int numGroupsY = (textureHeight + localSizeY - 1) / localSizeY;
#endif
                            m_JFAComputeSeed->Bind();
                            m_JFAComputeSeed->BindTexture(v.Framebuffer->GetColorAttachmentRendererID(2), 0); // [[texture(0)]]
                            m_JFAComputeSeed->BindTexture(v.JFATextureA->GetRendererID(), 1); // [[texture(1)]]
                            m_JFAComputeSeed->Dispatch(numGroupsX, numGroupsY, 1);
                            m_JFAComputeSeed->Unbind();
                            
                            m_JFAComputeShader->Bind();
                            
                            // The composite shader only draws the outline within
                            // ~8px of the mask (outline_distance + width/2 + softness),
                            // so the jump flood only needs to propagate seeds that
                            // far — not across the whole viewport. Capping the
                            // initial step turns ~log2(viewport) full-screen
                            // dispatches (~11 at 2K) into 5.
                            constexpr int kMaxOutlineDistancePx = 16;
                            int step = std::min((int)(std::max(v.ViewportSize.x, v.ViewportSize.y) / 2), kMaxOutlineDistancePx);
                            int passCount = 0;
                            
                            while (step > 0) {
                                // Determine which texture is the input and which is the output for this pass.
                                auto inputTexture = (passCount % 2 == 0) ? v.JFATextureA.get() : v.JFATextureB.get();
                                auto outputTexture = (passCount % 2 == 0) ? v.JFATextureB.get() : v.JFATextureA.get();
                                
                                // Set the textures and the step value for the GPU kernel.
                                m_JFAComputeShader->BindTexture(inputTexture->GetRendererID(), 0); // [[texture(0)]]
                                m_JFAComputeShader->BindTexture(outputTexture->GetRendererID(), 1); // [[texture(1)]]
                                m_JFAComputeShader->SetInt(&step, 0); // [[buffer(0)]]
                                
                                // Dispatch the compute kernel. 🚀
                                m_JFAComputeShader->Dispatch(numGroupsX, numGroupsY, 1);

								// Prepare for the next pass
								step /= 2;
								passCount += 1;
							}

							auto JFAResult = (passCount % 2 == 0) ? v.JFATextureA.get() : v.JFATextureB.get();

							m_JFAComputeShader->Unbind();

							if (showBuffers) {
								m_JFAComputeVisualize->Bind();
								m_JFAComputeVisualize->BindTexture(v.JFAResultTexture->GetRendererID(), 0); // [[texture(0)]]
								m_JFAComputeVisualize->BindTexture(JFAResult->GetRendererID(), 1); // [[texture(1)]]
								m_JFAComputeVisualize->Dispatch(numGroupsX, numGroupsY, 1);
								m_JFAComputeVisualize->Unbind();
							}

							// 3D viewports composite the outline over the
							// tonemapped LDR display texture; 2D viewports
							// still composite over the framebuffer directly.
							const uintptr_t sceneTexture = threeD
								? v.DisplayTexture->GetRendererID()
								: v.Framebuffer->GetColorAttachmentRendererID();

							m_JFAComposite->Bind();
							m_JFAComposite->BindTexture(v.JFACompositeTexture->GetRendererID(), 0); // [[texture(0)]]
							m_JFAComposite->BindTexture(JFAResult->GetRendererID(), 1); // [[texture(1)]]
							m_JFAComposite->BindTexture(sceneTexture, 2); // [[texture(2)]]
                            m_JFAComposite->Dispatch(numGroupsX, numGroupsY, 1);
                            m_JFAComposite->Unbind();

							if (threeD)
								v.DisplayTexture->Blit(v.JFACompositeTexture->GetRendererID());
							else
								v.Framebuffer->BlitToColorAttachment(0, v.JFACompositeTexture->GetRendererID());
                        }
                        
					}
					LOG_TRACE_STREAM << "End Viewports";
                    Graphics::Renderer::EndLoop();

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

		float lineHeight = 16 + GImGui->Style.FramePadding.y * 2.0f;
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

	// Per-viewport rendering controls (3D viewports only — 2D stays on the
	// legacy unlit path).
	void AbstractApplication::RenderSettingsUI(ViewPort& viewPort)
	{
		if (viewPort.cameraType != CameraType::ThreeD)
			return;

		Graphics::RenderSettings& settings = viewPort.renderSettings;
		bool changed = false;

		ImGui::PushID((int)viewPort.id);
		if (ImGui::CollapsingHeader(std::format("Rendering (Viewport {})", viewPort.id).c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
			const char* shadingModes[] = { "Unlit", "Lit (PBR)", "Matcap", "Debug Normals" };
			int shading = (int)settings.shading;
			if (ImGui::Combo("Shading", &shading, shadingModes, 4)) {
				settings.shading = (Graphics::SceneShading)shading;
				changed = true;
			}

			ImGui::SeparatorText("Key Light");
			changed |= ImGui::Checkbox("Headlight (follows camera)", &settings.headlight);
			changed |= ImGui::SliderFloat("Intensity", &settings.keyIntensity, 0.0f, 10.0f);
			if (!settings.headlight) {
				if (ImGui::SliderFloat3("Direction", &settings.keyDirection.x, -1.0f, 1.0f)) {
					if (glm::length(settings.keyDirection) < 1e-3f)
						settings.keyDirection = { 0.0f, -1.0f, 0.0f };
					changed = true;
				}
			}
			changed |= ImGui::ColorEdit3("Color", &settings.keyColor.x, ImGuiColorEditFlags_Float);

			ImGui::SeparatorText("Environment");
			changed |= ImGui::SliderFloat("Env Intensity", &settings.envIntensity, 0.0f, 4.0f);
			changed |= ImGui::Checkbox("Environment Background", &settings.environmentBackground);
			if (settings.environmentBackground)
				changed |= ImGui::SliderFloat("Background Blur", &settings.backgroundBlur, 0.0f, 5.0f);
			else
				changed |= ImGui::ColorEdit3("Background Color", &settings.backgroundColor.x, ImGuiColorEditFlags_Float);

			ImGui::SeparatorText("Post");
			const char* tonemapOps[] = { "Linear", "ACES", "AgX" };
			int tonemap = (int)settings.tonemap;
			if (ImGui::Combo("Tonemap", &tonemap, tonemapOps, 3)) {
				settings.tonemap = (Graphics::TonemapOperator)tonemap;
				changed = true;
			}
			changed |= ImGui::SliderFloat("Exposure", &settings.exposure, 0.1f, 4.0f);
			changed |= ImGui::Checkbox("Ambient Occlusion (GTAO)", &settings.aoEnabled);
			if (settings.aoEnabled) {
				changed |= ImGui::SliderFloat("AO Radius", &settings.aoRadius, 0.05f, 10.0f);
				changed |= ImGui::SliderFloat("AO Intensity", &settings.aoIntensity, 0.0f, 2.0f);
			}
		}
		ImGui::PopID();

		if (changed)
			m_updateAllViewPorts = true;
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


			glm::u32vec2 tmp = ViewPortIt->ViewportSize;
			ViewPortIt->ViewportSize = { viewportPanelSize.x, viewportPanelSize.y };
			
			//one of the viewports has resized
			// TODO: Make this more efficient by marking individual viewports as dirty
			if (tmp != ViewPortIt->ViewportSize) { m_updateAllViewPorts = true; }

			// 3D viewports show the tonemapped LDR output of the post chain;
			// 2D viewports show the framebuffer directly (legacy path).
			uint64_t textureID = ViewPortIt->cameraType == CameraType::ThreeD
				? (uint64_t)ViewPortIt->DisplayTexture->GetRendererID()
				: (uint64_t)ViewPortIt->Framebuffer->GetColorAttachmentRendererID();

			ImGui::Image(reinterpret_cast<void*>(textureID), ImVec2{ (float)ViewPortIt->ViewportSize.x, (float)ViewPortIt->ViewportSize.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
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

				for (int i = 2; i < colorAttachmentCount; i++) {
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

                
                string = std::format("Viewport {} - JumpFlood Textures", viewPort.id);
                ImGui::Text(string.c_str());
                ImGui::BeginChild(std::format("v{}colsJumpTextures", viewPort.id).c_str(), ImVec2(0, 200));
                ImGui::BeginColumns(string.c_str(), 4);

                    string = std::format("JFA Texture A");
                    ImGui::Text(string.c_str());
                    textureID = viewPort.JFATextureA->GetRendererID();
                    ImGui::Image(reinterpret_cast<void*>(textureID), ImGui::GetContentRegionAvail(), ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
                    ImGui::NextColumn();
                
                    string = std::format("JFA Texture B");
                    ImGui::Text(string.c_str());
                    textureID = viewPort.JFATextureB->GetRendererID();
                    ImGui::Image(reinterpret_cast<void*>(textureID), ImGui::GetContentRegionAvail(), ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
                    ImGui::NextColumn();
                
                    string = std::format("JFA Result");
                    ImGui::Text(string.c_str());
                    textureID = viewPort.JFAResultTexture->GetRendererID();
                    ImGui::Image(reinterpret_cast<void*>(textureID), ImGui::GetContentRegionAvail(), ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
                    ImGui::NextColumn();
                
                    string = std::format("JFA Composite");
                    ImGui::Text(string.c_str());
                    textureID = viewPort.JFACompositeTexture->GetRendererID();
                    ImGui::Image(reinterpret_cast<void*>(textureID), ImGui::GetContentRegionAvail(), ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
                    ImGui::NextColumn();

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

				{
					// Trackpad two-finger scroll behavior in 3D viewports
					// (Shift temporarily flips Pan<->Orbit; pinch zooms).
					const char* scrollActions[] = { "Pan", "Orbit", "Zoom" };
					int action = (int)Graphics::ThreeDCamera::s_TrackpadScrollAction;
					if (ImGui::Combo("Trackpad Scroll (3D)", &action, scrollActions, 3))
						Graphics::ThreeDCamera::s_TrackpadScrollAction = (Graphics::TrackpadScrollAction)action;
				}


			for (ViewPort& v : m_ViewPorts) {
				RenderSettingsUI(v);

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
				if (ImGui::Button(std::format("Reset Camera {}", v.id).c_str())) { v.ViewPortCamera->ResetFocalPoint(); v.update(); v.updateViewport = true; };
				auto zoom = v.ViewPortCamera->getZoom();
				ImGui::Text("Camera Zoom : %.20f", zoom);
			}


    		ImGui::End();
    	}
    }
}
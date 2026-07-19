#include "GraphicsCore.h"
#include "Renderer/RayTracing.h"

#include "Renderer/BatchRenderer.h"
#include "Renderer/ComputeShader.h"
#include "Renderer/Environment.h"

#include <Logger.h>

#if BUILDING_METAL
#include "Platform/Metal/MetalRayTracing.h"
#endif

namespace Graphics {

namespace {

	// std140 mirror of the RTParams block in RTCommon.h — keep in sync.
	struct RTParamsUBO {
		glm::mat4 invView;
		glm::mat4 view;
		glm::mat4 prevViewProj;
		glm::vec4 camPos;  // xyz, w = frame index
		glm::vec4 proj;    // P00, P11 (signed), P22, P32
		glm::vec4 screen;  // width, height, historyValid, unused
		glm::vec4 restir;  // initialCandidates, spatialTaps, spatialRadiusPx, temporalMClamp
		glm::vec4 gi;      // cascade0 interval, giIntensity, giEnabled, envMipCount
		glm::vec4 misc;    // emissiveTriCount, envIntensity, debugView, cascadeIndex
	};

	// Buffer bindings shared by every RT pass (see RTCommon.h).
	constexpr int kParamsSlot = 0;
	constexpr int kLightingSlot = 2;
	constexpr int kPositionsSlot = 4;
	constexpr int kAttribsSlot = 5;
	constexpr int kIndicesSlot = 6;
	constexpr int kMaterialsSlot = 7;
	constexpr int kEmissiveSlot = 8;
	constexpr int kTLASSlot = 9;

	// Cascade 0 probe spacing in pixels; doubles per cascade while the
	// per-probe direction count quadruples (constant texels per cascade).
	constexpr uint32_t kProbeSpacing0 = 8;

	struct Shaders {
		Ref<ComputeShader> primary, temporal, cascades, shade;
		bool initialized = false;
	};
	static Shaders s_Shaders;

	void BindSceneBuffers(const Ref<ComputeShader>& shader)
	{
#if BUILDING_METAL
		shader->BindBuffer(MetalRayTracing::GetPositionsBuffer(), kPositionsSlot);
		shader->BindBuffer(MetalRayTracing::GetAttribsBuffer(), kAttribsSlot);
		shader->BindBuffer(MetalRayTracing::GetIndexBuffer(), kIndicesSlot);
		shader->BindBuffer(MetalRayTracing::GetMaterialsBuffer(), kMaterialsSlot);
		shader->BindBuffer(MetalRayTracing::GetEmissiveBuffer(), kEmissiveSlot);
		shader->BindAccelerationStructure(MetalRayTracing::GetTLAS(), kTLASSlot);
		MetalRayTracing::MakeResident();
#endif
	}

	// Cascade i texture dimensions: probe grid (ceil of screen / spacing)
	// times the per-probe octahedral direction tile.
	glm::u32vec2 CascadeTextureSize(uint32_t width, uint32_t height, int cascade)
	{
		const uint32_t spacing = kProbeSpacing0 << cascade;
		const uint32_t probesX = (width + spacing - 1) / spacing;
		const uint32_t probesY = (height + spacing - 1) / spacing;
		const uint32_t dirsX = 4u << cascade;
		const uint32_t dirsY = 2u << cascade;
		return { probesX * dirsX, probesY * dirsY };
	}

	void EnsureResources(RayTracingViewport& vp, uint32_t width, uint32_t height)
	{
		if (vp.width == width && vp.height == height && vp.hitData)
			return;

		vp.hitData = Texture2D::Create(width, height, TextureFormat::RGBA32FLOAT);
		for (int i = 0; i < 2; i++) {
			vp.normalDepth[i] = Texture2D::Create(width, height, TextureFormat::RGBA16FLOAT);
			vp.reservoir[i] = Texture2D::Create(width, height, TextureFormat::RGBA32FLOAT);
		}
		for (int i = 0; i < RayTracingViewport::kCascadeCount; i++) {
			const glm::u32vec2 size = CascadeTextureSize(width, height, i);
			vp.cascades[i] = Texture2D::Create(size.x, size.y, TextureFormat::RGBA16FLOAT);
		}
		vp.rtColor = Texture2D::Create(width, height, TextureFormat::RGBA16FLOAT);

		vp.width = width;
		vp.height = height;
		vp.historyValid = false;
	}

} // namespace

	bool RayTracedRenderer::Supported()
	{
#if BUILDING_METAL
		return MetalRayTracing::Supported();
#else
		return false;
#endif
	}

	void RayTracedRenderer::Init()
	{
		if (s_Shaders.initialized || !Supported())
			return;
		s_Shaders.primary = ComputeShader::Create(std::filesystem::path("./Resource/ComputeShaders/RTPrimary.glsl"));
		s_Shaders.temporal = ComputeShader::Create(std::filesystem::path("./Resource/ComputeShaders/RTRestirTemporal.glsl"));
		s_Shaders.cascades = ComputeShader::Create(std::filesystem::path("./Resource/ComputeShaders/RTCascades.glsl"));
		s_Shaders.shade = ComputeShader::Create(std::filesystem::path("./Resource/ComputeShaders/RTRestirShade.glsl"));
		s_Shaders.initialized = true;
		LOG_INFO_STREAM << "RayTracedRenderer: initialized (hardware ray tracing available)";
	}

	bool RayTracedRenderer::IsInitialized()
	{
		return s_Shaders.initialized;
	}

	bool RayTracedRenderer::Render(RayTracingViewport& vp, const RenderSettings& settings,
		const LightingUBOData& lighting, const RayTracingCamera& camera,
		uintptr_t rasterColor, uintptr_t rasterDepth,
		uint32_t width, uint32_t height)
	{
		if (!s_Shaders.initialized || width == 0 || height == 0)
			return false;

#if BUILDING_METAL
		if (!MetalRayTracing::EnsureScene())
			return false;
#else
		return false;
#endif

		// Stale history is worse than none: reservoirs index into the light/
		// triangle lists, which just changed shape.
		const BatchRenderer::RetainedSceneView view = BatchRenderer::GetRetainedSceneView();
		if (view.geometryVersion != vp.geometryVersion || view.materialVersion != vp.materialVersion) {
			vp.geometryVersion = view.geometryVersion;
			vp.materialVersion = view.materialVersion;
			vp.historyValid = false;
		}

		EnsureResources(vp, width, height);

		const int cur = (int)(vp.frame & 1);
		const int prev = 1 - cur;

		RTParamsUBO params;
		params.invView = camera.invView;
		params.view = camera.view;
		params.prevViewProj = vp.prevViewProj;
		params.camPos = glm::vec4(camera.position, (float)(vp.frame % 4096));
		params.proj = { camera.proj[0][0], camera.proj[1][1], camera.proj[2][2], camera.proj[3][2] };
		params.screen = { (float)width, (float)height, vp.historyValid ? 1.0f : 0.0f, 0.0f };
		params.restir = { (float)settings.rtInitialCandidates, (float)settings.rtSpatialTaps,
			settings.rtSpatialRadius, settings.rtTemporalClamp };
		const float envMips = EnvironmentIBL::IsInitialized() ? (float)EnvironmentIBL::GetSpecularMipCount() : 1.0f;
		params.gi = { settings.rtGIRange, settings.rtGIIntensity, settings.rtGIEnabled ? 1.0f : 0.0f, envMips };
#if BUILDING_METAL
		const float emissiveCount = (float)MetalRayTracing::GetEmissiveTriangleCount();
#else
		const float emissiveCount = 0.0f;
#endif
		params.misc = { emissiveCount, settings.envIntensity, (float)settings.rtDebugView, 0.0f };

		const uintptr_t envMap = EnvironmentIBL::IsInitialized()
			? EnvironmentIBL::GetSpecularMap()->GetRendererID() : vp.rtColor->GetRendererID();

		// Metal dispatch takes total threads (the compute backend divides by
		// its fixed threadgroup size); the RT pipeline is Metal-only today.
		// ---- 1. Primary visibility: exact hit attributes per pixel ----
		s_Shaders.primary->Bind();
		s_Shaders.primary->BindTexture(vp.hitData->GetRendererID(), 0);
		s_Shaders.primary->BindTexture(vp.normalDepth[cur]->GetRendererID(), 1);
		s_Shaders.primary->SetData(&params, sizeof(params), kParamsSlot);
		s_Shaders.primary->SetData(&lighting, sizeof(lighting), kLightingSlot);
		BindSceneBuffers(s_Shaders.primary);
		s_Shaders.primary->Dispatch(width, height, 1);
		s_Shaders.primary->Unbind();

		// ---- 2. ReSTIR: initial candidates + temporal reuse ----
		s_Shaders.temporal->Bind();
		s_Shaders.temporal->BindTexture(vp.reservoir[cur]->GetRendererID(), 0);
		s_Shaders.temporal->BindSampledTexture(vp.hitData->GetRendererID(), 1);
		s_Shaders.temporal->BindSampledTexture(vp.normalDepth[cur]->GetRendererID(), 2);
		s_Shaders.temporal->BindSampledTexture(vp.normalDepth[prev]->GetRendererID(), 3);
		s_Shaders.temporal->BindSampledTexture(vp.reservoir[prev]->GetRendererID(), 4);
		s_Shaders.temporal->BindSampledTexture(envMap, 5);
		s_Shaders.temporal->SetData(&params, sizeof(params), kParamsSlot);
		s_Shaders.temporal->SetData(&lighting, sizeof(lighting), kLightingSlot);
		BindSceneBuffers(s_Shaders.temporal);
		s_Shaders.temporal->Dispatch(width, height, 1);
		s_Shaders.temporal->Unbind();

		// ---- 3. Radiance cascades, top-down (each merges into the one
		// below via the interval hierarchy) ----
		if (settings.rtGIEnabled) {
			for (int cascade = RayTracingViewport::kCascadeCount - 1; cascade >= 0; cascade--) {
				const int upper = std::min(cascade + 1, RayTracingViewport::kCascadeCount - 1);
				params.misc.w = (float)cascade;
				const glm::u32vec2 size = CascadeTextureSize(width, height, cascade);

				s_Shaders.cascades->Bind();
				s_Shaders.cascades->BindTexture(vp.cascades[cascade]->GetRendererID(), 0);
				s_Shaders.cascades->BindSampledTexture(vp.cascades[upper]->GetRendererID(), 1);
				s_Shaders.cascades->BindSampledTexture(vp.hitData->GetRendererID(), 2);
				s_Shaders.cascades->BindSampledTexture(vp.normalDepth[cur]->GetRendererID(), 3);
				s_Shaders.cascades->BindSampledTexture(envMap, 5);
				s_Shaders.cascades->SetData(&params, sizeof(params), kParamsSlot);
				s_Shaders.cascades->SetData(&lighting, sizeof(lighting), kLightingSlot);
				BindSceneBuffers(s_Shaders.cascades);
				s_Shaders.cascades->Dispatch(size.x, size.y, 1);
				s_Shaders.cascades->Unbind();
			}
			params.misc.w = 0.0f;
		}

		// ---- 4. Spatial reuse + shade + GI resolve + composite ----
		s_Shaders.shade->Bind();
		s_Shaders.shade->BindTexture(vp.rtColor->GetRendererID(), 0);
		s_Shaders.shade->BindSampledTexture(vp.hitData->GetRendererID(), 1);
		s_Shaders.shade->BindSampledTexture(vp.normalDepth[cur]->GetRendererID(), 2);
		s_Shaders.shade->BindSampledTexture(vp.reservoir[cur]->GetRendererID(), 3);
		s_Shaders.shade->BindSampledTexture(vp.cascades[0]->GetRendererID(), 4);
		s_Shaders.shade->BindSampledTexture(envMap, 5);
		s_Shaders.shade->BindSampledTexture(rasterColor, 6);
		s_Shaders.shade->BindSampledTexture(rasterDepth, 7);
		s_Shaders.shade->SetData(&params, sizeof(params), kParamsSlot);
		s_Shaders.shade->SetData(&lighting, sizeof(lighting), kLightingSlot);
		BindSceneBuffers(s_Shaders.shade);
		s_Shaders.shade->Dispatch(width, height, 1);
		s_Shaders.shade->Unbind();

		vp.frame++;
		vp.prevViewProj = camera.viewProj;
		vp.historyValid = true;
		return true;
	}

}

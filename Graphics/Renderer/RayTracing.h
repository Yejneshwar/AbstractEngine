#pragma once

// Ray-traced viewport pipeline: ReSTIR direct illumination (reservoir-based
// spatiotemporal importance resampling, Bitterli et al. 2020) plus
// radiance-cascade global illumination (Sannikov 2023, screen-anchored 3D
// variant with hardware-traced probe rays).
//
// Runs as a compute post-process over the rasterized viewport: primary
// visibility is re-traced against an acceleration structure built from the
// retained meshes, lighting is resolved per pixel, and the result replaces
// the raster color wherever the traced surface is the frontmost thing on
// screen (raster-only geometry — lines, grid, background — shows through).
//
// Only available on GPUs with hardware ray tracing (Supported()); callers
// must gate both shader loading and pipeline selection on it.

#include "GraphicsCore.h"
#include "Lighting.h"
#include "Texture.h"

#include <glm/glm.hpp>

namespace Graphics {

	// Per-viewport GPU state. Textures are created/resized lazily by Render.
	struct RayTracingViewport {
		static constexpr int kCascadeCount = 5;

		Ref<Texture> hitData;        // RGBA32F: primitive id bits, bary u, bary v, hit t (<0 miss)
		Ref<Texture> normalDepth[2]; // RGBA16F: world normal, view depth (ping/pong for temporal)
		Ref<Texture> reservoir[2];   // RGBA32F: packed DI reservoir (ping/pong)
		Ref<Texture> cascades[kCascadeCount]; // RGBA16F probe-direction radiance atlases
		Ref<Texture> rtColor;        // RGBA16F: final HDR (tonemap input in RayTraced mode)

		uint32_t width = 0, height = 0;
		uint64_t frame = 0;
		glm::mat4 prevViewProj = glm::mat4(1.0f);
		bool historyValid = false;
		uint64_t geometryVersion = 0, materialVersion = 0;
	};

	// Camera state for one viewport, extracted from the scene UBO.
	struct RayTracingCamera {
		glm::mat4 view;
		glm::mat4 invView;
		glm::mat4 proj;
		glm::mat4 viewProj;
		glm::vec3 position;
	};

	class RayTracedRenderer {
	public:
		enum class DebugView : int {
			Off = 0,
			GIOnly = 1,      // radiance-cascade irradiance
			DirectOnly = 2,  // ReSTIR direct lighting
			Normals = 3,     // traced world-space normals
			HitDistance = 4,
			HistoryLength = 5, // reservoir M (temporal convergence)
			Reflections = 6,   // traced/env specular term
		};

		// Hardware ray tracing available (Metal: MTLDevice.supportsRaytracing;
		// GL: never).
		static bool Supported();

		// Load the RT compute shaders. Call once, only when Supported().
		static void Init();
		static bool IsInitialized();

		// Render the viewport into vp.rtColor. rasterColor/rasterDepth are the
		// framebuffer's color0/depth attachment handles. Returns false when
		// there is nothing to trace (caller falls back to the raster path).
		static bool Render(RayTracingViewport& vp, const RenderSettings& settings,
			const LightingUBOData& lighting, const RayTracingCamera& camera,
			uintptr_t rasterColor, uintptr_t rasterDepth,
			uint32_t width, uint32_t height);
	};

}

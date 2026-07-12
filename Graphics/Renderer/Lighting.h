#pragma once

// C++ mirrors of Resources/Shaders/LightingDeclarations.h (std140 layouts —
// keep both sides in sync) plus the per-viewport render settings the
// application exposes in its UI.

#include <cstdint>
#include <glm/glm.hpp>

namespace Graphics {

	constexpr int kMaxDirLights = 4;
	constexpr int kMaxPointLights = 8;
	constexpr int kMaxMaterials = 256;

	constexpr uint32_t kLightingUBOBinding = 2;
	constexpr uint32_t kMaterialUBOBinding = 3;

	// Fragment texture slots used by the lit pipeline (kept clear of slot 0,
	// where the app binds its font atlas).
	constexpr uint32_t kEnvSpecularTextureSlot = 4;
	constexpr uint32_t kMatcapTextureSlot = 5;

	enum class SceneShading : int {
		Unlit = 0,
		Lit = 1,
		Matcap = 2,
		Normals = 3, // debug view: world-space normals as color
	};

	enum class TonemapOperator : int {
		Linear = 0,
		ACES = 1,
		AgX = 2,
	};

	// std140 mirror of LightingUBO (binding 2).
	struct LightingUBOData {
		struct DirLight {
			glm::vec4 direction; // xyz = direction light travels, w unused
			glm::vec4 color;     // rgb * intensity (linear)
		};
		struct PointLight {
			glm::vec4 position;  // xyz, w = influence radius (<= 0: unbounded)
			glm::vec4 color;     // rgb * intensity (linear)
		};

		DirLight dirLights[kMaxDirLights] = {};
		PointLight pointLights[kMaxPointLights] = {};
		glm::vec4 shIrradiance[9] = {};
		glm::vec4 params0 = { 0.0f, 0.0f, 1.0f, 1.0f }; // dirCount, pointCount, envIntensity, envSpecularMipCount
		glm::vec4 params1 = { 0.0f, 0.0f, 0.0f, 0.0f }; // shadingMode, outputLinear, backgroundBlurLod, unused
	};

	// std140 mirror of one MaterialUBO entry (binding 3).
	struct GpuMaterial {
		glm::vec4 baseColor = { 1.0f, 1.0f, 1.0f, 1.0f };
		glm::vec4 mrfx = { 0.0f, 0.5f, 0.0f, 0.0f }; // metallic, roughness, flags, unused
		glm::vec4 emissive = { 0.0f, 0.0f, 0.0f, 0.0f };
	};

	// Material flag bits (GpuMaterial::mrfx.z).
	enum MaterialFlags : int {
		MaterialFlag_FlatNormals = 1, // shade with facet normals (screen-space derivatives)
		MaterialFlag_VertexTint = 2,  // multiply baseColor by the per-vertex color
	};

	// Authoring-side material description; converted to GpuMaterial on upload.
	struct MaterialDesc {
		glm::vec4 baseColor = { 1.0f, 1.0f, 1.0f, 1.0f }; // sRGB-authored
		float metallic = 0.0f;
		float roughness = 0.5f;
		glm::vec3 emissive = { 0.0f, 0.0f, 0.0f }; // linear, premultiplied by intensity
		bool flatShading = false;
		bool vertexColorTint = true;
	};

	// Per-viewport rendering options surfaced in the UI. Only meaningful for
	// 3D viewports; 2D viewports stay on the unlit/legacy path.
	struct RenderSettings {
		SceneShading shading = SceneShading::Lit;

		// Key light. Headlight mode keeps it locked to the camera so models
		// are always lit no matter how you orbit.
		bool headlight = true;
		float keyIntensity = 2.0f;
		glm::vec3 keyDirection = glm::normalize(glm::vec3(0.4f, -1.0f, 0.25f));
		glm::vec3 keyColor = { 1.0f, 1.0f, 1.0f };

		float envIntensity = 1.0f;

		float exposure = 1.0f;
		TonemapOperator tonemap = TonemapOperator::ACES;

		bool aoEnabled = true;
		float aoRadius = 1.0f;    // world units
		float aoIntensity = 1.0f;

		bool environmentBackground = true;
		float backgroundBlur = 1.5f; // lod into the prefiltered environment
		glm::vec4 backgroundColor = { 0.0f, 0.0f, 0.0f, 1.0f }; // when environmentBackground is off
	};

}

#pragma once

// Procedural image-based-lighting environment, generated on the CPU once at
// startup: a "studio" HDR equirect (gradient sky + softbox area lights),
// its SH9 diffuse irradiance, a GGX-prefiltered specular mip chain, and a
// matcap for the low-power viewport shading mode.

#include "GraphicsCore.h"
#include "Texture.h"
#include <glm/glm.hpp>

namespace Graphics {

	class EnvironmentIBL
	{
	public:
		// Builds the environment and uploads the GPU textures. Requires a
		// live graphics context. Safe to call repeatedly (no-ops after the
		// first call).
		static void Init();
		static bool IsInitialized();

		// GGX-prefiltered equirect environment; mip = roughness * (mips-1).
		static const Ref<Texture2D>& GetSpecularMap();
		static uint32_t GetSpecularMipCount();

		// Cosine-convolved irradiance SH, packed for direct evaluation in
		// the shader (see LightingDeclarations.h). 9 coefficients.
		static const glm::vec4* GetIrradianceSH();

		static const Ref<Texture2D>& GetMatcap();
	};

}

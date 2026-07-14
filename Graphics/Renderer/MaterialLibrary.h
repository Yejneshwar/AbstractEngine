#pragma once

// Built-in library of named PBR (metallic-roughness) material presets,
// flavored for CAD/PCB work (copper, solder mask, FR4, ...) plus generic
// metals/plastics. Presets are plain descriptions; Instantiate() places one
// in the BatchRenderer's GPU material table (cached, one handle per name).

#include "BatchRenderer.h"
#include "Lighting.h"

#include <string>
#include <vector>

namespace Graphics {

	struct MaterialPreset {
		const char* name;
		MaterialDesc desc;
	};

	namespace MaterialLibrary {

		const std::vector<MaterialPreset>& Presets();

		// nullptr when the name is unknown.
		const MaterialDesc* Find(const std::string& name);

		// Handle into the GPU material table (0 — the default material — for
		// unknown names). Repeated calls share one table slot per preset.
		BatchRenderer::MaterialHandle Instantiate(const std::string& name);

	}

}

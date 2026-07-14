#include "MaterialLibrary.h"

#include <map>

namespace Graphics {

namespace {

	MaterialDesc Make(glm::vec4 baseColor, float metallic, float roughness,
	                  glm::vec3 emissive = { 0.0f, 0.0f, 0.0f }, bool flatShading = false)
	{
		MaterialDesc desc;
		desc.baseColor = baseColor; // sRGB-authored (the shader linearizes)
		desc.metallic = metallic;
		desc.roughness = roughness;
		desc.emissive = emissive;
		desc.flatShading = flatShading;
		desc.vertexColorTint = false; // presets define their own color
		return desc;
	}

} // namespace

	const std::vector<MaterialPreset>& MaterialLibrary::Presets()
	{
		// Metal base colors are the usual measured F0 values, converted to
		// the sRGB-authored space the material system expects.
		static const std::vector<MaterialPreset> s_Presets = {
			// PCB
			{ "Copper",            Make({ 0.96f, 0.80f, 0.72f, 1.0f }, 1.0f, 0.35f) },
			{ "Gold (ENIG)",       Make({ 1.00f, 0.88f, 0.60f, 1.0f }, 1.0f, 0.25f) },
			{ "Solder",            Make({ 0.92f, 0.92f, 0.91f, 1.0f }, 1.0f, 0.50f) },
			{ "FR4",               Make({ 0.76f, 0.71f, 0.50f, 1.0f }, 0.0f, 0.65f) },
			{ "Solder Mask Green", Make({ 0.05f, 0.35f, 0.16f, 1.0f }, 0.0f, 0.30f) },
			{ "Solder Mask Blue",  Make({ 0.05f, 0.16f, 0.42f, 1.0f }, 0.0f, 0.30f) },
			{ "Solder Mask Red",   Make({ 0.48f, 0.06f, 0.07f, 1.0f }, 0.0f, 0.30f) },
			{ "Solder Mask Black", Make({ 0.08f, 0.08f, 0.08f, 1.0f }, 0.0f, 0.35f) },
			{ "Silkscreen",        Make({ 0.95f, 0.95f, 0.95f, 1.0f }, 0.0f, 0.50f) },
			// Metals
			{ "Silver",            Make({ 0.99f, 0.98f, 0.96f, 1.0f }, 1.0f, 0.15f) },
			{ "Aluminum",          Make({ 0.96f, 0.96f, 0.97f, 1.0f }, 1.0f, 0.40f) },
			{ "Brushed Steel",     Make({ 0.79f, 0.80f, 0.81f, 1.0f }, 1.0f, 0.55f) },
			{ "Chrome",            Make({ 0.78f, 0.78f, 0.78f, 1.0f }, 1.0f, 0.08f) },
			{ "Brass",             Make({ 0.96f, 0.90f, 0.68f, 1.0f }, 1.0f, 0.30f) },
			{ "Titanium",          Make({ 0.77f, 0.74f, 0.70f, 1.0f }, 1.0f, 0.45f) },
			// Non-metals
			{ "Plastic Glossy",    Make({ 0.90f, 0.90f, 0.90f, 1.0f }, 0.0f, 0.15f) },
			{ "Plastic Matte",     Make({ 0.80f, 0.80f, 0.80f, 1.0f }, 0.0f, 0.70f) },
			{ "Rubber",            Make({ 0.12f, 0.12f, 0.12f, 1.0f }, 0.0f, 0.95f) },
			{ "Ceramic",           Make({ 0.97f, 0.96f, 0.94f, 1.0f }, 0.0f, 0.10f) },
			{ "Glass (Tinted)",    Make({ 0.85f, 0.93f, 0.95f, 0.35f }, 0.0f, 0.05f) },
			{ "LED White",         Make({ 1.00f, 1.00f, 1.00f, 1.0f }, 0.0f, 0.40f, { 4.0f, 4.0f, 3.6f }) },
		};
		return s_Presets;
	}

	const MaterialDesc* MaterialLibrary::Find(const std::string& name)
	{
		for (const MaterialPreset& preset : Presets())
			if (name == preset.name)
				return &preset.desc;
		return nullptr;
	}

	BatchRenderer::MaterialHandle MaterialLibrary::Instantiate(const std::string& name)
	{
		static std::map<std::string, BatchRenderer::MaterialHandle> s_Instantiated;

		auto it = s_Instantiated.find(name);
		if (it != s_Instantiated.end())
			return it->second;

		const MaterialDesc* desc = Find(name);
		if (!desc)
			return 0;

		const BatchRenderer::MaterialHandle handle = BatchRenderer::CreateMaterial(*desc);
		s_Instantiated[name] = handle;
		return handle;
	}

}

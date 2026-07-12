#include "ShowcaseLayer.h"

#include <Logger.h>
#include <imgui.h>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <cmath>
#include <format>

namespace GUI {

namespace {

	struct MeshData {
		std::vector<double> vertices; // xyz triplets
		std::vector<uint32_t> indices;
	};

	// UV sphere with shared vertices (smooth normals fall out of the
	// engine's normal generation).
	MeshData MakeSphere(const glm::dvec3& center, double radius, int rings = 24, int segments = 48)
	{
		MeshData mesh;
		for (int r = 0; r <= rings; ++r) {
			const double theta = glm::pi<double>() * (double)r / rings;
			for (int s = 0; s <= segments; ++s) {
				const double phi = glm::two_pi<double>() * (double)s / segments;
				mesh.vertices.push_back(center.x + radius * std::sin(theta) * std::cos(phi));
				mesh.vertices.push_back(center.y + radius * std::cos(theta));
				mesh.vertices.push_back(center.z + radius * std::sin(theta) * std::sin(phi));
			}
		}
		const int stride = segments + 1;
		for (int r = 0; r < rings; ++r) {
			for (int s = 0; s < segments; ++s) {
				const uint32_t i0 = r * stride + s;
				const uint32_t i1 = i0 + 1;
				const uint32_t i2 = i0 + stride;
				const uint32_t i3 = i2 + 1;
				mesh.indices.insert(mesh.indices.end(), { i0, i2, i1, i1, i2, i3 });
			}
		}
		return mesh;
	}

	// (p,q) torus knot swept with a circular cross-section.
	MeshData MakeTorusKnot(const glm::dvec3& center, double scale, double tubeRadius,
	                       int p = 2, int q = 3, int pathSteps = 200, int tubeSteps = 16)
	{
		MeshData mesh;
		auto knotPoint = [&](double t) {
			const double r = 2.0 + std::cos(q * t);
			return glm::dvec3(r * std::cos(p * t), std::sin(q * t), r * std::sin(p * t)) * scale;
		};

		for (int i = 0; i < pathSteps; ++i) {
			const double t = glm::two_pi<double>() * (double)i / pathSteps;
			const glm::dvec3 point = knotPoint(t);
			const glm::dvec3 tangent = glm::normalize(knotPoint(t + 0.01) - knotPoint(t - 0.01));
			glm::dvec3 normal = glm::normalize(glm::dvec3(point.x, 0.0, point.z));
			glm::dvec3 binormal = glm::normalize(glm::cross(tangent, normal));
			normal = glm::normalize(glm::cross(binormal, tangent));

			for (int j = 0; j < tubeSteps; ++j) {
				const double a = glm::two_pi<double>() * (double)j / tubeSteps;
				const glm::dvec3 v = center + point + (normal * std::cos(a) + binormal * std::sin(a)) * tubeRadius;
				mesh.vertices.insert(mesh.vertices.end(), { v.x, v.y, v.z });
			}
		}
		for (int i = 0; i < pathSteps; ++i) {
			const int ni = (i + 1) % pathSteps;
			for (int j = 0; j < tubeSteps; ++j) {
				const int nj = (j + 1) % tubeSteps;
				const uint32_t i0 = i * tubeSteps + j;
				const uint32_t i1 = i * tubeSteps + nj;
				const uint32_t i2 = ni * tubeSteps + j;
				const uint32_t i3 = ni * tubeSteps + nj;
				mesh.indices.insert(mesh.indices.end(), { i0, i2, i1, i1, i2, i3 });
			}
		}
		return mesh;
	}

	MeshData MakePlane(const glm::dvec3& center, double halfX, double halfZ)
	{
		MeshData mesh;
		const double xs[2] = { center.x - halfX, center.x + halfX };
		const double zs[2] = { center.z - halfZ, center.z + halfZ };
		for (int iz = 0; iz < 2; ++iz)
			for (int ix = 0; ix < 2; ++ix)
				mesh.vertices.insert(mesh.vertices.end(), { xs[ix], center.y, zs[iz] });
		mesh.indices = { 0, 2, 1, 1, 2, 3 };
		return mesh;
	}

} // namespace

	void ShowcaseLayer::AddMesh(const std::string& name, const std::vector<double>& vertices, const std::vector<uint32_t>& indices,
	                            const GUI::DataType::vec4& color, const Graphics::MaterialDesc& materialDesc, bool editable)
	{
		ShowcaseObject object;
		object.name = name;
		object.pickId = m_NextPickId++;
		object.material = Graphics::BatchRenderer::CreateMaterial(materialDesc);
		object.mesh = Graphics::BatchRenderer::CreateMesh(vertices, indices, color, object.pickId, object.material);
		object.editable = editable;
		m_Objects.push_back(object);
	}

	void ShowcaseLayer::OnAttach()
	{
		const GUI::DataType::vec4 white = { 1.0f, 1.0f, 1.0f, 1.0f };

		// Metallic (rows) x roughness (columns) sphere chart.
		constexpr int kGrid = 6;
		constexpr double kSpacing = 1.15;
		for (int m = 0; m < kGrid; ++m) {
			for (int r = 0; r < kGrid; ++r) {
				Graphics::MaterialDesc material;
				material.baseColor = { 0.72f, 0.20f, 0.16f, 1.0f };
				material.metallic = (float)m / (kGrid - 1);
				material.roughness = (float)r / (kGrid - 1);
				material.vertexColorTint = false;

				const glm::dvec3 center((r - (kGrid - 1) * 0.5) * kSpacing,
				                        1.0 + (kGrid - 1 - m) * kSpacing,
				                        0.0);
				const MeshData sphere = MakeSphere(center, 0.48);
				AddMesh(std::format("Sphere m{:.1f} r{:.1f}", material.metallic, material.roughness),
				        sphere.vertices, sphere.indices, white, material);
			}
		}

		// Gold-ish torus knot.
		{
			Graphics::MaterialDesc material;
			material.baseColor = { 1.0f, 0.78f, 0.34f, 1.0f };
			material.metallic = 1.0f;
			material.roughness = 0.22f;
			material.vertexColorTint = false;
			const MeshData knot = MakeTorusKnot({ 6.0, 2.5, 0.0 }, 0.55, 0.24);
			AddMesh("Torus Knot", knot.vertices, knot.indices, white, material);
		}

		// Flat-shaded sphere: same geometry, faceted via material flag.
		{
			Graphics::MaterialDesc material;
			material.baseColor = { 0.30f, 0.55f, 0.85f, 1.0f };
			material.roughness = 0.35f;
			material.flatShading = true;
			material.vertexColorTint = false;
			const MeshData sphere = MakeSphere({ -6.0, 2.5, 0.0 }, 1.0, 12, 18);
			AddMesh("Flat Sphere", sphere.vertices, sphere.indices, white, material);
		}

		// Emissive marker.
		{
			Graphics::MaterialDesc material;
			material.baseColor = { 1.0f, 1.0f, 1.0f, 1.0f };
			material.roughness = 0.4f;
			material.emissive = { 4.0f, 2.4f, 0.8f };
			material.vertexColorTint = false;
			const MeshData sphere = MakeSphere({ -6.0, 0.6, 2.5 }, 0.3, 16, 24);
			AddMesh("Emissive Sphere", sphere.vertices, sphere.indices, white, material);
		}

		// Ground plane (AO catcher).
		{
			Graphics::MaterialDesc material;
			material.baseColor = { 0.62f, 0.62f, 0.60f, 1.0f };
			material.roughness = 0.85f;
			material.vertexColorTint = false;
			const MeshData plane = MakePlane({ 0.0, -0.02, 0.0 }, 14.0, 10.0);
			AddMesh("Ground", plane.vertices, plane.indices, white, material);
		}

		// Wireframe demo (BasicShader / static-triangle path): an open
		// half-tube so both the barycentric wireframe (front) and the
		// diagnostic back-face coloring are visible at once.
		{
			MeshData shell;
			std::vector<double> shellNormals;
			constexpr int kSegments = 24;
			constexpr int kRings = 8;
			const double radius = 1.1;
			const double length = 2.8;
			const glm::dvec3 center(0.0, 1.4, 4.5);
			for (int r = 0; r <= kRings; ++r) {
				const double x = center.x - length * 0.5 + length * r / kRings;
				for (int s = 0; s <= kSegments; ++s) {
					const double a = glm::pi<double>() * s / kSegments; // half circle
					const glm::dvec3 n(0.0, std::sin(a), std::cos(a));
					shell.vertices.insert(shell.vertices.end(), { x, center.y + n.y * radius, center.z + n.z * radius });
					shellNormals.insert(shellNormals.end(), { n.x, n.y, n.z });
				}
			}
			const int stride = kSegments + 1;
			for (int r = 0; r < kRings; ++r) {
				for (int s = 0; s < kSegments; ++s) {
					const uint32_t i0 = r * stride + s;
					const uint32_t i1 = i0 + 1;
					const uint32_t i2 = i0 + stride;
					const uint32_t i3 = i2 + 1;
					shell.indices.insert(shell.indices.end(), { i0, i2, i1, i1, i2, i3 });
				}
			}
			Graphics::BatchRenderer::addData(shell.vertices, shellNormals, shell.indices, 1);
		}

		LOG_INFO_STREAM << "ShowcaseLayer: created " << m_Objects.size() << " objects (+ wireframe shell)";
	}

	void ShowcaseLayer::OnDetach()
	{
		for (const ShowcaseObject& object : m_Objects)
			Graphics::BatchRenderer::DestroyMesh(object.mesh);
		m_Objects.clear();
	}

	void ShowcaseLayer::OnDrawUpdate()
	{
		if (!m_Enabled)
			return;
		for (const ShowcaseObject& object : m_Objects)
			Graphics::BatchRenderer::DrawMesh(object.mesh);
	}

	void ShowcaseLayer::OnSelection(int objectId, bool state)
	{
		if (!state) {
			if (m_SelectedPickId == objectId)
				m_SelectedPickId = -1;
			return;
		}
		for (const ShowcaseObject& object : m_Objects) {
			if (object.pickId == objectId) {
				m_SelectedPickId = objectId;
				return;
			}
		}
	}

	void ShowcaseLayer::OnImGuiRender()
	{
		ImGui::Begin("PBR Showcase");
		ImGui::Checkbox("Show scene", &m_Enabled);
		ImGui::TextWrapped("Click an object in a 3D viewport (or pick below) to edit its material.");

		ShowcaseObject* selected = nullptr;
		for (ShowcaseObject& object : m_Objects)
			if (object.pickId == m_SelectedPickId)
				selected = &object;

		if (ImGui::BeginCombo("Object", selected ? selected->name.c_str() : "<none>")) {
			for (ShowcaseObject& object : m_Objects) {
				if (ImGui::Selectable(object.name.c_str(), &object == selected))
					m_SelectedPickId = object.pickId;
			}
			ImGui::EndCombo();
		}

		if (selected && selected->editable) {
			Graphics::MaterialDesc material = Graphics::BatchRenderer::GetMaterial(selected->material);
			bool changed = false;
			changed |= ImGui::ColorEdit4("Base Color", &material.baseColor.x, ImGuiColorEditFlags_Float);
			changed |= ImGui::SliderFloat("Metallic", &material.metallic, 0.0f, 1.0f);
			changed |= ImGui::SliderFloat("Roughness", &material.roughness, 0.0f, 1.0f);
			changed |= ImGui::ColorEdit3("Emissive", &material.emissive.x, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
			changed |= ImGui::Checkbox("Flat Shading", &material.flatShading);
			if (changed) {
				Graphics::BatchRenderer::UpdateMaterial(selected->material, material);
				this->UpdateLayer(true);
			}
		}

		ImGui::End();
	}

}

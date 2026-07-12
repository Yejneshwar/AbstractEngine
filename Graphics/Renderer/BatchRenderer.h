#pragma once
#include <cstdint>
#include <vector>
#include <Renderer/FrameBuffer.h>
#include <Renderer/Lighting.h>

#include "DataTypes.h"



namespace Graphics {

		struct Statistics
		{
			uint32_t DrawCalls = 0;
			uint32_t QuadCount = 0;
			uint32_t TriangleCount = 0;

			uint32_t GetTotalVertexCount() const { return QuadCount * 4; }
			uint32_t GetTotalIndexCount() const { return QuadCount * 6; }
		};

		class BatchRenderer {
		public:
			static void Init();
			static Statistics GetStats();
			static void ReCreateShaders();
			static void Shutdown();

			static void setRenderMode(int mode);

			static void BeginScene();

			static void setUpdateRequired(bool _state);
			static bool getUpdateRequired();

			// Whether an object is currently selected. When false, the
			// selection-mask pass (DrawSelected) is skipped entirely instead
			// of re-drawing the whole scene into the mask attachment.
			static void SetSelectionActive(bool active);

			// ---- Materials ------------------------------------------------
			// Materials live in a GPU table (see Lighting.h); meshes reference
			// them by handle. Handle 0 is the built-in default (white, rough
			// dielectric, tinted by the per-vertex color).
			using MaterialHandle = uint32_t;

			static MaterialHandle CreateMaterial(const MaterialDesc& desc);
			static void UpdateMaterial(MaterialHandle handle, const MaterialDesc& desc);
			static MaterialDesc GetMaterial(MaterialHandle handle);

			// Attach the material table, lighting textures (environment,
			// matcap) and other scene-wide GPU resources to the current
			// render target. Call once per viewport, after Framebuffer::Bind.
			static void BindSceneResources();

			static void addData(const std::vector<double>& vertices, const std::vector<double>& vertexNormals, const std::vector<uint32_t>& indices, const int id = -1);

			static void DrawMesh(const std::vector<double>& vertices, const std::vector<uint32_t>& indices, const GUI::DataType::vec4& color, const int id = -1);

			// ---- Retained meshes -------------------------------------------
			// Geometry is uploaded to the GPU ONCE at CreateMesh and drawn per
			// frame by handle — no per-frame CPU walk or re-upload. Use this
			// for meshes that don't change every frame (CAD/PCB geometry);
			// use DrawMesh(vertices, ...) only for genuinely dynamic data.
			//
			// Retained meshes render through the lit (PBR) pipeline. When no
			// normals are supplied they are generated (area-weighted smooth);
			// use a material with flatShading for faceted looks instead of
			// duplicating vertices.
			using MeshHandle = uint32_t; // 0 = invalid

			static MeshHandle CreateMesh(const std::vector<double>& vertices, const std::vector<uint32_t>& indices, const GUI::DataType::vec4& color, const int id = -1,
				MaterialHandle material = 0, const std::vector<double>& normals = {});
			static void DestroyMesh(MeshHandle handle);
			static void DrawMesh(MeshHandle handle);

			// Circles are billboard-expanded discs; `normal` is the disc's
			// world-space facing (defaults to +Z — the 2D/PCB plane) and is
			// what the Debug Normals view displays.
			static void DrawCircle(const GUI::DataType::vec3& position, float radius, const GUI::DataType::vec4& color, const int id = -1,
				const GUI::DataType::vec3& normal = GUI::DataType::vec3(0.0f, 0.0f, 1.0f));

			static void DrawCircle(const GUI::DataType::vec2& position, float radius, const GUI::DataType::vec4& color, const int id = -1,
				const GUI::DataType::vec3& normal = GUI::DataType::vec3(0.0f, 0.0f, 1.0f));

			static void DrawLine(const GUI::DataType::vec3& from, const GUI::DataType::vec3& to, const GUI::DataType::vec4& color, const int id = -1);

			static void DrawLine(const GUI::DataType::vec2& from, const GUI::DataType::vec2& to, const GUI::DataType::vec4& color, const int id = -1);

			static void DrawLine(const GUI::DataType::vec3& from, const GUI::DataType::vec3& to, const GUI::DataType::vec4& color, float thickness, const int id = -1);

			static void DrawLine(const GUI::DataType::vec2& from, const GUI::DataType::vec2& to, const GUI::DataType::vec4& color, float thickness, const int id = -1);

			static void DrawLines(const std::vector<GUI::DataType::vec3>& points, const std::vector<uint32_t>& indices, const GUI::DataType::vec4& color, const int id = -1, bool withArrows = false);

#if __APPLE__
            static void DrawLines(const std::vector<glm::vec3>& points, const std::vector<uint32_t>& indices, const GUI::DataType::vec4& color, const int id = -1, bool withArrows = false){
                DrawLines(GUI::convert_to_simd_vector<simd_float3, 3>(points), indices, color, id, withArrows);
            }
#endif

			static void DrawQuad(const GUI::DataType::vec3& p1, const GUI::DataType::vec3& p2, const GUI::DataType::vec3& p3, const GUI::DataType::vec3& p4, const GUI::DataType::vec4& color, const int id = -1);

			static void DrawQuad(const GUI::DataType::vec2& p1, const GUI::DataType::vec2& p2, const GUI::DataType::vec2& p3, const GUI::DataType::vec2& p4, const GUI::DataType::vec4& color, const int id = -1);

			static void DrawQuad(const GUI::DataType::vec2& position, float size, const GUI::DataType::vec4& color, const int id = -1);

			static void DrawQuad(const GUI::DataType::vec2& position, const GUI::DataType::vec2& size, const GUI::DataType::vec4& color, const int id = -1);

			static void DrawQuad(const GUI::DataType::vec3& position, float size, const GUI::DataType::vec4& color, const int id = -1);

			static void DrawQuad(const GUI::DataType::vec3& position, const GUI::DataType::vec2& size, const GUI::DataType::vec4& color, const int id = -1);


			static void DrawObround(const GUI::DataType::vec3& position, const GUI::DataType::vec2& size, const GUI::DataType::vec4& color, const int id = -1);

			static void DrawObround(const GUI::DataType::vec2& position, const GUI::DataType::vec2& size, const GUI::DataType::vec4& color, const int id = -1);

			static void DrawTrace(const GUI::DataType::vec3& from, const GUI::DataType::vec3& to, const GUI::DataType::vec4& color, float thickness = 1, const int id = -1);

			static void DrawTrace(const GUI::DataType::vec2& from, const GUI::DataType::vec2& to, const GUI::DataType::vec4& color, float thickness = 1, const int id = -1);



			static void EndScene();
			static void Flush();
		private:
			static void DrawCap(const GUI::DataType::vec3& start, const GUI::DataType::vec3& end, float thickness, const GUI::DataType::vec4& color, const int id = -1);

			static void QuadVertices(GUI::DataType::vec3 position, float size);
			static void QuadVertices(GUI::DataType::vec3 position, const  GUI::DataType::vec2& size);

			static void StartBatch();
			static void NextBatch();

			static void DrawSelected();
		};

}

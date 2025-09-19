#pragma once
#include <cstdint>
#include <vector>
#include <Renderer/Framebuffer.h>

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

			static void addData(const std::vector<double>& vertices, const std::vector<double>& vertexNormals, const std::vector<uint32_t>& indices, const int id = -1);

			static void DrawMesh(const std::vector<double>& vertices, const std::vector<uint32_t>& indices, const GUI::DataType::vec4& color, const int id = -1);

			static void DrawCircle(const GUI::DataType::vec3& position, float radius, const GUI::DataType::vec4& color, const int id = -1);

			static void DrawCircle(const GUI::DataType::vec2& position, float radius, const GUI::DataType::vec4& color, const int id = -1);

			static void DrawLine(const GUI::DataType::vec3& from, const GUI::DataType::vec3& to, const GUI::DataType::vec4& color, const int id = -1);

			static void DrawLine(const GUI::DataType::vec2& from, const GUI::DataType::vec2& to, const GUI::DataType::vec4& color, const int id = -1);

			static void DrawLine(const GUI::DataType::vec3& from, const GUI::DataType::vec3& to, const GUI::DataType::vec4& color, float thickness, const int id = -1);

			static void DrawLine(const GUI::DataType::vec2& from, const GUI::DataType::vec2& to, const GUI::DataType::vec4& color, float thickness, const int id = -1);

			static void DrawLines(const std::vector<GUI::DataType::vec3>& points, const std::vector<uint32_t>& indices, const GUI::DataType::vec4& color, const int id = -1, bool withArrows = false);


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

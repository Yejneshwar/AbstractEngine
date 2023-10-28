#pragma once
#include <cstdint>
#include <vector>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>



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

			static void addData(const std::vector<double>& vertices, const std::vector<uint32_t>& indices);

			static void DrawMesh(const std::vector<double>& vertices, const std::vector<uint32_t>& indices, const glm::vec4& color);

			static void DrawCircle(const glm::vec2& position, float radius, const glm::vec4& color);

			static void DrawLine(const glm::vec2& from, const glm::vec2& to, const glm::vec4& color);


			static void DrawQuad(const glm::vec2& p1, const glm::vec2& p2, const glm::vec2& p3, const glm::vec2& p4, const glm::vec4& color);

			static void DrawQuad(const glm::vec2& position, float size, const glm::vec4& color);

			static void DrawObround(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color);

			static void DrawCap(const glm::vec2& start, const glm::vec2& end, float thickness, const glm::vec4& color);

			static void DrawTrace(const glm::vec2& from, const glm::vec2& to, const glm::vec4& color, float thickness = 1);

			static void DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color);


			static void EndScene();
			static void Flush();
		private:
			static void QuadVertices(glm::vec3 position, float size);
			static void QuadVertices(glm::vec3 position, const  glm::vec2& size);

			static void StartBatch();
			static void NextBatch();
		};

}

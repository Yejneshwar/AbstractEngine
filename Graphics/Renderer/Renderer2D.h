#pragma once

#include "Renderer/OrthographicCamera.h"

#include "Renderer/Texture.h"

#include "Renderer/Camera.h"

//#include "Hazel/Scene/Components.h"

namespace Graphics {

	class Renderer2D
	{
	private:
		static void DrawQuad(const glm::vec2& position1, const glm::vec2& position2, const glm::vec2& position3, const glm::vec2& position4, const glm::vec4& color);
	public:
		static void Init();
		static void Shutdown();

		static void setRenderMode(int mode);

		static void BeginScene(const Camera& camera, const glm::mat4& transform);
		static void BeginScene(const OrthographicCamera& camera); // TODO: Remove
		static void BeginScene(const OrthographicCamera& camera, bool clear);
		static void EndScene();
		static void Flush();

		static void setUpdateRequired(bool _state);
		static bool getUpdateRequired();

		// Primitives
		static void DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color);
		static void DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color);
		static void DrawQuad(const glm::vec2& position, const glm::vec2& size, const Ref<Texture2D>& texture, float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f));
		static void DrawQuad(const glm::vec3& position, const glm::vec2& size, const Ref<Texture2D>& texture, float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f));

		static void DrawQuad(const glm::mat4& transform, const glm::vec4& color, int entityID = -1);
		static void DrawQuad(const glm::mat4& transform, const Ref<Texture2D>& texture, float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f), int entityID = -1);

		//static void DrawTriangle(const glm::vec2& position0, const glm::vec2& position1, const glm::vec2& position2, const glm::vec4& color, int entityID = -1);

		static void DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const glm::vec4& color);
		static void DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const glm::vec4& color);
		static void DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const Ref<Texture2D>& texture, float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f));
		static void DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const Ref<Texture2D>& texture, float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f));

		static void DrawCircle(const glm::mat4& transform, const glm::vec4& color, float thickness = 1.0f, float fade = 0.005f, int entityID = -1);
		
		static void DrawLineCap(const glm::vec2& start, const glm::vec2& end, float diameter, const glm::vec4& color, int entityID = -1);
		
		static void DrawLine(const glm::vec3& p0, const glm::vec3& p1, const glm::vec4& color, int entityID = -1);

		
		//static void DrawTriangle(const std::vector<glm::vec3> points, const glm::vec4& color, int entityID = -1);

		static void DrawRect(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color, int entityID = -1);
		static void DrawRect(const glm::mat4& transform, const glm::vec4& color, int entityID = -1);

		static float GetLineWidth();
		static void SetLineWidth(float width);

		static class Triangle {
		public:
			static void DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color, int entityID = -1);
			static void DrawObround(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color, int entityID = -1);
			static void DrawCircle(const glm::vec2& position, float diameter, const glm::vec4& color, int entityID = -1);
		};

		// Stats
		struct Statistics
		{
			uint32_t DrawCalls = 0;
			uint32_t QuadCount = 0;
			uint32_t TriangleCount = 0;

			uint32_t GetTotalVertexCount() const { return QuadCount * 4; }
			uint32_t GetTotalIndexCount() const { return QuadCount * 6; }
		};
		static void ResetStats();
		static Statistics GetStats();

		static void DrawTriangle(const glm::vec2& vertex1, const glm::vec2& vertex2, const glm::vec2& vertex3, const glm::vec4& color);

		static void DrawTriangle(const glm::vec3& vertex1, const glm::vec3& vertex2, const glm::vec3& vertex3, const glm::vec4& color);

		static void DrawTriangle(const glm::vec2& vertex1, const glm::vec2& vertex2, const glm::vec2& vertex3, const Ref<Texture2D>& texture, float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f));

		static void DrawTriangle(const glm::vec3& vertex1, const glm::vec3& vertex2, const glm::vec3& vertex3, const Ref<Texture2D>& texture, float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f));

		static void DrawTriangle(const glm::mat4& transform, const glm::vec3& vertex1, const glm::vec3& vertex2, const glm::vec3& vertex3, const glm::vec4& color, int entityID = -1);

		static void DrawLine(const glm::vec2& start, const glm::vec2& end, const float thickness, const glm::vec4& color, int entityID = -1);

	private:
		static void StartBatch();
		static void NextBatch();
	};

}

#include "Renderer/Renderer2D.h"

#include "Renderer/VertexArray.h"
#include "Renderer/Shader.h"
#include "Renderer/UniformBuffer.h"
#include "Renderer/RenderCommand.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/type_ptr.hpp>



namespace Graphics {

	struct QuadVertex
	{
		glm::vec3 Position;
		glm::vec4 Color;
		glm::vec2 TexCoord;
		float TexIndex;
		float TilingFactor;
		
		// Editor-only
		int EntityID;
	};

	struct TriangleVertex
	{
		glm::vec3 Position;
		glm::vec4 Color;
		//glm::vec2 TexCoord;
		//float TexIndex;
		//float TilingFactor;

		// Editor-only
		int EntityID;
	};

	struct CircleVertex
	{
		glm::vec3 WorldPosition;
		glm::vec3 LocalPosition;
		glm::vec4 Color;
		float Thickness;
		float Fade;

		// Editor-only
		int EntityID;
	};

	struct LineVertex
	{
		glm::vec3 Position;
		glm::vec4 Color;

		// Editor-only
		int EntityID;
	};

	struct Renderer2DData
	{
		static const uint32_t MaxQuads = 400000;
		static const uint32_t MaxVertices = MaxQuads * 4;
		static const uint32_t MaxIndices = MaxQuads * 6;
		static const uint32_t MaxTextureSlots = 32; // TODO: RenderCaps

		bool updateData = true;
		int currentRenderMode = 0x1B02; // this is GL_FILL the default opengl polygon mode

		Ref<VertexArray> QuadVertexArray;
		Ref<VertexBuffer> QuadVertexBuffer;
		Ref<Shader> QuadShader;
		Ref<Texture2D> WhiteTexture;

		Ref<VertexArray> TriangleVertexArray;
		Ref<VertexBuffer> TriangleVertexBuffer;
		Ref<Shader> TriangleShader;

		Ref<VertexArray> CircleVertexArray;
		Ref<VertexBuffer> CircleVertexBuffer;
		Ref<Shader> CircleShader;

		Ref<VertexArray> LineVertexArray;
		Ref<VertexBuffer> LineVertexBuffer;
		Ref<Shader> LineShader;

		uint32_t QuadIndexCount = 0;
		QuadVertex* QuadVertexBufferBase = nullptr;
		QuadVertex* QuadVertexBufferPtr = nullptr;

		uint32_t TriangleIndexCount = 0;
		TriangleVertex* TriangleVertexBufferBase = nullptr;
		TriangleVertex* TriangleVertexBufferPtr = nullptr;

		uint32_t CircleIndexCount = 0;
		CircleVertex* CircleVertexBufferBase = nullptr;
		CircleVertex* CircleVertexBufferPtr = nullptr;

		uint32_t LineVertexCount = 0;
		LineVertex* LineVertexBufferBase = nullptr;
		LineVertex* LineVertexBufferPtr = nullptr;

		float LineWidth = 2.0f;

		std::array<Ref<Texture2D>, MaxTextureSlots> TextureSlots;
		uint32_t TextureSlotIndex = 1; // 0 = white texture

		glm::vec4 QuadVertexPositions[4];
		glm::vec3 TriangleVertexPositions[3];

		Renderer2D::Statistics Stats;

		struct CameraData
		{
			glm::mat4 ViewProjection;
		};
		CameraData CameraBuffer;
		Ref<UniformBuffer> CameraUniformBuffer;
	};

	static Renderer2DData s_Data;

	void Renderer2D::Init()
	{
		

		s_Data.QuadVertexArray = VertexArray::Create();

		s_Data.QuadVertexBuffer = VertexBuffer::Create(s_Data.MaxVertices * sizeof(QuadVertex));
		s_Data.QuadVertexBuffer->SetLayout({
			{ ShaderDataType::Float3, "a_Position"     },
			{ ShaderDataType::Float4, "a_Color"        },
			{ ShaderDataType::Float2, "a_TexCoord"     },
			{ ShaderDataType::Float,  "a_TexIndex"     },
			{ ShaderDataType::Float,  "a_TilingFactor" },
			{ ShaderDataType::Int,    "a_EntityID"     }
		});
		s_Data.QuadVertexArray->AddVertexBuffer(s_Data.QuadVertexBuffer);

		s_Data.QuadVertexBufferBase = new QuadVertex[s_Data.MaxVertices];

		uint32_t* quadIndices = new uint32_t[s_Data.MaxIndices];

		uint32_t offset = 0;
		for (uint32_t i = 0; i < s_Data.MaxIndices; i += 6)
		{
			quadIndices[i + 0] = offset + 0;
			quadIndices[i + 1] = offset + 1;
			quadIndices[i + 2] = offset + 2;

			quadIndices[i + 3] = offset + 2;
			quadIndices[i + 4] = offset + 3;
			quadIndices[i + 5] = offset + 0;

			offset += 4;
		}

		Ref<IndexBuffer> quadIB = IndexBuffer::Create(quadIndices, s_Data.MaxIndices);
		s_Data.QuadVertexArray->SetIndexBuffer(quadIB);
		delete[] quadIndices;

		
		//Triangles
		s_Data.TriangleVertexArray = VertexArray::Create();

		s_Data.TriangleVertexBuffer = VertexBuffer::Create(s_Data.MaxVertices * sizeof(TriangleVertex));
		s_Data.TriangleVertexBuffer->SetLayout({
			{ ShaderDataType::Float3, "a_Position"     },
			{ ShaderDataType::Float4, "a_Color"        },
			//{ ShaderDataType::Float2, "a_TexCoord"     },
			//{ ShaderDataType::Float,  "a_TexIndex"     },
			//{ ShaderDataType::Float,  "a_TilingFactor" },
			{ ShaderDataType::Int,    "a_EntityID"     }
			});
		s_Data.TriangleVertexArray->AddVertexBuffer(s_Data.TriangleVertexBuffer);

		s_Data.TriangleVertexBufferBase = new TriangleVertex[s_Data.MaxVertices];

		uint32_t* triangleIndices = new uint32_t[s_Data.MaxIndices];

		uint32_t triangleOffset = 0;
		for (uint32_t i = 0; i < s_Data.MaxIndices; i += 3)
		{
			triangleIndices[i + 0] = triangleOffset + 0;
			triangleIndices[i + 1] = triangleOffset + 1;
			triangleIndices[i + 2] = triangleOffset + 2;
			
			triangleOffset += 3;
		}

		Ref<IndexBuffer> triangleIB = IndexBuffer::Create(triangleIndices, s_Data.MaxIndices);
		s_Data.TriangleVertexArray->SetIndexBuffer(triangleIB);
		delete[] triangleIndices;

		// Circles
		s_Data.CircleVertexArray = VertexArray::Create();

		s_Data.CircleVertexBuffer = VertexBuffer::Create(s_Data.MaxVertices * sizeof(CircleVertex));
		s_Data.CircleVertexBuffer->SetLayout({
			{ ShaderDataType::Float3, "a_WorldPosition" },
			{ ShaderDataType::Float3, "a_LocalPosition" },
			{ ShaderDataType::Float4, "a_Color"         },
			{ ShaderDataType::Float,  "a_Thickness"     },
			{ ShaderDataType::Float,  "a_Fade"          },
			{ ShaderDataType::Int,    "a_EntityID"      }
		});
		s_Data.CircleVertexArray->AddVertexBuffer(s_Data.CircleVertexBuffer);
		s_Data.CircleVertexArray->SetIndexBuffer(quadIB); // Use quad IB
		s_Data.CircleVertexBufferBase = new CircleVertex[s_Data.MaxVertices];

		// Lines
		s_Data.LineVertexArray = VertexArray::Create();

		s_Data.LineVertexBuffer = VertexBuffer::Create(s_Data.MaxVertices * sizeof(LineVertex));
		s_Data.LineVertexBuffer->SetLayout({
			{ ShaderDataType::Float3, "a_Position" },
			{ ShaderDataType::Float4, "a_Color"    },
			{ ShaderDataType::Int,    "a_EntityID" }
		});
		s_Data.LineVertexArray->AddVertexBuffer(s_Data.LineVertexBuffer);
		s_Data.LineVertexBufferBase = new LineVertex[s_Data.MaxVertices];

		//s_Data.WhiteTexture = Texture2D::Create(1, 1);
		//uint32_t whiteTextureData = 0xffffffff;
		//s_Data.WhiteTexture->SetData(&whiteTextureData, sizeof(uint32_t));

		//int32_t samplers[s_Data.MaxTextureSlots];
		//for (uint32_t i = 0; i < s_Data.MaxTextureSlots; i++)
		//	samplers[i] = i;

		//s_Data.QuadShader = Shader::Create("Resources/Shaders/BasicShader.glsl");

		s_Data.QuadShader = Shader::Create("Resources/Shaders/Renderer2D_Quad.glsl");
		s_Data.TriangleShader = Shader::Create("resources/Shaders/Renderer2D_Line.glsl");
		s_Data.CircleShader = Shader::Create("resources/Shaders/Renderer2D_Circle.glsl");
		s_Data.LineShader = Shader::Create("resources/Shaders/Renderer2D_Line.glsl");

		// Set first texture slot to 0
		s_Data.TextureSlots[0] = s_Data.WhiteTexture;

		s_Data.QuadVertexPositions[0] = { -0.5f, -0.5f, 0.0f, 1.0f };
		s_Data.QuadVertexPositions[1] = {  0.5f, -0.5f, 0.0f, 1.0f };
		s_Data.QuadVertexPositions[2] = {  0.5f,  0.5f, 0.0f, 1.0f };
		s_Data.QuadVertexPositions[3] = { -0.5f,  0.5f, 0.0f, 1.0f };

		s_Data.TriangleVertexPositions[0] = { -0.5f, -0.5f, 0.0f};
		s_Data.TriangleVertexPositions[1] = { 0.5f, -0.5f, 0.0f};
		s_Data.TriangleVertexPositions[2] = { 0.0f,  0.5f, 0.0f};

		s_Data.CameraUniformBuffer = UniformBuffer::Create(sizeof(Renderer2DData::CameraData), 3);
	}

	void Renderer2D::Shutdown()
	{

		delete[] s_Data.QuadVertexBufferBase;
		delete[] s_Data.TriangleVertexBufferBase;

	}

	void Renderer2D::setRenderMode(int mode)
	{
		s_Data.currentRenderMode = mode;
		RenderCommand::SetRenderMode(mode);
	}

	void Renderer2D::BeginScene(const OrthographicCamera& camera)
	{
		
		if (s_Data.currentRenderMode != 0x1B02) {
			RenderCommand::SetRenderMode(s_Data.currentRenderMode);
		}
		s_Data.CameraBuffer.ViewProjection = camera.GetViewProjectionMatrix();
		s_Data.CameraUniformBuffer->SetData(&s_Data.CameraBuffer, sizeof(Renderer2DData::CameraData));
		if(s_Data.updateData)
			StartBatch();
	}

	void Renderer2D::BeginScene(const OrthographicCamera& camera, bool clear)
	{


		s_Data.CameraBuffer.ViewProjection = camera.GetViewProjectionMatrix();
		s_Data.CameraUniformBuffer->SetData(&s_Data.CameraBuffer, sizeof(Renderer2DData::CameraData));

		if(clear)
			StartBatch();
	}

	void Renderer2D::BeginScene(const Camera& camera, const glm::mat4& transform)
	{
		

		s_Data.CameraBuffer.ViewProjection = camera.GetProjection() * glm::inverse(transform);
		s_Data.CameraUniformBuffer->SetData(&s_Data.CameraBuffer, sizeof(Renderer2DData::CameraData));

		StartBatch();
	}

	void Renderer2D::EndScene()
	{
		

		Flush();
	}

	void Renderer2D::StartBatch()
	{
		s_Data.QuadIndexCount = 0;
		s_Data.QuadVertexBufferPtr = s_Data.QuadVertexBufferBase;

		s_Data.TriangleIndexCount = 0;
		s_Data.TriangleVertexBufferPtr = s_Data.TriangleVertexBufferBase;

		s_Data.CircleIndexCount = 0;
		s_Data.CircleVertexBufferPtr = s_Data.CircleVertexBufferBase;

		s_Data.LineVertexCount = 0;
		s_Data.LineVertexBufferPtr = s_Data.LineVertexBufferBase;

		s_Data.TextureSlotIndex = 1;
	}

	void Renderer2D::Flush()
	{
		if (s_Data.QuadIndexCount)
		{
			uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.QuadVertexBufferPtr - (uint8_t*)s_Data.QuadVertexBufferBase);
			s_Data.QuadVertexBuffer->SetData(s_Data.QuadVertexBufferBase, dataSize);

			// Bind textures
			for (uint32_t i = 0; i < s_Data.TextureSlotIndex; i++)
				s_Data.TextureSlots[i]->Bind(i);

			s_Data.QuadShader->Bind();
			RenderCommand::DrawIndexed(s_Data.QuadVertexArray, s_Data.QuadIndexCount);
			s_Data.Stats.DrawCalls++;
		}
		
		if (s_Data.TriangleIndexCount)
		{
			uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.TriangleVertexBufferPtr - (uint8_t*)s_Data.TriangleVertexBufferBase);
			s_Data.TriangleVertexBuffer->SetData(s_Data.TriangleVertexBufferBase, dataSize);

			// Bind textures
			for (uint32_t i = 0; i < s_Data.TextureSlotIndex; i++)
				s_Data.TextureSlots[i]->Bind(i);

			s_Data.TriangleShader->Bind();
			RenderCommand::DrawIndexed(s_Data.TriangleVertexArray, s_Data.TriangleIndexCount);
			s_Data.Stats.DrawCalls++;
		}

		if (s_Data.CircleIndexCount)
		{
			uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.CircleVertexBufferPtr - (uint8_t*)s_Data.CircleVertexBufferBase);
			s_Data.CircleVertexBuffer->SetData(s_Data.CircleVertexBufferBase, dataSize);

			s_Data.CircleShader->Bind();
			RenderCommand::DrawIndexed(s_Data.CircleVertexArray, s_Data.CircleIndexCount);
			s_Data.Stats.DrawCalls++;
		}

		if (s_Data.LineVertexCount)
		{
			uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.LineVertexBufferPtr - (uint8_t*)s_Data.LineVertexBufferBase);
			s_Data.LineVertexBuffer->SetData(s_Data.LineVertexBufferBase, dataSize);

			s_Data.LineShader->Bind();
			RenderCommand::SetLineWidth(s_Data.LineWidth);
			RenderCommand::DrawLines(s_Data.LineVertexArray, s_Data.LineVertexCount);
			s_Data.Stats.DrawCalls++;
		}

		if (s_Data.currentRenderMode != 0x1B02) {
			RenderCommand::SetRenderModeToDefault();
		}
	}

	void Renderer2D::setUpdateRequired(bool _state)
	{	
		s_Data.updateData = _state;
	}

	bool Renderer2D::getUpdateRequired()
	{
		return s_Data.updateData;
	}

	void Renderer2D::NextBatch()
	{
		Flush();
		StartBatch();
	}

	void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color)
	{
		DrawQuad({ position.x, position.y, 0.0f }, size, color);
	}

	void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color)
	{
		

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });
		
		DrawQuad(transform, color);
	}

	void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor)
	{
		DrawQuad({ position.x, position.y, 0.0f }, size, texture, tilingFactor, tintColor);
	}

	void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor)
	{
		

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

		DrawQuad(transform, texture, tilingFactor, tintColor);
	}

	void Renderer2D::DrawQuad(const glm::vec2& position1, const glm::vec2& position2, const glm::vec2& position3, const glm::vec2& position4, const glm::vec4& color)
	{
		glm::vec2 size = glm::vec2(glm::length(position2 - position1), glm::length(position4 - position1));
		glm::vec2 center = (position1 + position2 + position3 + position4) * 0.25f;

		glm::vec2 xAxis = glm::normalize(position2 - position1);
		float angle = glm::atan(xAxis.y, xAxis.x);
		glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(center, 0.0f))
			* glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0, 0, 1))
			* glm::scale(glm::mat4(1.0f), glm::vec3(size, 1.0f));

		DrawQuad(transform, color);
	}

	void Renderer2D::DrawQuad(const glm::mat4& transform, const glm::vec4& color, int entityID)
	{
		

		constexpr size_t quadVertexCount = 4;
		const float textureIndex = 0.0f; // White Texture
		 glm::vec2 textureCoords[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };
		const float tilingFactor = 1.0f;

		if (s_Data.QuadIndexCount >= Renderer2DData::MaxIndices)
			NextBatch();

		for (size_t i = 0; i < quadVertexCount; i++)
		{
			s_Data.QuadVertexBufferPtr->Position = transform * s_Data.QuadVertexPositions[i];
			s_Data.QuadVertexBufferPtr->Color = color;
			s_Data.QuadVertexBufferPtr->TexCoord = textureCoords[i];
			s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
			s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
			s_Data.QuadVertexBufferPtr->EntityID = entityID;
			s_Data.QuadVertexBufferPtr++;
		}

		s_Data.QuadIndexCount += 6;

		s_Data.Stats.QuadCount++;
	}

	void Renderer2D::DrawQuad(const glm::mat4& transform, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor, int entityID)
	{
		

		constexpr size_t quadVertexCount = 4;
		glm::vec2 textureCoords[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };

		if (s_Data.QuadIndexCount >= Renderer2DData::MaxIndices)
			NextBatch();

		float textureIndex = 0.0f;
		for (uint32_t i = 1; i < s_Data.TextureSlotIndex; i++)
		{
			if (*s_Data.TextureSlots[i] == *texture)
			{
				textureIndex = (float)i;
				break;
			}
		}

		if (textureIndex == 0.0f)
		{
			if (s_Data.TextureSlotIndex >= Renderer2DData::MaxTextureSlots)
				NextBatch();

			textureIndex = (float)s_Data.TextureSlotIndex;
			s_Data.TextureSlots[s_Data.TextureSlotIndex] = texture;
			s_Data.TextureSlotIndex++;
		}

		for (size_t i = 0; i < quadVertexCount; i++)
		{
			s_Data.QuadVertexBufferPtr->Position = transform * s_Data.QuadVertexPositions[i];
			s_Data.QuadVertexBufferPtr->Color = tintColor;
			s_Data.QuadVertexBufferPtr->TexCoord = textureCoords[i];
			s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
			s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
			s_Data.QuadVertexBufferPtr->EntityID = entityID;
			s_Data.QuadVertexBufferPtr++;
		}

		s_Data.QuadIndexCount += 6;

		s_Data.Stats.QuadCount++;
	}

	


	void Renderer2D::DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const glm::vec4& color)
	{
		DrawRotatedQuad({ position.x, position.y, 0.0f }, size, rotation, color);
	}

	void Renderer2D::DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const glm::vec4& color)
	{
		

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::rotate(glm::mat4(1.0f), glm::radians(rotation), { 0.0f, 0.0f, 1.0f })
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

		DrawQuad(transform, color);
	}

	void Renderer2D::DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor)
	{
		DrawRotatedQuad({ position.x, position.y, 0.0f }, size, rotation, texture, tilingFactor, tintColor);
	}

	void Renderer2D::DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor)
	{
		

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::rotate(glm::mat4(1.0f), glm::radians(rotation), { 0.0f, 0.0f, 1.0f })
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

		DrawQuad(transform, texture, tilingFactor, tintColor);
	}

	void Renderer2D::DrawTriangle(const glm::vec2& vertex1, const glm::vec2& vertex2, const glm::vec2& vertex3, const glm::vec4& color)
	{
		DrawTriangle({ vertex1.x, vertex1.y, 0.0f }, { vertex2.x, vertex2.y, 0.0f }, { vertex3.x, vertex3.y, 0.0f }, color);
	}

	void Renderer2D::DrawTriangle(const glm::vec3& vertex1, const glm::vec3& vertex2, const glm::vec3& vertex3, const glm::vec4& color)
	{
		glm::mat4 transform(1.0f);
		DrawTriangle(transform, vertex1, vertex2, vertex3, color);
	}

	//void Renderer2D::DrawTriangle(const glm::vec2& vertex1, const glm::vec2& vertex2, const glm::vec2& vertex3, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor)
	//{
	//	DrawTriangle({ vertex1.x, vertex1.y, 0.0f }, { vertex2.x, vertex2.y, 0.0f }, { vertex3.x, vertex3.y, 0.0f }, texture, tilingFactor, tintColor);
	//}

	//void Renderer2D::DrawTriangle(const glm::vec3& vertex1, const glm::vec3& vertex2, const glm::vec3& vertex3, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor)
	//{
	//	glm::mat4 transform(1.0f);
	//	DrawTriangle(transform, vertex1, vertex2, vertex3, texture, tilingFactor, tintColor);
	//}

	void Renderer2D::DrawTriangle(const glm::mat4& transform, const glm::vec3& vertex1, const glm::vec3& vertex2, const glm::vec3& vertex3, const glm::vec4& color, int entityID)
	{
		constexpr size_t triangleVertexCount = 3;
		const float textureIndex = 0.0f;
		glm::vec2 textureCoords[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 0.5f, 1.0f } };
		const float tilingFactor = 1.0f;

		if (s_Data.TriangleIndexCount >= Renderer2DData::MaxIndices)
			NextBatch();

		for (size_t i = 0; i < triangleVertexCount; i++)
		{
			s_Data.TriangleVertexBufferPtr->Position = transform * glm::vec4(i == 0 ? vertex1 : i == 1 ? vertex2 : vertex3, 1.0f);
			s_Data.TriangleVertexBufferPtr->Color = color;
			//s_Data.TriangleVertexBufferPtr->TexCoord = textureCoords[i];
			//s_Data.TriangleVertexBufferPtr->TexIndex = textureIndex;
			//s_Data.TriangleVertexBufferPtr->TilingFactor = tilingFactor;
			s_Data.TriangleVertexBufferPtr->EntityID = entityID;
			s_Data.TriangleVertexBufferPtr++;
		}

		s_Data.TriangleIndexCount += 3;

		s_Data.Stats.TriangleCount++;
	}

	//void Renderer2D::DrawTriangle(const glm::vec2& position0, const glm::vec2& position1, const glm::vec2& position2, const glm::vec4& color, int entityID) {
	//	if (s_Data.TriangleIndexCount >= Renderer2DData::MaxIndices)
	//		NextBatch();

	//	float textureIndex = 0.0f; // white texture
	//	float tilingFactor = 1.0f;

	//	s_Data.TriangleVertexBufferPtr->Position = { position0.x, position0.y, 0.0f };
	//	s_Data.TriangleVertexBufferPtr->Color = color;
	//	s_Data.TriangleVertexBufferPtr->EntityID = entityID;
	//	s_Data.TriangleVertexBufferPtr++;

	//	s_Data.TriangleVertexBufferPtr->Position = { position1.x, position1.y, 0.0f };
	//	s_Data.TriangleVertexBufferPtr->Color = color;
	//	s_Data.TriangleVertexBufferPtr->EntityID = entityID;
	//	s_Data.TriangleVertexBufferPtr++;

	//	s_Data.TriangleVertexBufferPtr->Position = { position2.x, position2.y, 0.0f };
	//	s_Data.TriangleVertexBufferPtr->Color = color;
	//	s_Data.TriangleVertexBufferPtr->EntityID = entityID;
	//	s_Data.TriangleVertexBufferPtr++;

	//	s_Data.TriangleIndexCount += 3;
	//	s_Data.Stats.TriangleCount++;
	//}
	
	//void Renderer2D::DrawTriangle(const std::vector<glm::vec3> points, const glm::vec4& color, int entityID) {
	//	for (size_t i = 0; i < 3; i++){
	//		DrawLine(points[0], points[1], color);
	//		DrawLine(points[1], points[2], color);
	//		DrawLine(points[2], points[0], color);
	//	}
	//}
	void Renderer2D::DrawCircle(const glm::mat4& transform, const glm::vec4& color, float thickness /*= 1.0f*/, float fade /*= 0.005f*/, int entityID /*= -1*/)
	{
		

		// TODO: implement for circles
		// if (s_Data.QuadIndexCount >= Renderer2DData::MaxIndices)
		// 	NextBatch();

		for (size_t i = 0; i < 4; i++)
		{
			s_Data.CircleVertexBufferPtr->WorldPosition = transform * s_Data.QuadVertexPositions[i];
			s_Data.CircleVertexBufferPtr->LocalPosition = s_Data.QuadVertexPositions[i] * 2.0f;
			s_Data.CircleVertexBufferPtr->Color = color;
			s_Data.CircleVertexBufferPtr->Thickness = thickness;
			s_Data.CircleVertexBufferPtr->Fade = fade;
			s_Data.CircleVertexBufferPtr->EntityID = entityID;
			s_Data.CircleVertexBufferPtr++;
		}

		s_Data.CircleIndexCount += 6;

		s_Data.Stats.QuadCount++;
	}


	void Renderer2D::DrawRect(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color, int entityID)
	{
		glm::vec3 p0 = glm::vec3(position.x - size.x * 0.5f, position.y - size.y * 0.5f, position.z);
		glm::vec3 p1 = glm::vec3(position.x + size.x * 0.5f, position.y - size.y * 0.5f, position.z);
		glm::vec3 p2 = glm::vec3(position.x + size.x * 0.5f, position.y + size.y * 0.5f, position.z);
		glm::vec3 p3 = glm::vec3(position.x - size.x * 0.5f, position.y + size.y * 0.5f, position.z);

		DrawLine(p0, p1, color, entityID);
		DrawLine(p1, p2, color, entityID);
		DrawLine(p2, p3, color, entityID);
		DrawLine(p3, p0, color, entityID);
	}

	void Renderer2D::DrawRect(const glm::mat4& transform, const glm::vec4& color, int entityID)
	{
		glm::vec3 lineVertices[4];
		for (size_t i = 0; i < 4; i++)
			lineVertices[i] = transform * s_Data.QuadVertexPositions[i];

		DrawLine(lineVertices[0], lineVertices[1], color, entityID);
		DrawLine(lineVertices[1], lineVertices[2], color, entityID);
		DrawLine(lineVertices[2], lineVertices[3], color, entityID);
		DrawLine(lineVertices[3], lineVertices[0], color, entityID);
	}

	//void Renderer2D::DrawLineCap(const glm::vec2& start, const glm::vec2& end, float diameter, const glm::vec4& color, int entityID)
	//{
	//	// Calculate the direction and length of the line segment
	//	glm::vec2 direction = glm::normalize(end - start);
	//	float length = glm::distance(start, end);

	//	// Calculate the center of the cap
	//	glm::vec2 capCenter = end - (diameter / 2.0f) * direction;

	//	// Calculate the points of the cap
	//	glm::vec2 capTip = capCenter + (diameter / 2.0f) * direction;
	//	glm::vec2 capSide1 = capCenter + (diameter / 2.0f) * glm::rotate(direction, glm::radians(90.0f));
	//	glm::vec2 capSide2 = capCenter + (diameter / 2.0f) * glm::rotate(direction, glm::radians(-90.0f));

	//	// Draw the cap as a triangle fan
	//	const int numSegments = 16;
	//	const float angleStep = glm::radians(360.0f / numSegments);
	//	glm::vec2 prevPoint = capTip;
	//	for (int i = 0; i <= numSegments; i++)
	//	{
	//		float angle = angleStep * i;
	//		glm::vec2 point = capCenter + (diameter / 2.0f) * glm::vec2(std::cos(angle), std::sin(angle));
	//		DrawTriangle(capCenter, prevPoint, point, color);
	//		prevPoint = point;
	//	}
	//}

	void Renderer2D::DrawLineCap(const glm::vec2& start, const glm::vec2& end, float diameter, const glm::vec4& color, int entityID)
	{
		glm::vec2 direction = glm::normalize(end - start);
		glm::vec2 normal = glm::vec2(direction.y, -direction.x);

		int segments = 12; // Number of segments in the semicircle
		float radius = diameter * 0.5f;
		float angleIncrement = glm::pi<float>() / static_cast<float>(segments);


		glm::vec2 startCenter = start;
		glm::vec2 endCenter = end;

		//this will include the cap as the end
		//glm::vec2 endCenter = end - (direction * radius);

		// Generate the vertices of the semicircle
		std::vector<glm::vec2> startVertices;
		std::vector<glm::vec2> endVertices;
		startVertices.reserve(segments);
		endVertices.reserve(segments);

		for (int i = 0; i <= segments; i++)
		{
			float angle = angleIncrement * static_cast<float>(i);
			glm::vec2 startVertex = startCenter - (glm::rotate(normal, angle) * radius);
			startVertices.push_back(startVertex);
			glm::vec2 endVertex = endCenter + (glm::rotate(normal, angle) * radius);
			endVertices.push_back(endVertex);
		}

		// Draw the triangles that make up the semicircle
		for (int i = 0; i < segments; i++)
		{
			DrawTriangle(startCenter, startVertices[i], startVertices[i + 1], color);
			DrawTriangle(endCenter, endVertices[i], endVertices[i + 1], color);
		}
	}


	void Renderer2D::DrawLine(const glm::vec3& p0, const glm::vec3& p1, const glm::vec4& color, int entityID)
	{
		//if (s_Data.LineVertexCount + 2 > s_Data.LineVertexBufferCapacity)
		//{
		//	// Allocate a new vertex buffer with double the current capacity
		//	size_t newCapacity = s_Data.LineVertexBufferCapacity * 2;
		//	Vertex* newVertexBuffer = new Vertex[newCapacity];
		//	memcpy(newVertexBuffer, s_Data.LineVertexBuffer, s_Data.LineVertexCount * sizeof(Vertex));
		//	delete[] s_Data.LineVertexBuffer;
		//	s_Data.LineVertexBuffer = newVertexBuffer;
		//	s_Data.LineVertexBufferEnd = s_Data.LineVertexBuffer + newCapacity;
		//	s_Data.LineVertexBufferCapacity = newCapacity;
		//	s_Data.LineVertexBufferPtr = s_Data.LineVertexBuffer + s_Data.LineVertexCount;
		//}

		s_Data.LineVertexBufferPtr->Position = p0;
		s_Data.LineVertexBufferPtr->Color = color;
		s_Data.LineVertexBufferPtr->EntityID = entityID;
		s_Data.LineVertexBufferPtr++;

		s_Data.LineVertexBufferPtr->Position = p1;
		s_Data.LineVertexBufferPtr->Color = color;
		s_Data.LineVertexBufferPtr->EntityID = entityID;
		s_Data.LineVertexBufferPtr++;

		s_Data.LineVertexCount += 2;
	}
	
	void Renderer2D::DrawLine(const glm::vec2& start, const glm::vec2& end, const float thickness, const glm::vec4& color, int entityID)
	{

		// Calculate the direction vector of the line
		glm::vec2 dir = end - start;

		// Calculate the normal vector of the line
		glm::vec2 normal = glm::normalize(glm::vec2(-dir.y, dir.x));

		// Calculate the half-width of the line
		float halfWidth = thickness / 2.0f;

		// Calculate the four corners of the line rectangle
		glm::vec2 p1 = start + normal * halfWidth;
		glm::vec2 p2 = start - normal * halfWidth;
		glm::vec2 p3 = end - normal * halfWidth;
		glm::vec2 p4 = end + normal * halfWidth;

		DrawQuad(p1, p2, p3, p4, color);
		DrawLineCap(start, end, thickness, color);
	}


	float Renderer2D::GetLineWidth()
	{
		return s_Data.LineWidth;
	}

	void Renderer2D::SetLineWidth(float width)
	{
		s_Data.LineWidth = width;
	}

	void Renderer2D::ResetStats()
	{
		if (s_Data.updateData)
			memset(&s_Data.Stats, 0, sizeof(Statistics));
		else s_Data.Stats.DrawCalls = 0;
	}

	Renderer2D::Statistics Renderer2D::GetStats()
	{
		return s_Data.Stats;
	}

	void Renderer2D::Triangle::DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color, int entityID)
	{
		//Draw Quad using triangles
		// the below is aligned to bottom left
		////Top Left
		//DrawTriangle(position, position + glm::vec2(size.x, 0.0f), position + size, color);
		////Bottom Right
		//DrawTriangle(position, position + size, position + glm::vec2(0.0f, size.y), color);

		//use position as center
		glm::vec2 halfSize = size * 0.5f;
		glm::vec2 topLeft = position - halfSize;
		glm::vec2 topRight = position + glm::vec2(halfSize.x, -halfSize.y);
		glm::vec2 bottomLeft = position + glm::vec2(-halfSize.x, halfSize.y);
		glm::vec2 bottomRight = position + halfSize;
		
		DrawTriangle(topLeft, topRight, bottomLeft, color);
		DrawTriangle(topRight, bottomRight, bottomLeft, color);
	}

	void Renderer2D::Triangle::DrawCircle(const glm::vec2& position, float diameter, const glm::vec4& color, int entityID)
	{
		const int numSegments = 32;
		const float segmentAngle = 2.0f * glm::pi<float>() / numSegments;
		const float radius = diameter / 2.0f;

		for (int i = 0; i < numSegments; i++)
		{
			const glm::vec2 vertex1 = position + radius * glm::vec2(std::cos(i * segmentAngle), std::sin(i * segmentAngle));
			const glm::vec2 vertex2 = position + radius * glm::vec2(std::cos((i + 1) * segmentAngle), std::sin((i + 1) * segmentAngle));
			const glm::vec2 vertex3 = position;

			DrawTriangle(vertex1, vertex2, vertex3, color);
		}
	}

	void Renderer2D::Triangle::DrawObround(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color, int entityID)
	{
		//If the obround is a circle
		if (size.x == size.y) {
			DrawCircle(position, size.x, color);
			return;
		}
		//Position is the center of the obround
		//size is the width and height of the obround
		double obroundRadius = size.x / 2;
		double halfHeightWithoutCap = (size.y - (obroundRadius * 2))/2;
		glm::vec2 start = { position.x, position.y + halfHeightWithoutCap};
		glm::vec2 end = { position.x, position.y - halfHeightWithoutCap};


		DrawLine(start,end, size.x, color);
	}

}

#include "BatchRenderer.h"
#include <Renderer/Renderer.h>
#include <Renderer/Shader.h>
#include <Renderer/VertexArray.h>
#include <Renderer/UniformBuffer.h>
#include <Renderer/Texture.h>

#include <Logger.h>

namespace Graphics {
		struct UBODataFragment {
            GUI::DataType::vec4 triangleColor;
		};
		
		struct StaticTriangleVertex
		{
			GUI::DataType::int1 aID;
            GUI::DataType::vec3 Position;
            GUI::DataType::vec3 Normal;
            GUI::DataType::vec4 Color;
		};

		struct TriangleVertex
		{
            GUI::DataType::int1 aID;
            GUI::DataType::vec3 Position;
            GUI::DataType::vec4 Color;
		};
		
		struct QuadVertex
		{
            GUI::DataType::int1 aID;
            GUI::DataType::vec3 Position;
            GUI::DataType::vec4 Color;
		};
		
		struct CircleVertex
		{
            GUI::DataType::int1 aID;
            GUI::DataType::vec3 Position;
            GUI::DataType::vec3 CirclePosition;
            GUI::DataType::vec3 Normal;
            GUI::DataType::vec4 Color;
            GUI::DataType::float1 Radius;
		};
		
		struct LineVertex
		{
            GUI::DataType::int1 aID;
            GUI::DataType::vec3 Position;
            GUI::DataType::vec4 Color;
		};

		struct DrawList {
			std::vector<double> vertices;
			std::vector<double> normals;
			std::vector<uint32_t> indices;
			uint32_t indicesOffset = 0;
			bool updateBatch = false;
		};
		
		struct Renderer2DData
		{
			bool inScene = false;
			static const uint32_t MaxQuads = 8000;
			static const uint32_t MaxVertices = MaxQuads * 4;
			static const uint32_t MaxIndices = MaxQuads * 6;
			static const uint32_t MaxTextureSlots = 32; // TODO: RenderCaps
		
			bool updateData = true;
			int currentRenderMode = 0x1B02; // this is GL_FILL the default opengl polygon mode
		
			Graphics::Ref<Graphics::VertexArray> QuadVertexArray;
			Graphics::Ref<Graphics::VertexBuffer> QuadVertexBuffer;
			Graphics::Ref<Graphics::Shader> QuadShader;
			//Graphics::Ref<Graphics::Texture2D> WhiteTexture;
		
			Graphics::Ref<Graphics::VertexArray> StaticTriangleVertexArray;
			Graphics::Ref<Graphics::VertexBuffer> StaticTriangleVertexBuffer;
			Graphics::Ref<Graphics::IndexBuffer> StaticTriangleIndexBuffer;
			Graphics::Ref<Graphics::Shader> StaticTriangleShader;


			Graphics::Ref<Graphics::VertexArray> TriangleVertexArray;
			Graphics::Ref<Graphics::VertexBuffer> TriangleVertexBuffer;
			Graphics::Ref<Graphics::IndexBuffer> TriangleIndexBuffer;
			Graphics::Ref<Graphics::Shader> TriangleShader;


			Graphics::Ref<Graphics::VertexArray> CircleVertexArray;
			Graphics::Ref<Graphics::VertexBuffer> CircleVertexBuffer;
			Graphics::Ref<Graphics::Shader> CircleShader;
		
			Graphics::Ref<Graphics::VertexArray> LineVertexArray;
			Graphics::Ref<Graphics::VertexBuffer> LineVertexBuffer;

			Graphics::Ref<Graphics::VertexArray> IndexedLineVertexArray;
			Graphics::Ref<Graphics::VertexBuffer> IndexedLineVertexBuffer;
			Graphics::Ref<Graphics::IndexBuffer> IndexedLineIndexBuffer;
			Graphics::Ref<Graphics::Shader> LineShader;

			Graphics::Ref<Graphics::Shader> SelectedObjectShader;
	
			uint32_t QuadIndexCount = 0;
			QuadVertex* QuadVertexBufferBase = nullptr;
			QuadVertex* QuadVertexBufferPtr = nullptr;
		
			uint32_t StaticTriangleIndexCount = 0;
			StaticTriangleVertex* StaticTriangleVertexBufferBase = nullptr;
			StaticTriangleVertex* StaticTriangleVertexBufferPtr = nullptr;
			uint32_t StaticTriangleVertexBufferOffset = 0;

			uint32_t TriangleIndexCount = 0;
			TriangleVertex* TriangleVertexBufferBase = nullptr;
			TriangleVertex* TriangleVertexBufferPtr = nullptr;
			uint32_t* TriangleIndexBufferBase = nullptr;
			uint32_t* TriangleIndexBufferPtr = nullptr;
			uint32_t TriangleVertexBufferOffset = 0;
		
			uint32_t CircleIndexCount = 0;
			CircleVertex* CircleVertexBufferBase = nullptr;
			CircleVertex* CircleVertexBufferPtr = nullptr;
		
			uint32_t LineVertexCount = 0;
			LineVertex* LineVertexBufferBase = nullptr;
			LineVertex* LineVertexBufferPtr = nullptr;

			uint32_t IndexedLineIndexCount = 0;
			LineVertex* IndexedLineVertexBufferBase = nullptr;
			LineVertex* IndexedLineVertexBufferPtr = nullptr;
			uint32_t* IndexedLineIndexBufferBase = nullptr;
			uint32_t* IndexedLineIndexBufferPtr = nullptr;
			uint32_t IndexedLineVertexBufferOffset = 0;

		
			float LineWidth = 2.0f;
		
			//std::array<Graphics::Ref<Graphics::Texture2D>, MaxTextureSlots> TextureSlots;
			//uint32_t TextureSlotIndex = 1; // 0 = white texture
		
            GUI::DataType::vec4 QuadVertexPositions[4];
            GUI::DataType::vec3 TriangleVertexPositions[3];

			Statistics Stats;
		
			Graphics::Ref<Graphics::UniformBuffer> FragmentBuffer;

			std::array<GUI::DataType::vec3,4> quadVertices;

			DrawList storage;
		};
		
		static Renderer2DData s_Data;

		//Get quad vertices with position at center
		void BatchRenderer::QuadVertices(GUI::DataType::vec3 position, float size)
		{
            s_Data.quadVertices[0] = { position.x() - size/2, position.y() - size/2, position.z() };
            s_Data.quadVertices[1] = { position.x() + size/2, position.y() - size/2, position.z() };
            s_Data.quadVertices[2] = { position.x() + size/2, position.y() + size/2, position.z() };
            s_Data.quadVertices[3] = { position.x() - size/2, position.y() + size/2, position.z() };
		}

		//Get quad vertices with position at center
		void BatchRenderer::QuadVertices(GUI::DataType::vec3 position, const GUI::DataType::vec2& size)
		{
            s_Data.quadVertices[0] = { position.x() - size.x()/2, position.y() - size.y()/2, position.z() };
            s_Data.quadVertices[1] = { position.x() + size.x()/2, position.y() - size.y()/2, position.z() };
            s_Data.quadVertices[2] = { position.x() + size.x()/2, position.y() + size.y()/2, position.z() };
            s_Data.quadVertices[3] = { position.x() - size.x()/2, position.y() + size.y()/2, position.z() };
		}

		Statistics BatchRenderer::GetStats() {
			return s_Data.Stats;
		}

		inline void CreateShaders() {
			s_Data.StaticTriangleShader = Graphics::Shader::Create("./Resource/Shaders/BasicShader.glsl", false);
			s_Data.TriangleShader = Graphics::Shader::Create("./Resource/Shaders/TriangleShader.glsl", false);
			s_Data.CircleShader = Graphics::Shader::Create("./Resource/Shaders/CircleShader.glsl", false);
			s_Data.LineShader = Graphics::Shader::Create("./Resource/Shaders/LineShader.glsl", false);
			s_Data.SelectedObjectShader = Graphics::Shader::Create("./Resource/Shaders/SelectedObject.glsl", false);
		}

		void BatchRenderer::Init()
		{
			//Triangles
			s_Data.StaticTriangleVertexArray = Graphics::VertexArray::Create();

			s_Data.StaticTriangleVertexBuffer = Graphics::VertexBuffer::Create(s_Data.MaxVertices * sizeof(StaticTriangleVertex));
			s_Data.StaticTriangleVertexBuffer->SetLayout({
				{ Graphics::ShaderDataType::Int, "aID"},
				{ Graphics::ShaderDataType::Float3, "aPos"},
				{ Graphics::ShaderDataType::Float3, "aNormal"},
				{ Graphics::ShaderDataType::Float4, "aColor"}
			});
			s_Data.StaticTriangleVertexArray->AddVertexBuffer(s_Data.StaticTriangleVertexBuffer);
			s_Data.StaticTriangleIndexBuffer = Graphics::IndexBuffer::Create(s_Data.MaxIndices);
			s_Data.StaticTriangleVertexArray->SetIndexBuffer(s_Data.StaticTriangleIndexBuffer);

			s_Data.StaticTriangleVertexBufferBase = new StaticTriangleVertex[s_Data.MaxVertices];

			s_Data.TriangleVertexArray = Graphics::VertexArray::Create();

			s_Data.TriangleVertexBuffer = Graphics::VertexBuffer::Create(s_Data.MaxVertices * sizeof(TriangleVertex));
			s_Data.TriangleVertexBuffer->SetLayout({
				{ Graphics::ShaderDataType::Int, "aID"},
				{ Graphics::ShaderDataType::Float3, "aPos"},
				{ Graphics::ShaderDataType::Float4, "aColor"},

			});
			s_Data.TriangleVertexArray->AddVertexBuffer(s_Data.TriangleVertexBuffer);
			s_Data.TriangleIndexBuffer = Graphics::IndexBuffer::Create(s_Data.MaxIndices);
			s_Data.TriangleVertexArray->SetIndexBuffer(s_Data.TriangleIndexBuffer);

			s_Data.TriangleVertexBufferBase = new TriangleVertex[s_Data.MaxVertices];
			s_Data.TriangleIndexBufferBase = new uint32_t[s_Data.MaxIndices];

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

			// Circles
			s_Data.CircleVertexArray = Graphics::VertexArray::Create();

			s_Data.CircleVertexBuffer = Graphics::VertexBuffer::Create(s_Data.MaxVertices * sizeof(CircleVertex));
			s_Data.CircleVertexBuffer->SetLayout({
				{ Graphics::ShaderDataType::Int, "aID"},
				{ Graphics::ShaderDataType::Float3, "aPos" },
				{ Graphics::ShaderDataType::Float3, "aCirclePos" },
				{ Graphics::ShaderDataType::Float3, "aNormal" },
				{ Graphics::ShaderDataType::Float4, "aColor" },
				{ Graphics::ShaderDataType::Float, "aRadius" },
			});
			s_Data.CircleVertexArray->AddVertexBuffer(s_Data.CircleVertexBuffer);
			Graphics::Ref<Graphics::IndexBuffer> quadIB = Graphics::IndexBuffer::Create(quadIndices, s_Data.MaxIndices);
			s_Data.CircleVertexArray->SetIndexBuffer(quadIB); // Use quad IB
			s_Data.CircleVertexBufferBase =  new CircleVertex[s_Data.MaxVertices];
			delete[] quadIndices;

			//Lines
			s_Data.LineVertexArray = Graphics::VertexArray::Create();

			s_Data.LineVertexBuffer = Graphics::VertexBuffer::Create(s_Data.MaxVertices * sizeof(LineVertex));
			s_Data.LineVertexBuffer->SetLayout({
				{ Graphics::ShaderDataType::Int, "aID"},
				{ Graphics::ShaderDataType::Float3, "aPos" },
				{ Graphics::ShaderDataType::Float4, "aColor" },

			});
			s_Data.LineVertexArray->AddVertexBuffer(s_Data.LineVertexBuffer);
			s_Data.LineVertexBufferBase = new LineVertex[s_Data.MaxVertices];


			//IndexedLines
			s_Data.IndexedLineVertexArray = Graphics::VertexArray::Create();

			s_Data.IndexedLineVertexBuffer = Graphics::VertexBuffer::Create(s_Data.MaxVertices * sizeof(LineVertex));
			s_Data.IndexedLineVertexBuffer->SetLayout({
				{ Graphics::ShaderDataType::Int, "aID"},
				{ Graphics::ShaderDataType::Float3, "aPos" },
				{ Graphics::ShaderDataType::Float4, "aColor" },
			});
			s_Data.IndexedLineVertexArray->AddVertexBuffer(s_Data.IndexedLineVertexBuffer);
			s_Data.IndexedLineIndexBuffer = Graphics::IndexBuffer::Create(s_Data.MaxIndices);
			s_Data.IndexedLineVertexArray->SetIndexBuffer(s_Data.IndexedLineIndexBuffer);

			s_Data.IndexedLineVertexBufferBase = new LineVertex[s_Data.MaxVertices];
			s_Data.IndexedLineIndexBufferBase = new uint32_t[s_Data.MaxIndices];

			CreateShaders();

			GUI::DataType::vec4 triangleColor = GUI::DataType::vec4(1.0f, 0.5f, 0.2f, 1.0f);
			UBODataFragment uboDataFragment = UBODataFragment(triangleColor);

			s_Data.FragmentBuffer = Graphics::UniformBuffer::Create(sizeof(UBODataFragment), 1);
			s_Data.FragmentBuffer->SetData(&uboDataFragment, sizeof(UBODataFragment));
		}

		void BatchRenderer::ReCreateShaders() {
			CreateShaders();
		}

		void BatchRenderer::Shutdown()
		{
			delete[] s_Data.QuadVertexBufferBase;
			delete[] s_Data.StaticTriangleVertexBufferBase;
		}

		void BatchRenderer::BeginScene()
		{
			s_Data.inScene = true;
			StartBatch();
		}

		void BatchRenderer::setUpdateRequired(bool _state)
		{
			s_Data.updateData = _state;
		}

		bool BatchRenderer::getUpdateRequired()
		{
			return s_Data.updateData;
		}

		void BatchRenderer::EndScene()
		{
			Flush();
			s_Data.inScene = false;
		}

		//Draw the selected object
		void BatchRenderer::DrawSelected() {
			//All of the vertex array will still be vaild
			Renderer::DepthTest(false);
			s_Data.SelectedObjectShader->Bind();
			if (s_Data.StaticTriangleIndexCount)
			{
				Graphics::RenderCommand::DrawIndexed(s_Data.StaticTriangleVertexArray, (uint32_t)s_Data.storage.indices.size());
			}

			if (s_Data.TriangleIndexCount) {
				Graphics::RenderCommand::DrawIndexed(s_Data.TriangleVertexArray, s_Data.TriangleIndexCount);
			}

			if (s_Data.CircleIndexCount)
			{
				Graphics::RenderCommand::DrawIndexed(s_Data.CircleVertexArray, s_Data.CircleIndexCount);
			}

			if (s_Data.LineVertexCount)
			{
				Graphics::RenderCommand::DrawLines(s_Data.LineVertexArray, s_Data.LineVertexCount);
			}

			if (s_Data.IndexedLineIndexCount)
			{
				Graphics::RenderCommand::DrawLinesIndexed(s_Data.IndexedLineVertexArray, s_Data.IndexedLineIndexCount);
			}
			s_Data.SelectedObjectShader->Unbind();
			Renderer::DepthTest(true);
		}

		void BatchRenderer::Flush()
		{

			if (s_Data.StaticTriangleIndexCount)
			{

				uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.StaticTriangleVertexBufferPtr - (uint8_t*)s_Data.StaticTriangleVertexBufferBase);
				if (dataSize) {
					LOG_DEBUG_STREAM << "Data Size : " << dataSize;
					s_Data.StaticTriangleVertexBuffer->SetData(s_Data.StaticTriangleVertexBufferBase, dataSize);
					s_Data.StaticTriangleVertexBufferOffset += dataSize;


					uint32_t* triangleIndices = new uint32_t[s_Data.storage.indices.size()];
					for (size_t i = 0; i < s_Data.storage.indices.size(); i++) {
						triangleIndices[i] = s_Data.storage.indices.at(i);
					}
					s_Data.StaticTriangleIndexBuffer->SetData(triangleIndices, (uint32_t)s_Data.storage.indices.size(), 0);
					delete[] triangleIndices;
				}

				s_Data.StaticTriangleShader->Bind();
				Graphics::RenderCommand::DrawIndexed(s_Data.StaticTriangleVertexArray, (uint32_t)s_Data.storage.indices.size());
				s_Data.StaticTriangleShader->Unbind();
				//s_Data.Stats.DrawCalls++;
				//s_Data.TriangleIndices.clear();
			}

			if (s_Data.TriangleIndexCount) {
				uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.TriangleVertexBufferPtr - (uint8_t*)s_Data.TriangleVertexBufferBase);
				s_Data.TriangleVertexBuffer->SetData(s_Data.TriangleVertexBufferBase, dataSize, 0);
				s_Data.TriangleIndexBuffer->SetData(s_Data.TriangleIndexBufferBase, s_Data.TriangleIndexCount, 0);

				s_Data.TriangleShader->Bind();
				Graphics::RenderCommand::DrawIndexed(s_Data.TriangleVertexArray, s_Data.TriangleIndexCount);
				s_Data.TriangleShader->Unbind();
			}

			if (s_Data.CircleIndexCount)
			{
				uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.CircleVertexBufferPtr - (uint8_t*)s_Data.CircleVertexBufferBase);
				s_Data.CircleVertexBuffer->SetData(s_Data.CircleVertexBufferBase, dataSize, 0);

				s_Data.CircleShader->Bind();
				Graphics::RenderCommand::DrawIndexed(s_Data.CircleVertexArray, s_Data.CircleIndexCount);
				s_Data.CircleShader->Unbind();
				//s_Data.Stats.DrawCalls++;
			}

			if (s_Data.LineVertexCount)
			{
				uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.LineVertexBufferPtr - (uint8_t*)s_Data.LineVertexBufferBase);
				s_Data.LineVertexBuffer->SetData(s_Data.LineVertexBufferBase, dataSize, 0);

				s_Data.LineShader->Bind();
				Graphics::RenderCommand::DrawLines(s_Data.LineVertexArray, s_Data.LineVertexCount);
				s_Data.LineShader->Unbind();
			}

			if (s_Data.IndexedLineIndexCount)
			{
				uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.IndexedLineVertexBufferPtr - (uint8_t*)s_Data.IndexedLineVertexBufferBase);
				s_Data.IndexedLineVertexBuffer->SetData(s_Data.IndexedLineVertexBufferBase, dataSize, 0);
				s_Data.IndexedLineIndexBuffer->SetData(s_Data.IndexedLineIndexBufferBase, s_Data.IndexedLineIndexCount, 0);

				s_Data.LineShader->Bind();
				Graphics::RenderCommand::DrawLinesIndexed(s_Data.IndexedLineVertexArray, s_Data.IndexedLineIndexCount);
				s_Data.LineShader->Unbind();
			}

			DrawSelected();

		}

		void BatchRenderer::StartBatch()
		{
			s_Data.QuadIndexCount = 0;
			s_Data.QuadVertexBufferPtr = s_Data.QuadVertexBufferBase;

			//s_Data.StaticTriangleIndexCount = 0;
			s_Data.StaticTriangleVertexBufferPtr = s_Data.StaticTriangleVertexBufferBase;

			s_Data.TriangleIndexCount = 0;
			s_Data.TriangleVertexBufferOffset = 0;
			s_Data.TriangleVertexBufferPtr = s_Data.TriangleVertexBufferBase;
			s_Data.TriangleIndexBufferPtr = s_Data.TriangleIndexBufferBase;

			s_Data.CircleIndexCount = 0;
			s_Data.CircleVertexBufferPtr = s_Data.CircleVertexBufferBase;

			s_Data.LineVertexCount = 0;
			s_Data.LineVertexBufferPtr = s_Data.LineVertexBufferBase;

			s_Data.IndexedLineIndexCount = 0;
			s_Data.IndexedLineVertexBufferOffset = 0;
			s_Data.IndexedLineVertexBufferPtr = s_Data.IndexedLineVertexBufferBase;
			s_Data.IndexedLineIndexBufferPtr = s_Data.IndexedLineIndexBufferBase;

			if (s_Data.storage.updateBatch) {
				for (size_t i = 0; i < s_Data.storage.vertices.size(); i += 3) {
					s_Data.StaticTriangleVertexBufferPtr->aID = -1;
                    s_Data.StaticTriangleVertexBufferPtr->Position = { static_cast<float>(s_Data.storage.vertices.at(i)), static_cast<float>(s_Data.storage.vertices.at(i + 1)), static_cast<float>(s_Data.storage.vertices.at(i + 2)) };
                    s_Data.StaticTriangleVertexBufferPtr->Normal = { 1.0f,0.0f,0.0f };
                    s_Data.StaticTriangleVertexBufferPtr->Color = { 1.0f, 1.0f, 1.0f, 1.0f };
					s_Data.StaticTriangleVertexBufferPtr++;
				}

				s_Data.StaticTriangleIndexCount = (uint32_t)s_Data.storage.indices.size();
				s_Data.storage.updateBatch = false;
			}
		}

		void BatchRenderer::NextBatch()
		{
			Flush();
			StartBatch();
		}

		void BatchRenderer::addData(const std::vector<double>& vertices, const std::vector<double>& vertexNormals, const std::vector<uint32_t>& indices, const int id) {
			assert(!s_Data.inScene);
			assert((vertices.size() % 3) == 0);
			LOG_DEBUG_STREAM << "Adding data..." << " Vertices: " << vertices.size() << " Normals: " << vertexNormals.size() << " Indices : " << indices.size();

			s_Data.storage.vertices.insert(s_Data.storage.vertices.end(), vertices.begin(), vertices.end());

			if (vertexNormals.empty()) {
				for (size_t i = 0; i < vertices.size(); i += 3) {
					s_Data.storage.normals.push_back(0.0);
					s_Data.storage.normals.push_back(0.0);
					s_Data.storage.normals.push_back(0.0);
				}
			}
			else {
				assert((vertexNormals.size() % 3) == 0);
				s_Data.storage.normals.insert(s_Data.storage.normals.end(), vertexNormals.begin(), vertexNormals.end());
			}

			for (const auto& face : indices) {
				s_Data.storage.indices.push_back(face + s_Data.storage.indicesOffset);
			}

			s_Data.storage.indicesOffset += vertices.size() / 3;

			s_Data.storage.updateBatch = true;
		}

		void BatchRenderer::DrawMesh(const std::vector<double>& vertices, const std::vector<uint32_t>& indices, const GUI::DataType::vec4& color, const int id) {
			assert((s_Data.inScene) && (vertices.size() % 3 == 0));

			for (size_t i = 0; i < vertices.size(); i += 3) {
				s_Data.TriangleVertexBufferPtr->aID = id;
                s_Data.TriangleVertexBufferPtr->Position = { static_cast<float>(vertices.at(i)), static_cast<float>(vertices.at(i + 1)), static_cast<float>(vertices.at(i + 2)) };
				s_Data.TriangleVertexBufferPtr->Color = color;
				s_Data.TriangleVertexBufferPtr++;
			}

			for (const uint32_t& i : indices) {
				*s_Data.TriangleIndexBufferPtr = i + s_Data.TriangleVertexBufferOffset;
				s_Data.TriangleIndexBufferPtr++;
			}

			s_Data.TriangleIndexCount += indices.size();
			s_Data.TriangleVertexBufferOffset += (vertices.size() / 3);
		}

		void BatchRenderer::DrawCircle(const GUI::DataType::vec3& position, float radius ,const GUI::DataType::vec4& color, const int id) {
			assert(s_Data.inScene);

			QuadVertices(position, radius*2);

			for (unsigned int i = 0; i < 4; i++) {
				s_Data.CircleVertexBufferPtr->aID = id;
				s_Data.CircleVertexBufferPtr->Position = s_Data.quadVertices[i];
				s_Data.CircleVertexBufferPtr->CirclePosition = GUI::DataType::vec3(static_cast<float>(position.x()), static_cast<float>(position.y()), static_cast<float>(position.z()));
				s_Data.CircleVertexBufferPtr->Normal = GUI::DataType::vec3(static_cast <float>(0.0));
				s_Data.CircleVertexBufferPtr->Color = color;
				s_Data.CircleVertexBufferPtr->Radius = radius;
				s_Data.CircleVertexBufferPtr++;
			}

			s_Data.CircleIndexCount += 6;

			//s_Data.Stats.QuadCount++;
		}

		void BatchRenderer::DrawCircle(const GUI::DataType::vec2& position, float radius, const GUI::DataType::vec4& color, const int id) {
			assert(s_Data.inScene);

			DrawCircle(GUI::DataType::vec3(position.x(), position.y() ,0.0), radius, color, id);
		}


		void BatchRenderer::DrawLine(const GUI::DataType::vec3& from, const GUI::DataType::vec3& to, const GUI::DataType::vec4& color, const int id) {
			assert(s_Data.inScene);

			s_Data.LineVertexBufferPtr->aID = id;
			s_Data.LineVertexBufferPtr->Position = GUI::DataType::vec3(from);
			s_Data.LineVertexBufferPtr->Color = color;
			s_Data.LineVertexBufferPtr++;

			s_Data.LineVertexBufferPtr->aID = id;
			s_Data.LineVertexBufferPtr->Position = GUI::DataType::vec3(to);
			s_Data.LineVertexBufferPtr->Color = color;
			s_Data.LineVertexBufferPtr++;

			s_Data.LineVertexCount += 2;

		}

		void BatchRenderer::DrawLine(const GUI::DataType::vec2& from, const GUI::DataType::vec2& to, const GUI::DataType::vec4& color, const int id)
		{
			DrawLine(GUI::DataType::vec3(from.x(), from.y(), 0.0), GUI::DataType::vec3(to.x(), to.y(), 0.0), color, id);
		}

		void BatchRenderer::DrawLine(const GUI::DataType::vec3& from, const GUI::DataType::vec3& to, const GUI::DataType::vec4& color, float thickness, const int id)
		{
			assert(s_Data.inScene);
			GUI::DataType::vec3 dir = GUI::DataOp::normalize(to - from);
			GUI::DataType::vec3 normal = GUI::DataType::vec3(-dir.y(), dir.x(), dir.z());
			GUI::DataType::vec3 p1 = from + normal * thickness / 2.0f;
			GUI::DataType::vec3 p2 = to + normal * thickness / 2.0f;
			GUI::DataType::vec3 p3 = to - normal * thickness / 2.0f;
			GUI::DataType::vec3 p4 = from - normal * thickness / 2.0f;
			DrawQuad(p1, p2, p3, p4, color, id);
		}

		void BatchRenderer::DrawLine(const GUI::DataType::vec2& from, const GUI::DataType::vec2& to, const GUI::DataType::vec4& color, float thickness, const int id)
		{
			DrawLine(GUI::DataType::vec3(from.x(), from.y(), 0.0), GUI::DataType::vec3(to.x(), to.y(), 0.0), color, thickness, id);
		}

		void BatchRenderer::DrawLines(const std::vector<GUI::DataType::vec3>& points, const std::vector<uint32_t>& indices, const GUI::DataType::vec4& color, const int id, bool withArrows) {
			assert((s_Data.inScene));
			int count = 0;
			float arrowSize = 0.5f;
			for (size_t i = 0; i < points.size(); i ++) {
				s_Data.IndexedLineVertexBufferPtr->aID = (uint32_t)i;
				s_Data.IndexedLineVertexBufferPtr->Position = points.at(i);
				s_Data.IndexedLineVertexBufferPtr->Color = color;
				s_Data.IndexedLineVertexBufferPtr++;
			}

			for (const uint32_t& i : indices) {
				*s_Data.IndexedLineIndexBufferPtr = i + s_Data.IndexedLineVertexBufferOffset;
				s_Data.IndexedLineIndexBufferPtr++;
			}

			if (withArrows) {
				for (int i = 1; i < indices.size(); i += 2, count+=2) {

					GUI::DataType::vec3 direction = GUI::DataOp::normalize(points.at(indices[i]) - points.at(indices[i-1]));
					GUI::DataType::vec3 perpendicular(-direction.y(), direction.x(), 0.0f);
					GUI::DataType::vec3 arrowBase = points.at(indices[i]) - (direction * 0.15f);

					s_Data.IndexedLineVertexBufferPtr->aID = id;
					s_Data.IndexedLineVertexBufferPtr->Position = arrowBase + (perpendicular * (0.15f/2.0f));
					s_Data.IndexedLineVertexBufferPtr->Color = color;
					s_Data.IndexedLineVertexBufferPtr++;

					*s_Data.IndexedLineIndexBufferPtr = indices[i] + s_Data.IndexedLineVertexBufferOffset;
					s_Data.IndexedLineIndexBufferPtr++;

					*s_Data.IndexedLineIndexBufferPtr = (count) + s_Data.IndexedLineVertexBufferOffset + (uint32_t)points.size();
					s_Data.IndexedLineIndexBufferPtr++;

					s_Data.IndexedLineVertexBufferPtr->aID = id;
					s_Data.IndexedLineVertexBufferPtr->Position = arrowBase - (perpendicular * (0.15f / 2.0f));
					s_Data.IndexedLineVertexBufferPtr->Color = color;
					s_Data.IndexedLineVertexBufferPtr++;

					*s_Data.IndexedLineIndexBufferPtr = indices[i] + s_Data.IndexedLineVertexBufferOffset;
					s_Data.IndexedLineIndexBufferPtr++;

					*s_Data.IndexedLineIndexBufferPtr = (count + 1) + s_Data.IndexedLineVertexBufferOffset + (uint32_t)points.size();
					s_Data.IndexedLineIndexBufferPtr++;
				}

			}

			s_Data.IndexedLineIndexCount += (indices.size() + (count*2));
			s_Data.IndexedLineVertexBufferOffset += (points.size() + count);
		}

		void BatchRenderer::DrawQuad(const GUI::DataType::vec3& p1, const GUI::DataType::vec3& p2, const GUI::DataType::vec3& p3, const GUI::DataType::vec3& p4, const GUI::DataType::vec4& color, const int id) {
			assert(s_Data.inScene);
			s_Data.TriangleVertexBufferPtr->aID = id;
			s_Data.TriangleVertexBufferPtr->Position = GUI::DataType::vec3(p1.x(), p1.y(), p1.z());
			s_Data.TriangleVertexBufferPtr->Color = color;
			s_Data.TriangleVertexBufferPtr++;

			s_Data.TriangleVertexBufferPtr->aID = id;
			s_Data.TriangleVertexBufferPtr->Position = GUI::DataType::vec3(p2.x(), p2.y(), p2.z());
			s_Data.TriangleVertexBufferPtr->Color = color;
			s_Data.TriangleVertexBufferPtr++;

			s_Data.TriangleVertexBufferPtr->aID = id;
			s_Data.TriangleVertexBufferPtr->Position = GUI::DataType::vec3(p3.x(), p3.y(), p3.z());
			s_Data.TriangleVertexBufferPtr->Color = color;
			s_Data.TriangleVertexBufferPtr++;

			s_Data.TriangleVertexBufferPtr->aID = id;
			s_Data.TriangleVertexBufferPtr->Position = GUI::DataType::vec3(p4.x(), p4.y(), p4.z());
			s_Data.TriangleVertexBufferPtr->Color = color;
			s_Data.TriangleVertexBufferPtr++;

			*s_Data.TriangleIndexBufferPtr = 0 + s_Data.TriangleVertexBufferOffset;
			s_Data.TriangleIndexBufferPtr++;
			*s_Data.TriangleIndexBufferPtr = 1 + s_Data.TriangleVertexBufferOffset;
			s_Data.TriangleIndexBufferPtr++;
			*s_Data.TriangleIndexBufferPtr = 2 + s_Data.TriangleVertexBufferOffset;
			s_Data.TriangleIndexBufferPtr++;
			*s_Data.TriangleIndexBufferPtr = 2 + s_Data.TriangleVertexBufferOffset;
			s_Data.TriangleIndexBufferPtr++;
			*s_Data.TriangleIndexBufferPtr = 3 + s_Data.TriangleVertexBufferOffset;
			s_Data.TriangleIndexBufferPtr++;
			*s_Data.TriangleIndexBufferPtr = 0 + s_Data.TriangleVertexBufferOffset;
			s_Data.TriangleIndexBufferPtr++;



			s_Data.TriangleIndexCount += 6;
			s_Data.TriangleVertexBufferOffset += 4;
		}

		void BatchRenderer::DrawQuad(const GUI::DataType::vec2& p1, const GUI::DataType::vec2& p2, const GUI::DataType::vec2& p3, const GUI::DataType::vec2& p4, const GUI::DataType::vec4& color, const int id) {
			DrawQuad(GUI::DataType::vec3(p1.x(), p1.y(),0.0), GUI::DataType::vec3(p2.x(), p2.y(), 0.0), GUI::DataType::vec3(p3.x(), p3.y(), 0.0), GUI::DataType::vec3(p4.x(), p4.y(), 0.0),color,id);
		}

		void BatchRenderer::DrawQuad(const GUI::DataType::vec2& position, float size, const GUI::DataType::vec4& color, const int id) {
			assert(s_Data.inScene);

			QuadVertices(GUI::DataType::vec3(position.x(), position.y(), 0.0), size);

			DrawQuad(s_Data.quadVertices[0], s_Data.quadVertices[1], s_Data.quadVertices[2], s_Data.quadVertices[3], color);

		}

		void BatchRenderer::DrawQuad(const GUI::DataType::vec2& position, const GUI::DataType::vec2& size, const GUI::DataType::vec4& color, const int id) {
			assert(s_Data.inScene);

			QuadVertices(GUI::DataType::vec3(position.x(), position.y(), 0.0), size);

			DrawQuad(s_Data.quadVertices[0], s_Data.quadVertices[1], s_Data.quadVertices[2], s_Data.quadVertices[3], color);

		}

		void BatchRenderer::DrawQuad(const GUI::DataType::vec3& position, float size, const GUI::DataType::vec4& color, const int id) {
			assert(s_Data.inScene);

			QuadVertices(GUI::DataType::vec3(position.x(), position.y(), position.z()), size);

			DrawQuad(s_Data.quadVertices[0], s_Data.quadVertices[1], s_Data.quadVertices[2], s_Data.quadVertices[3], color, id);

		}

		void BatchRenderer::DrawQuad(const GUI::DataType::vec3& position, const GUI::DataType::vec2& size, const GUI::DataType::vec4& color, const int id) {
			assert(s_Data.inScene);

			QuadVertices(GUI::DataType::vec3(position.x(), position.y(), position.z()), size);

			DrawQuad(s_Data.quadVertices[0], s_Data.quadVertices[1], s_Data.quadVertices[2], s_Data.quadVertices[3], color, id);

		}

		//Draws Cap at start point
		void BatchRenderer::DrawCap(const GUI::DataType::vec3& start, const GUI::DataType::vec3& end, float thickness, const GUI::DataType::vec4& color, const int id) {
			//Line Caps - params: center point to draw at and two points to the side.
			assert(s_Data.inScene);

			GUI::DataType::vec2 direction = GUI::DataOp::normalize(end - start);
			GUI::DataType::vec2 normal = GUI::DataType::vec2(direction.y(), -direction.x());

			int segments = 12; // Number of segments in the semicircle
			float radius = thickness * 0.5f;
			float angleIncrement = glm::pi<float>() / static_cast<float>(segments);

			s_Data.TriangleVertexBufferPtr->aID = id;
			s_Data.TriangleVertexBufferPtr->Position = GUI::DataType::vec3(start.x(), start.y(), start.z());
			s_Data.TriangleVertexBufferPtr->Color = color;
			s_Data.TriangleVertexBufferPtr++;


			for (int i = 0; i <= segments; i++) {
				float angle = angleIncrement * 1 * i;
				s_Data.TriangleVertexBufferPtr->aID = id;
                GUI::DataType::vec2 sp = GUI::DataType::vec2(start) - (GUI::DataOp::rotate(normal, angle) * radius);
				s_Data.TriangleVertexBufferPtr->Position = GUI::DataType::vec3(sp.x(), sp.y(),start.z());
				s_Data.TriangleVertexBufferPtr->Color = color;
				s_Data.TriangleVertexBufferPtr++;

				if (i == segments) continue;
				*s_Data.TriangleIndexBufferPtr = 0 + s_Data.TriangleVertexBufferOffset;
				s_Data.TriangleIndexBufferPtr++;
				*s_Data.TriangleIndexBufferPtr = 1 + s_Data.TriangleVertexBufferOffset + i;
				s_Data.TriangleIndexBufferPtr++;
				*s_Data.TriangleIndexBufferPtr = 2 + s_Data.TriangleVertexBufferOffset + i;
				s_Data.TriangleIndexBufferPtr++;
			}


			s_Data.TriangleIndexCount += (3 * segments);
			s_Data.TriangleVertexBufferOffset += (segments + 2);

		}


		void BatchRenderer::DrawTrace(const GUI::DataType::vec3& from, const GUI::DataType::vec3& to, const GUI::DataType::vec4& color, float thickness, const int id)
		{
			assert(s_Data.inScene);
			GUI::DataType::vec3 dir = GUI::DataOp::normalize(to - from);
			GUI::DataType::vec3 normal = GUI::DataType::vec3(-dir.y(), dir.x(), dir.z());
			GUI::DataType::vec3 p1 = from + normal * thickness / 2.0f;
			GUI::DataType::vec3 p2 = to + normal * thickness / 2.0f;
			GUI::DataType::vec3 p3 = to - normal * thickness / 2.0f;
			GUI::DataType::vec3 p4 = from - normal * thickness / 2.0f;
			DrawCap(from, to, thickness, color, id);
			DrawQuad(p1, p2, p3, p4, color, id);
			DrawCap(to, from, thickness, color, id);
		}

		void BatchRenderer::DrawTrace(const GUI::DataType::vec2& from, const GUI::DataType::vec2& to, const GUI::DataType::vec4& color, float thickness, const int id)
		{
			DrawTrace(GUI::DataType::vec3(from.x(), from.y(), 0.0), GUI::DataType::vec3(to.x(), to.y(), 0.0), color, thickness, id);
		}

		void BatchRenderer::DrawObround(const GUI::DataType::vec3& position, const GUI::DataType::vec2& size, const GUI::DataType::vec4& color, const int id)
		{
			//If the obround is a circle
			if (size.x() == size.y()) {
				DrawCircle(position, size.x() / 2, color, id);
				return;
			}
			//Position is the center of the obround
			//size is the width and height of the obround
			float obroundRadius;
			float halfHeightWithoutCap;
			GUI::DataType::vec3 start;
			GUI::DataType::vec3 end;
			float thickness = size.x() < size.y() ? size.x() : size.y();

			if (size.x() < size.y()) {
				obroundRadius = size.x() / 2;
				halfHeightWithoutCap = (size.y() - (obroundRadius * 2)) / 2;
				start = { position.x(), position.y() + halfHeightWithoutCap, position.z() };
				end = { position.x(), position.y() - halfHeightWithoutCap, position.z() };
			}
			else {
				obroundRadius = size.y() / 2;
				halfHeightWithoutCap = (size.x() - (obroundRadius * 2)) / 2;
				start = { position.x() - halfHeightWithoutCap, position.y(), position.z() };
				end = { position.x() + halfHeightWithoutCap, position.y(), position.z() };
			}

			DrawTrace(start, end, color, thickness, id);
		}

		void BatchRenderer::DrawObround(const GUI::DataType::vec2& position, const GUI::DataType::vec2& size, const GUI::DataType::vec4& color, const int id)
		{
			DrawObround(GUI::DataType::vec3(position.x(), position.y(), 0.0f), size, color, id);
		}

}

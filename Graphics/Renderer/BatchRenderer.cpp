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
			std::vector<float> vertices;
			std::vector<float> normals;
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

			// Runtime-growable capacities. MaxVertices/MaxIndices are only the
			// initial sizes; the staging arrays and GPU buffers grow on demand
			// so arbitrarily large scenes never overrun the batch (previously
			// exceeding 32k vertices silently corrupted the heap).
			uint32_t StaticTriangleVertexCapacity = MaxVertices;
			uint32_t StaticTriangleIndexCapacity = MaxIndices;
			uint32_t TriangleVertexCapacity = MaxVertices;
			uint32_t TriangleIndexCapacity = MaxIndices;
			uint32_t CircleVertexCapacity = MaxVertices;
			uint32_t CircleIndexCapacity = MaxIndices;
			uint32_t LineVertexCapacity = MaxVertices;
			uint32_t IndexedLineVertexCapacity = MaxVertices;
			uint32_t IndexedLineIndexCapacity = MaxIndices;

			// Current sizes of the GPU-side buffers, in elements (resized in
			// Flush when the staging data outgrew them).
			uint32_t StaticTriangleVertexBufferGPUCount = MaxVertices;
			uint32_t StaticTriangleIndexBufferGPUCount = MaxIndices;
			uint32_t TriangleVertexBufferGPUCount = MaxVertices;
			uint32_t TriangleIndexBufferGPUCount = MaxIndices;
			uint32_t CircleVertexBufferGPUCount = MaxVertices;
			uint32_t CircleIndexBufferGPUCount = MaxIndices;
			uint32_t LineVertexBufferGPUCount = MaxVertices;
			uint32_t IndexedLineVertexBufferGPUCount = MaxVertices;
			uint32_t IndexedLineIndexBufferGPUCount = MaxIndices;
		
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

		// Grow a CPU staging array (preserving the used prefix and the write
		// cursor) so batch submission can never write out of bounds.
		template<typename V>
		static void GrowStagingArray(V*& base, V*& ptr, uint32_t usedCount, uint32_t& capacity, uint32_t neededCount)
		{
			if (neededCount <= capacity)
				return;
			uint32_t newCapacity = capacity ? capacity : 1024;
			while (newCapacity < neededCount)
				newCapacity *= 2;
			V* newBase = new V[newCapacity];
			if (base && usedCount)
				memcpy(newBase, base, (size_t)usedCount * sizeof(V));
			delete[] base;
			base = newBase;
			ptr = newBase + usedCount;
			capacity = newCapacity;
		}

		//Get quad vertices with position at center
		void BatchRenderer::QuadVertices(GUI::DataType::vec3 position, float size)
		{
            s_Data.quadVertices[0] = { position._X_ - size/2, position._Y_ - size/2, position._Z_ };
            s_Data.quadVertices[1] = { position._X_ + size/2, position._Y_ - size/2, position._Z_ };
            s_Data.quadVertices[2] = { position._X_ + size/2, position._Y_ + size/2, position._Z_ };
            s_Data.quadVertices[3] = { position._X_ - size/2, position._Y_ + size/2, position._Z_ };
		}

		//Get quad vertices with position at center
		void BatchRenderer::QuadVertices(GUI::DataType::vec3 position, const GUI::DataType::vec2& size)
		{
            s_Data.quadVertices[0] = { position._X_ - size._X_/2, position._Y_ - size._Y_/2, position._Z_ };
            s_Data.quadVertices[1] = { position._X_ + size._X_/2, position._Y_ - size._Y_/2, position._Z_ };
            s_Data.quadVertices[2] = { position._X_ + size._X_/2, position._Y_ + size._Y_/2, position._Z_ };
            s_Data.quadVertices[3] = { position._X_ - size._X_/2, position._Y_ + size._Y_/2, position._Z_ };
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

		static bool s_SelectionActive = false;

		void BatchRenderer::SetSelectionActive(bool active)
		{
			s_SelectionActive = active;
		}

		// -------------------------------------------------------------------
		// Retained meshes: geometry lives in a persistent GPU arena, uploaded
		// only when meshes are created/destroyed. Per-frame submission is a
		// handle push; drawing is one indexed-range draw per visible mesh.
		// -------------------------------------------------------------------
		struct RetainedMeshRange
		{
			uint32_t vertexOffset = 0;  // in vertices
			uint32_t vertexCount = 0;
			uint32_t indexOffset = 0;   // in indices
			uint32_t indexCount = 0;
			bool alive = false;
		};

		struct RetainedMeshStorage
		{
			std::vector<TriangleVertex> vertices;   // CPU arena (kept for growth re-uploads)
			std::vector<uint32_t> indices;          // baked with the mesh's vertex offset
			std::vector<RetainedMeshRange> meshes;  // handle - 1 indexes this
			std::vector<BatchRenderer::MeshHandle> visible; // this scene's submissions

			Graphics::Ref<Graphics::VertexArray> MeshVertexArray;
			Graphics::Ref<Graphics::VertexBuffer> MeshVertexBuffer;
			Graphics::Ref<Graphics::IndexBuffer> MeshIndexBuffer;

			uint32_t gpuVertexCapacity = 0;
			uint32_t gpuIndexCapacity = 0;
			bool gpuDirty = false;
			bool initialized = false;
		};

		static RetainedMeshStorage s_Retained;

		static void InitRetained()
		{
			if (s_Retained.initialized)
				return;

			constexpr uint32_t kInitialVertices = 4096;
			constexpr uint32_t kInitialIndices = 8192;

			s_Retained.MeshVertexArray = Graphics::VertexArray::Create();
			s_Retained.MeshVertexBuffer = Graphics::VertexBuffer::CreateRetained(kInitialVertices * sizeof(TriangleVertex), "RetainedMeshVB");
			s_Retained.MeshVertexBuffer->SetLayout({
				{ Graphics::ShaderDataType::Int, "aID"},
				{ Graphics::ShaderDataType::Float3, "aPos"},
				{ Graphics::ShaderDataType::Float4, "aColor"},
			});
			s_Retained.MeshVertexArray->AddVertexBuffer(s_Retained.MeshVertexBuffer);
			s_Retained.MeshIndexBuffer = Graphics::IndexBuffer::CreateRetained(kInitialIndices, "RetainedMeshIB");
			s_Retained.MeshVertexArray->SetIndexBuffer(s_Retained.MeshIndexBuffer);

			s_Retained.gpuVertexCapacity = kInitialVertices;
			s_Retained.gpuIndexCapacity = kInitialIndices;
			s_Retained.initialized = true;
		}

		BatchRenderer::MeshHandle BatchRenderer::CreateMesh(const std::vector<double>& vertices, const std::vector<uint32_t>& indices, const GUI::DataType::vec4& color, const int id)
		{
			assert(vertices.size() % 3 == 0);
			if (vertices.empty() || indices.empty())
				return 0;

			InitRetained();

			RetainedMeshRange range;
			range.vertexOffset = (uint32_t)s_Retained.vertices.size();
			range.vertexCount = (uint32_t)(vertices.size() / 3);
			range.indexOffset = (uint32_t)s_Retained.indices.size();
			range.indexCount = (uint32_t)indices.size();
			range.alive = true;

			s_Retained.vertices.reserve(s_Retained.vertices.size() + range.vertexCount);
			for (size_t i = 0; i < vertices.size(); i += 3) {
				TriangleVertex vertex;
				vertex.aID = id;
				vertex.Position = GUI::DataType::vec3(vertices.at(i), vertices.at(i + 1), vertices.at(i + 2));
				vertex.Color = color;
				s_Retained.vertices.push_back(vertex);
			}

			s_Retained.indices.reserve(s_Retained.indices.size() + range.indexCount);
			for (const uint32_t& index : indices) {
				s_Retained.indices.push_back(index + range.vertexOffset);
			}

			s_Retained.meshes.push_back(range);
			s_Retained.gpuDirty = true;

			return (MeshHandle)s_Retained.meshes.size();
		}

		void BatchRenderer::DestroyMesh(MeshHandle handle)
		{
			if (handle == 0 || handle > s_Retained.meshes.size())
				return;
			RetainedMeshRange& dead = s_Retained.meshes[handle - 1];
			if (!dead.alive)
				return;
			dead.alive = false;

			// Compact the arenas; handles stay valid because ranges are
			// looked up through the (stable) meshes table.
			std::vector<TriangleVertex> newVertices;
			std::vector<uint32_t> newIndices;
			newVertices.reserve(s_Retained.vertices.size());
			newIndices.reserve(s_Retained.indices.size());

			for (RetainedMeshRange& mesh : s_Retained.meshes) {
				if (!mesh.alive)
					continue;
				const uint32_t newVertexOffset = (uint32_t)newVertices.size();
				const uint32_t newIndexOffset = (uint32_t)newIndices.size();

				newVertices.insert(newVertices.end(),
					s_Retained.vertices.begin() + mesh.vertexOffset,
					s_Retained.vertices.begin() + mesh.vertexOffset + mesh.vertexCount);
				for (uint32_t i = 0; i < mesh.indexCount; i++) {
					const uint32_t oldIndex = s_Retained.indices[mesh.indexOffset + i];
					newIndices.push_back(oldIndex - mesh.vertexOffset + newVertexOffset);
				}

				mesh.vertexOffset = newVertexOffset;
				mesh.indexOffset = newIndexOffset;
			}

			s_Retained.vertices.swap(newVertices);
			s_Retained.indices.swap(newIndices);
			s_Retained.gpuDirty = true;
			LOG_DEBUG_STREAM << "DestroyMesh: retained arena now " << s_Retained.vertices.size() << " vertices, " << s_Retained.indices.size() << " indices";
		}

		void BatchRenderer::DrawMesh(MeshHandle handle)
		{
			assert(s_Data.inScene);
			if (handle == 0 || handle > s_Retained.meshes.size() || !s_Retained.meshes[handle - 1].alive)
				return;
			s_Retained.visible.push_back(handle);
		}

		// Upload pending arena changes (rare) and draw this scene's visible
		// retained meshes with the given shader.
		static void FlushRetained(const Graphics::Ref<Graphics::Shader>& shader)
		{
			if (s_Retained.visible.empty())
				return;

			if (s_Retained.gpuDirty) {
				uint32_t neededVertices = (uint32_t)s_Retained.vertices.size();
				uint32_t neededIndices = (uint32_t)s_Retained.indices.size();
				if (neededVertices > s_Retained.gpuVertexCapacity) {
					uint32_t newCapacity = s_Retained.gpuVertexCapacity;
					while (newCapacity < neededVertices) newCapacity *= 2;
					s_Retained.MeshVertexBuffer->ResizeBuffer(newCapacity * sizeof(TriangleVertex));
					s_Retained.gpuVertexCapacity = newCapacity;
				}
				if (neededIndices > s_Retained.gpuIndexCapacity) {
					uint32_t newCapacity = s_Retained.gpuIndexCapacity;
					while (newCapacity < neededIndices) newCapacity *= 2;
					s_Retained.MeshIndexBuffer->ResizeBuffer(newCapacity);
					s_Retained.gpuIndexCapacity = newCapacity;
				}
				if (neededVertices)
					s_Retained.MeshVertexBuffer->SetData(s_Retained.vertices.data(), neededVertices * sizeof(TriangleVertex), 0);
				if (neededIndices)
					s_Retained.MeshIndexBuffer->SetData(s_Retained.indices.data(), neededIndices, 0);
				s_Retained.gpuDirty = false;
			}

			shader->Bind();
			for (const BatchRenderer::MeshHandle& handle : s_Retained.visible) {
				const RetainedMeshRange& mesh = s_Retained.meshes[handle - 1];
				Graphics::RenderCommand::DrawIndexedRange(s_Retained.MeshVertexArray, mesh.indexCount, mesh.indexOffset * (uint32_t)sizeof(uint32_t));
			}
			shader->Unbind();
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

			// Retained meshes participate in the selection mask too.
			for (const BatchRenderer::MeshHandle& handle : s_Retained.visible) {
				const RetainedMeshRange& mesh = s_Retained.meshes[handle - 1];
				Graphics::RenderCommand::DrawIndexedRange(s_Retained.MeshVertexArray, mesh.indexCount, mesh.indexOffset * (uint32_t)sizeof(uint32_t));
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
					if (s_Data.StaticTriangleVertexCapacity > s_Data.StaticTriangleVertexBufferGPUCount) {
						s_Data.StaticTriangleVertexBuffer->ResizeBuffer(s_Data.StaticTriangleVertexCapacity * sizeof(StaticTriangleVertex));
						s_Data.StaticTriangleVertexBufferGPUCount = s_Data.StaticTriangleVertexCapacity;
					}
					s_Data.StaticTriangleVertexBuffer->SetData(s_Data.StaticTriangleVertexBufferBase, dataSize);
					s_Data.StaticTriangleVertexBufferOffset += dataSize;


					if (s_Data.StaticTriangleIndexCapacity > s_Data.StaticTriangleIndexBufferGPUCount) {
						s_Data.StaticTriangleIndexBuffer->ResizeBuffer(s_Data.StaticTriangleIndexCapacity);
						s_Data.StaticTriangleIndexBufferGPUCount = s_Data.StaticTriangleIndexCapacity;
					}
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
				if (s_Data.TriangleVertexCapacity > s_Data.TriangleVertexBufferGPUCount) {
					s_Data.TriangleVertexBuffer->ResizeBuffer(s_Data.TriangleVertexCapacity * sizeof(TriangleVertex));
					s_Data.TriangleVertexBufferGPUCount = s_Data.TriangleVertexCapacity;
				}
				if (s_Data.TriangleIndexCapacity > s_Data.TriangleIndexBufferGPUCount) {
					s_Data.TriangleIndexBuffer->ResizeBuffer(s_Data.TriangleIndexCapacity);
					s_Data.TriangleIndexBufferGPUCount = s_Data.TriangleIndexCapacity;
				}
				s_Data.TriangleVertexBuffer->SetData(s_Data.TriangleVertexBufferBase, dataSize, 0);
				s_Data.TriangleIndexBuffer->SetData(s_Data.TriangleIndexBufferBase, s_Data.TriangleIndexCount, 0);

				s_Data.TriangleShader->Bind();
				Graphics::RenderCommand::DrawIndexed(s_Data.TriangleVertexArray, s_Data.TriangleIndexCount);
				s_Data.TriangleShader->Unbind();
			}

			if (s_Data.CircleIndexCount)
			{
				uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.CircleVertexBufferPtr - (uint8_t*)s_Data.CircleVertexBufferBase);
				if (s_Data.CircleVertexCapacity > s_Data.CircleVertexBufferGPUCount) {
					s_Data.CircleVertexBuffer->ResizeBuffer(s_Data.CircleVertexCapacity * sizeof(CircleVertex));
					s_Data.CircleVertexBufferGPUCount = s_Data.CircleVertexCapacity;
				}
				// The circle batch shares a repeating quad index pattern; if the
				// vertex batch outgrew it, rebuild the pattern index buffer.
				if (s_Data.CircleIndexCapacity > s_Data.CircleIndexBufferGPUCount) {
					const uint32_t patternIndices = s_Data.CircleIndexCapacity;
					uint32_t* quadIndices = new uint32_t[patternIndices];
					uint32_t offset = 0;
					for (uint32_t i = 0; i + 5 < patternIndices; i += 6)
					{
						quadIndices[i + 0] = offset + 0;
						quadIndices[i + 1] = offset + 1;
						quadIndices[i + 2] = offset + 2;
						quadIndices[i + 3] = offset + 2;
						quadIndices[i + 4] = offset + 3;
						quadIndices[i + 5] = offset + 0;
						offset += 4;
					}
					s_Data.CircleVertexArray->SetIndexBuffer(Graphics::IndexBuffer::Create(quadIndices, patternIndices));
					delete[] quadIndices;
					s_Data.CircleIndexBufferGPUCount = patternIndices;
				}
				s_Data.CircleVertexBuffer->SetData(s_Data.CircleVertexBufferBase, dataSize, 0);

				s_Data.CircleShader->Bind();
				Graphics::RenderCommand::DrawIndexed(s_Data.CircleVertexArray, s_Data.CircleIndexCount);
				s_Data.CircleShader->Unbind();
				//s_Data.Stats.DrawCalls++;
			}

			if (s_Data.LineVertexCount)
			{
				uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.LineVertexBufferPtr - (uint8_t*)s_Data.LineVertexBufferBase);
				if (s_Data.LineVertexCapacity > s_Data.LineVertexBufferGPUCount) {
					s_Data.LineVertexBuffer->ResizeBuffer(s_Data.LineVertexCapacity * sizeof(LineVertex));
					s_Data.LineVertexBufferGPUCount = s_Data.LineVertexCapacity;
				}
				s_Data.LineVertexBuffer->SetData(s_Data.LineVertexBufferBase, dataSize, 0);

				s_Data.LineShader->Bind();
				Graphics::RenderCommand::DrawLines(s_Data.LineVertexArray, s_Data.LineVertexCount);
				s_Data.LineShader->Unbind();
			}

			if (s_Data.IndexedLineIndexCount)
			{
				uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.IndexedLineVertexBufferPtr - (uint8_t*)s_Data.IndexedLineVertexBufferBase);
				if (s_Data.IndexedLineVertexCapacity > s_Data.IndexedLineVertexBufferGPUCount) {
					s_Data.IndexedLineVertexBuffer->ResizeBuffer(s_Data.IndexedLineVertexCapacity * sizeof(LineVertex));
					s_Data.IndexedLineVertexBufferGPUCount = s_Data.IndexedLineVertexCapacity;
				}
				if (s_Data.IndexedLineIndexCapacity > s_Data.IndexedLineIndexBufferGPUCount) {
					s_Data.IndexedLineIndexBuffer->ResizeBuffer(s_Data.IndexedLineIndexCapacity);
					s_Data.IndexedLineIndexBufferGPUCount = s_Data.IndexedLineIndexCapacity;
				}
				s_Data.IndexedLineVertexBuffer->SetData(s_Data.IndexedLineVertexBufferBase, dataSize, 0);
				s_Data.IndexedLineIndexBuffer->SetData(s_Data.IndexedLineIndexBufferBase, s_Data.IndexedLineIndexCount, 0);

				s_Data.LineShader->Bind();
				Graphics::RenderCommand::DrawLinesIndexed(s_Data.IndexedLineVertexArray, s_Data.IndexedLineIndexCount);
				s_Data.LineShader->Unbind();
			}

			// Retained meshes: uploaded once, drawn by handle.
			FlushRetained(s_Data.TriangleShader);

			// The selection-mask pass re-draws every batch; only pay for it
			// when something is actually selected.
			if (s_SelectionActive)
				DrawSelected();

		}

		void BatchRenderer::StartBatch()
		{
			s_Retained.visible.clear();

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
				GrowStagingArray(s_Data.StaticTriangleVertexBufferBase, s_Data.StaticTriangleVertexBufferPtr,
					0, s_Data.StaticTriangleVertexCapacity, (uint32_t)(s_Data.storage.vertices.size() / 3));
				if ((uint32_t)s_Data.storage.indices.size() > s_Data.StaticTriangleIndexCapacity)
					s_Data.StaticTriangleIndexCapacity = (uint32_t)s_Data.storage.indices.size();

				for (size_t i = 0; i < s_Data.storage.vertices.size(); i += 3) {
					s_Data.StaticTriangleVertexBufferPtr->aID = 1;
                    s_Data.StaticTriangleVertexBufferPtr->Position = GUI::DataType::vec3(s_Data.storage.vertices.at(i), s_Data.storage.vertices.at(i + 1), s_Data.storage.vertices.at(i + 2));                   
                    s_Data.StaticTriangleVertexBufferPtr->Normal = GUI::DataType::vec3( 1.0f,0.0f,0.0f );
                    s_Data.StaticTriangleVertexBufferPtr->Color = GUI::DataType::vec4( 1.0f, 1.0f, 1.0f, 1.0f );
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

			GrowStagingArray(s_Data.TriangleVertexBufferBase, s_Data.TriangleVertexBufferPtr,
				s_Data.TriangleVertexBufferOffset, s_Data.TriangleVertexCapacity,
				s_Data.TriangleVertexBufferOffset + (uint32_t)(vertices.size() / 3));
			GrowStagingArray(s_Data.TriangleIndexBufferBase, s_Data.TriangleIndexBufferPtr,
				s_Data.TriangleIndexCount, s_Data.TriangleIndexCapacity,
				s_Data.TriangleIndexCount + (uint32_t)indices.size());

			for (size_t i = 0; i < vertices.size(); i += 3) {
				s_Data.TriangleVertexBufferPtr->aID = id;
                s_Data.TriangleVertexBufferPtr->Position = GUI::DataType::vec3( vertices.at(i), vertices.at(i + 1), vertices.at(i + 2) );
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

			const uint32_t circleVerticesUsed = (uint32_t)(s_Data.CircleVertexBufferPtr - s_Data.CircleVertexBufferBase);
			GrowStagingArray(s_Data.CircleVertexBufferBase, s_Data.CircleVertexBufferPtr,
				circleVerticesUsed, s_Data.CircleVertexCapacity, circleVerticesUsed + 4);
			if (s_Data.CircleIndexCount + 6 > s_Data.CircleIndexCapacity)
				s_Data.CircleIndexCapacity *= 2;

			QuadVertices(position, radius*2);

			for (unsigned int i = 0; i < 4; i++) {
				s_Data.CircleVertexBufferPtr->aID = id;
				s_Data.CircleVertexBufferPtr->Position = s_Data.quadVertices[i];
				s_Data.CircleVertexBufferPtr->CirclePosition = GUI::DataType::vec3(static_cast<float>(position._X_), static_cast<float>(position._Y_), static_cast<float>(position._Z_));
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

			DrawCircle(GUI::DataType::vec3(position._X_, position._Y_ ,0.0), radius, color, id);
		}


		void BatchRenderer::DrawLine(const GUI::DataType::vec3& from, const GUI::DataType::vec3& to, const GUI::DataType::vec4& color, const int id) {
			assert(s_Data.inScene);

			GrowStagingArray(s_Data.LineVertexBufferBase, s_Data.LineVertexBufferPtr,
				s_Data.LineVertexCount, s_Data.LineVertexCapacity, s_Data.LineVertexCount + 2);

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
			DrawLine(GUI::DataType::vec3(from._X_, from._Y_, 0.0), GUI::DataType::vec3(to._X_, to._Y_, 0.0), color, id);
		}

		void BatchRenderer::DrawLine(const GUI::DataType::vec3& from, const GUI::DataType::vec3& to, const GUI::DataType::vec4& color, float thickness, const int id)
		{
			assert(s_Data.inScene);
			GUI::DataType::vec3 dir = GUI::DataOp::normalize(to - from);
			GUI::DataType::vec3 normal = GUI::DataType::vec3(-dir._Y_, dir._X_, dir._Z_);
			GUI::DataType::vec3 p1 = from + normal * thickness / 2.0f;
			GUI::DataType::vec3 p2 = to + normal * thickness / 2.0f;
			GUI::DataType::vec3 p3 = to - normal * thickness / 2.0f;
			GUI::DataType::vec3 p4 = from - normal * thickness / 2.0f;
			DrawQuad(p1, p2, p3, p4, color, id);
		}

		void BatchRenderer::DrawLine(const GUI::DataType::vec2& from, const GUI::DataType::vec2& to, const GUI::DataType::vec4& color, float thickness, const int id)
		{
			DrawLine(GUI::DataType::vec3(from._X_, from._Y_, 0.0), GUI::DataType::vec3(to._X_, to._Y_, 0.0), color, thickness, id);
		}

		void BatchRenderer::DrawLines(const std::vector<GUI::DataType::vec3>& points, const std::vector<uint32_t>& indices, const GUI::DataType::vec4& color, const int id, bool withArrows) {
			assert((s_Data.inScene));

			// Worst case with arrows: +2 vertices and +4 indices per segment.
			const uint32_t maxExtraVerts = withArrows ? (uint32_t)indices.size() : 0;
			const uint32_t maxExtraIndices = withArrows ? (uint32_t)indices.size() * 2 : 0;
			GrowStagingArray(s_Data.IndexedLineVertexBufferBase, s_Data.IndexedLineVertexBufferPtr,
				s_Data.IndexedLineVertexBufferOffset, s_Data.IndexedLineVertexCapacity,
				s_Data.IndexedLineVertexBufferOffset + (uint32_t)points.size() + maxExtraVerts);
			GrowStagingArray(s_Data.IndexedLineIndexBufferBase, s_Data.IndexedLineIndexBufferPtr,
				s_Data.IndexedLineIndexCount, s_Data.IndexedLineIndexCapacity,
				s_Data.IndexedLineIndexCount + (uint32_t)indices.size() + maxExtraIndices);

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
					GUI::DataType::vec3 perpendicular(-direction._Y_, direction._X_, 0.0f);
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

			GrowStagingArray(s_Data.TriangleVertexBufferBase, s_Data.TriangleVertexBufferPtr,
				s_Data.TriangleVertexBufferOffset, s_Data.TriangleVertexCapacity,
				s_Data.TriangleVertexBufferOffset + 4);
			GrowStagingArray(s_Data.TriangleIndexBufferBase, s_Data.TriangleIndexBufferPtr,
				s_Data.TriangleIndexCount, s_Data.TriangleIndexCapacity,
				s_Data.TriangleIndexCount + 6);

			s_Data.TriangleVertexBufferPtr->aID = id;
			s_Data.TriangleVertexBufferPtr->Position = GUI::DataType::vec3(p1._X_, p1._Y_, p1._Z_);
			s_Data.TriangleVertexBufferPtr->Color = color;
			s_Data.TriangleVertexBufferPtr++;

			s_Data.TriangleVertexBufferPtr->aID = id;
			s_Data.TriangleVertexBufferPtr->Position = GUI::DataType::vec3(p2._X_, p2._Y_, p2._Z_);
			s_Data.TriangleVertexBufferPtr->Color = color;
			s_Data.TriangleVertexBufferPtr++;

			s_Data.TriangleVertexBufferPtr->aID = id;
			s_Data.TriangleVertexBufferPtr->Position = GUI::DataType::vec3(p3._X_, p3._Y_, p3._Z_);
			s_Data.TriangleVertexBufferPtr->Color = color;
			s_Data.TriangleVertexBufferPtr++;

			s_Data.TriangleVertexBufferPtr->aID = id;
			s_Data.TriangleVertexBufferPtr->Position = GUI::DataType::vec3(p4._X_, p4._Y_, p4._Z_);
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
			DrawQuad(GUI::DataType::vec3(p1._X_, p1._Y_,0.0), GUI::DataType::vec3(p2._X_, p2._Y_, 0.0), GUI::DataType::vec3(p3._X_, p3._Y_, 0.0), GUI::DataType::vec3(p4._X_, p4._Y_, 0.0),color,id);
		}

		void BatchRenderer::DrawQuad(const GUI::DataType::vec2& position, float size, const GUI::DataType::vec4& color, const int id) {
			assert(s_Data.inScene);

			QuadVertices(GUI::DataType::vec3(position._X_, position._Y_, 0.0), size);

			DrawQuad(s_Data.quadVertices[0], s_Data.quadVertices[1], s_Data.quadVertices[2], s_Data.quadVertices[3], color);

		}

		void BatchRenderer::DrawQuad(const GUI::DataType::vec2& position, const GUI::DataType::vec2& size, const GUI::DataType::vec4& color, const int id) {
			assert(s_Data.inScene);

			QuadVertices(GUI::DataType::vec3(position._X_, position._Y_, 0.0), size);

			DrawQuad(s_Data.quadVertices[0], s_Data.quadVertices[1], s_Data.quadVertices[2], s_Data.quadVertices[3], color);

		}

		void BatchRenderer::DrawQuad(const GUI::DataType::vec3& position, float size, const GUI::DataType::vec4& color, const int id) {
			assert(s_Data.inScene);

			QuadVertices(GUI::DataType::vec3(position._X_, position._Y_, position._Z_), size);

			DrawQuad(s_Data.quadVertices[0], s_Data.quadVertices[1], s_Data.quadVertices[2], s_Data.quadVertices[3], color, id);

		}

		void BatchRenderer::DrawQuad(const GUI::DataType::vec3& position, const GUI::DataType::vec2& size, const GUI::DataType::vec4& color, const int id) {
			assert(s_Data.inScene);

			QuadVertices(GUI::DataType::vec3(position._X_, position._Y_, position._Z_), size);

			DrawQuad(s_Data.quadVertices[0], s_Data.quadVertices[1], s_Data.quadVertices[2], s_Data.quadVertices[3], color, id);

		}

		//Draws Cap at start point
		void BatchRenderer::DrawCap(const GUI::DataType::vec3& start, const GUI::DataType::vec3& end, float thickness, const GUI::DataType::vec4& color, const int id) {
			//Line Caps - params: center point to draw at and two points to the side.
			assert(s_Data.inScene);

			GUI::DataType::vec2 direction = GUI::DataOp::normalize(end - start);
			GUI::DataType::vec2 normal = GUI::DataType::vec2(direction._Y_, -direction._X_);

			int segments = 12; // Number of segments in the semicircle
			float radius = thickness * 0.5f;
			float angleIncrement = glm::pi<float>() / static_cast<float>(segments);

			GrowStagingArray(s_Data.TriangleVertexBufferBase, s_Data.TriangleVertexBufferPtr,
				s_Data.TriangleVertexBufferOffset, s_Data.TriangleVertexCapacity,
				s_Data.TriangleVertexBufferOffset + (uint32_t)(segments + 2));
			GrowStagingArray(s_Data.TriangleIndexBufferBase, s_Data.TriangleIndexBufferPtr,
				s_Data.TriangleIndexCount, s_Data.TriangleIndexCapacity,
				s_Data.TriangleIndexCount + (uint32_t)(3 * segments));

			s_Data.TriangleVertexBufferPtr->aID = id;
			s_Data.TriangleVertexBufferPtr->Position = GUI::DataType::vec3(start._X_, start._Y_, start._Z_);
			s_Data.TriangleVertexBufferPtr->Color = color;
			s_Data.TriangleVertexBufferPtr++;


			for (int i = 0; i <= segments; i++) {
				float angle = angleIncrement * 1 * i;
				s_Data.TriangleVertexBufferPtr->aID = id;
                GUI::DataType::vec2 sp = GUI::DataType::vec2(start) - (GUI::DataOp::rotate(normal, angle) * radius);
				s_Data.TriangleVertexBufferPtr->Position = GUI::DataType::vec3(sp._X_, sp._Y_,start._Z_);
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
			GUI::DataType::vec3 normal = GUI::DataType::vec3(-dir._Y_, dir._X_, dir._Z_);
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
			DrawTrace(GUI::DataType::vec3(from._X_, from._Y_, 0.0), GUI::DataType::vec3(to._X_, to._Y_, 0.0), color, thickness, id);
		}

		void BatchRenderer::DrawObround(const GUI::DataType::vec3& position, const GUI::DataType::vec2& size, const GUI::DataType::vec4& color, const int id)
		{
			//If the obround is a circle
			if (size._X_ == size._Y_) {
				DrawCircle(position, size._X_ / 2, color, id);
				return;
			}
			//Position is the center of the obround
			//size is the width and height of the obround
			float obroundRadius;
			float halfHeightWithoutCap;
			GUI::DataType::vec3 start;
			GUI::DataType::vec3 end;
			float thickness = size._X_ < size._Y_ ? size._X_ : size._Y_;

			if (size._X_ < size._Y_) {
				obroundRadius = size._X_ / 2;
				halfHeightWithoutCap = (size._Y_ - (obroundRadius * 2)) / 2;
				start = { position._X_, position._Y_ + halfHeightWithoutCap, position._Z_ };
				end = { position._X_, position._Y_ - halfHeightWithoutCap, position._Z_ };
			}
			else {
				obroundRadius = size._Y_ / 2;
				halfHeightWithoutCap = (size._X_ - (obroundRadius * 2)) / 2;
				start = { position._X_ - halfHeightWithoutCap, position._Y_, position._Z_ };
				end = { position._X_ + halfHeightWithoutCap, position._Y_, position._Z_ };
			}

			DrawTrace(start, end, color, thickness, id);
		}

		void BatchRenderer::DrawObround(const GUI::DataType::vec2& position, const GUI::DataType::vec2& size, const GUI::DataType::vec4& color, const int id)
		{
			DrawObround(GUI::DataType::vec3(position._X_, position._Y_, 0.0f), size, color, id);
		}

}

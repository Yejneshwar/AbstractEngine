// OpenGL Starter.cpp : Defines the entry point for the application.
//

#include <iostream> 
#include <string>

#include <AbstractApplication.h>
#include <Renderer/Renderer.h>
#include <Renderer/Shader.h>
#include <Core/Layer.h>
#include <glm/gtc/type_ptr.hpp>
#include <Renderer/Texture.h>
#include <Renderer/BatchRenderer.h>

namespace GUI {

	class ObjectLayer : public Layer {

        static const uint32_t MaxQuads = 10;
        static const uint32_t MaxVertices = MaxQuads * 4;
        static const uint32_t MaxIndices = MaxQuads * 6;
        glm::vec4 triangleColor = glm::vec4(1.0f, 0.5f, 0.2f, 0.1f);

        struct UBODataFragmentAttached {
            glm::vec4 triangleColor;
            int selectedObject;
        };

        struct TriangleVertex {
            int aID;
            glm::vec3 aPos;
            glm::vec3 aNormal;
            glm::vec3 aColor;

        };

        UBODataFragmentAttached uboDataFragment = UBODataFragmentAttached(triangleColor, -1);

		Graphics::Ref<Graphics::Shader> m_BasicShader;
        Graphics::Ref<Graphics::Shader> m_SelectedObjectShader;
        Graphics::Ref<Graphics::VertexArray> TriangleVertexArray = Graphics::VertexArray::Create();
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Vertex data
        //float vertices[30+30] = {
        //    // positions        //normals      // colors
        //     0.5f, -0.5f, 0.0f, 0.0, 0.0, -1.0, 1.0f, 0.0f, 0.0f, 1.0f,  // bottom right
        //    -0.5f, -0.5f, 0.0f, 0.0, 0.0, -1.0, 0.0f, 1.0f, 0.0f, 1.0f,  // bottom left
        //     0.0f,  0.5f, 0.0f, 0.0, 0.0, -1.0, 0.0f, 0.0f, 1.0f, 1.0f,   // top
        //     0.5f, -0.5f, 1.0f, 0.0, 0.0, 1.0, 1.0f, 0.0f, 0.0f, 2.0f,  // bottom right
        //    -0.5f, -0.5f, 1.0f, 0.0, 0.0, 1.0, 0.0f, 1.0f, 0.0f, 2.0f,  // bottom left
        //     0.0f,  0.5f, 1.0f, 0.0, 0.0, 1.0, 0.0f, 0.0f, 1.0f, 2.0f   // top 
        //};
        //Graphics::Ref<Graphics::VertexBuffer> TriangleVertexBuffer = Graphics::VertexBuffer::Create(vertices, sizeof(vertices));
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /////OR/////
        ///////////////If using a vertex buffer/////////////////////////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        Graphics::Ref<Graphics::VertexBuffer> TriangleVertexBuffer = Graphics::VertexBuffer::Create(MaxVertices * sizeof(TriangleVertex));
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        Graphics::Ref<Graphics::UniformBuffer> fragmentBuffer = Graphics::UniformBuffer::Create(sizeof(UBODataFragmentAttached), 1); // REMEBER TO SET BINDING TO 1 in the shader

        Graphics::Ref<Graphics::IndexBuffer> triangleIB;

        uint32_t* triangleIndices = new uint32_t[MaxIndices];

        uint32_t offset = 0;

	public:
		ObjectLayer() : Layer("ObjectLayer") {
		}
		~ObjectLayer() {
		}

		void OnAttach() {

            m_BasicShader = Graphics::Shader::Create("./Resource/Shaders/BasicShader.glsl", false);
            m_SelectedObjectShader = Graphics::Shader::Create("./Resource/Shaders/SelectedObject.glsl", false);

            ///////////////If using a vertex buffer/////////////////////////////////////////////////////////////////////////////////////////////
            ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            TriangleVertex* TriangleVertexBufferBase = new TriangleVertex[MaxVertices];
            TriangleVertex* TriangleVertexBufferPtr = TriangleVertexBufferBase;
            
            TriangleVertexBufferPtr->aID = 1;
            TriangleVertexBufferPtr->aPos = { 0.5f, -0.5f, 0.0f };
            TriangleVertexBufferPtr->aNormal = { 0.0f, 0.0f, -1.0f };
            TriangleVertexBufferPtr->aColor = { 1.0f, 0.0f, 0.0f };
            TriangleVertexBufferPtr++;
            TriangleVertexBufferPtr->aID = 1;
            TriangleVertexBufferPtr->aPos = { -0.5f, -0.5f, 0.0f };
            TriangleVertexBufferPtr->aNormal = { 0.0f, 0.0f, -1.0f };
            TriangleVertexBufferPtr->aColor = { 0.0f, 1.0f, 0.0f };
            TriangleVertexBufferPtr++;
            TriangleVertexBufferPtr->aID = 1;
            TriangleVertexBufferPtr->aPos = { 0.0f, 0.5f, 0.0f };
            TriangleVertexBufferPtr->aNormal = { 0.0f, 0.0f, -1.0f };
            TriangleVertexBufferPtr->aColor = { 0.0f, 0.0f, 1.0f };
            TriangleVertexBufferPtr++;

            TriangleVertexBufferPtr->aID = 2;
            TriangleVertexBufferPtr->aPos = { 0.5f, -0.5f, 1.0f };
            TriangleVertexBufferPtr->aNormal = { 0.0f, 0.0f, 1.0f };
            TriangleVertexBufferPtr->aColor = { 1.0f, 0.0f, 0.0f };
            TriangleVertexBufferPtr++;
            TriangleVertexBufferPtr->aID = 2;
            TriangleVertexBufferPtr->aPos = { -0.5f, -0.5f, 1.0f };
            TriangleVertexBufferPtr->aNormal = { 0.0f, 0.0f, 1.0f };
            TriangleVertexBufferPtr->aColor = { 0.0f, 1.0f, 0.0f };
            TriangleVertexBufferPtr++;
            TriangleVertexBufferPtr->aID = 2;
            TriangleVertexBufferPtr->aPos = { 0.0f, 0.5f, 1.0f };
            TriangleVertexBufferPtr->aNormal = { 0.0f, 0.0f, 1.0f };
            TriangleVertexBufferPtr->aColor = { 0.0f, 0.0f, 1.0f };
            TriangleVertexBufferPtr++;
            
            uint32_t dataSize = (uint32_t)((uint8_t*)TriangleVertexBufferPtr - (uint8_t*)TriangleVertexBufferBase);
            std::cout << "Data Size : " << dataSize << std::endl;
            TriangleVertexBuffer->SetData(TriangleVertexBufferBase, dataSize);
            ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

            TriangleVertexBuffer->SetLayout({
                    { Graphics::ShaderDataType::Int, "aID"},
                    { Graphics::ShaderDataType::Float3, "aPos"},
                    { Graphics::ShaderDataType::Float3, "aNormal"},
                    { Graphics::ShaderDataType::Float3, "aColor"},
                });
            TriangleVertexArray->AddVertexBuffer(TriangleVertexBuffer);


            // Uncomment if drawing indexed
            ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            //for (uint32_t i = 0; i < MaxIndices; i += 3)
            //{
            //    triangleIndices[i + 0] = offset + 0;
            //    triangleIndices[i + 1] = offset + 1;
            //    triangleIndices[i + 2] = offset + 2;
            //
            //    offset += 3;
            //}
            //
            //triangleIB = Graphics::IndexBuffer::Create(triangleIndices, MaxIndices);
            //TriangleVertexArray->SetIndexBuffer(triangleIB);
            //delete[] triangleIndices;
            ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		}

		void OnDrawUpdate() {

            fragmentBuffer->SetData(&uboDataFragment, sizeof(UBODataFragmentAttached));
            m_BasicShader->Bind();

            //If drawing indexed//
            //Graphics::RenderCommand::DrawIndexed(TriangleVertexArray);
            //      OR      //
            Graphics::RenderCommand::DrawNonIndexed(TriangleVertexArray, 6);

            m_BasicShader->Unbind();

            Graphics::Renderer::DepthTest(false);
            m_SelectedObjectShader->Bind();
            Graphics::RenderCommand::DrawNonIndexed(TriangleVertexArray, 6);
            m_SelectedObjectShader->Unbind();
            Graphics::Renderer::DepthTest(true);

            Graphics::BatchRenderer::DrawQuad({ 0.0f, 0.0f, 1.5f }, 1.0f, { 0.0f, 1.0f, 0.0f, 1.0f }, 5);

            Graphics::BatchRenderer::DrawCircle({ 1.0f,1.0f,-1.5f }, 1.0f, {0.4f,0.7f,0.3f, 1.0f}, 6);

            Graphics::BatchRenderer::DrawLine({ 1.0f,-1.0f }, { 1.0f, -2.0f }, {0.7f,0.3f,0.4f,1.0f}, 7);

            Graphics::BatchRenderer::DrawObround({ -1.0f, 1.0f, 1.5f }, { 0.7f, 0.9f }, {1.0f, 0.1f, 7.0f, 1.0f}, 8);

            Graphics::BatchRenderer::DrawTrace({ -1.0f,-1.0f, 1.5f }, { -1.0f, -2.0f, 1.5f }, { 0.7f,0.3f,0.4f,1.0f }, 0.5, 9);
            //Graphics::BatchRenderer::DrawMesh(const std::vector<double>&vertices, const std::vector<uint32_t>&indices, const glm::vec4 & color, const int id = -1);
            //Graphics::BatchRenderer::DrawLines(const std::vector<glm::vec3>&points, const std::vector<uint32_t>&indices, const glm::vec4 & color, const int id = -1, bool withArrows = false);
		}

        void OnSelection(int objectId, bool state) {
            LOG_DEBUG_STREAM << "Object " << objectId << " selected / deselected: " << state;
            if (state) {
                uboDataFragment.selectedObject = objectId;
            }
            else {
				uboDataFragment.selectedObject = -1;
			}
        }

        void createShader() {
            m_BasicShader = Graphics::Shader::Create("./Resource/Shaders/BasicShader.glsl", false);
            m_SelectedObjectShader = Graphics::Shader::Create("./Resource/Shaders/SelectedObject.glsl", false);
        }

        void OnImGuiRender() {
            // ImGui window
            ImGui::Begin("Triangle Color");
            ImGui::ColorEdit4("Color", glm::value_ptr(uboDataFragment.triangleColor));
            ImGui::End();
        }

	};


	class TestGUI : public AbstractApplication {
	private:
	public:
		TestGUI(const ApplicationSpecification& spec)
			:AbstractApplication(spec)
		{
//            PushLayer(new ObjectLayer());
		}

		~TestGUI() {
			std::cout << "TestGUI Destructor" << std::endl;
		}
	};

	
	TestGUI* CreateApplication(ApplicationCommandLineArgs args)
	{
		ApplicationSpecification spec;
		spec.Name = "TestGUI";
		spec.CommandLineArgs = args;
		return new TestGUI(spec);
	}
}
int main(int argc, char** argv)

{
	auto app = GUI::CreateApplication({ argc, argv });

	app->Run();

	delete app;
}

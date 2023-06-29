// OpenGL Starter.cpp : Defines the entry point for the application.
//

#include <iostream> 
#include <string>

#include <AbstractApplication.h>
#include <Renderer/Renderer.h>
#include <Renderer/Shader.h>
#include <Core/Layer.h>
#include <glm/gtc/type_ptr.hpp>

namespace GUI {

	class GridLayer : public Layer {
		Graphics::Ref<Graphics::Shader> m_gridShader;
	public:
		//All events for this layer are handled by the camera in AbstractApplication
		GridLayer() : Layer("GridLayer") {
		}

		~GridLayer() {
		}

		void OnAttach() {
			m_gridShader = Graphics::Shader::Create("./Resources/Shaders/Grid.glsl", false);
		}

		void OnUpdate() override {	
			m_gridShader->Bind();
			Graphics::Renderer::DrawGridTriangles();
		}
	};

	class ObjectLayer : public Layer {

        static const uint32_t MaxQuads = 20000;
        static const uint32_t MaxVertices = MaxQuads * 4;
        static const uint32_t MaxIndices = MaxQuads * 6;
        glm::vec3 triangleColor = glm::vec3(1.0f, 0.5f, 0.2f);

        struct UBODataFragment {
            glm::vec3 triangleColor;
        };

        struct TriangleVertex {
            glm::vec3 aPos;
            glm::vec3 aColor;
        };

        UBODataFragment uboDataFragment = UBODataFragment(triangleColor);

		Graphics::Ref<Graphics::Shader> m_BasicShader;
        // Vertex data
        float vertices[18] = {
            // positions         // colors
             0.5f, -0.5f, 0.0f,  1.0f, 0.0f, 0.0f,  // bottom right
            -0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f,  // bottom left
             0.0f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f   // top 
        };
        Graphics::Ref<Graphics::VertexArray> TriangleVertexArray = Graphics::VertexArray::Create();
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        Graphics::Ref<Graphics::VertexBuffer> TriangleVertexBuffer = Graphics::VertexBuffer::Create(vertices, sizeof(vertices));
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /////OR/////
        ///////////////If using a vertex buffer/////////////////////////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //Graphics::Ref<Graphics::VertexBuffer> TriangleVertexBuffer = Graphics::VertexBuffer::Create(MaxVertices * sizeof(TriangleVertex));
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        Graphics::Ref<Graphics::UniformBuffer> fragmentBuffer = Graphics::UniformBuffer::Create(sizeof(UBODataFragment), 1); // REMEBER TO SET BINDING TO 1 in the shader

        Graphics::Ref<Graphics::IndexBuffer> triangleIB;

        uint32_t* triangleIndices = new uint32_t[MaxIndices];

        uint32_t offset = 0;

	public:
		ObjectLayer() : Layer("ObjectLayer") {
		}
		~ObjectLayer() {
		}

		void OnAttach() {

            m_BasicShader = Graphics::Shader::Create("./Resources/Shaders/BasicShader.glsl", false);

            ///////////////If using a vertex buffer/////////////////////////////////////////////////////////////////////////////////////////////
            ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            //TriangleVertex* TriangleVertexBufferBase = new TriangleVertex[MaxVertices];
            //TriangleVertex* TriangleVertexBufferPtr = TriangleVertexBufferBase;
            //
            //TriangleVertexBufferPtr->aPos = { 0.5f, -0.5f, 0.0f };
            //TriangleVertexBufferPtr->aColor = { 1.0f, 0.0f, 0.0f };
            //TriangleVertexBufferPtr++;
            //TriangleVertexBufferPtr->aPos = { -0.5f, -0.5f, 0.0f };
            //TriangleVertexBufferPtr->aColor = { 0.0f, 1.0f, 0.0f };
            //TriangleVertexBufferPtr++;
            //TriangleVertexBufferPtr->aPos = { 0.0f, 0.5f, 0.0f };
            //TriangleVertexBufferPtr->aColor = { 0.0f, 0.0f, 1.0f };
            //TriangleVertexBufferPtr++;
            //
            //uint32_t dataSize = (uint32_t)((uint8_t*)TriangleVertexBufferPtr - (uint8_t*)TriangleVertexBufferBase);
            //std::cout << "Data Size : " << dataSize << std::endl;
            //TriangleVertexBuffer->SetData(TriangleVertexBufferBase, dataSize);
            ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

            TriangleVertexBuffer->SetLayout({
                    { Graphics::ShaderDataType::Float3, "aPos"},
                    { Graphics::ShaderDataType::Float3, "aColor"}
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

		void OnUpdate() {

            fragmentBuffer->SetData(&uboDataFragment, sizeof(UBODataFragment));
            m_BasicShader->Bind();

            //If drawing indexed//
            //Graphics::RenderCommand::DrawIndexed(TriangleVertexArray);
            //      OR      //
            Graphics::RenderCommand::DrawNonIndexed(TriangleVertexArray, 3);

		}

        void OnImGuiRender() {
            // ImGui window
            ImGui::Begin("Triangle Color");
            ImGui::ColorEdit3("Color", glm::value_ptr(uboDataFragment.triangleColor));
            ImGui::End();

            {
                static float f = 0.0f;
                static int counter = 0;

                ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

                ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)

                ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f

                if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
                    counter++;
                ImGui::SameLine();
                ImGui::Text("counter = %d", counter);
                auto io = ImGui::GetIO();
                ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
                ImGui::End();
            }
        }

	};


	class TestGUI : public AbstractApplication {
	private:
	public:
		TestGUI(const ApplicationSpecification& spec)
			:AbstractApplication(spec)
		{ 
			PushLayer(new GridLayer());
            PushLayer(new ObjectLayer());
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
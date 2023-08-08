// OpenGL Starter.cpp : Defines the entry point for the application.
//

//TO DO: Even though the oit shader can render opaque vertices, it would be best for performance to render them first with an opaque shader and then render the transparent ones.
//The min and max alpha values for the oit shader can be set with [UBOSCENE].alphaMin & [UBOSCENE].alphaWidth.


//https://github.com/nvpro-samples/vk_order_independent_transparency

//If Graphics::FramebufferTextureFormat::RGBA8 is being used in the framebuffer.
//Unbind the imgui shader and textures, change the start colorattachment index in openglframebuffer.cpp to 0.
//The current oit shader implementation will not work with the Depth buffer in the framebuffer.

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
			m_gridShader = Graphics::Shader::Create("./Resources/Shaders/Grid.glsl");
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
        glm::vec4 triangleColor = glm::vec4(1.0f, 0.5f, 0.2f, 0.1f);

        struct UBODataFragment {
            glm::vec4 triangleColor;
        };

        struct TriangleVertex {
            glm::vec3 aPos;
            glm::vec3 aColor;
        };

        UBODataFragment uboDataFragment = UBODataFragment(triangleColor);

		Graphics::Ref<Graphics::Shader> m_BasicShader;
        // Vertex data
        float vertices[18+18] = {
            // positions         // colors
             0.5f, -0.5f, 0.0f,  1.0f, 0.0f, 0.0f,  // bottom right
            -0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f,  // bottom left
             0.0f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f,  // top
             0.5f, -0.5f, 1.0f,  1.0f, 0.0f, 0.0f,  // bottom right
            -0.5f, -0.5f, 1.0f,  0.0f, 1.0f, 0.0f,  // bottom left
             0.0f,  0.5f, 1.0f,  0.0f, 0.0f, 1.0f   // top 
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

            m_BasicShader = Graphics::Shader::Create("./Resources/Shaders/BasicShader.glsl");

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
            Graphics::RenderCommand::DrawNonIndexed(TriangleVertexArray, 6);

		}

        void OnImGuiRender() {
            // ImGui window
            ImGui::Begin("Triangle Color");
            ImGui::ColorEdit4("Color", glm::value_ptr(uboDataFragment.triangleColor));
            ImGui::End();
        }
	};

    class OITTest : public Layer {
    private:
        static const uint32_t MaxQuads = 20000;
        static const uint32_t MaxVertices = MaxQuads * 4;
        static const uint32_t MaxIndices = MaxQuads * 6;
        glm::vec4 triangleColor = glm::vec4(1.0f, 0.5f, 0.8f, 1.0f);

        struct UBODataFragment {
            glm::vec4 triangleColor;
        };

        struct TriangleVertex {
            glm::vec3 inPosition;
            glm::vec3 inNormal;
            glm::vec4 inColor;
        };

        UBODataFragment uboDataFragment1 = UBODataFragment(glm::vec4(1.0,0.0,0.0,1.0));
        UBODataFragment uboDataFragment2 = UBODataFragment(triangleColor);
        UBODataFragment uboDataFragment3 = UBODataFragment(triangleColor);


        Graphics::Ref<Graphics::Shader> m_oitShader;
        Graphics::Ref<Graphics::Shader> m_opaqueShader;

        float verticesOpaque[60] = {
            // positions         // colors
             0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f,  // bottom right
            -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f,  // bottom left
             0.0f,  0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f,  // top
             0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f,  // bottom right
            -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f,  // bottom left
             0.0f,  0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f   // top 
        };
        // Vertex data
        float vertices[60+30] = {
            // positions         // normal
             0.5f, -0.5f, 1.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f,  // bottom right
            -0.5f, -0.5f, 1.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f,  // bottom left
             0.0f,  0.5f, 1.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.5f,  // top
             0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f,  // bottom right
            -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f,  // bottom left
             0.0f,  0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.5f,   // top 
             0.5f, -0.5f, 2.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f,  // bottom right
            -0.5f, -0.5f, 2.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f,  // bottom left
             0.0f,  0.5f, 2.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.5f   // top 

        };
        Graphics::Ref<Graphics::VertexArray> TriangleVertexArray = Graphics::VertexArray::Create();

        Graphics::Ref<Graphics::VertexBuffer> TriangleVertexBuffer = Graphics::VertexBuffer::Create(vertices, sizeof(vertices));

        Graphics::Ref<Graphics::VertexArray> OpaqueVertexArray = Graphics::VertexArray::Create();

        Graphics::Ref<Graphics::VertexBuffer> OpaqueVertexBuffer = Graphics::VertexBuffer::Create(verticesOpaque, sizeof(verticesOpaque));

        Graphics::Ref<Graphics::UniformBuffer> fragmentBuffer = Graphics::UniformBuffer::Create(sizeof(UBODataFragment), 1); // REMEBER TO SET BINDING TO 1 in the shader

        Graphics::Ref<Graphics::IndexBuffer> triangleIB;

        uint32_t* triangleIndices = new uint32_t[MaxIndices];

        uint32_t offset = 0;

        float* makeTriangleArray() {

        }



    public:
        OITTest() : Layer("OITTest") {
		}
        ~OITTest() {

        }

        void OnAttach() {

            const std::string passDefine = "#define PASS PASS_SHADER\n";
            const std::string colorDefine = "#define PASS PASS_COLOR\n";


			m_oitShader = Graphics::Shader::Create("./Resources/Shaders/OIT/oitSimple.glsl",passDefine,colorDefine, false, false);

            const std::string passDefineOpaque = "#define PASS PASS_SHADER\n";
            const std::string colorDefineOpaque = "#define PASS PASS_OPAQUE\n";

            m_opaqueShader = Graphics::Shader::Create("./Resources/Shaders/OIT/oitSimple.glsl", passDefineOpaque, colorDefineOpaque, false, false);

            TriangleVertexBuffer->SetLayout({
                { Graphics::ShaderDataType::Float3, "inPosition"},
                { Graphics::ShaderDataType::Float3, "inNormal"},
                { Graphics::ShaderDataType::Float4, "inColor"}
            });


            TriangleVertexArray->AddVertexBuffer(TriangleVertexBuffer);

            OpaqueVertexBuffer->SetLayout({
                { Graphics::ShaderDataType::Float3, "inPosition"},
                { Graphics::ShaderDataType::Float3, "inNormal"},
                { Graphics::ShaderDataType::Float4, "inColor"}
            });
            OpaqueVertexArray->AddVertexBuffer(OpaqueVertexBuffer);
		}

        void OnUpdate() {
            Graphics::Renderer::BindOtiBuffers();
            //m_opaqueShader->Bind();

            //Graphics::RenderCommand::DrawNonIndexed(OpaqueVertexArray, 6);

            //m_opaqueShader->Unbind();
            m_oitShader->Bind();

            fragmentBuffer->SetData(&uboDataFragment1, sizeof(UBODataFragment));
            Graphics::RenderCommand::DrawNonIndexed(TriangleVertexArray, 3);

            fragmentBuffer->SetData(&uboDataFragment2, sizeof(UBODataFragment));
            Graphics::RenderCommand::DrawNonIndexed(TriangleVertexArray, 3,3);

            fragmentBuffer->SetData(&uboDataFragment3, sizeof(UBODataFragment));
            Graphics::RenderCommand::DrawNonIndexed(TriangleVertexArray, 3,6);


            m_oitShader->Unbind();
            Graphics::Renderer::UnBindOtiBuffers();




		}
        void OnImGuiRender() {
            ImGui::Begin("Triangle Color");
            ImGui::ColorEdit4("Color", glm::value_ptr(uboDataFragment1.triangleColor));
            ImGui::ColorEdit4("Color", glm::value_ptr(uboDataFragment2.triangleColor));
            ImGui::ColorEdit4("Color", glm::value_ptr(uboDataFragment3.triangleColor));

            ImGui::End();
		}
    };

	class TestGUI : public AbstractApplication {
	private:
	public:
		TestGUI(const ApplicationSpecification& spec)
			:AbstractApplication(spec)
		{ 
            //Because of opacity and draw order, The grid should always be the last layer
            PushLayer(new OITTest());
            //PushLayer(new ObjectLayer());
			//PushLayer(new GridLayer());
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
// OpenGL Starter.cpp : Defines the entry point for the application.
//

#include "OpenGL Starter.h"

#include <iostream>
#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "ImGuiHandler/ImGuiHandler.h"
#include <Renderer/Shader.h>
#include <Renderer/Renderer2D.h>
#include <Renderer/RenderCommand.h>
#include <cstddef>
#include <Renderer/VertexArray.h>
#include <Renderer/UniformBuffer.h>
#include <Renderer/3DCamera.h>

struct UBODataVertex {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 projection;
};

struct UBODataFragment {
    glm::vec3 triangleColor;
};

struct TriangleVertex {
    glm::vec3 aPos;
    glm::vec3 aColor;
};


auto camera = Graphics::ThreeDCamera(glm::radians(180.0f), 800.0f / 600.0f, 0.1f, 1000.0f);
glm::mat4 model = glm::mat4(1.0f);
glm::mat4 view = camera.GetViewMatrix();
glm::mat4 projection = camera.GetProjection();

UBODataVertex uboDataVertex;

UBODataFragment uboDataFragment;

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    if (ImGui::GetIO().WantCaptureMouse) {
        return;
    }

    camera.OnMouseScroll(yoffset);
    uboDataVertex.view = camera.GetViewMatrix();  // Set your view matrix here
    uboDataVertex.projection = camera.GetProjection();  // Set your projection matrix here
}


void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (ImGui::GetIO().WantCaptureMouse) {
        return;
    }

    const auto& leftButton = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
    const auto& rightButton = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT);
    if (!leftButton && !rightButton) {
        camera.SetMousePos({ xpos,ypos });
        return;
    }


    camera.OnUpdate(xpos, ypos, leftButton, rightButton);
    uboDataVertex.view = camera.GetViewMatrix();  // Set your view matrix here
    uboDataVertex.projection = camera.GetProjection();  // Set your projection matrix here

}

int main() {

    if (__cplusplus == 202101L) std::cout << "C++23";
    else if (__cplusplus == 202002L) std::cout << "C++20";
    else if (__cplusplus == 201703L) std::cout << "C++17";
    else if (__cplusplus == 201402L) std::cout << "C++14";
    else if (__cplusplus == 201103L) std::cout << "C++11";
    else if (__cplusplus == 199711L) std::cout << "C++98";
    else std::cout << "pre-standard C++." << __cplusplus;
    std::cout << "\n";

    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // GLFW configuration
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);

    // Create a window
    GLFWwindow* window = glfwCreateWindow(800, 600, "ImGui, Glad, GLFW, and GLM", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetCursorPosCallback(window, mouse_callback);

    // Load OpenGL using Glad
    if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize Glad" << std::endl;
        return -1;
    }

    // Get the number of supported extensions
    GLint numExtensions;
    glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);

    // Print the extensions
    std::cout << "Supported Extensions:" << std::endl;
    for (GLint i = 0; i < numExtensions; ++i) {
        const char* extension = reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, i));
        std::cout << extension << std::endl;
    }

    ImGuiHandler imgui(window, "#version 330");


    // Vertex data
    float vertices[] = {
        // positions         // colors
         0.5f, -0.5f, 0.0f,  1.0f, 0.0f, 0.0f,  // bottom right
        -0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f,  // bottom left
         0.0f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f   // top 
    };


    // Vertex data
    //float vertices[] = {
    //    // positions         // colors
    //     0.5f, -0.5f, 0.0f,
    //    -0.5f, -0.5f, 0.0f,
    //     0.0f,  0.5f, 0.0f
    //};

    // Link shaders to a shader program
    auto shader = Graphics::Shader::Create("./Resources/Shaders/BasicShader.glsl");
    GLuint shaderProgram = shader->GetId();

    static const uint32_t MaxQuads = 20000;
    static const uint32_t MaxVertices = MaxQuads * 4;
    static const uint32_t MaxIndices = MaxQuads * 6;
    Graphics::Ref<Graphics::VertexArray> TriangleVertexArray = Graphics::VertexArray::Create();



    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Graphics::Ref<Graphics::VertexBuffer> TriangleVertexBuffer = Graphics::VertexBuffer::Create(vertices, sizeof(vertices));
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    /////OR/////

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //Graphics::Ref<Graphics::VertexBuffer> TriangleVertexBuffer = Graphics::VertexBuffer::Create(MaxVertices * sizeof(TriangleVertex));
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



    uint32_t* triangleIndices = new uint32_t[MaxIndices];

    uint32_t offset = 0;
    for (uint32_t i = 0; i < MaxIndices; i += 3)
    {
        triangleIndices[i + 0] = offset + 0;
        triangleIndices[i + 1] = offset + 1;
        triangleIndices[i + 2] = offset + 2;

        offset += 3;
    }

    // Uncomment if drawing index
    // Graphics::Ref<Graphics::IndexBuffer> triangleIB = Graphics::IndexBuffer::Create(triangleIndices, MaxIndices);
    //TriangleVertexArray->SetIndexBuffer(triangleIB);
    delete[] triangleIndices;




    // Main loop
    glm::vec3 triangleColor(1.0f, 0.5f, 0.2f);

    

    uboDataVertex.model = model;  // Set your model matrix here
    uboDataVertex.view = view;  // Set your view matrix here
    uboDataVertex.projection = projection;  // Set your projection matrix here

    uboDataFragment.triangleColor = triangleColor;  // Set the color to white
    
    auto vertexBuffer = Graphics::UniformBuffer::Create(sizeof(UBODataVertex),0);
    auto fragmentBuffer = Graphics::UniformBuffer::Create(sizeof(UBODataFragment),1); ////// REMEBER TO SET BINDING TO 1 in the shader
    
    vertexBuffer->SetData(&uboDataVertex, sizeof(UBODataVertex));
    fragmentBuffer->SetData(&uboDataFragment, sizeof(UBODataFragment));
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



    
 
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);


        glUseProgram(shaderProgram);
        //      OR      //
        //shader->Bind();
        vertexBuffer->SetData(&uboDataVertex, sizeof(UBODataVertex));
        fragmentBuffer->SetData(&uboDataFragment, sizeof(UBODataFragment)); // If shader is using ubo.triangleColor then this is needed to update the data

        //Graphics::RenderCommand::DrawIndexed(TriangleVertexArray);
        //      OR      //
        Graphics::RenderCommand::DrawNonIndexed(TriangleVertexArray, 3);


        


        //The update function has to be after all of the above!
        //Due to the render function called within update.
        imgui.Update([&]() {
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
		});

        glfwSwapBuffers(window);
    }

    glDeleteProgram(shaderProgram);

    glfwDestroyWindow(window);
    glfwTerminate();
}
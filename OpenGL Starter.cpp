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
#include <cstddef>

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

struct UBODataVertex {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 projection;
};

struct UBODataFragment {
    glm::vec3 triangleColor;
};

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

    // Link shaders to a shader program
    auto shader = Graphics::Shader::Create("./Resources/Shaders/BasicShader.glsl");
    GLuint shaderProgram = shader->GetId();

    // Vertex data
    float vertices[] = {
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
         0.0f,  0.5f, 0.0f
    };

    // Create VAO and VBO
    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Main loop
    glm::vec3 triangleColor(1.0f, 0.5f, 0.2f);


    glm::mat4 model(1.0f);
    glm::mat4 view(1.0f);
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);
    view = glm::translate(view, glm::vec3(0.0f, 0.0f, -3.0f));

    UBODataVertex uboDataVertex;
    uboDataVertex.model = model;  // Set your model matrix here
    uboDataVertex.view = view;  // Set your view matrix here
    uboDataVertex.projection = projection;  // Set your projection matrix here
    
    UBODataFragment uboDataFragment;
    uboDataFragment.triangleColor = triangleColor;  // Set the color to white
    
    GLuint uboVertex, uboFragment;
    glCreateBuffers(1, &uboVertex);
    glCreateBuffers(1, &uboFragment);
    
    
    glNamedBufferStorage(uboVertex, sizeof(UBODataVertex), &uboDataVertex, GL_DYNAMIC_STORAGE_BIT);
    glNamedBufferStorage(uboFragment, sizeof(UBODataFragment), &uboDataFragment, GL_DYNAMIC_STORAGE_BIT);
    
    
    // Bind buffer objects to binding points
    GLuint bindingIndexVertex = 0;  // Vertex shader binding point
    GLuint bindingIndexFragment = 1;  // Fragment shader binding point ////// REMEBER TO SET BINDING TO 1 in the shader
    glBindBufferBase(GL_UNIFORM_BUFFER, bindingIndexVertex, uboVertex);
    glBindBufferBase(GL_UNIFORM_BUFFER, bindingIndexFragment, uboFragment);
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////




 
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);


        glUseProgram(shaderProgram);
        //      OR      //
        //shader->Bind();

        glNamedBufferSubData(uboVertex, 0, sizeof(UBODataVertex), &uboDataVertex);

        glNamedBufferSubData(uboFragment, 0, sizeof(UBODataFragment), &uboDataFragment);

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);

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

    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &VAO);
    glDeleteProgram(shaderProgram);

    glfwDestroyWindow(window);
    glfwTerminate();
}